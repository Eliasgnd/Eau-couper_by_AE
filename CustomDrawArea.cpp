#include "CustomDrawArea.h"
#include "skeletonizer.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>
#include <cmath>
#include <limits>
#include <QPainterPathStroker>
#include <QWheelEvent>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QLineF>
#include <QMap>
#include <algorithm> // pour std::sort
#include <QInputDialog>
#include <QLineEdit>
#include <limits>
#include <QtGlobal>
#include <algorithm>
#include <QList>
#include <QPainterPath>
#include <QTouchEvent>
#include <QPinchGesture>
#include <utility>
#include <QTransform>

CustomDrawArea::CustomDrawArea(QWidget *parent)
    : QWidget(parent),
    m_drawing(false),
    m_smoothingEnabled(false),
    m_smoothingLevel(1),
    m_drawMode(DrawMode::Freehand),
    m_drawingLineOrCircle(false),
    m_gommeErasing(false),
    m_selectedShapeIndex(-1),
    m_shapeMoving(false),
    m_scale(1.0),
    m_offset(0,0),
    m_panningActive(false),
    m_nextShapeId(1),
    m_currentText("Votre texte ici"),
    m_textFont("Arial", 16),
    m_snapToGrid(false),
    m_gridSpacing(20),
    m_minPointDistance(2.0),
    m_copyAnchor(0,0),
    m_pasteMode(false),
    m_lastSelectClick(0,0)

{
    setMouseTracking(true);
    setAttribute(Qt::WA_AcceptTouchEvents, true); // Activation des événements tactiles
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
    setStyleSheet("background-color: white;");

    grabGesture(Qt::PinchGesture); // Déjà présent

    initCanvas();
    pushState(); // État initial

    setAttribute(Qt::WA_TouchPadAcceptSingleTouchEvents);


    /* ────────────── Gestes tactiles : une seule instance ─────────────── */
    m_touchReader = new TouchGestureReader(this);
    m_touchReader->setDevicePath("/dev/input/event9");       // adapte eventX

    connect(m_touchReader, &TouchGestureReader::pinchZoomDetected,
            this, &CustomDrawArea::onPinchZoom);

    connect(m_touchReader, &TouchGestureReader::twoFingerPanDetected,
            this, &CustomDrawArea::onTwoFingerPan);

    connect(m_touchReader, &TouchGestureReader::twoFingersTouchChanged,
            this, &CustomDrawArea::setTwoFingersOn);

    m_touchReader->start();          // ← on lance le thread UNE seule fois
    initializeLimitRect();

}

CustomDrawArea::~CustomDrawArea()
{
    if (m_touchReader) {
        m_touchReader->stop();
    }
}


// juste après vos #include, avant toute méthode :


QList<QPainterPath> CustomDrawArea::separateIntoSubpaths(const QPainterPath &path)
{
    QList<QPainterPath> subpaths;
    QPainterPath currentSubpath;

    for (int i = 0; i < path.elementCount(); ++i) {
        const QPainterPath::Element &e = path.elementAt(i);
        if (e.isMoveTo()) {
            if (!currentSubpath.isEmpty()) {
                subpaths.append(currentSubpath);
            }
            currentSubpath = QPainterPath();
            currentSubpath.moveTo(e.x, e.y);
        } else {
            currentSubpath.lineTo(e.x, e.y);
        }
    }

    if (!currentSubpath.isEmpty()) {
        subpaths.append(currentSubpath);
    }

    return subpaths;
}

// Calcule l’aire signée d’un sous‑chemin (≈ 0 si <3 points)
static double signedArea(const QPainterPath &p)
{
    int n = p.elementCount();
    if (n < 3) return 0.0;

    double A = 0.0;
    auto ex = [&](int i){ return p.elementAt(i).x; };
    auto ey = [&](int i){ return p.elementAt(i).y; };

    for (int i = 0; i < n-1; ++i)
        A += ex(i) * ey(i+1) - ex(i+1) * ey(i);
    A += ex(n-1) * ey(0) - ex(0) * ey(n-1);   // fermeture
    return 0.5 * A;                            // signe ⇢ orientation
}


// Parcourt path, détecte chaque sous‐chemin démarré par moveTo(),
// et reconstruit un nouveau QPainterPath où chacun est fermé.
static QPainterPath closeAllSubpaths(const QPainterPath &path) {
    QPainterPath result;
    int n = path.elementCount();
    if (n == 0) return result;

    // indice du début du sous‐chemin courant
    int start = 0;
    for (int i = 1; i <= n; ++i) {
        // on détecte soit la fin du tableau, soit un nouveau moveTo → fin de sous‐chemin
        bool endOfSubpath = (i == n) || path.elementAt(i).isMoveTo();
        if (endOfSubpath) {
            // extrait et ferme le sous‐chemin [start .. i-1]
            QPainterPath::Element e0 = path.elementAt(start);
            result.moveTo(e0.x, e0.y);
            for (int j = start + 1; j < i; ++j) {
                auto e = path.elementAt(j);
                result.lineTo(e.x, e.y);
            }
            result.closeSubpath();  // <-- ferme ce contour
            start = i;
        }
    }
    return result;
}




void CustomDrawArea::initCanvas()
{
    QSize physSize = size() * devicePixelRatioF();
    m_canvas = QImage(physSize, QImage::Format_ARGB32_Premultiplied);
    m_canvas.setDevicePixelRatio(devicePixelRatioF());
    m_canvas.fill(Qt::white);

    m_eraserMask = QImage(physSize, QImage::Format_ARGB32_Premultiplied);
    m_eraserMask.setDevicePixelRatio(devicePixelRatioF());
    m_eraserMask.fill(Qt::transparent);
}


QRectF m_limitRect;  // zone limite de déplacement, calculée au départ

void CustomDrawArea::initializeLimitRect()
{
    QSizeF widgetSize = size() * devicePixelRatioF();
    QPointF topLeft = QPointF(0, 0);
    QPointF bottomRight = QPointF(widgetSize.width(), widgetSize.height());

    // Converti en repère logique (à zoom 100%)
    m_limitRect = QRectF(topLeft / m_scale, bottomRight / m_scale);
}



void CustomDrawArea::applyPanDelta(const QPointF &delta) {
    m_offset -= delta;
    clampOffsetToCanvas(); // nouvelle méthode utilisée ici
    update();
}


QRectF CustomDrawArea::calculateVisibleRect(const QPointF &offset) const
{
    QSizeF widgetSize = size() * devicePixelRatioF();
    QPointF topLeft = (QPointF(0, 0) - offset) / m_scale;
    QPointF bottomRight = (QPointF(widgetSize.width(), widgetSize.height()) - offset) / m_scale;
    return QRectF(topLeft, bottomRight);
}



QRectF CustomDrawArea::currentVisibleLogicalRect() const {
    QSizeF widgetSize = size() * devicePixelRatioF();
    QPointF topLeft = (QPointF(0, 0) - m_offset) / m_scale;
    QPointF bottomRight = (QPointF(widgetSize.width(), widgetSize.height()) - m_offset) / m_scale;
    return QRectF(topLeft, bottomRight);
}

void CustomDrawArea::clampOffsetToCanvas() {
    QSizeF widgetSize = size() * devicePixelRatioF();
    QSizeF canvasSize = m_canvas.size() * m_scale;

    qreal maxOffsetX = 0;
    qreal maxOffsetY = 0;
    qreal minOffsetX = widgetSize.width() - canvasSize.width();
    qreal minOffsetY = widgetSize.height() - canvasSize.height();

    // Si le canvas est plus petit que la vue, on centre
    if (canvasSize.width() < widgetSize.width()) {
        minOffsetX = maxOffsetX = (widgetSize.width() - canvasSize.width()) / 2.0;
    }

    if (canvasSize.height() < widgetSize.height()) {
        minOffsetY = maxOffsetY = (widgetSize.height() - canvasSize.height()) / 2.0;
    }

    m_offset.setX(std::clamp(m_offset.x(), minOffsetX, maxOffsetX));
    m_offset.setY(std::clamp(m_offset.y(), minOffsetY, maxOffsetY));
}





