#include "CustomDrawArea.h"
#include "skeletonizer.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTouchEvent>
#include <QGestureEvent>
#include <QPinchGesture>
#include <QPainterPathStroker>
#include <QInputDialog>
#include <QLineEdit>
#include <QSvgRenderer>
#include <QTransform>
#include <QDebug>
#include <QLineF>
#include <QList>
#include <QPainterPath>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <limits>
#include <QLoggingCategory>
#include <QVarLengthArray>
#include <utility>

// Catégories (préfixe stable = "ae.draw")
Q_LOGGING_CATEGORY(lcDrawMode, "ae.draw.mode")
Q_LOGGING_CATEGORY(lcSnap,     "ae.draw.snap")
Q_LOGGING_CATEGORY(lcGesture,  "ae.draw.gesture")
Q_LOGGING_CATEGORY(lcPerf,     "ae.draw.perf")

static bool isPrimaryMode(CustomDrawArea::DrawMode m)
{
    using DM = CustomDrawArea::DrawMode;
    return m == DM::Freehand  || m == DM::PointParPoint ||
           m == DM::Line      || m == DM::Rectangle    ||
           m == DM::Circle    || m == DM::Text         ||
           m == DM::ThinText;
}

CustomDrawArea::CustomDrawArea(QWidget *parent)
    : QWidget(parent),
    m_drawing(false),
    m_smoothingLevel(1),
    m_savedSmoothingLevel(1),
    m_drawMode(DrawMode::Freehand),
    m_lastPrimaryMode(DrawMode::Freehand),
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
    m_copyAnchor(0,0),
    m_pasteMode(false),
    m_lastSelectClick(0,0),
    m_snapToGrid(false),
    m_showGrid(true),
    m_gridSpacing(20),
    m_minPointDistance(2.0)

{
    setMouseTracking(true);
    setAttribute(Qt::WA_AcceptTouchEvents, true); // Activation des événements tactiles
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

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
    m_handleRenderer.load(QStringLiteral(":/icons/rotate.svg"));

}

CustomDrawArea::~CustomDrawArea()
{
    if (m_touchReader) {
        m_touchReader->stop();
    }
}

