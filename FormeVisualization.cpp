#include "FormeVisualization.h"
#include "qtimer.h"
#include <QVBoxLayout>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QResizeEvent>
#include <QDebug>
#include <QtMath>
#include <QTransform>
#include <QApplication>
#include <QPainterPathStroker>  // N'oubliez pas d'inclure ce header
#include <QMessageBox>

FormeVisualization::FormeVisualization(QWidget *parent)
    : QWidget(parent),
    currentModel(ShapeModel::Type::Circle),
    currentLargeur(0),
    currentLongueur(0),
    shapeCount(0),
    spacing(0),
    m_isCustomMode(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    graphicsView = new QGraphicsView(this);
    scene = new QGraphicsScene(this);

    const int drawingWidth = 600;
    const int drawingHeight = 400;
    scene->setSceneRect(0, 0, drawingWidth, drawingHeight);

    const int viewWidth = 600;
    const int viewHeight = 400;
    graphicsView->setFixedSize(viewWidth, viewHeight);
    graphicsView->setFrameStyle(QFrame::NoFrame);
    graphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    graphicsView->setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    graphicsView->setScene(scene);
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    progressBar = new QProgressBar(this);
    progressBar->setValue(0);
    progressBar->setVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid #00BCD4;"
        "    border-radius: 5px;"
        "    background: #f3f3f3;"
        "    text-align: center;"
        "    font-size: 12px;"
        "    color: black;"
        "}"
        "QProgressBar::chunk {"
        "    background: #00BCD4;"
        "    width: 10px;"
        "}"
        );

    // Note : Le compteur est désormais géré dans MainWindow, pas dans ce widget
    layout->addWidget(graphicsView);
    layout->addWidget(progressBar);
    setLayout(layout);

    redraw();
}

QPainterPath FormeVisualization::bufferedPath(const QPainterPath &path, int spacing)
{
    // Si aucun espacement n'est demandé, renvoyer le chemin original
    if (spacing <= 0)
        return path;

    QPainterPathStroker stroker;
    // Ici, spacing est interprété comme la largeur totale désirée.
    // Le contour généré s'étend d'environ spacing/2 de chaque côté.
    stroker.setWidth(spacing);

    QPainterPath strokePath = stroker.createStroke(path);
    // Le buffer correspond à l'union du chemin original et de son contour élargi
    return path.united(strokePath);
}

void FormeVisualization::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void FormeVisualization::setModel(ShapeModel::Type model)
{
    if (!editingEnabled)
        return;
    if (m_decoupeEnCours)
        return;

    currentModel = model;
    setPredefinedMode();
    redraw();
    update();
}

void FormeVisualization::updateDimensions(int largeur, int longueur)
{
    if (!editingEnabled)
        return;
    if (m_decoupeEnCours) {
        qDebug() << "[DEBUG] Message d’erreur : découpe déjà en cours dans updateDimensions";
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();        return;
    }
    currentLargeur = largeur;
    currentLongueur = longueur;
    if (m_isCustomMode && !m_customShapes.isEmpty())
        displayCustomShapes(m_customShapes);
    else
        redraw();
}

void FormeVisualization::setShapeCount(int count, ShapeModel::Type type, int width, int height)
{
    if (!editingEnabled)
        return;
    if (m_decoupeEnCours) {
        qDebug() << "[DEBUG] Message d’erreur : découpe déjà en cours dans setShapeCount";
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
        return;
    }

    shapeCount = count;
    currentModel = type;
    currentLargeur = width;
    currentLongueur = height;
    if (m_isCustomMode && !m_customShapes.isEmpty())
        displayCustomShapes(m_customShapes);
    else
        redraw();
}

void FormeVisualization::setSpacing(int newSpacing)
{
    if (!editingEnabled)
        return;
    if (m_decoupeEnCours) {
        qDebug() << "[DEBUG] Message d’erreur : découpe déjà en cours dans setSpacing";
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
        return;
    }

    spacing = newSpacing;
    emit spacingChanged(newSpacing);
    if (m_isCustomMode && !m_customShapes.isEmpty())
        displayCustomShapes(m_customShapes);
    else
        redraw();
}