// Appeler initializeLimitRect() une fois au démarrage du mode custom, zoom à 100%



void CustomDrawArea::updateCanvas()
{
    QImage newCanvas(size() * devicePixelRatioF(), QImage::Format_ARGB32_Premultiplied);
    newCanvas.setDevicePixelRatio(devicePixelRatioF());
    newCanvas.fill(Qt::transparent);

    QPainter painter(&newCanvas);
    painter.setRenderHint(QPainter::Antialiasing, m_smoothingEnabled);

    // Stylo noir, épaisseur 1, pas de remplissage
    QPen pen(Qt::black, 1);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);  // <- IMPORTANT

    // Dessin des formes finales
    for (const Shape &shape : m_shapes) {
        painter.drawPath(shape.path);
    }
    painter.end();

    // Application de la gomme (mask)
    QPainter maskPainter(&newCanvas);
    maskPainter.setRenderHint(QPainter::Antialiasing, true);
    maskPainter.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    maskPainter.drawImage(0, 0, m_eraserMask);
    maskPainter.end();

    m_canvas = newCanvas;



}

void CustomDrawArea::pushState()
{
    m_undoStack.append(m_shapes);
    //qDebug() << "pushState: pile size =" << m_undoStack.size();
}

void CustomDrawArea::undoLastAction()
{
    if (!m_undoStack.isEmpty()) {
        m_shapes = m_undoStack.takeLast();
        updateCanvas();
        update();
       // qDebug() << "undoLastAction: stack size =" << m_undoStack.size();
    }
}

QPainterPath CustomDrawArea::combineSegments(const QList<QPainterPath> &segments)
{
    QPainterPath combined;
    for (const QPainterPath &seg : segments)
        combined.addPath(seg);
    return combined;
}

bool CustomDrawArea::isSmoothingEnabled() const { return m_smoothingEnabled; }

void CustomDrawArea::setSmoothingEnabled(bool enabled)
{
    if (m_smoothingEnabled == enabled)      // rien à faire
        return;

    m_smoothingEnabled = enabled;
    m_freehandPoints.clear();
    update();
    emit smoothingChanged(enabled);         // ← NOUVEAU
}

void CustomDrawArea::toggleSmoothing()
{
    setSmoothingEnabled(!m_smoothingEnabled);
}
void CustomDrawArea::setSmoothingLevel(int level)
{
    if (level == 0)
        m_smoothingLevel = 0;
    else if (level == 1)
        m_smoothingLevel = 1;
    else
        m_smoothingLevel = 2;
    m_freehandPoints.clear();
    update();
}

void CustomDrawArea::setDrawMode(DrawMode mode)
{
    if (m_selectMode && m_connectSelectionMode) {
        cancelSelection();
    }
    if (m_closeMode) {
           m_closeMode = false;
           emit closeModeChanged(false);
       }

    m_drawMode = mode;
    //qDebug() << "New mode:" << static_cast<int>(m_drawMode);
    m_drawing = false;
    m_drawingLineOrCircle = false;
    m_freehandPoints.clear();
    m_selectedShapeIndex = -1;
    m_shapeMoving = false;
    m_panningActive = false;
    m_gommeErasing = false;
    update();
}

CustomDrawArea::DrawMode CustomDrawArea::getDrawMode() const { return m_drawMode; }

QList<QPolygonF> CustomDrawArea::getCustomShapes() const
{
    QList<QPolygonF> list;

    for (const Shape &shape : m_shapes)
    {
        /* 1. on sépare le QPainterPath en véritables sous‑chemins */
        QList<QPainterPath> subs = separateIntoSubpaths(shape.path);

        /* 2. chaque sous‑chemin devient son propre polygone       */
        for (const QPainterPath &sp : subs)
        {
            QPolygonF poly;
            for (int i = 0; i < sp.elementCount(); ++i)
                poly << QPointF(sp.elementAt(i).x, sp.elementAt(i).y);

            if (!poly.isEmpty())
                list.append(poly);
        }
    }
    return list;
}

void CustomDrawArea::clearDrawing()
{
    m_shapes.clear();
    m_freehandPoints.clear();
    m_undoStack.clear();
    m_eraserMask.fill(Qt::transparent);
    updateCanvas();
    update();
}

void CustomDrawArea::setEraserRadius(qreal radius)
{
    m_gommeRadius = radius;
    update();
}

double CustomDrawArea::distance(const QPointF &p1, const QPointF &p2)
{
    return std::sqrt(std::pow(p2.x()-p1.x(), 2) + std::pow(p2.y()-p1.y(), 2));
}

QList<QPointF> CustomDrawArea::applyChaikinAlgorithm(const QList<QPointF>& inputPoints, int iterations)
{
    if (inputPoints.size() < 2)
        return inputPoints;
    QList<QPointF> result = inputPoints;
    for (int iter = 0; iter < iterations; ++iter) {
        QList<QPointF> newPoints;
        newPoints.append(result.first());
        for (int i = 0; i < result.size()-1; ++i) {
            QPointF p0 = result[i];
            QPointF p1 = result[i+1];
            QPointF Q = p0 * 0.75 + p1 * 0.25;
            QPointF R = p0 * 0.25 + p1 * 0.75;
            newPoints.append(Q);
            newPoints.append(R);
        }
        newPoints.append(result.last());
        result = newPoints;
    }
    return result;
}

QPainterPath CustomDrawArea::generateBezierPath(const QList<QPointF>& pts)
{
    QPainterPath path;
    if (pts.size() < 2)
        return path;
    path.moveTo(pts.first());
    for (int i = 1; i < pts.size()-1; ++i) {
        QPointF p0 = pts[i-1];
        QPointF p1 = pts[i];
        QPointF mid = (p0 + p1) / 2;
        path.quadTo(p0, mid);
    }
    path.lineTo(pts.last());
    return path;
}

QPainterPath CustomDrawArea::generateRawPath(const QList<QPointF>& pts)
{
    QPainterPath path;
    if (pts.isEmpty())
        return path;
    path.moveTo(pts.first());
    for (int i = 1; i < pts.size(); ++i)
        path.lineTo(pts[i]);
    return path;
}

QList<QPointF> CustomDrawArea::applyLowPassFilter(const QList<QPointF>& points, double alpha) const
{
    if (points.isEmpty())
        return points;
    QList<QPointF> result;
    result.reserve(points.size());
    QPointF prev = points.first();
    result.append(prev);
    for (int i = 1; i < points.size(); ++i) {
        QPointF p = points[i] * alpha + prev * (1.0 - alpha);
        result.append(p);
        prev = p;
    }
    return result;
}

double CustomDrawArea::smoothingAlpha() const
{
    switch (m_smoothingLevel) {
    case 0: return 0.5;
    case 1: return 0.35;
    default: return 0.25;
    }
}

int CustomDrawArea::computeSmoothingIterations(const QList<QPointF> &pts) const
{
    int baseIter = (m_smoothingLevel == 0) ? 1 :
                   (m_smoothingLevel == 1 ? 2 : 3);
    if (pts.size() < 2)
        return baseIter;
    double total = 0.0;
    for (int i = 1; i < pts.size(); ++i)
        total += QLineF(pts[i-1], pts[i]).length();
    double avg = total / (pts.size() - 1);
    if (avg < m_lowSpeedThreshold)
        baseIter += 1;
    return baseIter;
}

