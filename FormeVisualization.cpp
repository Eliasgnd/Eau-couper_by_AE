#include "FormeVisualization.h"

#include <QApplication>
#include <QBrush>
#include <QCoreApplication>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QResizeEvent>
#include <QVBoxLayout>

class MovablePathItem : public QGraphicsPathItem {
public:
    MovablePathItem() {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
        QPen p;
        p.setColor(Qt::black);
        p.setWidth(1);
        p.setStyle(Qt::SolidLine);
        p.setCapStyle(Qt::SquareCap);
        p.setJoinStyle(Qt::MiterJoin);
        p.setCosmetic(true);
        setPen(p);
        setBrush(Qt::NoBrush);
    }

    void setBasePath(const QPainterPath &p) {
        basePath_ = p;
        QGraphicsPathItem::setPath(p);
    }

    const QPainterPath &basePath() const {
        return basePath_;
    }

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
        if (change == QGraphicsItem::ItemPositionChange && scene()) {
            QPointF desired = value.toPointF();
            QRectF rect = path().boundingRect();
            QTransform t;
            t.translate(desired.x(), desired.y());
            t.translate(transformOriginPoint().x(), transformOriginPoint().y());
            t.rotate(rotation());
            t.translate(-transformOriginPoint().x(), -transformOriginPoint().y());
            QRectF mapped = t.mapRect(rect);
            QRectF bounds = scene()->sceneRect();
            if (!bounds.contains(mapped)) {
                QPointF fixed = desired;
                if (mapped.left() < bounds.left()) {
                    fixed.setX(fixed.x() + bounds.left() - mapped.left());
                }
                if (mapped.top() < bounds.top()) {
                    fixed.setY(fixed.y() + bounds.top() - mapped.top());
                }
                if (mapped.right() > bounds.right()) {
                    fixed.setX(fixed.x() - (mapped.right() - bounds.right()));
                }
                if (mapped.bottom() > bounds.bottom()) {
                    fixed.setY(fixed.y() - (mapped.bottom() - bounds.bottom()));
                }
                return fixed;
            }
        }
        return QGraphicsPathItem::itemChange(change, value);
    }

private:
    QPainterPath basePath_;
};

static FormeVisualization *g_instance = nullptr;

void FormeVisualization::messageForwarder(QtMsgType type,
                                          const QMessageLogContext &ctx,
                                          const QString &msg) {
    if (g_instance && g_instance->previousHandler) {
        g_instance->previousHandler(type, ctx, msg);
    }
}

FormeVisualization::FormeVisualization(QWidget *parent)
    : QWidget(parent) {
    scene = new QGraphicsScene(this);

    graphicsView = new QGraphicsView(scene, this);
    graphicsView->setRenderHint(QPainter::Antialiasing, true);
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    graphicsView->setInteractive(true);
    graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    graphicsView->viewport()->installEventFilter(this);

    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(graphicsView);
    layout->addWidget(progressBar);
    setLayout(layout);

    QRectF plate(0.0, 0.0, m_sheetMm.width(), m_sheetMm.height());
    scene->setSceneRect(plate);
    QPen borderPen;
    borderPen.setColor(Qt::gray);
    borderPen.setWidth(1);
    borderPen.setCosmetic(true);
    m_sheetBorder = scene->addRect(plate, borderPen, Qt::NoBrush);
    if (m_sheetBorder) {
        m_sheetBorder->setZValue(1.0);
    }

    connect(scene, &QGraphicsScene::selectionChanged,
            this, &FormeVisualization::handleSelectionChanged);

    if (qApp) {
        g_instance = this;
        previousHandler = qInstallMessageHandler(FormeVisualization::messageForwarder);
        connect(qApp, &QCoreApplication::aboutToQuit,
                this, &FormeVisualization::cleanup);
    }
}

FormeVisualization::~FormeVisualization() {
    cleanup();
}

void FormeVisualization::cleanup() {
    if (previousHandler) {
        qInstallMessageHandler(previousHandler);
        previousHandler = nullptr;
    }

    g_instance = nullptr;

    if (scene) {
        scene->blockSignals(true);
        scene->clear();
    }

    if (graphicsView && graphicsView->scene() == scene) {
        graphicsView->setScene(nullptr);
    }

    delete scene;
    scene = nullptr;
}