void FormeVisualization::setPredefinedMode()
{
    qDebug() << "[DEBUG] setPredefinedMode() appelé";

    if (m_decoupeEnCours) {
        qDebug() << "[DEBUG] Message d’erreur : découpe déjà en cours dans setPredefinedMode";
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();        return;
    }

    m_isCustomMode = false;
    m_customShapes.clear();
    emit optimizationStateChanged(false);
    redraw();
}

/*
   ****** OPTIMIZE PLACEMENT 1 (avec caching et test rapide) ******
*/
void FormeVisualization::optimizePlacement() {
    if (m_decoupeEnCours) {
        qDebug() << "[DEBUG] Message d’erreur : découpe déjà en cours dans optimizePlacement";
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();        return;
    }

    // Remise à zéro de l'espacement
    setSpacing(0);

    emit optimizationStateChanged(true);
    scene->clear();
    progressBar->setVisible(true);
    progressBar->setValue(0);

    const int drawingWidth = 600;
    const int drawingHeight = 400;
    QRectF containerRect(0, 0, drawingWidth, drawingHeight);

    double sizeFactorX = static_cast<double>(drawingWidth) / 600;
    double sizeFactorY = static_cast<double>(drawingHeight) / 400;
    int adaptedLargeur = currentLargeur * sizeFactorX;
    int adaptedLongueur = currentLongueur * sizeFactorY;

    QPainterPath prototypePath;
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        for (const QPolygonF &poly : m_customShapes)
            prototypePath.addPolygon(poly);
        prototypePath = prototypePath.simplified();
        QRectF customBounds = prototypePath.boundingRect();
        double scaleX = (customBounds.width() > 0) ? (adaptedLargeur / customBounds.width()) : 1.0;
        double scaleY = (customBounds.height() > 0) ? (adaptedLongueur / customBounds.height()) : 1.0;
        QTransform scaleTransform;
        scaleTransform.scale(scaleX, scaleY);
        prototypePath = scaleTransform.map(prototypePath);
    } else {
        QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(currentModel, adaptedLargeur, adaptedLongueur);
        if (shapesList.isEmpty()) {
            //qDebug() << "Erreur : Aucun prototype de forme disponible.";
            progressBar->setVisible(false);
            return;
        }
        QGraphicsItem *prototype = shapesList.first();
        if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(prototype))
            prototypePath = pathItem->path();
        else if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(prototype))
            prototypePath.addRect(rectItem->rect());
        else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(prototype))
            prototypePath.addEllipse(ellipseItem->rect());
        else if (auto polyItem = dynamic_cast<QGraphicsPolygonItem*>(prototype))
            prototypePath.addPolygon(polyItem->polygon());
        else {
            //qDebug() << "Erreur : Type de forme inconnu pour l'optimisation.";
            progressBar->setVisible(false);
            return;
        }
    }

    QRectF protoRect = prototypePath.boundingRect();
    QPointF center = protoRect.center();

    const int step = 5;
    int count = shapeCount;
    QList<int> angles = {0, 180, 90};

    QList<QPainterPath> placedPaths;
    int shapesPlaced = 0;
    int totalPositions = ((drawingWidth / step) + 1) * ((drawingHeight / step) + 1);
    progressBar->setMinimum(0);
    progressBar->setMaximum(totalPositions);
    int progressCounter = 0;

    for (int y = 0; y <= drawingHeight && shapesPlaced < count; y += step) {
        for (int x = 0; x <= drawingWidth && shapesPlaced < count; x += step) {
            progressCounter++;
            progressBar->setValue(progressCounter);
            qApp->processEvents();

            for (int angle : angles) {
                QTransform trans;
                trans.translate(x, y);
                trans.translate(center.x(), center.y());
                trans.rotate(angle);
                trans.translate(-center.x(), -center.y());
                QPainterPath candidate = trans.map(prototypePath);

                QRectF candRect = candidate.boundingRect();
                QTransform adjust;
                adjust.translate(x - candRect.x(), y - candRect.y());
                candidate = adjust.map(candidate);
                candidate.closeSubpath();

                if (!containerRect.contains(candidate.boundingRect()))
                    continue;

                bool collision = false;
                for (const QPainterPath &existing : placedPaths) {
                    if (candidate.intersects(existing)) {
                        collision = true;
                        break;
                    }
                }
                if (!collision) {
                    QGraphicsPathItem *item = new QGraphicsPathItem(candidate);
                    item->setPen(QPen(Qt::black, 1));
                    item->setBrush(Qt::NoBrush);
                    item->setFlag(QGraphicsItem::ItemIsMovable, true);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
                    scene->addItem(item);
                    placedPaths.append(candidate);
                    shapesPlaced++;
                    break;
                }
            }
            if (shapesPlaced >= count)
                break;
        }
    }

    //qDebug() << "Formes placées:" << shapesPlaced;
    emit shapesPlacedCount(shapesPlaced);
    emit optimizationStateChanged(true);
    progressBar->setVisible(false);
}