void CustomDrawArea::mousePressEvent(QMouseEvent *event)
{
    if (m_twoFingersOn)
        return;

    QPointF pos = snapIfNeeded( (event->pos() - m_offset) / m_scale );

    if (m_pasteMode) {
        pasteCopiedShapes(pos);
        return;
    }

    // ————————— Mode fermeture —————————
    if (m_closeMode) {
        const double tol = 25.0;
        int hit = -1;

        // Trouver la forme cliquée
        for (int i = m_shapes.size() - 1; i >= 0; --i) {
            QPainterPathStroker stroker;
            stroker.setWidth(tol);
            if (stroker.createStroke(m_shapes[i].path).contains(pos)) {
                hit = i;
                break;
            }
        }

        if (hit >= 0) {
            QPainterPath &path = m_shapes[hit].path;

            if (path.isEmpty()) {
                m_closeMode = false;
                emit closeModeChanged(false);
                return;
            }

            QPointF start = path.elementAt(0);
            QPointF end = path.elementAt(path.elementCount() - 1);
            double distance = QLineF(start, end).length();

            //qDebug() << "Début-fin:" << start << "→" << end << " (distance:" << distance << ")";

            if (distance > 2.0) {
                path.lineTo(start);  // fermer avec ligne
                //qDebug() << "✅ Ligne ajoutée entre extrémités.";
            }

            path.closeSubpath();  // finalise fermeture
            //qDebug() << "✔ Chemin fermé.";

            pushState();
            updateCanvas();
            update();
            m_closeMode = false;
            emit closeModeChanged(false);
            return; // important
        }

        // Aucun hit, on quitte quand même le mode
        m_closeMode = false;
        emit closeModeChanged(false);
        return;
    }


    if (m_selectMode && m_drawMode != DrawMode::Deplacer) {
        const double tol = 25.0;
            int hitShape = -1;
            for (int i = m_shapes.size() - 1; i >= 0; --i) {
                QPainterPathStroker stroker;
                stroker.setWidth(tol);
                if (stroker.createStroke(m_shapes[i].path).contains(pos)) {
                    hitShape = i;
                    break;
                }
            }

            if (hitShape >= 0) {
                m_lastSelectClick = pos;
                if (m_selectedShapes.contains(hitShape))
                    m_selectedShapes.removeAll(hitShape);
                else
                    m_selectedShapes.append(hitShape);

                if (m_connectSelectionMode && m_selectedShapes.size() == 2) {
                    connectNearestEndpoints(m_selectedShapes[0], m_selectedShapes[1]);
                    mergeShapesAndConnector(m_selectedShapes[0], m_selectedShapes[1]);
                    m_selectMode = false;
                    m_selectedShapes.clear();
                    m_connectSelectionMode = false;
                    emit shapeSelection(false);
                } else if (!m_connectSelectionMode) {
                    emit multiSelectionModeChanged(true);
                }
                update();
            }

            return;
        }

    //qDebug() << "mousePressEvent: Pos" << pos;

    if (!m_drawing && (m_drawMode == DrawMode::Freehand ||
                       m_drawMode == DrawMode::PointParPoint ||
                       m_drawMode == DrawMode::Line ||
                       m_drawMode == DrawMode::Circle ||
                       m_drawMode == DrawMode::Rectangle))
    {
        pushState();
    }
    if (m_drawMode == DrawMode::Deplacer)
        pushState();

    switch (m_drawMode)
    {
    case DrawMode::Freehand:
        m_drawing = true;
        m_freehandPoints.clear();
        if (m_freehandPoints.isEmpty() ||
            !m_smoothingEnabled ||
            distance(m_freehandPoints.last(), pos) >= m_minPointDistance)
        {
            m_freehandPoints.append(pos);
        }
        break;
    case DrawMode::PointParPoint:
        if (!m_drawing) {
            m_drawing = true;
            m_freehandPoints.clear();
        }
        if (m_freehandPoints.isEmpty() ||
            !m_smoothingEnabled ||
            distance(m_freehandPoints.last(), pos) >= m_minPointDistance)
        {
            m_freehandPoints.append(pos);
        }
        break;
    case DrawMode::Line:
    case DrawMode::Circle:
    case DrawMode::Rectangle:
        m_drawingLineOrCircle = true;
        m_startPoint = pos;
        m_currentPoint = pos;
        break;
    case DrawMode::Supprimer:
    {
        bool removed = false;
        for (int i = m_shapes.size()-1; i >= 0; --i) {
            QPainterPath path = m_shapes.at(i).path;
            if (path.contains(pos)) {
                m_shapes.removeAt(i);
                removed = true;
                //qDebug() << "Supprimer: tracé supprimé à" << pos;
            } else {
                QPainterPathStroker stroker;
                stroker.setWidth(5);
                if (stroker.createStroke(path).contains(pos)) {
                    m_shapes.removeAt(i);
                    removed = true;
                    //qDebug() << "Supprimer: tracé supprimé (contour) à" << pos;
                }
            }
        }
        if (removed) {
            pushState();
            updateCanvas();
        }
        break;
    }
    case DrawMode::Gomme:
        m_gommeErasing = true;
        m_gommeCenter = pos;
        //qDebug() << "Gomme: début à" << m_gommeCenter;
        break;
    case DrawMode::Deplacer:
    {
        if (!m_selectedShapes.isEmpty()) {
            QRectF box;
            for (int idx : std::as_const(m_selectedShapes)) {
                if (idx >=0 && idx < m_shapes.size())
                    box = box.united(m_shapes[idx].path.boundingRect());
            }
            QRectF tolBox = box.adjusted(-10, -10, 10, 10);
            if (tolBox.contains(pos)) {
                m_shapeMoving = true;
                m_lastMovePoint = pos;
            }
        } else {
            QPainterPathStroker stroker;
            stroker.setWidth(10);
            double minDist = std::numeric_limits<double>::max();
            int selectedIndex = -1;
            for (int i = 0; i < m_shapes.size(); ++i) {
                QPainterPath thickPath = stroker.createStroke(m_shapes[i].path);
                if (thickPath.contains(pos)) {
                    QRectF bounds = m_shapes[i].path.boundingRect();
                    QPointF center = bounds.center();
                    double d = distance(pos, center);
                    if (d < minDist) {
                        minDist = d;
                        selectedIndex = i;
                    }
                }
            }
            if (selectedIndex != -1) {
                m_selectedShapeIndex = selectedIndex;
                m_shapeMoving = true;
                m_lastMovePoint = pos;
            }
        }
        break;
    }
    case DrawMode::Pan:
        m_panningActive = true;
        m_lastMovePoint = pos;
        break;
    case DrawMode::Text:
    {
        // Utilisation d'un QInputDialog pour saisir le texte
        bool ok = false;
        QString text = QInputDialog::getText(this, tr("Saisir un texte"),
                                             tr("Texte :"), QLineEdit::Normal,
                                             m_currentText, &ok);
        if (ok && !text.isEmpty())
        {
            m_currentText = text;
            QPainterPath textPath;
            textPath.addText(pos, m_textFont, text);

            // Découpage du chemin en sous-chemins (chaque sous-chemin démarre avec moveTo)
            QList<QPainterPath> letterPaths;
            if (textPath.elementCount() > 0) {
                QPainterPath currentSubpath;
                for (int i = 0; i < textPath.elementCount(); ++i) {
                    QPainterPath::Element e = textPath.elementAt(i);
                    if (e.isMoveTo()) {
                        // Si currentSubpath n'est pas vide, ajouter la précédente lettre
                        if (!currentSubpath.isEmpty())
                        {
                            letterPaths.append(currentSubpath);
                            currentSubpath = QPainterPath();
                        }
                        currentSubpath.moveTo(e.x, e.y);
                    } else {
                        currentSubpath.lineTo(e.x, e.y);
                    }
                }
                if (!currentSubpath.isEmpty())
                    letterPaths.append(currentSubpath);
            }

            // Pour chaque sous-chemin, créer une Shape avec un identifiant unique
            for (const QPainterPath &letterPath : letterPaths) {
                Shape letterShape;
                letterShape.path = letterPath;
                letterShape.originalId = m_nextShapeId++;  // Attribue un identifiant unique pour chaque lettre
                m_shapes.append(letterShape);
            }
            pushState();
            updateCanvas();
            update();
        }
        break;
    }
    // ────────────────────────────────────────────────────────────────
    //  Mode ThinText  → squelette affiné + lissage Chaikin
    // ────────────────────────────────────────────────────────────────
    case DrawMode::ThinText:
    {
        bool ok = false;
        QString text = QInputDialog::getText(this,
                                             tr("Saisir un texte"),
                                             tr("Texte :"),
                                             QLineEdit::Normal,
                                             m_currentText, &ok);
        if (!ok || text.isEmpty())
            break;

        m_currentText = text;

        const int oversample = 16;                 //  ➜ augmenter pour des bords plus fins
        QFont thinFont = m_textFont;
        thinFont.setWeight(QFont::Thin);

        // Construction du chemin complet à la position cliquée
        QPainterPath textPath;
        textPath.addText(pos, thinFont, text);

        // Découpage en sous-chemins correspondant à chaque lettre
        QList<QPainterPath> letterPaths;
        if (textPath.elementCount() > 0) {
            QPainterPath currentSubpath;
            for (int i = 0; i < textPath.elementCount(); ++i) {
                QPainterPath::Element e = textPath.elementAt(i);
                if (e.isMoveTo()) {
                    if (!currentSubpath.isEmpty()) {
                        letterPaths.append(currentSubpath);
                        currentSubpath = QPainterPath();
                    }
                    currentSubpath.moveTo(e.x, e.y);
                } else {
                    currentSubpath.lineTo(e.x, e.y);
                }
            }
            if (!currentSubpath.isEmpty())
                letterPaths.append(currentSubpath);
        }

        // Squelettisation et création d'une Shape pour chaque lettre
        for (const QPainterPath &letterPath : letterPaths) {
            if (letterPath.isEmpty())
                continue;

            QRectF br = letterPath.boundingRect();

            QSize imgSz = (br.size() * oversample).toSize() + QSize(8, 8);
            QImage img(imgSz, QImage::Format_Grayscale8);
            img.fill(255);

            QPainter p(&img);
            p.setRenderHint(QPainter::Antialiasing, false);
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::black);
            p.translate(4 - br.left() * oversample,
                        4 - br.top()  * oversample);
            p.scale(oversample, oversample);
            p.drawPath(letterPath);

            QImage skelImg = Skeletonizer::thin(img);
            QPainterPath skelPath = Skeletonizer::bitmapToPath(skelImg);

            QTransform tr;
            tr.scale(1.0 / oversample, 1.0 / oversample);
            skelPath = tr.map(skelPath);
            skelPath.translate(br.left() - 4.0 / oversample,
                               br.top()  - 4.0 / oversample);

            skelPath = Skeletonizer::smoothPath(skelPath, 3, 1.0);

            Shape s;
            s.path       = skelPath;
            s.originalId = m_nextShapeId++;
            m_shapes.append(s);
        }

        pushState();
        updateCanvas();
        update();
        break;
    }
    }  // Fin du switch
    QWidget::mousePressEvent(event);
    update();
}