static QPen defaultPen() {
    QPen p;
    p.setColor(Qt::black);
    p.setWidth(1);
    p.setStyle(Qt::SolidLine);
    p.setCapStyle(Qt::SquareCap);
    p.setJoinStyle(Qt::MiterJoin);
    p.setCosmetic(true);
    return p;
}

static QPointF clampPositionToScene(const QRectF &itemRect,
                                    const QRectF &bounds,
                                    const QPointF &desired) {
    QPointF result = desired;
    QRectF moved = itemRect.translated(desired);
    if (moved.left() < bounds.left()) {
        result.setX(result.x() + bounds.left() - moved.left());
    }
    if (moved.top() < bounds.top()) {
        result.setY(result.y() + bounds.top() - moved.top());
    }
    if (moved.right() > bounds.right()) {
        result.setX(result.x() - (moved.right() - bounds.right()));
    }
    if (moved.bottom() > bounds.bottom()) {
        result.setY(result.y() - (moved.bottom() - bounds.bottom()));
    }
    return result;
}

static void removeExistingShapeItems(QGraphicsScene *sc,
                                    QGraphicsRectItem *border,
                                    QList<QGraphicsItem*> &markers) {
    QList<QGraphicsItem*> items = sc->items();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        bool isBorder = (it == border);
        bool isMarker = markers.contains(it);
        bool isPath = qgraphicsitem_cast<QGraphicsPathItem*>(it) != nullptr; // ou MovablePathItem*
        if (!isBorder && !isMarker && isPath) {
            sc->removeItem(it);
            delete it;
        }
    }
}

static QPainterPath combinePolygons(const QList<QPolygonF> &shapes) {
    QPainterPath combined;
    for (int i = 0; i < shapes.size(); ++i) {
        const QPolygonF &poly = shapes.at(i);
        combined.addPolygon(poly);
    }
    return combined;
}

static void applyDefaultStyle(QGraphicsPathItem *item) {
    if (!item) {
        return;
    }
    item->setPen(defaultPen());
    item->setBrush(Qt::NoBrush);
}

static bool isPathItem(QGraphicsItem *it) {
    return dynamic_cast<QGraphicsPathItem*>(it) != nullptr;
}

void FormeVisualization::applySize(MovablePathItem *item, qreal W, qreal H) {
    if (!item) {
        return;
    }

    QPainterPath base = item->basePath();
    QRectF br = base.boundingRect();

    if (br.width() <= 0.000001 || br.height() <= 0.000001) {
        return;
    }

    qreal w = br.width();
    qreal h = br.height();
    qreal cx = br.center().x();
    qreal cy = br.center().y();
    qreal sx = 0.0;
    qreal sy = 0.0;
    sx = W / w;
    sy = H / h;

    QTransform t;
    t.translate(-cx, -cy);
    t.scale(sx, sy);
    t.translate(cx, cy);

    QPainterPath scaled = t.map(base);
    item->QGraphicsPathItem::setPath(scaled);

    item->setTransform(QTransform());

    QRectF sbr = scaled.boundingRect();
    QPointF origin = sbr.center();
    item->setTransformOriginPoint(origin);

    qreal area = sbr.width() * sbr.height();
    if (area > 1000000.0) {
        item->setCacheMode(QGraphicsItem::NoCache);
    } else {
        item->setCacheMode(QGraphicsItem::ItemCoordinateCache);
    }
}

void FormeVisualization::addPathWithLOD(const QPainterPath &path, const QPointF &pos) {
    if (!scene) {
        return;
    }

    MovablePathItem *item = new MovablePathItem;
    item->setBasePath(path);

    applySize(item, currentLargeur, currentLongueur);

    QRectF br = item->path().boundingRect();
    QPointF target = pos - br.topLeft();
    QRectF bounds = scene->sceneRect();
    target = clampPositionToScene(br, bounds, target);
    item->setPos(target);
    scene->addItem(item);
}