void CustomDrawArea::setHighQuality(bool enabled)
{
    if (m_highQuality == enabled)
        return;
    m_highQuality = enabled;
    updateCanvas();
    update();
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

// Approxime un QPainterPath en une liste de segments droits.
// Utilise toSubpathPolygons() pour linéariser les courbes.
static QVector<QLineF> pathToLines(const QPainterPath& path)
{
    QVector<QLineF> lines;
    const auto polys = path.toSubpathPolygons(); // approximation des courbes
    for (const QPolygonF& poly : polys) {
        for (int i = 0; i + 1 < poly.size(); ++i) {
            lines.push_back(QLineF(poly[i], poly[i+1]));
        }
    }
    return lines;
}

static QPointF pathStart(const QPainterPath &p) {
    if (p.elementCount() == 0)
        return {};
    auto e = p.elementAt(0);
    return {e.x, e.y};
}

static QPointF pathEnd(const QPainterPath &p) {
    if (p.elementCount() == 0)
        return {};
    auto e = p.elementAt(p.elementCount() - 1);
    return {e.x, e.y};
}

static bool pathsTouch(const QPainterPath &a, const QPainterPath &b, qreal tol = 2.0) {
    QPointF a0 = pathStart(a);
    QPointF a1 = pathEnd(a);
    QPointF b0 = pathStart(b);
    QPointF b1 = pathEnd(b);
    return QLineF(a0, b0).length() <= tol ||
           QLineF(a0, b1).length() <= tol ||
           QLineF(a1, b0).length() <= tol ||
           QLineF(a1, b1).length() <= tol;
}


static inline QRect logicalCircleToWidgetRect(const QPointF& c, qreal r,
                                              qreal scale, const QPointF& offset)
{
    QRectF lr(c.x() - r - 2, c.y() - r - 2, (r + 2) * 2, (r + 2) * 2); // +2px marge AA
    lr.translate(offset);
    lr.setSize(lr.size() * scale);
    return lr.toAlignedRect();
}

// Petit helper générique : rect logique -> rect widget (avec marge AA)
static inline QRect logicalToWidgetRect(const QRectF& rLogical,
                                        qreal scale, const QPointF& offset,
                                        qreal aaMargin = 2.0)
{
    QRectF r = rLogical.adjusted(-aaMargin, -aaMargin, aaMargin, aaMargin);
    r.translate(offset);
    r.setSize(r.size() * scale);
    return r.toAlignedRect();
}

void CustomDrawArea::initCanvas()
{
    const QSize physSize = size() * devicePixelRatioF();

    m_canvas = QImage(physSize, QImage::Format_ARGB32_Premultiplied);
    m_canvas.setDevicePixelRatio(devicePixelRatioF());
    m_canvas.fill(Qt::white);
}

void CustomDrawArea::applyPanDelta(const QPointF &delta) {
    m_offset -= delta;
    clampOffsetToCanvas(); // nouvelle méthode utilisée ici
    update();
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

void CustomDrawArea::updateCanvas()
{
    // Reconstruit TOUT le canvas en repère logique
    const qreal dpr = m_canvas.devicePixelRatioF();
    const QRectF fullLogical(0, 0,
                             m_canvas.width()  / dpr,
                             m_canvas.height() / dpr);
    updateCanvas(fullLogical);  // délègue à la surcharge partielle
}

// Reconstruit uniquement une zone logique du canvas
void CustomDrawArea::updateCanvas(const QRectF& logicalDirty)
{
    if (logicalDirty.isEmpty())
        return;

    // Le canvas existe déjà et a son DPR configuré.
    QPainter p(&m_canvas);
    p.setRenderHint(QPainter::Antialiasing, m_highQuality);

    // Clip sur la zone sale en repère logique
    const QRectF clip = logicalDirty.adjusted(-1, -1, 1, 1); // léger pad
    p.setClipRect(clip);

    // ⚠️ Important : nettoyer la zone dans le canvas (fond transparent)
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(clip, Qt::transparent);

    // Stylo utilisé pour redessiner les formes qui touchent la zone
    QPen pen(Qt::black, m_highQuality ? 2.0 : 1.0);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    // Ne redessine que les formes intersectant logicalDirty
    for (const Shape& s : std::as_const(m_shapes)) {
        if (s.path.isEmpty()) continue;
        if (s.path.boundingRect().intersects(clip))
            p.drawPath(s.path);
    }
    p.end();
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

QRectF CustomDrawArea::selectedShapesBounds() const
{
    QRectF bounds;
    bool first = true;
    for (int idx : m_selectedShapes) {
        if (idx >= 0 && idx < m_shapes.size()) {
            QRectF br = m_shapes[idx].path.boundingRect();
            if (first) {
                bounds = br;
                first = false;
            } else {
                bounds = bounds.united(br);
            }
        }
    }
    return bounds;
}

void CustomDrawArea::applyEraserAt(const QPointF &center)
{
    QList<Shape> newShapes;
    const double epsilon = 0.001;
    const qreal  r = m_gommeRadius;

    // Chaînage des segments contigus pour reconstruire des polylignes
    auto mergeSegments = [](QList<QLineF> segs) {
        QList<QPainterPath> merged;
        const qreal tol = 0.5;
        while (!segs.isEmpty()) {
            QLineF cur = segs.takeFirst();
            QList<QPointF> chain{cur.p1(), cur.p2()};
            bool progress = true;
            while (progress) {
                progress = false;
                for (int i = 0; i < segs.size(); ++i) {
                    const QLineF s = segs[i];
                    if (QLineF(chain.last(), s.p1()).length() <= tol) {
                        chain.append(s.p2()); segs.removeAt(i); progress = true; break;
                    }
                    if (QLineF(chain.last(), s.p2()).length() <= tol) {
                        chain.append(s.p1()); segs.removeAt(i); progress = true; break;
                    }
                    if (QLineF(chain.first(), s.p2()).length() <= tol) {
                        chain.prepend(s.p1()); segs.removeAt(i); progress = true; break;
                    }
                    if (QLineF(chain.first(), s.p1()).length() <= tol) {
                        chain.prepend(s.p2()); segs.removeAt(i); progress = true; break;
                    }
                }
            }
            QPainterPath p; p.moveTo(chain.first());
            for (int i = 1; i < chain.size(); ++i) p.lineTo(chain[i]);
            merged.append(p);
        }
        return merged;
    };

    for (const Shape &shape : std::as_const(m_shapes)) {
        if (shape.path.isEmpty()) { newShapes.append(shape); continue; }

        // 1) Linéarise le chemin (gère quadTo/cubicTo)
        const QVector<QLineF> lines = pathToLines(shape.path);
        if (lines.isEmpty()) { newShapes.append(shape); continue; }

        QList<QLineF> kept;
        bool changed = false;

        for (const QLineF& seg : lines) {
            QPointF p1 = seg.p1(), p2 = seg.p2();
            const bool p1Inside = QLineF(p1, center).length() <= r + epsilon;
            const bool p2Inside = QLineF(p2, center).length() <= r + epsilon;

            // Intersection analytique segment / disque
            QPointF d = p2 - p1;
            const double A = d.x()*d.x() + d.y()*d.y();
            const double B = 2.0*((p1.x()-center.x())*d.x() + (p1.y()-center.y())*d.y());
            const double C = (p1.x()-center.x())*(p1.x()-center.x()) +
                             (p1.y()-center.y())*(p1.y()-center.y()) - r*r;
            const double disc = B*B - 4*A*C;

            QVector<double> tVals;
            if (A > 0 && disc >= 0) {
                const double s = std::sqrt(disc);
                const double t1 = (-B - s) / (2*A);
                const double t2 = (-B + s) / (2*A);
                if (t1 >= 0 && t1 <= 1) tVals.push_back(t1);
                if (t2 >= 0 && t2 <= 1) tVals.push_back(t2);
                std::sort(tVals.begin(), tVals.end());
            }

            auto addSeg = [&](const QPointF& a, const QPointF& b){
                if (QLineF(a,b).length() > 0.0) kept.append(QLineF(a,b));
            };

            if (!p1Inside && !p2Inside) {
                if (tVals.size() >= 2) {
                    changed = true;
                    const QPointF iA = p1 + d * tVals[0];
                    const QPointF iB = p1 + d * tVals[1];
                    if ((iA - p1).manhattanLength() > 0.1) addSeg(p1, iA);
                    if ((p2 - iB).manhattanLength() > 0.1) addSeg(iB, p2);
                } else if (tVals.size() == 1) {
                    changed = true; /* passe tangent → on rogne un bout négligeable */
                } else {
                    addSeg(p1, p2); // intact
                }
            } else if (!p1Inside && p2Inside) {
                if (!tVals.isEmpty()) {
                    changed = true;
                    const QPointF i = p1 + d * tVals.first();
                    if ((i - p1).manhattanLength() > 0.1) addSeg(p1, i);
                }
            } else if (p1Inside && !p2Inside) {
                if (!tVals.isEmpty()) {
                    changed = true;
                    const QPointF i = p1 + d * tVals.last();
                    if ((p2 - i).manhattanLength() > 0.1) addSeg(i, p2);
                }
            } else {
                // les deux dedans → segment entièrement effacé
                changed = true;
            }
        }

        if (!changed) {
            newShapes.append(shape);
        } else {
            const auto merged = mergeSegments(kept);
            for (const QPainterPath& mp : merged) {
                Shape ns; ns.path = mp; ns.originalId = m_nextShapeId++;
                newShapes.append(ns);
            }
        }
    }

    if (newShapes != m_shapes) {
        m_shapes = newShapes;
        if (!m_deferredErase) updateCanvas(); // commit si pas en mode “bulk”
    }
    if (!m_deferredErase) update();
}

void CustomDrawArea::updateRotationHandle()
{
    if (m_selectedShapes.isEmpty())
        return;

    QRectF bounds = selectedShapesBounds();
    QPointF center = bounds.center();
    m_rotationCenter = center;

    // Réinitialise l'angle du groupe lorsqu'une nouvelle sélection est faite
    m_groupRotationAngle = 0.0;

    m_rotationHandle = QPointF(center.x(), bounds.top() - 20);
    QPointF delta = m_rotationHandle - center;
    m_rotationHandlePos.radius = std::hypot(delta.x(), delta.y());
    m_rotationHandlePos.angleOffset = std::atan2(delta.y(), delta.x()) -
                                      m_groupRotationAngle;
}

void CustomDrawArea::setSmoothingLevel(int level)
{
    m_smoothingLevel = std::clamp(level, 0, 10);  // si tu limites le slider à 0-10

    qDebug() << "Niveau de lissage défini à :" << m_smoothingLevel << ", alpha:" << smoothingAlpha();

    m_freehandPoints.clear();
    update();
    emit smoothingLevelChanged(m_smoothingLevel);
}


void CustomDrawArea::setDrawMode(DrawMode mode)
{
    qCDebug(lcDrawMode) << "setDrawMode() appelé, mode =" << static_cast<int>(mode);
    qCDebug(lcDrawMode) << "Demande =" << static_cast<int>(mode)
                        << "| Courant =" << static_cast<int>(m_drawMode);

    // 1) keep track of the last primary tool before we overwrite m_drawMode
    if (isPrimaryMode(mode))
        m_lastPrimaryMode = mode;

    // Garde-fou : toute valeur hors enum ramène au mode libre
    const int minMode = static_cast<int>(DrawMode::Freehand);
    const int maxMode = static_cast<int>(DrawMode::ThinText);
    int requested = static_cast<int>(mode);
    if (requested < minMode || requested > maxMode) {
        qCWarning(lcDrawMode) << "Mode invalide:" << requested << "-> retour Freehand";
        revertToFreehand();
        return;
    }

    // Sinon, activation normale
    DrawMode previousMode = m_drawMode;
    if (m_selectMode)
        cancelSelection();

    if (m_closeMode) {
        m_closeMode = false;
        emit closeModeChanged(false);
    }

    if (mode == DrawMode::Deplacer && !m_deplacerMode) {
        m_deplacerMode = true;
        emit deplacerModeChanged(true);  // ← ce signal est ESSENTIEL
        qDebug() << "[setDrawMode] Signal deplacerModeChanged(true) émis";
    }


    if (m_drawMode == DrawMode::Deplacer && mode != DrawMode::Deplacer) {
        qDebug() << "[setDrawMode] 🔶 Désactivation mode DEPLACER";
        m_deplacerMode = false;
        emit deplacerModeChanged(false);
    }




    m_drawMode = mode;
    m_drawing = false;
    m_drawingLineOrCircle = false;
    m_freehandPoints.clear();
    m_selectedShapeIndex = -1;
    m_shapeMoving = false;
    m_panningActive = false;
    m_gommeErasing = false;

    if (previousMode == DrawMode::Freehand && mode != DrawMode::Freehand)
        m_savedSmoothingLevel = m_smoothingLevel;

    if (m_drawMode == DrawMode::Freehand)
        setSmoothingLevel(m_savedSmoothingLevel);
    else
        setSmoothingLevel(0);

    update();
    emit drawModeChanged(mode);
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
    const double minAlpha = 0.125;
    const double maxAlpha = 1;
    const int maxLevel = 10;

    double t = static_cast<double>(m_smoothingLevel) / maxLevel;
    double curvedT = std::pow(t, 0.5);  // racine carrée pour que les petits niveaux montent vite

    return maxAlpha - (maxAlpha - minAlpha) * curvedT;
}



int CustomDrawArea::computeSmoothingIterations(const QList<QPointF> &pts) const
{
    int baseIter = 1 + m_smoothingLevel / 3;  // augmente progressivement
    baseIter = std::min(baseIter, 5);  // limite à 5

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

    QPointF raw = (event->pos() - m_offset) / m_scale;
    QPointF pos = clampToCanvas(snapIfNeeded(raw));
    QPointF clickPos = clampToCanvas(raw);

    if (!m_selectedShapes.isEmpty()) {
        QRectF bounds = selectedShapesBounds();
        QPointF center = bounds.center();
        if (!m_rotating)
            m_rotationCenter = center;

        // Recalculer la position du handle avant de tester la distance
        qreal totalAngle = m_groupRotationAngle + m_rotationHandlePos.angleOffset;
        QPointF offset(std::cos(totalAngle) * m_rotationHandlePos.radius,
                       std::sin(totalAngle) * m_rotationHandlePos.radius);
        m_rotationHandle = center + offset;
    }


    // Maintenant tester la distance
    if (!m_selectedShapes.isEmpty() &&
        QLineF(clickPos, m_rotationHandle).length() < 10) {

        m_rotating = true;

        // idx est déjà connu ici
        m_lastAngle = std::atan2(clickPos.y() - m_rotationCenter.y(),
                                 clickPos.x() - m_rotationCenter.x());

        event->accept();
        return;
    }


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
            pushState();
            QPainterPath &p = m_shapes[hit].path;
            if (p.elementCount() > 1) {
                QPointF s = pathStart(p);
                QPointF e = pathEnd(p);
                if (QLineF(s, e).length() > 0.01)
                    p.lineTo(s);         // simple visual closing
            }
        }

        m_closeMode = false;
        emit closeModeChanged(false);
        updateCanvas();
        update();
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
            bool wasFirst = !m_selectedShapes.isEmpty() &&
                            m_selectedShapes.first() == hitShape;
            if (m_selectedShapes.contains(hitShape))
                m_selectedShapes.removeAll(hitShape);
            else
                m_selectedShapes.append(hitShape);
            if (m_connectSelectionMode && m_selectedShapes.size() == 1)
                update();
            if (m_connectSelectionMode && m_selectedShapes.size() == 2) {
                connectNearestEndpoints(m_selectedShapes[0], m_selectedShapes[1]);
                mergeShapesAndConnector(m_selectedShapes[0], m_selectedShapes[1]);
                m_selectMode = false;
                m_selectedShapes.clear();
                m_connectSelectionMode = false;
                emit shapeSelection(false);
            } else if (!m_connectSelectionMode) {
                emit multiSelectionModeChanged(true);
                if (wasFirst || m_selectedShapes.size() == 1)
                    updateRotationHandle();
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
            m_smoothingLevel == 0 ||
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
            m_smoothingLevel == 0 ||
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
    {
        // On veut un seul "Undo" pour tout le stroke → snapshot maintenant
        pushState();

        m_gommeErasing  = true;
        m_lastEraserPos = m_gommeCenter = pos;

        // On coupe déjà à la position initiale, mais sans reconstruire immédiatement le canvas
        m_deferredErase = true;          // les applyEraserAt ne feront pas updateCanvas()
        m_eraseTimer.start();
        m_lastEraseCommitMs = 0;

        applyEraserAt(m_gommeCenter);    // coupe vectorielle immédiate
        m_dirty = logicalCircleToWidgetRect(m_gommeCenter, m_gommeRadius, m_scale, m_offset);
        commitEraseIfNeeded(false);      // petit commit si besoin

        event->accept();
        return;
    }
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
        // Saisie du texte
        bool ok = false;
        QString text = QInputDialog::getText(this, tr("Saisir un texte"),
                                             tr("Texte :"), QLineEdit::Normal,
                                             m_currentText, &ok);
        if (!ok || text.isEmpty())
            break;

        m_currentText = text;

        // Snapshot pour UN seul undo sur l'opération entière
        pushState();  // ← avant mutation

        // Construit le chemin complet, puis découpe en sous-chemins
        QPainterPath textPath;
        textPath.addText(pos, m_textFont, text);

        QList<QPainterPath> letterPaths;
        if (textPath.elementCount() > 0) {
            QPainterPath currentSubpath;
            for (int i = 0; i < textPath.elementCount(); ++i) {
                const QPainterPath::Element e = textPath.elementAt(i);
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

        // Ajoute chaque sous-chemin comme Shape + calcule la zone sale logique
        QRectF dirtyLogical;   // ← zone à reconstruire dans le canvas
        for (const QPainterPath &letterPath : letterPaths) {
            if (letterPath.isEmpty()) continue;

            Shape s;
            s.path = letterPath;
            s.originalId = m_nextShapeId++;
            m_shapes.append(s);

            dirtyLogical = dirtyLogical.united(letterPath.boundingRect());
        }

        // ⚡ Mise à jour partielle (canvas + repaint écran limité)
        if (!dirtyLogical.isEmpty()) {
            updateCanvas(dirtyLogical);
            update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));
        }
        break;
    }
    // ────────────────────────────────────────────────────────────────
    //  Mode ThinText  → squelette affiné + lissage Chaikin
    // ────────────────────────────────────────────────────────────────
    case DrawMode::ThinText:
    {
        // Saisie du texte
        bool ok = false;
        QString text = QInputDialog::getText(this,
                                             tr("Saisir un texte"),
                                             tr("Texte :"),
                                             QLineEdit::Normal,
                                             m_currentText, &ok);
        if (!ok || text.isEmpty())
            break;

        m_currentText = text;

        // Police "fine"
        QFont thinFont = m_textFont;
        thinFont.setWeight(QFont::Thin);

        // Snapshot pour UN seul undo sur l'opération entière
        pushState();  // ← avant mutation

        // Construit le chemin complet, puis découpe en sous-chemins
        QPainterPath textPath;
        textPath.addText(pos, thinFont, text);

        QList<QPainterPath> letterPaths;
        if (textPath.elementCount() > 0) {
            QPainterPath currentSubpath;
            for (int i = 0; i < textPath.elementCount(); ++i) {
                const QPainterPath::Element e = textPath.elementAt(i);
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

        // Squelettisation lettre par lettre + union des bounds pour MAJ partielle
        QRectF dirtyLogical;                  // ← zone à reconstruire dans le canvas
        for (const QPainterPath &letterPath : letterPaths) {
            if (letterPath.isEmpty()) continue;

            // 1) rendu bitmap (oversample)
            const QRectF br = letterPath.boundingRect();
            const int oversample = 8; // 8 suffit généralement
            QSize imgSz = (br.size() * oversample).toSize() + QSize(8, 8);
            QImage img(imgSz, QImage::Format_Grayscale8);
            img.fill(255);

            {
                QPainter p(&img);
                p.setRenderHint(QPainter::Antialiasing, false);
                p.setPen(Qt::NoPen);
                p.setBrush(Qt::black);
                p.translate(4 - br.left()*oversample,
                            4 - br.top() *oversample);
                p.scale(oversample, oversample);
                p.drawPath(letterPath); // plein
            }

            // 2) amincissement (thinning)
            QImage skelImg = Skeletonizer::thin(img);

            // 3) vectorisation
            QPainterPath skelPath = Skeletonizer::bitmapToPath(skelImg);
            QTransform tr;
            tr.scale(1.0/oversample, 1.0/oversample);
            skelPath = tr.map(skelPath);
            skelPath.translate(br.left()-4.0/oversample,
                               br.top() -4.0/oversample);

            // 4) lissage doux
            skelPath = Skeletonizer::smoothPath(skelPath,
                                                /*chaikinPasses=*/1,
                                                /*epsilon=*/0.05);

            // 5) on stocke la lettre amincie
            Shape s;
            s.path = skelPath;
            s.originalId = m_nextShapeId++;
            m_shapes.append(s);

            dirtyLogical = dirtyLogical.united(skelPath.boundingRect());
        }

        // ⚡ Mise à jour partielle (canvas + repaint écran limité)
        if (!dirtyLogical.isEmpty()) {
            updateCanvas(dirtyLogical);
            update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));
        }
        break;
    }
    }  // Fin du switch
    QWidget::mousePressEvent(event);
    update();
}

void CustomDrawArea::mouseMoveEvent(QMouseEvent *event)
{
    QPointF raw = (event->pos() - m_offset) / m_scale;
    QPointF pos = clampToCanvas(snapIfNeeded(raw));

    if (m_rotating && !m_selectedShapes.isEmpty()) {
        qreal currentAngle = std::atan2(pos.y() - m_rotationCenter.y(), pos.x() - m_rotationCenter.x());
        qreal deltaAngle = currentAngle - m_lastAngle;
        // Normalise l'angle pour éviter des sauts de ±2π lorsque la souris
        // traverse l'axe horizontal, ce qui provoquait des translations
        // parasites de la forme lors d'une rotation.
        while (deltaAngle > M_PI)
            deltaAngle -= 2 * M_PI;
        while (deltaAngle < -M_PI)
            deltaAngle += 2 * M_PI;

        QTransform rotation;
        rotation.translate(m_rotationCenter.x(), m_rotationCenter.y());
        rotation.rotateRadians(deltaAngle);
        rotation.translate(-m_rotationCenter.x(), -m_rotationCenter.y());

        for (int idx : m_selectedShapes) {
            if (idx >= 0 && idx < m_shapes.size()) {
                m_shapes[idx].path = rotation.map(m_shapes[idx].path);
                m_shapes[idx].rotationAngle += deltaAngle;
            }
        }

        m_groupRotationAngle += deltaAngle;

        // Met à jour la position du handle
        qreal totalAngle = m_groupRotationAngle + m_rotationHandlePos.angleOffset;
        QPointF offset(std::cos(totalAngle) * m_rotationHandlePos.radius,
                       std::sin(totalAngle) * m_rotationHandlePos.radius);
        m_rotationHandle = m_rotationCenter + offset;

        updateCanvas();
        update();

        m_lastAngle = currentAngle;
        return;
    }

    if (m_twoFingersOn)
        return;

    //    pos = (event->pos() - m_offset) / m_scale;    <--- Pose problème pour SnapGrid

    switch (m_drawMode)
    {
    case DrawMode::Freehand:
        if (!m_drawing) return;
        if (m_freehandPoints.isEmpty() ||
            m_smoothingLevel == 0 ||
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
        if (!m_gommeErasing) break;

        const QPointF prev = m_lastEraserPos;
        m_gommeCenter = pos;

        // Coupe vectorielle le long du déplacement
        eraseAlong(prev, m_gommeCenter);
        m_lastEraserPos = m_gommeCenter;

        // ➜ Invalide AUSSI l’overlay (ancien et nouveau cercle)
        const QRect rPrev = logicalCircleToWidgetRect(prev,          m_gommeRadius, m_scale, m_offset);
        const QRect rNow  = logicalCircleToWidgetRect(m_gommeCenter, m_gommeRadius, m_scale, m_offset);
        m_dirty = m_dirty.united(rPrev).united(rNow);

        // Commit throttlé
        commitEraseIfNeeded(false);

        event->accept();
        return;
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
    if (m_rotating) {
        m_rotating = false;
        m_lastAngle = 0;  // Important : reset l'angle de référence
        pushState();      // Pour l'annulation
        event->accept();  // Stoppe la propagation
        update();
        return;
    }

    QPointF raw = (event->pos() - m_offset) / m_scale;
    QPointF pos = clampToCanvas(snapIfNeeded(raw));

    event->accept();

    switch (m_drawMode)
    {
    case DrawMode::Freehand:
    {
        if (!m_drawing)
            return;
        if (m_freehandPoints.isEmpty() ||
            m_smoothingLevel == 0 ||
            distance(m_freehandPoints.last(), pos) >= m_minPointDistance)
            m_freehandPoints.append(pos);
        QList<QPointF> finalPoints;
        if (m_smoothingLevel > 0 && m_freehandPoints.size() >= 2) {
            QList<QPointF> pts = m_freehandPoints;
            if (m_lowPassFilterEnabled)
                pts = applyLowPassFilter(pts, smoothingAlpha());
            int iterations = computeSmoothingIterations(pts);
            finalPoints = applyChaikinAlgorithm(pts, iterations);
        } else {
            finalPoints = m_freehandPoints;
        }
        QPainterPath path = (m_smoothingLevel > 0)
                                ? generateBezierPath(finalPoints)  // Chaikin + Bézier
                                : generateRawPath(finalPoints);    // segments bruts
        Shape s;
        s.path = path;
        s.originalId = m_nextShapeId++; // Identifiant unique
        m_shapes.append(s);
        const QRectF dirtyLogical = s.path.boundingRect();
        updateCanvas(dirtyLogical);

        // Limite la zone repaintée à l’écran
        update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));

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
            const QRectF dirtyLogical = s.path.boundingRect();
            updateCanvas(dirtyLogical);
            update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));

            m_drawingLineOrCircle = false;
        }
        break;
    }
    case DrawMode::PointParPoint:
        break;
    case DrawMode::Gomme:
    {
        if (m_gommeErasing) {
            m_gommeErasing = false;

            m_dirty = m_dirty.united(logicalCircleToWidgetRect(m_gommeCenter, m_gommeRadius, m_scale, m_offset));

            // Dernier commit forcé, puis on revient au mode normal de commit
            commitEraseIfNeeded(true);
            m_deferredErase = false;

            update();
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
        const auto &points = touchEvent->points();
        if (points.size() == 2) {
            const auto &p1 = points[0];
            const auto &p2 = points[1];
            qreal curDist  = QLineF(p1.position(),      p2.position()).length();
            qreal prevDist = QLineF(p1.lastPosition(),  p2.lastPosition()).length();
            if (prevDist > 0) {
                qreal factor = curDist / prevDist;
                QPointF center = (p1.position() + p2.position()) / 2.0;
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
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, m_highQuality);
    painter.setClipRegion(event->region());

    // Repère logique
    painter.translate(m_offset);
    painter.scale(m_scale, m_scale);

    const QRect dirty = event->rect();

    // 1) Grille (dessinée en-dessous)
    if (m_showGrid) {
        drawGrid(painter, dirty);
    }

    // Le canvas contient déjà le résultat réel (formes moins la gomme vectorielle)
    painter.drawImage(0, 0, m_canvas);

    // 3) Overlay de sélection (UNIQUEMENT les formes sélectionnées)
    if (!m_selectedShapes.isEmpty()) {
        QRectF  bounds = selectedShapesBounds();
        QPointF center = bounds.center();
        if (!m_rotating)
            m_rotationCenter = center;

        // Position actuelle du handle (rayon constant + angle cumulé)
        qreal   totalAngle = m_groupRotationAngle + m_rotationHandlePos.angleOffset;
        QPointF hoffset(std::cos(totalAngle) * m_rotationHandlePos.radius,
                        std::sin(totalAngle) * m_rotationHandlePos.radius);
        m_rotationHandle = m_rotationCenter + hoffset;

        QRect widgetBounds = logicalToWidgetRect(bounds, m_scale, m_offset);
        if (dirty.intersects(widgetBounds)) {
            QPen normal(Qt::black, 2);
            normal.setCosmetic(true);
            normal.setCapStyle(Qt::RoundCap);
            normal.setJoinStyle(Qt::RoundJoin);

            QPen halo(Qt::cyan, 6);         // halo plus épais
            halo.setCosmetic(true);
            halo.setCapStyle(Qt::RoundCap);
            halo.setJoinStyle(Qt::RoundJoin);

            for (int i : std::as_const(m_selectedShapes)) {
                if (i < 0 || i >= m_shapes.size()) continue;
                const QPainterPath &path = m_shapes[i].path;

                painter.setPen(halo);
                painter.setBrush(Qt::NoBrush);
                painter.drawPath(path);

                painter.setPen(normal);
                painter.drawPath(path);
            }

            // Icône de rotation (si proche & SVG valide)
            const double maxDistance = 150.0;
            if (QLineF(m_rotationCenter, m_rotationHandle).length() <= maxDistance &&
                m_handleRenderer.isValid())
            {
                const int iconSize = 24;
                painter.save();
                painter.translate(m_rotationHandle);
                QRectF svgRect(-iconSize/2.0, -iconSize/2.0, iconSize, iconSize);
                m_handleRenderer.render(&painter, svgRect);
                painter.restore();
            }
        }
    }

    // 4) Prévisualisations temps réel (ne modifient PAS m_canvas)

    // Ligne
    if (m_drawMode == DrawMode::Line && m_drawingLineOrCircle) {
        QRect widgetRect = logicalToWidgetRect(QRectF(m_startPoint, m_currentPoint).normalized(), m_scale, m_offset);
        if (dirty.intersects(widgetRect)) {
            painter.setPen(QPen(Qt::gray, 2, Qt::DashLine));
            painter.drawLine(m_startPoint, m_currentPoint);
        }
    }

    // Cercle
    if (m_drawMode == DrawMode::Circle && m_drawingLineOrCircle) {
        QPointF c = (m_startPoint + m_currentPoint) / 2.0;
        qreal   r = QLineF(m_startPoint, m_currentPoint).length() / 2.0;
        QRect widgetRect = logicalCircleToWidgetRect(c, r, m_scale, m_offset);
        if (dirty.intersects(widgetRect)) {
            painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
            painter.drawEllipse(c, r, r);
        }
    }

    // Rectangle
    if (m_drawMode == DrawMode::Rectangle && m_drawingLineOrCircle) {
        QRect widgetRect = logicalToWidgetRect(QRectF(m_startPoint, m_currentPoint), m_scale, m_offset);
        if (dirty.intersects(widgetRect)) {
            painter.setPen(QPen(Qt::blue, 2, Qt::DashLine));
            painter.drawRect(QRectF(m_startPoint, m_currentPoint));
        }
    }

    // Point‑par‑point
    if (m_drawMode == DrawMode::PointParPoint && !m_freehandPoints.isEmpty()) {
        QRectF logical = QPolygonF(m_freehandPoints).boundingRect();
        if (m_drawing)
            logical = logical.united(QRectF(m_freehandPoints.last(), m_currentPoint).normalized());
        QRect widgetRect = logicalToWidgetRect(logical, m_scale, m_offset);
        if (dirty.intersects(widgetRect)) {
            painter.setPen(QPen(Qt::magenta, 2, Qt::DashLine));
            for (int i = 0; i < m_freehandPoints.size() - 1; ++i)
                painter.drawLine(m_freehandPoints[i], m_freehandPoints[i + 1]);
            if (m_drawing)
                painter.drawLine(m_freehandPoints.last(), m_currentPoint);
        }
    }

    // Freehand (aperçu lissé uniquement pour le trait en cours)
    if (m_drawMode == DrawMode::Freehand && !m_freehandPoints.isEmpty()) {
        QRectF logical = QPolygonF(m_freehandPoints).boundingRect();
        if (dirty.intersects(logicalToWidgetRect(logical, m_scale, m_offset))) {
            painter.setPen(QPen(Qt::gray, 2, Qt::DashLine));
            QPainterPath preview;
            if (m_smoothingLevel > 0 && m_freehandPoints.size() >= 2) {
                QList<QPointF> pts = m_freehandPoints;
                if (m_lowPassFilterEnabled)
                    pts = applyLowPassFilter(pts, smoothingAlpha());
                int it = computeSmoothingIterations(pts);
                pts = applyChaikinAlgorithm(pts, it);
                preview = generateBezierPath(pts);
            } else {
                preview = generateRawPath(m_freehandPoints);
            }
            painter.drawPath(preview);
        }
    }

    // Gomme (curseur)
    if (m_drawMode == DrawMode::Gomme && m_gommeErasing) {
        QRect widgetRect = logicalCircleToWidgetRect(m_gommeCenter, m_gommeRadius, m_scale, m_offset);
        if (dirty.intersects(widgetRect)) {
            painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
            painter.drawEllipse(m_gommeCenter, m_gommeRadius, m_gommeRadius);
        }
    }
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
    const QRectF dirtyLogical = s.path.boundingRect();
    updateCanvas(dirtyLogical);
    update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));
}