void CustomDrawArea::mouseMoveEvent(QMouseEvent *event)
{
    if (m_twoFingersOn)
        return;

    QPointF pos = snapIfNeeded( (event->pos() - m_offset) / m_scale );

    switch (m_drawMode)
    {
    case DrawMode::Freehand:
        if (!m_drawing) return;
        if (m_freehandPoints.isEmpty() ||
            !m_smoothingEnabled ||
            distance(m_freehandPoints.last(), pos) >= m_minPointDistance)
        {
            m_freehandPoints.append(pos);
        }
        update();
        break;
    case DrawMode::PointParPoint:
        if (m_drawing) {
            m_currentPoint = pos;
            update();
        }
        break;
    case DrawMode::Line:
    case DrawMode::Circle:
    case DrawMode::Rectangle:
        if (m_drawingLineOrCircle)
            m_currentPoint = pos;
        update();
        break;
    case DrawMode::Gomme:
    {
        if (m_gommeErasing) {
            m_gommeCenter = pos; // Mise à jour de la position de la gomme
            update();
            m_gommeCenter = pos;
            update();

            if (!m_gommeErasing)
                break; // On s'arrête ici : on ne touche pas aux formes

            QList<Shape> newShapes;
            bool hasChanged = false; // Drapeau pour détecter si des changements sont effectués
            const double epsilon = 0.001; // tolérance pour déterminer l'intérieur
            //const double maxSegmentSize = 5.0; // Limite de taille en pixels pour un segment

            // Parcourir toutes les formes existantes
            for (const Shape &shape : qAsConst(m_shapes)) {
                if (shape.path.isEmpty())
                    continue;
                int shapeId = shape.originalId; // Conserver l'identifiant d'origine
                QPainterPath original = shape.path;
                int count = original.elementCount();
                if (count < 2) {
                    newShapes.append(shape);
                    continue;
                }

                // Détection d'un chemin fermé par comparaison du premier et du dernier point
                bool hasCloseSubpath = false;
                if (count > 1) {
                    QPainterPath::Element first = original.elementAt(0);
                    QPainterPath::Element last = original.elementAt(count - 1);
                    if (std::abs(first.x - last.x) < epsilon &&
                        std::abs(first.y - last.y) < epsilon)
                        hasCloseSubpath = true;
                }

                int lastIndex = hasCloseSubpath ? count : (count - 1);
                QList<QPainterPath> localSegments;

                // Parcourir chaque segment (en traitant le segment de fermeture si nécessaire)
                for (int j = 0; j < lastIndex; ++j) {
                    int nextIndex = (j + 1) % count; // permet de traiter le segment [dernier -> premier] si fermé
                    QPainterPath::Element e1 = original.elementAt(j);
                    QPainterPath::Element e2 = original.elementAt(nextIndex);
                    QPointF p1(e1.x, e1.y), p2(e2.x, e2.y);

                    // On considère un point comme "à l'intérieur" si sa distance est <= rayon + epsilon
                    bool p1Inside = (QLineF(p1, m_gommeCenter).length() <= m_gommeRadius + epsilon);
                    bool p2Inside = (QLineF(p2, m_gommeCenter).length() <= m_gommeRadius + epsilon);

                    // Ajouter un drapeau de changement si la gomme touche un segment
                    if (p1Inside || p2Inside) {
                        hasChanged = true;
                    }


                    QPointF d = p2 - p1;
                    double A = d.x() * d.x() + d.y() * d.y();
                    double B = 2 * ((p1.x() - m_gommeCenter.x()) * d.x() + (p1.y() - m_gommeCenter.y()) * d.y());
                    double C = (p1.x() - m_gommeCenter.x()) * (p1.x() - m_gommeCenter.x()) +
                               (p1.y() - m_gommeCenter.y()) * (p1.y() - m_gommeCenter.y()) -
                               m_gommeRadius * m_gommeRadius;
                    double discriminant = B * B - 4 * A * C;

                    QVector<double> tValues;
                    if (A != 0 && discriminant >= 0) {
                        double sqrtDisc = std::sqrt(discriminant);
                        double t1 = (-B - sqrtDisc) / (2 * A);
                        double t2 = (-B + sqrtDisc) / (2 * A);
                        if (t1 >= 0 && t1 <= 1)
                            tValues.push_back(t1);
                        if (t2 >= 0 && t2 <= 1)
                            tValues.push_back(t2);
                        std::sort(tValues.begin(), tValues.end());
                    }

                    auto makeSegment = [&](const QPointF &start, const QPointF &end) -> QPainterPath {
                        QPainterPath seg;
                        seg.moveTo(start);
                        seg.lineTo(end);
                        return seg;
                    };

                    // Si aucun des points n'est à l'intérieur et aucune intersection n'est détectée,
                    // on conserve le segment entier
                    if (!p1Inside && !p2Inside) {
                        if (tValues.size() >= 2) {
                            double tA = tValues[0], tB = tValues[1];
                            QPointF ipA = p1 + d * tA;
                            QPointF ipB = p1 + d * tB;
                            if ((ipA - p1).manhattanLength() > 0.1)
                                localSegments.append(makeSegment(p1, ipA));
                            if ((p2 - ipB).manhattanLength() > 0.1)
                                localSegments.append(makeSegment(ipB, p2));
                        } else if (tValues.size() == 1) {
                            // Cas tangent : on supprime entièrement le segment
                            // (ou vous pouvez choisir de conserver la partie extérieure si désiré)
                        } else {
                            localSegments.append(makeSegment(p1, p2));
                        }
                    }
                    else if (!p1Inside && p2Inside) {
                        if (!tValues.isEmpty()) {
                            double t = tValues.first();
                            QPointF ip = p1 + d * t;
                            if ((ip - p1).manhattanLength() > 0.1)
                                localSegments.append(makeSegment(p1, ip));
                        }
                    }
                    else if (p1Inside && !p2Inside) {
                        if (!tValues.isEmpty()) {
                            double t = tValues.last();
                            QPointF ip = p1 + d * t;
                            if ((p2 - ip).manhattanLength() > 0.1)
                                localSegments.append(makeSegment(ip, p2));
                        }
                    }
                    // Si les deux points sont à l'intérieur, on ne garde rien (segment effacé)
                } // Fin de la boucle sur les segments

                // Pour chaque segment restant, créer une nouvelle Shape en gardant l'identifiant d'origine
                for (const QPainterPath &seg : localSegments) {
                    Shape ns;
                    ns.path = seg;
                    ns.originalId = shapeId;
                    newShapes.append(ns);
                }
            } // Fin de parcours de m_shapes

            QList<Shape> mergedShapes;
            const double mergeThreshold = 5.0; // seuil en pixels pour considérer deux segments comme contigus

            // Fusion des segments par originalId (optionnel)
            QMap<int, QList<QPainterPath>> segmentsById;
            for (const Shape &s : newShapes)
                segmentsById[s.originalId].append(s.path);

            for (auto it = segmentsById.constBegin(); it != segmentsById.constEnd(); ++it) {
                int id = it.key();
                QList<QPainterPath> segs = it.value();

                // On suppose ici que les segments sont dans l'ordre (sinon, vous devrez trier)
                QPainterPath mergedPath = segs.first();
                for (int i = 1; i < segs.size(); ++i) {
                    // On récupère le dernier point du mergedPath et le premier point du segment courant
                    QPainterPath::Element lastEl = mergedPath.elementAt(mergedPath.elementCount() - 1);
                    QPainterPath::Element firstEl = segs[i].elementAt(0);
                    QPointF lastPoint(lastEl.x, lastEl.y);
                    QPointF firstPoint(firstEl.x, firstEl.y);

                    if (QLineF(lastPoint, firstPoint).length() < mergeThreshold)
                        mergedPath.addPath(segs[i]); // Fusionner si les points se rejoignent
                    else {
                        // Sinon, on sauvegarde le mergedPath courant et on démarre un nouveau chemin
                        Shape ns;
                        ns.path = mergedPath;
                        ns.originalId = id;
                        mergedShapes.append(ns);
                        mergedPath = segs[i];
                    }
                }
                // Ajouter le dernier mergedPath
                Shape ns;
                ns.path = mergedPath;
                ns.originalId = id;
                mergedShapes.append(ns);
            }

            // Maintenant, n'assignez mergedShapes qu'en fusionnant les segments contigus
            if (mergedShapes != m_shapes) {
                m_shapes = mergedShapes;
                updateCanvas();
            }
            update();

        }
        break;
    }
    case DrawMode::Deplacer:
        if (m_shapeMoving) {
            QPointF delta = pos - m_lastMovePoint;
            if (!m_selectedShapes.isEmpty()) {
                for (int idx : std::as_const(m_selectedShapes)) {
                    if (idx >= 0 && idx < m_shapes.size())
                        m_shapes[idx].path.translate(delta);
                }
            } else if (m_selectedShapeIndex >= 0) {
                m_shapes[m_selectedShapeIndex].path.translate(delta);
            }
            m_lastMovePoint = pos;
            updateCanvas();
            update();
        }
        break;
    case DrawMode::Pan:
        if (m_panningActive) {
            QPointF delta = pos - m_lastMovePoint;
            m_offset += delta;
            m_lastMovePoint = pos;
            update();
        }
        break;
    default:
        break;
    }
    QWidget::mouseMoveEvent(event);
}