static bool isShapeItem(QGraphicsItem *it,
                        QGraphicsRectItem *border,
                        const QList<QGraphicsItem*> &markers) {
    if (!it) {
        return false;
    }
    if (it == border) {
        return false;
    }
    if (markers.contains(it)) {
        return false;
    }
    if (dynamic_cast<QGraphicsPathItem*>(it)) {
        return true;
    }
    return false;
}

void FormeVisualization::displayCustomShapes(const QList<QPolygonF> &shapes) {
    if (warnIfCutting()) {
        return;
    }

    removeExistingShapeItems(scene, m_sheetBorder, m_cutMarkers);

    if (m_isCustomMode) {
        m_customShapes = shapes;
    }

    if (shapes.isEmpty()) {
        emit shapesPlacedCount(0);
        return;
    }

    QPainterPath combined = combinePolygons(shapes);

    qreal W = static_cast<qreal>(currentLargeur);
    qreal H = static_cast<qreal>(currentLongueur);
    qreal cellW = W + spacing;
    qreal cellH = H + spacing;

    QRectF bounds = scene->sceneRect();
    int cols = qMax(1, int(bounds.width() / cellW));
    int rows = qMax(1, int(bounds.height() / cellH));
    int totalCells = cols * rows;
    int limit = qMin(shapeCount, totalCells);
    int placed = 0;

    for (int i = 0; i < limit; ++i) {
        int col = i % cols;
        int row = i / cols;
        QPointF cellPos;
        cellPos.setX(bounds.left() + col * cellW);
        cellPos.setY(bounds.top() + row * cellH);

        MovablePathItem *item = new MovablePathItem;
        item->setBasePath(combined);

        applySize(item, W, H);

        QRectF br = item->path().boundingRect();
        QPointF pos = cellPos - br.topLeft();
        pos = clampPositionToScene(br, bounds, pos);
        item->setPos(pos);
        QRectF r = item->mapRectToScene(br);
        if (bounds.contains(r)) {
            scene->addItem(item);
            placed += 1;
        } else {
            delete item;
        }
    }

    emit shapesPlacedCount(placed);
}

void FormeVisualization::updateDimensions(int largeur, int longueur) {
    if (!editingEnabled) {
        return;
    }
    if (warnIfCutting()) {
        return;
    }

    currentLargeur = largeur;
    currentLongueur = longueur;

    QList<QGraphicsItem*> items = scene->items();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *g = items.at(i);
        MovablePathItem *it = dynamic_cast<MovablePathItem*>(g);
        if (!it) {
            continue;
        }
        QRectF br = it->path().boundingRect();
        QPointF anchor = it->pos() + br.topLeft();
        qreal rot = it->rotation();
        applySize(it, currentLargeur, currentLongueur);
        it->setRotation(rot);
        QRectF br2 = it->path().boundingRect();
        QPointF newPos = anchor - br2.topLeft();
        it->setPos(newPos);
    }

    graphicsView->viewport()->update();
}

void FormeVisualization::setModel(ShapeModel::Type model) {
    if (!editingEnabled) {
        return;
    }
    if (warnIfCutting()) {
        return;
    }

    currentModel = model;
    redraw();
}

void FormeVisualization::setShapeCount(int count, ShapeModel::Type type, int width, int height) {
    if (!editingEnabled) {
        return;
    }
    if (warnIfCutting()) {
        return;
    }

    shapeCount = count;
    currentModel = type;
    currentLargeur = width;
    currentLongueur = height;
    redraw();
}

void FormeVisualization::setSpacing(int newSpacing) {
    if (!editingEnabled) {
        return;
    }
    if (warnIfCutting()) {
        return;
    }

    spacing = newSpacing;
    emit spacingChanged(newSpacing);
    redraw();
}

void FormeVisualization::setPredefinedMode() {
    if (warnIfCutting()) {
        return;
    }
    m_isCustomMode = false;
    m_customShapes.clear();
    emit optimizationStateChanged(false);
    redraw();
}

void FormeVisualization::setCustomMode() {
    m_isCustomMode = true;
}

void FormeVisualization::optimizePlacement() {
    redraw();
    emit optimizationStateChanged(true);
}

void FormeVisualization::optimizePlacement2() {
    optimizePlacement();
}