void FormeVisualization::optimizePlacement2() {
    if (m_decoupeEnCours) {
        qDebug() << "[DEBUG] Message d’erreur : découpe déjà en cours dans optimizePlacement2";
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();        return;
    }

    // Remise à zéro de l'espacement
    setSpacing(0);
    scene->clear();
    graphicsView->update();

    progressBar->setVisible(true);
    progressBar->setValue(0);

    const int drawingWidth = 600;
    const int drawingHeight = 400;
    const int viewWidth = 600;
    const int viewHeight = 400;
    double sizeFactorX = static_cast<double>(drawingWidth) / viewWidth;
    double sizeFactorY = static_cast<double>(drawingHeight) / viewHeight;

    int adaptedLargeur = currentLargeur * sizeFactorX;
    int adaptedLongueur = currentLongueur * sizeFactorY;

    QPainterPath prototypePath;
    if (m_isCustomMode && !m_customShapes.isEmpty()) {
        for (const QPolygonF &poly : m_customShapes)
            prototypePath.addPolygon(poly);
        QRectF customBounds = prototypePath.boundingRect();
        double scaleX = (customBounds.width() > 0) ? (adaptedLargeur / customBounds.width()) : 1.0;
        double scaleY = (customBounds.height() > 0) ? (adaptedLongueur / customBounds.height()) : 1.0;
        QTransform scaleTransform;
        scaleTransform.scale(scaleX, scaleY);
        prototypePath = scaleTransform.map(prototypePath);
    } else {
        QList<QGraphicsItem*> shapesList = ShapeModel::generateShapes(currentModel, adaptedLargeur, adaptedLongueur);
        if (shapesList.isEmpty()) {
            //qDebug() << "Erreur : Aucun prototype de forme disponible.";
            return;
        }
        QGraphicsItem *prototype = shapesList.first();
        if (auto pathItem = dynamic_cast<QGraphicsPathItem*>(prototype))
            prototypePath = pathItem->path();
        else if (auto rectItem = dynamic_cast<QGraphicsRectItem*>(prototype))
            prototypePath.addRect(rectItem->rect());
        else if (auto ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(prototype))
            prototypePath.addEllipse(ellipseItem->rect());
        else if (auto polyItem = dynamic_cast<QGraphicsPolygonItem*>(prototype))
            prototypePath.addPolygon(polyItem->polygon());
        else {
            //qDebug() << "Erreur : Type de forme inconnu pour l'optimisation.";
            return;
        }
    }

    // Normalisation du prototype pour que son origine soit (0,0)
    QRectF bounds = prototypePath.boundingRect();
    QTransform normTransform;
    normTransform.translate(-bounds.x(), -bounds.y());
    prototypePath = normTransform.map(prototypePath);
    prototypePath.closeSubpath();

    QList<QPainterPath> placedPaths;
    int shapesPlaced = 0;
    int count = shapeCount;
    const int step = 5;
    bool finished = false;
    QRectF containerRect(0, 0, drawingWidth, drawingHeight);

    int totalPositions = ((drawingWidth / step) + 1) * ((drawingHeight / step) + 1);
    progressBar->setMinimum(0);
    progressBar->setMaximum(totalPositions);
    int progressCounter = 0;

    for (int y = 0; y <= drawingHeight && !finished; y += step) {
        for (int x = 0; x <= drawingWidth && !finished; x += step) {
            progressCounter++;
            progressBar->setValue(progressCounter);
            qApp->processEvents();

            QTransform candidateTransform;
            candidateTransform.translate(x, y);
            QPainterPath candidate = candidateTransform.map(prototypePath);
            candidate.closeSubpath();

            if (!containerRect.contains(candidate.boundingRect()))
                continue;

            bool collision = false;
            for (const QPainterPath &existing : placedPaths) {
                if (candidate.intersects(existing)) {
                    collision = true;
                    break;
                }
            }
            if (!collision) {
                QGraphicsPathItem *item = new QGraphicsPathItem(candidate);
                item->setPen(QPen(Qt::black, 1));
                item->setBrush(Qt::NoBrush);
                item->setFlag(QGraphicsItem::ItemIsMovable, true);
                item->setFlag(QGraphicsItem::ItemIsSelectable, true);
                scene->addItem(item);
                placedPaths.append(candidate);
                shapesPlaced++;
                if (shapesPlaced >= count) {
                    finished = true;
                    break;
                }
            }
        }
    }
    //qDebug() << "Formes optimisées placées:" << shapesPlaced;
    emit shapesPlacedCount(shapesPlaced);
    progressBar->setVisible(false);
}