void CustomDrawArea::addImportedLogoSubpath(const QPainterPath &subpath)
{
    Shape s;
    s.path = subpath;
    s.originalId = m_nextShapeId++;  // Génère un identifiant unique pour ce trait
    m_shapes.append(s);
    pushState();
    const QRectF dirtyLogical = s.path.boundingRect();
    updateCanvas(dirtyLogical);
    update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));
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
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();


    // Si on venait du mode déplacement, repasse en mode standard
    if (m_drawMode == DrawMode::Deplacer)
        restoreLastPrimaryMode();
    m_selectMode = true;
    m_connectSelectionMode = true;
    m_selectedShapes.clear();
    updateRotationHandle();
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
        updateRotationHandle();
        update();
    }
}

void CustomDrawArea::toggleMultiSelectMode()
{
    if (!m_selectMode) {
        cancelCloseMode();
        cancelSupprimerMode();
        cancelDeplacerMode();
        cancelGommeMode();
        if (m_drawMode == DrawMode::Deplacer)
            restoreLastPrimaryMode();
        m_selectMode = true;
        m_connectSelectionMode = false;
        m_selectedShapes.clear();
        updateRotationHandle();
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

    // 1) Calcule la zone sale AVANT suppression
    QRectF dirtyLogical;                                              // ← NEW
    for (int idx : std::as_const(m_selectedShapes)) {
        if (idx >= 0 && idx < m_shapes.size())
            dirtyLogical = dirtyLogical.united(m_shapes[idx].path.boundingRect());
    }

    // 2) Supprime
    std::sort(m_selectedShapes.begin(), m_selectedShapes.end(), std::greater<int>());
    pushState();
    for (int idx : std::as_const(m_selectedShapes)) {
        if (idx >= 0 && idx < m_shapes.size())
            m_shapes.removeAt(idx);
    }
    m_selectedShapes.clear();
    updateRotationHandle();

    // 3) ⚡ Mise à jour partielle (canvas + repaint)
    updateCanvas(dirtyLogical);                                        // ← NEW
    update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));      // ← NEW

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