void FormeVisualization::optimizePlacementComplex() {
    optimizePlacement();
}

double FormeVisualization::evaluateWasteArea(const QList<QPainterPath>&, int, int) {
    return 0.0;
}

void FormeVisualization::colorPositionRed(const QPoint &p) {
    addCutMarker(p, Qt::red, true);
}

void FormeVisualization::colorPositionBlue(const QPoint &p) {
    addCutMarker(p, Qt::blue, true);
}

void FormeVisualization::moveSelectedShapes(qreal dx, qreal dy) {
    QList<QGraphicsItem*> items = scene->selectedItems();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        bool marker = m_cutMarkers.contains(it);
        bool border = (it == m_sheetBorder);
        if (!marker && !border) {
            it->moveBy(dx, dy);
        }
    }
}

void FormeVisualization::rotateSelectedShapes(qreal angle) {
    QList<QGraphicsItem*> items = scene->selectedItems();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        QGraphicsPathItem *path = dynamic_cast<QGraphicsPathItem*>(it);
        if (!path) {
            continue;
        }
        qreal a = path->rotation();
        a += angle;
        path->setRotation(a);
    }
}

void FormeVisualization::deleteSelectedShapes() {
    QList<QGraphicsItem*> items = scene->selectedItems();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        bool marker = m_cutMarkers.contains(it);
        bool border = (it == m_sheetBorder);
        if (marker || border) {
            continue;
        }
        scene->removeItem(it);
        delete it;
    }
    graphicsView->viewport()->update();
}

void FormeVisualization::addShapeBottomRight() {
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        QPainterPath combined;
        for (int i = 0; i < m_customShapes.size(); ++i) {
            combined.addPolygon(m_customShapes.at(i));
        }
        QPointF pos = scene->sceneRect().bottomRight();
        addPathWithLOD(combined, pos);
    }
}

bool FormeVisualization::validateShapes() {
    QRectF sr = scene->sceneRect();
    QList<QGraphicsItem*> items = scene->items();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        QGraphicsPathItem *path = dynamic_cast<QGraphicsPathItem*>(it);
        if (path) {
            QRectF r = path->sceneBoundingRect();
            if (!sr.contains(r)) {
                return false;
            }
        }
    }
    return true;
}

void FormeVisualization::resetAllShapeColors() {
    QList<QGraphicsItem*> items = scene->items();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        QGraphicsPathItem *shape = dynamic_cast<QGraphicsPathItem*>(it);
        if (shape) {
            applyDefaultStyle(shape);
        }
    }
    graphicsView->viewport()->update();
}

QList<QPoint> FormeVisualization::getBlackPixels() {
    QList<QPoint> pixels;
    QSize size = scene->sceneRect().size().toSize();
    if (size.isEmpty()) {
        return pixels;
    }

    QImage img(size, QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter painter(&img);
    scene->render(&painter);
    for (int x = 0; x < img.width(); ++x) {
        for (int y = 0; y < img.height(); ++y) {
            if (img.pixelColor(x, y) == Qt::black) {
                pixels.append(QPoint(x, y));
            }
        }
    }
    return pixels;
}

void FormeVisualization::startDecoupeProgress(int maxSteps) {
    progressBar->setRange(0, maxSteps);
    progressBar->setValue(0);
    progressBar->setVisible(true);
}

void FormeVisualization::updateDecoupeProgress(int step) {
    progressBar->setValue(step);
}

void FormeVisualization::endDecoupeProgress() {
    progressBar->setVisible(false);
}

void FormeVisualization::resetCutMarkers() {
    for (int i = 0; i < m_cutMarkers.size(); ++i) {
        QGraphicsItem *it = m_cutMarkers.at(i);
        scene->removeItem(it);
        delete it;
    }
    m_cutMarkers.clear();
}

void FormeVisualization::setDecoupeEnCours(bool etat) {
    m_decoupeEnCours = etat;
}

void FormeVisualization::applyLayout(const LayoutData &layout) {
    if (!m_isCustomMode) {
        return;
    }
    currentLargeur = layout.largeur;
    currentLongueur = layout.longueur;
    spacing = layout.spacing;
    int idx = 0;
    QList<QGraphicsItem*> items = scene->items(Qt::AscendingOrder);
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        QGraphicsPathItem *path = dynamic_cast<QGraphicsPathItem*>(it);
        if (!path) {
            continue;
        }
        if (idx < layout.items.size()) {
            path->setPos(layout.items[idx].x, layout.items[idx].y);
            path->setRotation(layout.items[idx].rotation);
        }
        idx += 1;
    }
}

