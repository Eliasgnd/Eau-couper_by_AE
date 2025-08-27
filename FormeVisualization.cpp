#include "FormeVisualization.h"

#include <QVBoxLayout>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QGraphicsEllipseItem>
#include <QImage>
#include <QPainter>
#include <QtMath>

// Lightweight path item that keeps its geometry inside the scene rectangle
class MovablePathItem : public QGraphicsPathItem {
public:
    using QGraphicsPathItem::QGraphicsPathItem;
protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
        if (change == ItemPositionChange && scene()) {
            QPointF newPos = value.toPointF();
            QRectF r = mapRectToScene(path().boundingRect());
            r.translate(newPos - pos());
            QRectF bounds = scene()->sceneRect();
            if (!bounds.contains(r)) {
                QPointF adj = newPos;
                if (r.left() < bounds.left())  adj.rx() += bounds.left() - r.left();
                if (r.top() < bounds.top())    adj.ry() += bounds.top() - r.top();
                if (r.right() > bounds.right()) adj.rx() -= r.right() - bounds.right();
                if (r.bottom() > bounds.bottom()) adj.ry() -= r.bottom() - bounds.bottom();
                return adj;
            }
        }
        return QGraphicsPathItem::itemChange(change, value);
    }
};

FormeVisualization::FormeVisualization(QWidget *parent)
    : QWidget(parent)
{
    scene = new QGraphicsScene(this);
    graphicsView = new QGraphicsView(scene, this);
    graphicsView->setRenderHint(QPainter::Antialiasing);
    graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    graphicsView->viewport()->installEventFilter(this);

    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(graphicsView);
    layout->addWidget(progressBar);

    QRectF rect(0,0,m_sheetMm.width(),m_sheetMm.height());
    scene->setSceneRect(rect);
    m_sheetBorder = scene->addRect(rect, QPen(Qt::gray));
    m_sheetBorder->setZValue(1);

    connect(scene, &QGraphicsScene::selectionChanged,
            this, &FormeVisualization::handleSelectionChanged);
}

void FormeVisualization::addPathWithLOD(const QPainterPath &path, const QPointF &pos)
{
    if (!scene)
        return;
    auto *item = new MovablePathItem(path);
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setFlag(QGraphicsItem::ItemIsSelectable);
    item->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    applySize(item, currentLargeur, currentLongueur);
    scene->addItem(item);
    QRectF br = item->path().boundingRect();
    item->setPos(pos - br.topLeft());
}

void FormeVisualization::displayCustomShapes(const QList<QPolygonF>& shapes)
{
    if (warnIfCutting())
        return;

    // remove existing path items except border and cut markers
    QList<QGraphicsItem*> toRemove;
    for (QGraphicsItem *it : scene->items()) {
        if (it == m_sheetBorder || m_cutMarkers.contains(it))
            continue;
        if (dynamic_cast<QGraphicsPathItem*>(it))
            toRemove.append(it);
    }
    for (QGraphicsItem *it : toRemove) { scene->removeItem(it); delete it; }

    if (m_isCustomMode)
        m_customShapes = shapes;

    if (shapes.isEmpty()) {
        emit shapesPlacedCount(0);
        graphicsView->viewport()->update();
        return;
    }

    QPainterPath combined;
    for (const QPolygonF &poly : shapes)
        combined.addPolygon(poly);

    qreal W = currentLargeur;
    qreal H = currentLongueur;
    qreal cellW = W + spacing;
    qreal cellH = H + spacing;
    int cols = qMax(1, int(scene->sceneRect().width() / cellW));
    int rows = qMax(1, int(scene->sceneRect().height() / cellH));
    int max = qMin(shapeCount, cols * rows);
    int placed = 0;

    for (int i = 0; i < max; ++i) {
        int col = i % cols;
        int row = i / cols;
        auto *item = new MovablePathItem(combined);
        item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setFlag(QGraphicsItem::ItemIsSelectable);
        item->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
        applySize(item, W, H);
        QRectF br = item->path().boundingRect();
        QPointF topLeft(col * cellW, row * cellH);
        QPointF finalPos = topLeft - br.topLeft();
        QRectF r = br.translated(finalPos);
        if (scene->sceneRect().contains(r)) {
            scene->addItem(item);
            item->setPos(finalPos);
            ++placed;
        } else {
            delete item;
        }
    }

    emit shapesPlacedCount(placed);
    graphicsView->viewport()->update();
}

void FormeVisualization::updateDimensions(int largeur, int longueur)
{
    if (!editingEnabled || warnIfCutting())
        return;
    currentLargeur = largeur;
    currentLongueur = longueur;
    for (QGraphicsItem *it : scene->items()) {
        if (auto *item = dynamic_cast<QGraphicsPathItem*>(it)) {
            QPointF anchor = item->mapRectToScene(item->path().boundingRect()).topLeft();
            qreal rot = item->rotation();
            applySize(item, currentLargeur, currentLongueur);
            item->setRotation(rot);
            QPointF newTL = item->mapRectToScene(item->path().boundingRect()).topLeft();
            item->setPos(item->pos() + (anchor - newTL));
        }
    }
    graphicsView->viewport()->update();
}