void CustomDrawArea::mouseReleaseEvent(QMouseEvent *event)
{
    QPointF pos = snapIfNeeded( (event->pos() - m_offset) / m_scale );

    event->accept();

    switch (m_drawMode)
    {
    case DrawMode::Freehand:
    {
        if (!m_drawing)
            return;
        if (m_freehandPoints.isEmpty() ||
            !m_smoothingEnabled ||
            distance(m_freehandPoints.last(), pos) >= m_minPointDistance)
            m_freehandPoints.append(pos);
        QList<QPointF> finalPoints;
        if (m_smoothingEnabled && m_freehandPoints.size() >= 2) {
            QList<QPointF> pts = m_freehandPoints;
            if (m_lowPassFilterEnabled)
                pts = applyLowPassFilter(pts, smoothingAlpha());
            int iterations = computeSmoothingIterations(pts);
            finalPoints = applyChaikinAlgorithm(pts, iterations);
        } else {
            finalPoints = m_freehandPoints;
        }
        QPainterPath path = m_smoothingEnabled
                            ? generateBezierPath(finalPoints)  // Chaikin + Bézier
                            : generateRawPath(finalPoints);    // segments bruts
        Shape s;
        s.path = path;
        s.originalId = m_nextShapeId++; // Identifiant unique
        m_shapes.append(s);
        {
            QPainter maskPainter(&m_eraserMask);
            maskPainter.setRenderHint(QPainter::Antialiasing, true);
            maskPainter.setCompositionMode(QPainter::CompositionMode_Source);
            maskPainter.fillRect(path.boundingRect(), Qt::transparent);
            maskPainter.end();
        }
        updateCanvas();
        m_drawing = false;
        m_freehandPoints.clear();
        break;
    }
    case DrawMode::Line:
    case DrawMode::Circle:
    case DrawMode::Rectangle:
    {
        if (m_drawingLineOrCircle) {
            QPainterPath path;
            if (m_drawMode == DrawMode::Line) {
                path.moveTo(m_startPoint);
                path.lineTo(pos);
            } else if (m_drawMode == DrawMode::Circle) {
                QPointF center = (m_startPoint + pos) / 2;
                qreal radius = QLineF(m_startPoint, pos).length() / 2;
                const int segments = 36;
                path.moveTo(center.x() + radius, center.y());
                for (int i = 1; i <= segments; ++i) {
                    qreal angle = (2 * M_PI * i) / segments;
                    QPointF pt(center.x() + radius * std::cos(angle),
                               center.y() + radius * std::sin(angle));
                    path.lineTo(pt);
                }
                path.closeSubpath();
            } else if (m_drawMode == DrawMode::Rectangle) {
                QRectF rect(m_startPoint, pos);
                path.addRect(rect);
            }
            Shape s;
            s.path = path;
            s.originalId = m_nextShapeId++; // Identifiant unique
            m_shapes.append(s);
            {
                QPainter maskPainter(&m_eraserMask);
                maskPainter.setRenderHint(QPainter::Antialiasing, true);
                maskPainter.setCompositionMode(QPainter::CompositionMode_Source);
                maskPainter.fillRect(path.boundingRect(), Qt::transparent);
                maskPainter.end();
            }
            updateCanvas();
            m_drawingLineOrCircle = false;
        }
        break;
    }
    case DrawMode::PointParPoint:
        break;
    case DrawMode::Gomme:
    {
        if (m_gommeErasing) {
            // Créer un chemin représentant la zone de la gomme
            QPainterPath eraserPath;
            eraserPath.addEllipse(m_gommeCenter, m_gommeRadius, m_gommeRadius);

            QList<Shape> newShapes;
            // Pour chaque forme existante
            for (const Shape &shape : std::as_const(m_shapes)) {
                // Vérifier si la gomme intersecte réellement la forme
                if (shape.path.intersects(eraserPath)) {
                    QPainterPath newPath = shape.path.subtracted(eraserPath);
                    // Si la soustraction produit quelque chose et que son boundingRect est raisonnable, on garde le résultat.
                    QRectF br = newPath.boundingRect();
                    if (!newPath.isEmpty() && br.width() > 1 && br.height() > 1) {
                        Shape ns;
                        ns.path = newPath;
                        ns.originalId = shape.originalId;  // Conserver l'ID du trait
                        newShapes.append(ns);
                    } else {
                        // Sinon, conserver le shape original pour éviter qu'il disparaisse
                        newShapes.append(shape);
                    }
                } else {
                    // Si la gomme n'intersecte pas, on conserve le shape tel quel
                    newShapes.append(shape);
                }
            }
            m_shapes = newShapes;
            updateCanvas();
            update();
            m_gommeErasing = false;
            pushState();
        }
        break;
    }
    case DrawMode::Deplacer:
        if (m_shapeMoving) {
            m_shapeMoving = false;
            if (m_selectedShapes.isEmpty())
                m_selectedShapeIndex = -1;
            pushState();
        }
        break;
    case DrawMode::Pan:
        m_panningActive = false;
        break;
    case DrawMode::Supprimer:
        break;
    case DrawMode::Text:  // Ajouté pour couvrir le mode Text, même si rien n'est fait ici
        break;
    case DrawMode::ThinText:  // Ajouté pour le mode texte fin
        break;
    }
    QWidget::mouseReleaseEvent(event);
    update();
}