void CustomDrawArea::closeCurrentShape()
{
    if (m_selectedShapes.isEmpty())
        return;

    // Zone AVANT
    QRectF beforeUnion;                                               // ← NEW
    for (int idx : m_selectedShapes) {
        if (idx >= 0 && idx < m_shapes.size())
            beforeUnion = beforeUnion.united(m_shapes[idx].path.boundingRect());
    }

    pushState();

    // Fermeture
    for (int idx : m_selectedShapes) {
        if (idx < 0 || idx >= m_shapes.size()) continue;
        QPainterPath &path = m_shapes[idx].path;
        if (!path.isEmpty())
            path.closeSubpath();
    }

    // Zone APRES
    QRectF afterUnion;                                                // ← NEW
    for (int idx : m_selectedShapes) {
        if (idx >= 0 && idx < m_shapes.size())
            afterUnion = afterUnion.united(m_shapes[idx].path.boundingRect());
    }

    // Union et petite marge pour l’AA
    QRectF dirtyLogical = beforeUnion.united(afterUnion).adjusted(-2,-2,2,2); // ← NEW

    // ⚡ Mise à jour partielle (canvas + repaint)
    updateCanvas(dirtyLogical);                                       // ← NEW
    update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));     // ← NEW
}

void CustomDrawArea::startCloseMode()
{
    cancelSelection();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();
    m_selectedShapes.clear();
    m_closeMode = true;
    emit closeModeChanged(true);
}