void FormeVisualization::redraw()
{
    scene->clear();
    m_isCustomMode = false;
    m_customShapes.clear();

    if (shapeCount <= 0)
        return;

    const int drawingWidth = 600;
    const int drawingHeight = 400;
    const int viewWidth = 600;
    const int viewHeight = 400;

    double sizeFactorX = static_cast<double>(drawingWidth) / viewWidth;
    double sizeFactorY = static_cast<double>(drawingHeight) / viewHeight;

    int adaptedLargeur = currentLargeur * sizeFactorX;
    int adaptedLongueur = currentLongueur * sizeFactorY;

    QList<QGraphicsItem*> shapes = ShapeModel::generateShapes(currentModel, adaptedLargeur, adaptedLongueur);
    if (shapes.isEmpty()) {
        //qDebug() << "Erreur : generateShapes a retourné une liste vide.";
        emit shapesPlacedCount(0);
        return;
    }
    QGraphicsItem *prototype = shapes.first();

    QRectF bounds = prototype->boundingRect();
    qreal effectiveWidth = bounds.width();
    qreal effectiveHeight = bounds.height();
    if (effectiveWidth <= 0 || effectiveHeight <= 0) {
        //qDebug() << "Erreur : dimensions invalides pour la forme.";
        emit shapesPlacedCount(0);
        return;
    }
    qreal cellWidth = effectiveWidth + spacing;
    qreal cellHeight = effectiveHeight + spacing;

    int maxCols = qFloor(drawingWidth / cellWidth);
    int maxRows = qFloor(drawingHeight / cellHeight);
    int totalCells = maxCols * maxRows;
    int shapesToPlace = qMin(shapeCount, totalCells);

    //qDebug() << "Grille :" << maxCols << "colonnes x" << maxRows << "lignes ="
    //         << totalCells << "cellules. Placement de" << shapesToPlace << "formes.";

    for (int i = 0; i < shapesToPlace; ++i) {
        int col = i % maxCols;
        int row = i / maxCols;
        qreal xPos = col * cellWidth;
        qreal yPos = row * cellHeight;

        QGraphicsItem *shapeCopy = nullptr;
        if (auto rect = dynamic_cast<QGraphicsRectItem*>(prototype))
            shapeCopy = new QGraphicsRectItem(rect->rect());
        else if (auto ellipse = dynamic_cast<QGraphicsEllipseItem*>(prototype))
            shapeCopy = new QGraphicsEllipseItem(ellipse->rect());
        else if (auto polygon = dynamic_cast<QGraphicsPolygonItem*>(prototype))
            shapeCopy = new QGraphicsPolygonItem(polygon->polygon());
        else if (auto path = dynamic_cast<QGraphicsPathItem*>(prototype))
            shapeCopy = new QGraphicsPathItem(path->path());

        if (!shapeCopy) {
            //qDebug() << "Erreur : Impossible de copier la forme !";
            continue;
        }
        QPointF offsetCorrection(-bounds.x(), -bounds.y());
        shapeCopy->setPos(xPos + offsetCorrection.x(), yPos + offsetCorrection.y());
        shapeCopy->setFlag(QGraphicsItem::ItemIsMovable, true);
        shapeCopy->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem(shapeCopy);
    }
    //qDebug() << "Formes prédéfinies placées:" << shapesToPlace;
    emit shapesPlacedCount(shapesToPlace);
}