void CustomDrawArea::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Pour le mode PointParPoint, on effectue la fermeture du chemin
    if (m_drawMode == DrawMode::PointParPoint) {
        //QPointF pos = (event->pos() - m_offset) / m_scale;  // Déclaré ici uniquement s'il est nécessaire
        if (!m_freehandPoints.isEmpty()) {
            QPainterPath path = generateRawPath(m_freehandPoints);
            Shape s;
            s.path = path;
            s.originalId = m_nextShapeId++;
            m_shapes.append(s);
            updateCanvas();
            pushState();
        }
        m_freehandPoints.clear();
        m_drawing = false;
        update();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void CustomDrawArea::wheelEvent(QWheelEvent *event)
{
    double oldScale = m_scale;
    QPointF mousePos = event->position();
    if (event->angleDelta().y() > 0)
        m_scale *= 1.1;
    else
        m_scale /= 1.1;
    if (m_scale < 1.0)
        m_scale = 1.0;
    if (m_scale > 10.0)
        m_scale = 10.0;
    m_offset = mousePos - (m_scale / oldScale) * (mousePos - m_offset);
    if (m_scale == 1.0)
        m_offset = QPointF(0,0);
    emit zoomChanged(m_scale);
    update();
}

bool CustomDrawArea::event(QEvent *event)
{
    if (event->type() == QEvent::TouchBegin ||
        event->type() == QEvent::TouchUpdate ||
        event->type() == QEvent::TouchEnd)
    {
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        const QList<QTouchEvent::TouchPoint> &points = touchEvent->touchPoints();

        if (points.size() == 2) {
            const auto &p1 = points[0];
            const auto &p2 = points[1];
            qreal curDist = QLineF(p1.pos(), p2.pos()).length();
            qreal prevDist = QLineF(p1.lastPos(), p2.lastPos()).length();
            if (prevDist > 0) {
                qreal factor = curDist / prevDist;
                QPointF center = (p1.pos() + p2.pos()) / 2.0;
                onPinchZoom(center, factor);
            }
        }
        /*  IMPORTANT → on ne renvoie PAS true :
            on laisse Qt continuer la propagation pour le recognizer interne. */
    }
    return QWidget::event(event);
}

void CustomDrawArea::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.scale(m_zoomFactor, m_zoomFactor);    
    painter.setRenderHint(QPainter::Antialiasing, m_smoothingEnabled);

    painter.fillRect(rect(), Qt::white);

    QPointF logicalTopLeft = (QPointF(0,0) - m_offset) / m_scale;
    QPointF logicalBottomRight = (QPointF(width(), height()) - m_offset) / m_scale;
    int spacing = m_gridSpacing;
    qreal left   = std::floor(logicalTopLeft.x() / spacing) * spacing;
    qreal top    = std::floor(logicalTopLeft.y() / spacing) * spacing;
    qreal right  = std::ceil(logicalBottomRight.x() / spacing) * spacing;
    qreal bottom = std::ceil(logicalBottomRight.y() / spacing) * spacing;

    painter.save();
    painter.translate(m_offset);
    painter.scale(m_scale, m_scale);

    // Récupère toutes les formes une seule fois
    auto shapes = getCustomShapes();

    // ─── Surbrillance des segments sélectionnés ───────────────────
    // ─── Surbrillance des formes sélectionnées ───────────────────
    if (!m_selectedShapes.isEmpty()) {
        QPen selPen(Qt::cyan, 4, Qt::SolidLine);
        painter.setPen(selPen);
        // on dessine entièrement chaque shape sélectionnée
        for (int idx : m_selectedShapes) {
            painter.drawPath(m_shapes[idx].path);
        }
    }
    // -- connecteurs en trait noir continu --
    // Dessin des formes (incluant désormais tes connecteurs)
    QPen normalPen(Qt::black, 1);
    normalPen.setCosmetic(true);
    painter.setPen(normalPen);
    painter.setBrush(Qt::NoBrush);
    for (const Shape &s : m_shapes) {
        if (s.path.isEmpty())
            continue;
        painter.drawPath(s.path);
    }


    QPen gridPen(QColor(220,220,220));
    gridPen.setStyle(Qt::DotLine);
    painter.setPen(gridPen);
    for (qreal x = left; x <= right; x += spacing)
        painter.drawLine(QPointF(x, top), QPointF(x, bottom));
    for (qreal y = top; y <= bottom; y += spacing)
        painter.drawLine(QPointF(left, y), QPointF(right, y));

    painter.drawImage(0, 0, m_canvas);

    if (m_drawMode == DrawMode::Line && m_drawingLineOrCircle) {
        painter.setPen(QPen(Qt::gray, 2, Qt::DashLine));
        painter.drawLine(m_startPoint, m_currentPoint);
    }
    if (m_drawMode == DrawMode::Circle && m_drawingLineOrCircle) {
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        QPointF center = (m_startPoint + m_currentPoint) / 2;
        qreal radius = QLineF(m_startPoint, m_currentPoint).length() / 2;
        QRectF preview(center.x()-radius, center.y()-radius, 2*radius, 2*radius);

        // Créer un chemin fermé pour le cercle
        QPainterPath circlePath;
        circlePath.addEllipse(preview);
        circlePath.closeSubpath();  // Facultatif, car addEllipse() crée déjà un chemin fermé
        painter.drawPath(circlePath);
    }
    if (m_drawMode == DrawMode::Rectangle && m_drawingLineOrCircle) {
        painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
        QRectF previewRect(m_startPoint, m_currentPoint);

        QPainterPath rectPath;
        rectPath.addRect(previewRect);
        rectPath.closeSubpath();  // Facultatif ici aussi, car addRect() crée déjà un chemin fermé
        painter.drawPath(rectPath);
    }
    if (m_drawMode == DrawMode::PointParPoint && !m_freehandPoints.isEmpty()) {
        painter.setPen(QPen(Qt::magenta, 2, Qt::DashLine));
        for (int i = 0; i < m_freehandPoints.size() - 1; ++i) {
            painter.drawLine(m_freehandPoints[i], m_freehandPoints[i + 1]);
        }
        if (m_drawing) {
            painter.drawLine(m_freehandPoints.last(), m_currentPoint);
        }
    }
    /* === Aperçu temps-réel du tracé libre =========================== */
    if (m_drawMode == DrawMode::Freehand && !m_freehandPoints.isEmpty())
    {
        painter.setPen(QPen(Qt::gray, 2, Qt::DashLine));

        QPainterPath preview;
        if (m_smoothingEnabled && m_freehandPoints.size() >= 2) {
            QList<QPointF> pts = m_freehandPoints;
            if (m_lowPassFilterEnabled)
                pts = applyLowPassFilter(pts, smoothingAlpha());
            int it = computeSmoothingIterations(pts);
            pts = applyChaikinAlgorithm(pts, it);
            preview = generateBezierPath(pts);        // lissé
        } else {
            preview = generateRawPath(m_freehandPoints); // segments bruts
        }

        painter.drawPath(preview);
    }
    /* ================================================================ */

    if (m_drawMode == DrawMode::Gomme && m_gommeErasing) {
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.drawEllipse(m_gommeCenter, m_gommeRadius, m_gommeRadius);
    }

    painter.restore();
    //QRectF visible = currentVisibleLogicalRect();
    //qDebug() << "📐 Zone logique visible actuelle =" << visible;
    //qDebug() << "📏 Zone limite logique autorisée =" << m_limitRect;

}