void CustomDrawArea::cancelCloseMode()
{
    if (!m_closeMode)
        return;
    m_closeMode = false;
    emit closeModeChanged(false);

    // Sortie du mode fermeture : retour au mode standard
    restoreLastPrimaryMode();
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

QPointF CustomDrawArea::snapIfNeeded(const QPointF &p) const {
    // si snap désactivé, on retourne directement p
    if (!m_snapToGrid) {
        return p;
    }

    if (Q_UNLIKELY(lcSnap().isDebugEnabled())) {
        qCDebug(lcSnap) << "Snap activé pour" << p;
    }
    auto roundTo = [this](qreal v){
        return std::round(v / m_gridSpacing) * m_gridSpacing;
    };
    return { roundTo(p.x()), roundTo(p.y()) };
}

QPointF CustomDrawArea::clampToCanvas(const QPointF &p) const {
    qreal w = m_canvas.width()  / m_canvas.devicePixelRatioF();
    qreal h = m_canvas.height() / m_canvas.devicePixelRatioF();
    qreal x = std::clamp(p.x(), 0.0, w);
    qreal y = std::clamp(p.y(), 0.0, h);
    return {x, y};
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

    QRectF dirtyLogical;                                                // ← NEW
    for (const Shape &s : std::as_const(m_copiedShapes)) {
        Shape newShape = s;
        QTransform t;
        t.translate(delta.x(), delta.y());
        newShape.path = t.map(s.path);
        m_shapes.append(newShape);

        dirtyLogical = dirtyLogical.united(newShape.path.boundingRect()); // ← NEW
    }

    // ⚡ mise à jour partielle (canvas + repaint)
    updateCanvas(dirtyLogical);                                           // ← NEW
    update(logicalToWidgetRect(dirtyLogical, m_scale, m_offset));         // ← NEW

    m_pasteMode = false;
    m_selectedShapes.clear();
    emit multiSelectionModeChanged(false);
    cancelSelection();
}

void CustomDrawArea::startDeplacerMode()
{
    qDebug() << "[startDeplacerMode] Appelée";
    cancelSelection();
    cancelCloseMode();
    cancelSupprimerMode();
    cancelGommeMode();
    setDrawMode(DrawMode::Deplacer);  // ← c'est ici que le mode est activé
}



void CustomDrawArea::cancelDeplacerMode()
{
    if (!m_deplacerMode) {
        qDebug() << "[cancelDeplacerMode] Rien à faire (déjà désactivé)";
        return;
    }
    qDebug() << "[cancelDeplacerMode] Désactivation du mode déplacer";
    m_deplacerMode = false;
    emit deplacerModeChanged(false);

    // Lorsque l'utilisateur quitte ce mode, on rétablit le dernier mode
    // principal utilisé pour éviter toute incohérence de l'état interne
    restoreLastPrimaryMode();
}

void CustomDrawArea::startSupprimerMode()
{
    cancelSelection();     // facultatif selon ton usage
    cancelCloseMode();
    cancelDeplacerMode();
    cancelGommeMode();

    m_supprimerMode = true;
    emit supprimerModeChanged(true);
}

void CustomDrawArea::cancelSupprimerMode()
{
    if (!m_supprimerMode) return;

    m_supprimerMode = false;
    emit supprimerModeChanged(false);

    // Retour immédiat au mode précédent
    restoreLastPrimaryMode();
}

void CustomDrawArea::startGommeMode()
{
    cancelSelection();
    cancelCloseMode();
    cancelDeplacerMode();
    cancelSupprimerMode();
    cancelGommeMode();

    m_gommeMode = true;
    emit gommeModeChanged(true);
}

void CustomDrawArea::cancelGommeMode()
{
    if (!m_gommeMode) return;

    m_gommeMode = false;
    emit gommeModeChanged(false);

    // Récupère automatiquement le mode de dessin précédent
    restoreLastPrimaryMode();
}

// ---------------------------------------------------------------------------
//  Helper: force le retour au mode Freehand
// ---------------------------------------------------------------------------
void CustomDrawArea::revertToFreehand()
{
    m_drawMode = DrawMode::Freehand;
    m_lastPrimaryMode = DrawMode::Freehand;
    m_drawing = false;
    m_drawingLineOrCircle = false;
    m_freehandPoints.clear();
    m_selectedShapeIndex = -1;
    m_shapeMoving = false;
    m_panningActive = false;
    m_gommeErasing = false;

    setSmoothingLevel(m_savedSmoothingLevel);

    update();
    emit drawModeChanged(m_drawMode);
}

void CustomDrawArea::restoreLastPrimaryMode()
{
    setDrawMode(m_lastPrimaryMode);
}

void CustomDrawArea::restorePreviousMode()
{
    setDrawMode(m_lastPrimaryMode);
}

void CustomDrawArea::eraseAlong(const QPointF& from, const QPointF& to)
{
    const qreal dist = QLineF(from, to).length();
    if (dist <= 0.001) {
        applyEraserAt(to); // coupe vectoriellement à 'to'
        m_dirty = m_dirty.united(logicalCircleToWidgetRect(to, m_gommeRadius, m_scale, m_offset));
        return;
    }

    const qreal step = qMax<qreal>(1.0, m_gommeRadius * 0.5);
    const int n = qMax(1, int(dist / step));
    for (int i = 1; i <= n; ++i) {
        const qreal t = qreal(i) / qreal(n);
        const QPointF c = from + t * (to - from);
        applyEraserAt(c); // modifie m_shapes (pas de canvas tant que m_deferredErase = true)
        m_dirty = m_dirty.united(logicalCircleToWidgetRect(c, m_gommeRadius, m_scale, m_offset));
    }
}

void CustomDrawArea::commitEraseIfNeeded(bool force)
{
    const qint64 now = m_eraseTimer.elapsed();
    if (force || now - m_lastEraseCommitMs >= 12) { // ~80 FPS max
        const bool prev = m_deferredErase;
        m_deferredErase = false;   // autorise updateCanvas() à réellement reconstruire
        updateCanvas();            // canvas reconstruit depuis m_shapes
        m_deferredErase = prev;

        m_lastEraseCommitMs = now;

        if (!m_dirty.isNull()) {
            update(m_dirty);       // repaint restreint
            m_dirty = QRect();
        }
    }
}

void CustomDrawArea::drawGrid(QPainter& painter, const QRect& dirtyRect)
{
    // On suppose que la transform (translate/scale) est déjà appliquée
    const bool oldAA = painter.testRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Stylo "cheap" (cap/join plats et largeur cosmétique 1px)
    QPen gridPen(QColor(220, 220, 220), 0, Qt::DotLine);
    gridPen.setCosmetic(true);
    gridPen.setCapStyle(Qt::FlatCap);
    gridPen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(gridPen);

    // Étendue visible en repère logique limitée à dirtyRect
    const QPointF logicalTopLeft     = (dirtyRect.topLeft()     - m_offset) / m_scale;
    const QPointF logicalBottomRight = (dirtyRect.bottomRight() - m_offset) / m_scale;

    const int   spacing = m_gridSpacing;
    const qreal left    = std::floor(logicalTopLeft.x()     / spacing) * spacing;
    const qreal top     = std::floor(logicalTopLeft.y()     / spacing) * spacing;
    const qreal right   = std::ceil (logicalBottomRight.x() / spacing) * spacing;
    const qreal bottom  = std::ceil (logicalBottomRight.y() / spacing) * spacing;

    // Après le calcul de left/top/right/bottom :
    const int vCount = int((right - left) / spacing) + 1;
    const int hCount = int((bottom - top) / spacing) + 1;

    // Seuil simple : si > 6000 lignes, on espace artificiellement la grille
    if (vCount * hCount > 6000) {
        const int factor = qCeil(std::sqrt((vCount * hCount) / 6000.0));
        QVarLengthArray<QLineF, 1024> lines;
        for (qreal x = left; x <= right; x += spacing * factor)
            lines.append(QLineF(x, top, x, bottom));
        for (qreal y = top; y <= bottom; y += spacing * factor)
            lines.append(QLineF(left, y, right, y));
        painter.drawLines(lines.constData(), lines.size());
        painter.setRenderHint(QPainter::Antialiasing, oldAA);
        return;
    }

    // ⚡️ Batch : on prépare toutes les lignes d'un coup
    QVarLengthArray<QLineF, 1024> lines;
    lines.reserve( (int)((right - left) / spacing + 2) + (int)((bottom - top) / spacing + 2) );

    for (qreal x = left; x <= right; x += spacing)
        lines.append(QLineF(x, top, x, bottom));
    for (qreal y = top; y <= bottom; y += spacing)
        lines.append(QLineF(left, y, right, y));

    // Un seul appel de dessin pour toutes les lignes
    if (!lines.isEmpty())
        painter.drawLines(lines.constData(), lines.size());

    painter.setRenderHint(QPainter::Antialiasing, oldAA);
}