LayoutData FormeVisualization::captureCurrentLayout(const QString &name) const {
    LayoutData layout;
    layout.name = name;
    layout.largeur = currentLargeur;
    layout.longueur = currentLongueur;
    layout.spacing = spacing;
    QList<QGraphicsItem*> items = scene->items(Qt::AscendingOrder);
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        QGraphicsPathItem *path = dynamic_cast<QGraphicsPathItem*>(it);
        if (!path) {
            continue;
        }
        LayoutItem li;
        li.x = path->pos().x();
        li.y = path->pos().y();
        li.rotation = path->rotation();
        layout.items.append(li);
    }
    return layout;
}

void FormeVisualization::setSheetSizeMm(const QSizeF &mm) {
    if (mm == m_sheetMm) {
        return;
    }
    m_sheetMm = mm;
    m_aspect = m_sheetMm.width() / m_sheetMm.height();
    QRectF rect(0.0, 0.0, m_sheetMm.width(), m_sheetMm.height());
    scene->setSceneRect(rect);
    if (m_sheetBorder) {
        m_sheetBorder->setRect(rect);
    }
    emit sheetSizeMmChanged(m_sheetMm);
    graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

int FormeVisualization::heightForWidth(int w) const {
    return int(w / m_aspect);
}

QSize FormeVisualization::sizeHint() const {
    return QSize(400, heightForWidth(400));
}

QSize FormeVisualization::minimumSizeHint() const {
    return QSize(200, heightForWidth(200));
}

void FormeVisualization::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

bool FormeVisualization::eventFilter(QObject *watched, QEvent *event) {
    if (watched == graphicsView->viewport()) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                QPointF scenePos = graphicsView->mapToScene(me->pos());
                QGraphicsItem *item = scene->itemAt(scenePos, graphicsView->transform());
                if (item && !m_cutMarkers.contains(item)) {
                    bool sel = item->isSelected();
                    item->setSelected(!sel);
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

QPainterPath FormeVisualization::bufferedPath(const QPainterPath &path, int spacing) {
    Q_UNUSED(spacing);
    return path;
}

void FormeVisualization::handleSelectionChanged() {
    QList<QGraphicsItem*> items = scene->selectedItems();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        QGraphicsPathItem *shape = dynamic_cast<QGraphicsPathItem*>(it);
        if (shape) {
            applyDefaultStyle(shape);
        }
    }
}

bool FormeVisualization::warnIfCutting() {
    return m_decoupeEnCours;
}

void FormeVisualization::addCutMarker(const QPoint &p, const QColor &color, bool center) {
    int r = 3;
    QPointF pos;
    if (center) {
        pos.setX(p.x() - r);
        pos.setY(p.y() - r);
    } else {
        pos.setX(p.x());
        pos.setY(p.y());
    }
    QGraphicsEllipseItem *it = scene->addEllipse(pos.x(), pos.y(), r * 2, r * 2,
                                                 QPen(Qt::NoPen), QBrush(color));
    it->setZValue(2.0);
    m_cutMarkers.append(it);
}

int FormeVisualization::countPlacedShapes() const {
    int count = 0;
    QList<QGraphicsItem*> items = scene->items();
    for (int i = 0; i < items.size(); ++i) {
        QGraphicsItem *it = items.at(i);
        if (dynamic_cast<QGraphicsPathItem*>(it)) {
            count += 1;
        }
    }
    return count;
}

void FormeVisualization::redraw() {
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        displayCustomShapes(m_customShapes);
    } else {
        QList<QPolygonF> polys = ShapeModel::shapePolygons(currentModel,
                                                            currentLargeur,
                                                            currentLongueur);
        displayCustomShapes(polys);
    }
}