void CustomDrawArea::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    initCanvas();
    updateCanvas();
}

void CustomDrawArea::addImportedLogo(const QPainterPath &logoPath) {
    Shape s;
    s.path = logoPath;
    s.originalId = m_nextShapeId++; // Génère un identifiant unique
    m_shapes.append(s);
    pushState();
    updateCanvas();
    update();
}

void CustomDrawArea::addImportedLogoSubpath(const QPainterPath &subpath)
{
    Shape s;
    s.path = subpath;
    s.originalId = m_nextShapeId++;  // Génère un identifiant unique pour ce trait
    m_shapes.append(s);
    pushState();
    updateCanvas();
    update();
}

void CustomDrawArea::setTextFont(const QFont &font)
{
    m_textFont = font;
    update(); // Redessine le widget pour prendre en compte la nouvelle police (si nécessaire)
}

QFont CustomDrawArea::getTextFont() const
{
    return m_textFont;
}


// Slot : calcule et stocke le plus court segment entre extrémités
// relie deux formes données
void CustomDrawArea::connectNearestEndpoints(int idx1, int idx2)
{
    //qDebug() << "→ connectNearestEndpoints appelée avec" << idx1 << "et" << idx2;

    auto shapesPoly = getCustomShapes();
    const QPolygonF &poly1 = shapesPoly[idx1];
    const QPolygonF &poly2 = shapesPoly[idx2];
    if (poly1.size() < 2 || poly2.size() < 2) return;

    // On ne prend que les extrémités
    QVector<QPointF> ends1{ poly1.first(), poly1.last() };
    QVector<QPointF> ends2{ poly2.first(), poly2.last() };

    double bestD = std::numeric_limits<double>::infinity();
    QLineF  bestL;
    for (auto &a : ends1) {
      for (auto &b : ends2) {
        double d = QLineF(a, b).length();
        if (d < bestD) {
          bestD = d;
          bestL = QLineF(a, b);
        }
      }
    }

    if (bestD < std::numeric_limits<double>::infinity()) {
        // Crée un QPainterPath pour ce segment
        QPainterPath path;
        path.moveTo(bestL.p1());
        path.lineTo(bestL.p2());

        // Ajoute-le comme un Shape normal
        Shape connector;
        connector.path = path;
        connector.originalId = m_nextShapeId++;

        // Vérifie si ce segment existe déjà (évite doublons)
        bool alreadyExists = false;
        for (const Shape& existing : m_shapes) {
            if (existing.path == path) {
                alreadyExists = true;
                break;
            }
        }

        if (!alreadyExists) {
            m_shapes.append(connector);
            //qDebug() << "→ Connecteur ajouté entre" << bestL.p1() << "et" << bestL.p2();
        } else {
            //qDebug() << "⚠ Connecteur déjà existant";
        }

        //qDebug() << "→ connecteur ajouté. Taille totale m_shapes =" << m_shapes.size();

        // Enregistre l'état et redessine
        pushState();
        updateCanvas();
        update();

    }
}



void CustomDrawArea::startShapeSelection()
{
    // Si on était en mode fermeture, on l'annule
    cancelCloseMode();
    m_selectMode = true;
    m_connectSelectionMode = true;
    m_selectedShapes.clear();
    emit shapeSelection(true);
    update();
}


void CustomDrawArea::cancelSelection()
{
    if (m_selectMode) {
        m_selectMode = false;
        m_selectedShapes.clear();
        if (m_connectSelectionMode)
            emit shapeSelection(false);
        else
            emit multiSelectionModeChanged(false);
        m_connectSelectionMode = false;
        update();
    }
}

void CustomDrawArea::toggleMultiSelectMode()
{
    if (!m_selectMode) {
        cancelCloseMode();
        if (m_drawMode == DrawMode::Deplacer)
            m_drawMode = DrawMode::Freehand;
        m_selectMode = true;
        m_connectSelectionMode = false;
        m_selectedShapes.clear();
        emit multiSelectionModeChanged(true);
    } else if (!m_connectSelectionMode) {
        cancelSelection();
    }
    update();
}


void CustomDrawArea::deleteSelectedShapes()
{
    if (m_selectedShapes.isEmpty())
        return;

    std::sort(m_selectedShapes.begin(), m_selectedShapes.end(), std::greater<int>());
    pushState();
    for (int idx : std::as_const(m_selectedShapes)) {
        if (idx >= 0 && idx < m_shapes.size())
            m_shapes.removeAt(idx);
    }
    m_selectedShapes.clear();
    updateCanvas();
    update();
    emit multiSelectionModeChanged(false);
    m_selectMode = false;
}