void FormeVisualization::displayCustomShapes(const QList<QPolygonF>& shapes)
{
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
        return;
    }

    scene->clear();

    if (shapes.isEmpty()) {
        //qDebug() << "Aucune forme personnalisée à afficher.";
        return;
    }

    m_isCustomMode = true;
    m_customShapes = shapes;

    const int drawingWidth = 600;
    const int drawingHeight = 400;
    const int viewWidth = 600;
    const int viewHeight = 400;
    double sizeFactorX = static_cast<double>(drawingWidth) / viewWidth;
    double sizeFactorY = static_cast<double>(drawingHeight) / viewHeight;

    QPainterPath combinedPath;
    for (const QPolygonF &poly : shapes) {
        combinedPath.addPolygon(poly);
    }
    QRectF polyBounds = combinedPath.boundingRect();
    if (polyBounds.width() <= 0 || polyBounds.height() <= 0) {
        //qDebug() << "Erreur : dimensions invalides pour le dessin custom combiné.";
        return;
    }

    qreal desiredWidthInScene = currentLargeur * sizeFactorX;
    qreal desiredHeightInScene = currentLongueur * sizeFactorY;
    qreal scaleX = desiredWidthInScene / polyBounds.width();
    qreal scaleY = desiredHeightInScene / polyBounds.height();
    QTransform transform;
    transform.scale(scaleX, scaleY);
    QPainterPath scaledPath = transform.map(combinedPath);
    QRectF scaledBounds = scaledPath.boundingRect();

    qreal cellWidth = scaledBounds.width() + spacing;
    qreal cellHeight = scaledBounds.height() + spacing;
    int maxCols = qFloor(drawingWidth / cellWidth);
    int maxRows = qFloor(drawingHeight / cellHeight);
    int totalCells = maxCols * maxRows;
    int shapesToPlace = qMin(shapeCount, totalCells);

    for (int i = 0; i < shapesToPlace; ++i) {
        int col = i % maxCols;
        int row = i / maxCols;
        qreal xPos = col * cellWidth;
        qreal yPos = row * cellHeight;

        QGraphicsPathItem *item = new QGraphicsPathItem(scaledPath);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setFlag(QGraphicsItem::ItemIsMovable, true);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        QPointF offset = -scaledBounds.topLeft();
        item->setPos(xPos + offset.x(), yPos + offset.y());
        scene->addItem(item);
    }

    //qDebug() << "Affichage de" << shapesToPlace << "copies du dessin custom dans FormeVisualization.";
    emit shapesPlacedCount(shapesToPlace);
}

void FormeVisualization::moveSelectedShapes(qreal dx, qreal dy)
{
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
        return;
    }
    // Parcours de tous les items sélectionnés dans la scène
    for (QGraphicsItem *item : scene->selectedItems()) {
        item->moveBy(dx, dy);
    }
}