void FormeVisualization::setModel(ShapeModel::Type model)
{
    if (!editingEnabled || warnIfCutting())
        return;
    currentModel = model;
    redraw();
}

void FormeVisualization::setShapeCount(int count, ShapeModel::Type type, int width, int height)
{
    if (!editingEnabled || warnIfCutting())
        return;
    shapeCount = count;
    currentModel = type;
    currentLargeur = width;
    currentLongueur = height;
    redraw();
}

void FormeVisualization::setSpacing(int newSpacing)
{
    if (!editingEnabled || warnIfCutting())
        return;
    spacing = newSpacing;
    emit spacingChanged(newSpacing);
    redraw();
}

void FormeVisualization::setPredefinedMode()
{
    if (warnIfCutting())
        return;
    m_isCustomMode = false;
    m_customShapes.clear();
    emit optimizationStateChanged(false);
    redraw();
}

void FormeVisualization::setCustomMode()
{
    m_isCustomMode = true;
}

void FormeVisualization::optimizePlacement()
{
    redraw();
    emit optimizationStateChanged(true);
}

void FormeVisualization::optimizePlacement2()
{
    optimizePlacement();
}

void FormeVisualization::optimizePlacementComplex()
{
    optimizePlacement();
}

double FormeVisualization::evaluateWasteArea(const QList<QPainterPath>&, int, int)
{
    return 0.0;
}

void FormeVisualization::colorPositionRed(const QPoint& p)
{
    addCutMarker(p, Qt::red, true);
}

void FormeVisualization::colorPositionBlue(const QPoint& p)
{
    addCutMarker(p, Qt::blue, true);
}

void FormeVisualization::moveSelectedShapes(qreal dx, qreal dy)
{
    for (QGraphicsItem *it : scene->selectedItems())
        if (!m_cutMarkers.contains(it) && it != m_sheetBorder)
            it->moveBy(dx, dy);
}

void FormeVisualization::rotateSelectedShapes(qreal angle)
{
    for (QGraphicsItem *it : scene->selectedItems()) {
        if (auto *path = dynamic_cast<QGraphicsPathItem*>(it)) {
            QPointF center = path->path().boundingRect().center();
            path->setTransformOriginPoint(center);
            path->setRotation(path->rotation() + angle);
        }
    }
}

void FormeVisualization::deleteSelectedShapes()
{
    QList<QGraphicsItem*> items = scene->selectedItems();
    for (QGraphicsItem *it : items) {
        if (m_cutMarkers.contains(it) || it == m_sheetBorder)
            continue;
        scene->removeItem(it);
        delete it;
    }
    graphicsView->viewport()->update();
}

void FormeVisualization::addShapeBottomRight()
{
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        QPainterPath combined;
        for (const QPolygonF &poly : m_customShapes)
            combined.addPolygon(poly);
        addPathWithLOD(combined, scene->sceneRect().bottomRight());
    }
}

bool FormeVisualization::validateShapes()
{
    QRectF sr = scene->sceneRect();
    for (QGraphicsItem *it : scene->items()) {
        if (auto *path = dynamic_cast<QGraphicsPathItem*>(it))
            if (!sr.contains(path->sceneBoundingRect()))
                return false;
    }
    return true;
}

void FormeVisualization::resetAllShapeColors()
{
    for (QGraphicsItem *it : scene->items()) {
        if (auto *shape = dynamic_cast<QGraphicsPathItem*>(it)) {
            QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            pen.setCosmetic(true);
            shape->setPen(pen);
        }
    }
    graphicsView->viewport()->update();
}