void CustomDrawArea::mergeShapesAndConnector(int idx1, int idx2)
{
    if (idx1 == idx2 || idx1 < 0 || idx2 < 0 || idx1 >= m_shapes.size() || idx2 >= m_shapes.size())
        return;

    const QPainterPath& path1 = m_shapes[idx1].path;
    const QPainterPath& path2 = m_shapes[idx2].path;

    QVector<QPointF> points1, points2;

    for (int i = 0; i < path1.elementCount(); ++i)
        points1.append(QPointF(path1.elementAt(i).x, path1.elementAt(i).y));
    for (int i = 0; i < path2.elementCount(); ++i)
        points2.append(QPointF(path2.elementAt(i).x, path2.elementAt(i).y));

    if (points1.size() < 2 || points2.size() < 2)
        return;

    // Extraire extrémités
    QPointF p1_start = points1.first();
    QPointF p1_end   = points1.last();
    QPointF p2_start = points2.first();
    QPointF p2_end   = points2.last();

    // Calcul des distances
    double d00 = QLineF(p1_start, p2_start).length();
    double d01 = QLineF(p1_start, p2_end).length();
    double d10 = QLineF(p1_end,   p2_start).length();
    double d11 = QLineF(p1_end,   p2_end).length();

    //qDebug() << "Distances entre extrémités :";
    //qDebug() << "p1_start <-> p2_start =" << d00;
    //qDebug() << "p1_start <-> p2_end   =" << d01;
    //qDebug() << "p1_end   <-> p2_start =" << d10;
    //qDebug() << "p1_end   <-> p2_end   =" << d11;

    // Trouver la plus petite
    double minDist = d00;
    int config = 0;  // 0: d00, 1: d01, 2: d10, 3: d11

    if (d01 < minDist) { minDist = d01; config = 1; }
    if (d10 < minDist) { minDist = d10; config = 2; }
    if (d11 < minDist) { minDist = d11; config = 3; }

    //qDebug() << "Configuration la plus proche : case" << config << "avec distance =" << minDist;

    // Réorienter les points pour tracer dans le bon ordre
    if (config == 0) {
        std::reverse(points1.begin(), points1.end());
    } else if (config == 1) {
        std::reverse(points1.begin(), points1.end());
        std::reverse(points2.begin(), points2.end());
    } else if (config == 3) {
        std::reverse(points2.begin(), points2.end());
    }
    // config == 2 est déjà dans le bon ordre

    // Fusionner avec lien entre extrémités
    QVector<QPointF> mergedPoints = points1;
    mergedPoints.append(points2.first());  // créer le lien
    mergedPoints += points2;

    QPainterPath mergedPath;
    mergedPath.moveTo(mergedPoints[0]);
    for (int i = 1; i < mergedPoints.size(); ++i)
        mergedPath.lineTo(mergedPoints[i]);

    // Supprimer les anciennes formes
    // On suppose que le connecteur est le dernier élément de m_shapes
    int connectorIdx = m_shapes.size() - 1;

    QVector<int> toRemove = { idx1, idx2, connectorIdx };
    std::sort(toRemove.begin(), toRemove.end(), std::greater<int>()); // Supprimer du plus grand vers le plus petit

    for (int idx : toRemove) {
        if (idx >= 0 && idx < m_shapes.size()) {
            //qDebug() << "Suppression forme originalId=" << m_shapes[idx].originalId;
            m_shapes.removeAt(idx);
        }
    }


    Shape s;
    s.path = mergedPath;
    s.originalId = m_nextShapeId++;
    m_shapes.append(s);

    //qDebug() << "Fusion effectuée. Total points:" << mergedPoints.size();
    pushState();
    updateCanvas();
    update();
}



// Ajoute le nœud à une structure de données
void CustomDrawArea::addNode(const QPointF& position)
{
    // Si tu veux garder une liste de nœuds
    m_nodes.append(position);
}



void CustomDrawArea::closeCurrentShape()
{
    // Rien à faire si aucune forme n'est sélectionnée
    if (m_selectedShapes.isEmpty())
        return;

    // Sauvegarde pour undo
    pushState();

    // Pour chaque forme sélectionnée, on ferme le sous‑chemin
    //qDebug() << "🔁 Fermeture des formes sélectionnées...";
    //qDebug() << "📌 Formes sélectionnées:" << m_selectedShapes;
    //qDebug() << "📌 Nombre total de formes:" << m_shapes.size();

    for (int idx : m_selectedShapes) {
        if (idx < 0 || idx >= m_shapes.size()) {
            qWarning() << "❌ Index de sélection invalide:" << idx;
            continue;
        }
        QPainterPath &path = m_shapes[idx].path;
        if (!path.isEmpty()) {
            //qDebug() << "📏 Element count:" << path.elementCount();
            path.closeSubpath();
        }
    }

    // Mise à jour
    updateCanvas();
    update();
}

void CustomDrawArea::startCloseMode()
{
    cancelSelection();  // <- déjà présent, garde-le
    m_selectedShapes.clear();  // <- à ajouter si pas déjà là
    m_closeMode = true;
    emit closeModeChanged(true);
}

void CustomDrawArea::cancelCloseMode()
{
    if (!m_closeMode)
        return;
    m_closeMode = false;
    emit closeModeChanged(false);
}

void CustomDrawArea::onPinchZoom(const QPointF &center, qreal scaleFactor)
{
    //qDebug() << "🔍 Zoom appliqué depuis gesture:";
    //qDebug() << "   ➤ Facteur reçu =" << scaleFactor;
    //qDebug() << "   ➤ Zoom avant =" << m_scale;

    double oldScale = m_scale;
    m_scale *= scaleFactor;

    if (m_scale < 1.0) m_scale = 1.0;
    if (m_scale > 10.0) m_scale = 10.0;

    // Recentrer autour du point entre les deux doigts
    QPointF before = (center - m_offset) / oldScale;
    QPointF after = before * m_scale;
    m_offset = center - after;

    m_drawing = false; // désactive l’écriture pendant le zoom
    emit zoomChanged(m_scale);
    update();
}


bool CustomDrawArea::gestureEvent(QGestureEvent *event)
{
    if (QGesture *pinch = event->gesture(Qt::PinchGesture)) {
        return pinchTriggered(static_cast<QPinchGesture *>(pinch));
    }
    return false;
}


bool CustomDrawArea::pinchTriggered(QPinchGesture *gesture)
{
    if (!gesture)
        return false;

    if (gesture->state() == Qt::GestureStarted ||
        gesture->state() == Qt::GestureUpdated) {

        const QPointF center = gesture->centerPoint();
        const qreal scaleFactor = gesture->scaleFactor();

        onPinchZoom(center, scaleFactor);
        return true;
    }

    return false;
}

void CustomDrawArea::handlePinchZoom(QPointF screenCenter, qreal factor)
{
    // Ignorer les zooms trop petits ou trop violents
    if (factor < 0.9 || factor > 1.1) return;

    // Arrêter toute interaction en cours
    m_drawing = false;
    m_drawingLineOrCircle = false;
    m_gommeErasing = false;

    // Convertir le point écran (centre du zoom) en coordonnées logiques AVANT zoom
    QPointF logicalBefore = (screenCenter - m_offset) / m_scale;

    // Appliquer le facteur de zoom
    m_scale *= factor;

    // Calculer le nouvel offset pour garder logicalBefore sous le curseur
    m_offset = screenCenter - logicalBefore * m_scale;

    update();
}

void CustomDrawArea::onTwoFingerPan(const QPointF &delta)
{
    //qDebug() << "🖐️ Delta reçu:" << delta;

    applyPanDelta(delta);
}


void CustomDrawArea::setTwoFingersOn(bool active)
{
    m_twoFingersOn = active;
    //qDebug() << "[DEBUG] Two fingers state changed:" << active;
    if (active) {
        m_drawing = false;
        m_drawingLineOrCircle = false;
        m_gommeErasing = false;
        m_freehandPoints.clear();
        update();
    }
}

QPointF CustomDrawArea::snapIfNeeded(const QPointF &p) const
{
    if (!m_snapToGrid) return p;

    auto roundTo = [this](qreal v){
        return std::round(v / m_gridSpacing) * m_gridSpacing;
    };
    return { roundTo(p.x()), roundTo(p.y()) };
}

void CustomDrawArea::setGridSpacing(int px)
{
    m_gridSpacing = qMax(1, px);   // sécurité : pas 0
    update();                      // redessine la grille
}

void CustomDrawArea::copySelectedShapes()
{
    m_copiedShapes.clear();
    for (int idx : std::as_const(m_selectedShapes)) {
        if (idx >= 0 && idx < m_shapes.size())
            m_copiedShapes.append(m_shapes[idx]);
    }
    m_copyAnchor = m_lastSelectClick;
}

void CustomDrawArea::enablePasteMode()
{
    if (!m_copiedShapes.isEmpty())
        m_pasteMode = true;
}

void CustomDrawArea::pasteCopiedShapes(const QPointF &dest)
{
    if (!m_pasteMode || m_copiedShapes.isEmpty())
        return;

    QPointF delta = dest - m_copyAnchor;
    pushState();
    for (const Shape &s : std::as_const(m_copiedShapes)) {
        Shape newShape = s;
        QTransform t;
        t.translate(delta.x(), delta.y());
        newShape.path = t.map(s.path);
        m_shapes.append(newShape);
    }
    updateCanvas();
    update();
    m_pasteMode = false;
    m_selectedShapes.clear();
    emit multiSelectionModeChanged(false);
    cancelSelection();
}