void FormeVisualization::rotateSelectedShapes(qreal angleDelta)
{
    if (m_decoupeEnCours) {
        QMessageBox* msg = new QMessageBox(QMessageBox::Warning,
                                           "Découpe en cours",
                                           "Impossible de modifier les paramètres ou la forme pendant la découpe.",
                                           QMessageBox::Ok,
                                           this);
        msg->setModal(false);
        msg->show();
        return;
    }
    for (QGraphicsItem *item : scene->selectedItems()) {
        item->setTransformOriginPoint(item->boundingRect().center());   // ← NOUVEAU
        item->setRotation(item->rotation() + angleDelta);
    }
}

void FormeVisualization::setCustomMode() {
    m_isCustomMode = true;
    emit optimizationStateChanged(false);
}

// -----------------------------------------------------------------------------
// Ajout d’un point rouge
// -----------------------------------------------------------------------------
void FormeVisualization::colorPositionRed(const QPoint& p)
{
    auto *dot = scene->addEllipse(p.x()-1, p.y()-1, 2, 2,
                                  QPen(Qt::NoPen), QBrush(Qt::red));
    m_cutMarkers << dot;
    graphicsView->viewport()->update();
}

// -----------------------------------------------------------------------------
// Ajout d’un point bleu
// -----------------------------------------------------------------------------
void FormeVisualization::colorPositionBlue(const QPoint& p)
{
    auto *dot = scene->addEllipse(p.x(), p.y(), 2, 2,
                                  QPen(Qt::NoPen), QBrush(Qt::blue));
    m_cutMarkers << dot;
    graphicsView->viewport()->update();
}

// -----------------------------------------------------------------------------
// Remise à zéro des marqueurs de découpe
// -----------------------------------------------------------------------------
void FormeVisualization::resetCutMarkers()
{
    for (auto *item : std::as_const(m_cutMarkers)) {
        scene->removeItem(item);
        delete item;
    }
    m_cutMarkers.clear();
}

QGraphicsScene* FormeVisualization::getScene() const {
    return scene;
}

// ======================================================================================================================
//Balaye la scène pour trouver les pixels noirs des formes et ainsi avoir les coordonnées de chaque pixel dans une liste
// ======================================================================================================================



QList<QPoint> FormeVisualization::getBlackPixels() {

    QList<QPoint> pixelsNoirs;

    // Convertir la scène en image
    QImage image(scene->sceneRect().size().toSize(), QImage::Format_RGB32);
    image.fill(Qt::white);  // Fond blanc pour éviter les erreurs
    QPainter painter(&image);
    scene->render(&painter);

    // Parcourir chaque pixel de l'image

    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            if (image.pixelColor(x, y) == Qt::black) {
                pixelsNoirs.append(QPoint(x, y));
                //qDebug() << "Pixel noir à :" << QPoint(x, y);  //simplement pour visualiser les coordonnées des pixels noirs.
            }
        }

    }

    return pixelsNoirs;
}

// Démarre la barre de progression avec un maximum de étapes
void FormeVisualization::startDecoupeProgress(int maxSteps) {
    progressBar->setMaximum(maxSteps);
    progressBar->setValue(0);
    progressBar->setVisible(true);
}

// Met à jour la valeur courante de la barre de progression
void FormeVisualization::updateDecoupeProgress(int currentStep) {
    progressBar->setValue(currentStep);
}

// Cache la barre de progression en fin de découpe
void FormeVisualization::endDecoupeProgress() {
    progressBar->setVisible(false);
}

void FormeVisualization::setEditingEnabled(bool enabled)
{
    editingEnabled = enabled;
}

bool FormeVisualization::isEditingEnabled() const
{
    return editingEnabled;
}

void FormeVisualization::setDecoupeEnCours(bool etat)
{
    m_decoupeEnCours = etat;
    // Interdire ou autoriser le déplacement manuel des items
    for (QGraphicsItem *item : scene->items()) {
        item->setFlag(QGraphicsItem::ItemIsMovable, !etat);
    }
}

bool FormeVisualization::isDecoupeEnCours() const
{
    return m_decoupeEnCours;
}

QGraphicsView* FormeVisualization::getGraphicsView() const
{
    return graphicsView;
}