QList<QPoint> FormeVisualization::getBlackPixels()
{
    QList<QPoint> pixels;
    QImage img(scene->sceneRect().size().toSize(), QImage::Format_RGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    scene->render(&p);
    for (int x = 0; x < img.width(); ++x)
        for (int y = 0; y < img.height(); ++y)
            if (img.pixelColor(x,y) == Qt::black)
                pixels.append(QPoint(x,y));
    return pixels;
}

void FormeVisualization::startDecoupeProgress(int maxSteps)
{
    progressBar->setRange(0, maxSteps);
    progressBar->setValue(0);
    progressBar->setVisible(true);
}

void FormeVisualization::updateDecoupeProgress(int step)
{
    progressBar->setValue(step);
}

void FormeVisualization::endDecoupeProgress()
{
    progressBar->setVisible(false);
}

void FormeVisualization::resetCutMarkers()
{
    for (QGraphicsItem *it : m_cutMarkers) {
        scene->removeItem(it);
        delete it;
    }
    m_cutMarkers.clear();
}

void FormeVisualization::setDecoupeEnCours(bool etat)
{
    m_decoupeEnCours = etat;
}

void FormeVisualization::applyLayout(const LayoutData &layout)
{
    if (!m_isCustomMode)
        return;
    currentLargeur = layout.largeur;
    currentLongueur = layout.longueur;
    spacing = layout.spacing;
    int idx = 0;
    for (QGraphicsItem *it : scene->items(Qt::AscendingOrder)) {
        if (auto *path = dynamic_cast<QGraphicsPathItem*>(it)) {
            if (idx < layout.items.size()) {
                path->setPos(layout.items[idx].x, layout.items[idx].y);
                path->setRotation(layout.items[idx].rotation);
            }
            ++idx;
        }
    }
}

LayoutData FormeVisualization::captureCurrentLayout(const QString &name) const
{
    LayoutData layout; layout.name = name;
    layout.largeur = currentLargeur;
    layout.longueur = currentLongueur;
    layout.spacing = spacing;
    for (QGraphicsItem *it : scene->items(Qt::AscendingOrder)) {
        if (auto *path = dynamic_cast<QGraphicsPathItem*>(it)) {
            LayoutItem li; li.x = path->pos().x(); li.y = path->pos().y();
            li.rotation = path->rotation();
            layout.items.append(li);
        }
    }
    return layout;
}

void FormeVisualization::setSheetSizeMm(const QSizeF& mm)
{
    if (mm == m_sheetMm)
        return;
    m_sheetMm = mm;
    m_aspect = m_sheetMm.width() / m_sheetMm.height();
    QRectF rect(0,0,m_sheetMm.width(), m_sheetMm.height());
    scene->setSceneRect(rect);
    if (m_sheetBorder)
        m_sheetBorder->setRect(rect);
    emit sheetSizeMmChanged(m_sheetMm);
    graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

int FormeVisualization::heightForWidth(int w) const
{
    return int(w / m_aspect);
}

QSize FormeVisualization::sizeHint() const
{
    return QSize(400, heightForWidth(400));
}

QSize FormeVisualization::minimumSizeHint() const
{
    return QSize(200, heightForWidth(200));
}

void FormeVisualization::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    graphicsView->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

bool FormeVisualization::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == graphicsView->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        auto *me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            QPointF scenePos = graphicsView->mapToScene(me->pos());
            QGraphicsItem *item = scene->itemAt(scenePos, graphicsView->transform());
            if (item && !m_cutMarkers.contains(item))
                item->setSelected(!item->isSelected());
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

QPainterPath FormeVisualization::bufferedPath(const QPainterPath &path, int spacing)
{
    Q_UNUSED(spacing);
    return path;
}

void FormeVisualization::handleSelectionChanged()
{
    for (QGraphicsItem *item : scene->selectedItems())
        if (auto shape = dynamic_cast<QGraphicsPathItem*>(item)) {
            QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
            pen.setCosmetic(true);
            shape->setPen(pen);
        }
}

bool FormeVisualization::warnIfCutting()
{
    return m_decoupeEnCours;
}

void FormeVisualization::addCutMarker(const QPoint& p, const QColor& color, bool center)
{
    constexpr int r = 3;
    QPointF pos = center ? QPointF(p.x()-r, p.y()-r) : QPointF(p);
    auto *item = scene->addEllipse(pos.x(), pos.y(), r*2, r*2,
                                   QPen(Qt::NoPen), QBrush(color));
    item->setZValue(2);
    m_cutMarkers.append(item);
}

int FormeVisualization::countPlacedShapes() const
{
    int count = 0;
    for (QGraphicsItem *it : scene->items())
        if (dynamic_cast<QGraphicsPathItem*>(it))
            ++count;
    return count;
}

void FormeVisualization::redraw()
{
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        displayCustomShapes(m_customShapes);
    } else {
        QList<QPolygonF> polys = ShapeModel::shapePolygons(currentModel,
                                                            currentLargeur,
                                                            currentLongueur);
        displayCustomShapes(polys);
    }
}

void FormeVisualization::applySize(QGraphicsPathItem *item, qreal W, qreal H)
{
    if (!item)
        return;
    QPainterPath p = item->path();
    QRectF br = p.boundingRect();
    if (br.width() <= 0 || br.height() <= 0)
        return;
    QPointF c = br.center();
    QTransform t;
    t.translate(c.x(), c.y());
    t.scale(W / br.width(), H / br.height());
    t.translate(-c.x(), -c.y());
    p = t.map(p);
    item->setPath(p);
    item->setTransform(QTransform());
    QPen pen(Qt::black, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    pen.setCosmetic(true);
    item->setPen(pen);
    item->setBrush(Qt::NoBrush);
    QRectF br2 = item->path().boundingRect();
    item->setTransformOriginPoint(br2.center());
    qreal area = br2.width() * br2.height();
    if (area > 1e6)
        item->setCacheMode(QGraphicsItem::NoCache);
    else
        item->setCacheMode(QGraphicsItem::ItemCoordinateCache);
}

