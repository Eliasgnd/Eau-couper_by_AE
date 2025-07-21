#include "inventaire.h"
#include "ui_inventaire.h"
#include "ShapeModel.h"
#include "MainWindow.h"
#include "Language.h"
#include "ScreenUtils.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QPainterPath>

#include <QFile>
#include <QDir>
#include <QStandardPaths>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include "draggablelistwidget.h"
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
#include <QSvgRenderer>

// -----------------------------------------------------------------------------
// Static instance (singleton)
// -----------------------------------------------------------------------------
Inventaire* Inventaire::instance = nullptr;

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------
Inventaire::Inventaire(QWidget *parent)
    : QWidget(parent), ui(new Ui::Inventaire)
{
    ui->setupUi(this);

    loadCustomShapes();

    if (m_baseShapeOrder.isEmpty()) {
        m_baseShapeOrder = {ShapeModel::Type::Circle,
                            ShapeModel::Type::Rectangle,
                            ShapeModel::Type::Triangle,
                            ShapeModel::Type::Star,
                            ShapeModel::Type::Heart};
    }

    updateTranslations(currentLanguage);

    // Place window on secondary screen if available
    ScreenUtils::placeOnSecondaryScreen(this);

    // Navigation
    connect(ui->buttonMenu, &QPushButton::clicked, this, &Inventaire::goToMainWindow);

    // Search bar
    connect(ui->searchBar, &QLineEdit::textChanged, this, &Inventaire::onSearchTextChanged);
    connect(ui->buttonClearSearch, &QPushButton::clicked, this, &Inventaire::onClearSearchClicked);
    connect(ui->buttonNewFolder, &QPushButton::clicked, this, &Inventaire::onCreateFolderClicked);

    // Initial display
    displayShapes();

}

Inventaire::~Inventaire()
{
    qDebug() << "Fermeture de l'Inventaire (l'instance reste vivante via le singleton)";
    delete ui;
}

// -----------------------------------------------------------------------------
// Singleton accessor
// -----------------------------------------------------------------------------
Inventaire* Inventaire::getInstance()
{
    if (!instance) {
        qDebug() << "Création de l'instance Inventaire";
        instance = new Inventaire();
    }
    return instance;
}


QPixmap Inventaire::renderColoredSvg(const QString &filePath, const QColor &color, const QSize &size)
{
    QSvgRenderer renderer(filePath);
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Crée un masque noir/blanc à partir du SVG
    QImage mask(size, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    QPainter maskPainter(&mask);
    renderer.render(&maskPainter);
    maskPainter.end();

    // Colore la forme
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(pixmap.rect(), color);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.drawImage(0, 0, mask);
    painter.end();

    return pixmap;
}


// -----------------------------------------------------------------------------
// Navigation helpers
// -----------------------------------------------------------------------------
void Inventaire::goToMainWindow()
{
    hide();
    MainWindow::getInstance()->showFullScreen();
}

// -----------------------------------------------------------------------------
// UI – Build the inventory grid
// -----------------------------------------------------------------------------
void Inventaire::displayShapes(const QString &filter /* = QString() */)
{
    if (!ui->scrollAreaInventaire)
        return;

    // Clear previous content, if any
    if (QWidget *oldWidget = ui->scrollAreaInventaire->widget()) {
        ui->scrollAreaInventaire->takeWidget();
        oldWidget->deleteLater();
    }

    inFolderView = false;
    currentFolder.clear();

    DraggableListWidget *listWidget = new DraggableListWidget();
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setMovement(QListView::Snap);
    listWidget->setSpacing(25);

    ui->scrollAreaInventaire->setWidget(listWidget);
    ui->scrollAreaInventaire->setWidgetResizable(true);

    const QString f = filter.trimmed().toLower();

    for (const InventaireFolder &folder : m_folders) {
        if (!folder.parentFolder.isEmpty())
            continue;
        if (!filter.isEmpty() && !folder.name.toLower().contains(f))
            continue;

        QFrame *card = createFolderCard(folder.name);
        QListWidgetItem *item = new QListWidgetItem(listWidget);
        item->setSizeHint(card->size());
        item->setData(Qt::UserRole, 0);
        item->setData(Qt::UserRole + 1, folder.name);
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        listWidget->addItem(item);
        listWidget->setItemWidget(item, card);
    }

    // ---------------------------------------------------------------------
    // Built-in shapes
    // ---------------------------------------------------------------------
    for (ShapeModel::Type type : m_baseShapeOrder) {
        QString name = baseShapeName(type, currentLanguage);
        if (!f.isEmpty() && !name.toLower().contains(f))
            continue;
        const QString folder = m_baseShapeFolders.value(type);
        if (!folder.isEmpty() && !inFolderView)
            continue;
        if (inFolderView && folder != currentFolder)
            continue;

        QFrame *frame = createBaseShapeCard(type, name);
        QListWidgetItem *item = new QListWidgetItem(listWidget);
        item->setSizeHint(frame->size());
        item->setData(Qt::UserRole, 1);
        item->setData(Qt::UserRole + 1, static_cast<int>(type));
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        listWidget->addItem(item);
        listWidget->setItemWidget(item, frame);
    }

    // ---------------------------------------------------------------------
    // Custom shapes (filtered)
    // ---------------------------------------------------------------------
    for (int i = 0; i < m_customShapes.size(); ++i) {
        const CustomShapeData &data = m_customShapes[i];
        if (!data.folder.isEmpty() && !inFolderView)
            continue;
        if (inFolderView && data.folder != currentFolder)
            continue;
        if (!filter.isEmpty() && !data.name.toLower().contains(f))
            continue;

        QFrame *customFrame = addCustomShapeToGrid(i);
        QListWidgetItem *item = new QListWidgetItem(listWidget);
        item->setSizeHint(customFrame->size());
        item->setData(Qt::UserRole, 2);
        item->setData(Qt::UserRole + 1, data.name);
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        listWidget->addItem(item);
        listWidget->setItemWidget(item, customFrame);
    }
    ui->buttonClearSearch->setVisible(!inFolderView);
    inFolderView = false;
    currentFolder.clear();
    connect(listWidget, &QListWidget::itemClicked, this, &Inventaire::onItemClicked);
    connect(listWidget->model(), &QAbstractItemModel::rowsMoved, this, [this, listWidget](const QModelIndex&, int, int, const QModelIndex&, int){ applyReorderFromList(listWidget); });
    connect(listWidget, &DraggableListWidget::dragStarted, this, [this](){ m_dragInProgress = true; });
    connect(listWidget, &DraggableListWidget::dragFinished, this, [this](){ m_dragInProgress = false; });
    ui->buttonMenu->setVisible(true);
    update();
}

// -----------------------------------------------------------------------------
// Build a single custom shape thumbnail
// -----------------------------------------------------------------------------
QFrame* Inventaire::addCustomShapeToGrid(int index)
{
    if (index < 0 || index >= m_customShapes.size())
        return nullptr;

    const CustomShapeData &data = m_customShapes.at(index);

    // Build a path containing every polygon of the shape
    QPainterPath path;
    for (const QPolygonF &poly : data.polygons)
        path.addPolygon(poly);

    auto *scene = new QGraphicsScene();
    auto *item  = new QGraphicsPathItem(path);
    item->setPen(QPen(Qt::black, 4));
    item->setBrush(Qt::NoBrush);
    scene->addItem(item);
    scene->setSceneRect(item->boundingRect().adjusted(-5, -5, 5, 5));

    auto *view = new QGraphicsView(scene);
    view->setFixedSize(120, 120);
    view->setStyleSheet("background-color: white;");
    view->setAlignment(Qt::AlignCenter);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    auto *frame = new QFrame();
    frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
    frame->setFixedSize(150, 220);

    auto *label = new QLabel(data.name);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: black; font-size: 16px;");

    // ――― menu button (rename / delete) ―――
    auto *menuButton = new QPushButton("…");
    menuButton->setFixedSize(25, 25);
    menuButton->setStyleSheet("border: none; font-size: 14px;");

    auto *menu = new QMenu(menuButton);
    QAction *renameAction = menu->addAction(currentLanguage == Language::French ? "Renommer"  : "Rename");
    QAction *deleteAction = menu->addAction(currentLanguage == Language::French ? "Supprimer" : "Delete");

    const QString currentFolderName = m_customShapes[index].folder;

    if (!currentFolderName.isEmpty()) {
        QAction *removeFromFolder = menu->addAction("❌ Retirer du dossier");
        connect(removeFromFolder, &QAction::triggered, this, [this, index]() {
            if (index < 0 || index >= m_customShapes.size())
                return;

            QString oldFolder = m_customShapes[index].folder;
            // Cherche le dossier parent
            QString parentFolder;
            for (const InventaireFolder &f : m_folders) {
                if (f.name == oldFolder) {
                    parentFolder = f.parentFolder;
                    break;
                }
            }

            // Réaffecte le dossier parent à la forme (ou rien si on est à la racine)
            m_customShapes[index].folder = parentFolder;


            saveCustomShapes();

            if (parentFolder.isEmpty()) {
                displayShapes();  // retour à la racine
            } else {
                displayShapesInFolder(parentFolder, ui->searchBar->text());  // retour au dossier parent
            }
        });

    }

    QMenu *folderSubMenu = menu->addMenu("Ajouter au dossier");

    // ➕ Toujours proposer la création d'un nouveau dossier
    QAction *newFolderAction = folderSubMenu->addAction("➕ Créer un nouveau dossier...");
    connect(newFolderAction, &QAction::triggered, this, [this, index]() {
        bool ok;
        QString name = QInputDialog::getText(this, "Nouveau dossier", "Nom du dossier :", QLineEdit::Normal, "", &ok);
        if (ok && !name.trimmed().isEmpty()) {
            QString cleanName = name.trimmed();
            QString parent = inFolderView ? currentFolder : "";
            m_folders.append({ cleanName, parent });
            if (index >= 0 && index < m_customShapes.size()) {
                m_customShapes[index].folder = cleanName;
            }

            saveCustomShapes();
            if (inFolderView)
                displayShapesInFolder(currentFolder, ui->searchBar->text());
            else
                displayShapes(ui->searchBar->text());
        }
    });

        for (const InventaireFolder &folder : m_folders) {
            if (inFolderView && folder.parentFolder != currentFolder)
                continue;  // 👉 dans un dossier, ne proposer que ses sous-dossiers

            if (!inFolderView && !folder.parentFolder.isEmpty())
                continue;  // 👉 dans l'inventaire principal, ne proposer que les dossiers racines

            qDebug() << " - " << folder.name;
            QAction *folderAction = folderSubMenu->addAction(folder.name);

            connect(folderAction, &QAction::triggered, this,
                    [this, folder, frame]() {
                        bool ok = false;
                        int idx = frame->property("CustomShapeIndex").toInt(&ok);
                        if (ok && idx >= 0 && idx < m_customShapes.size()) {
                            m_customShapes[idx].folder = folder.name;
                            saveCustomShapes();
                            displayShapes();
                        }
                    });
        }

    connect(menuButton, &QPushButton::clicked, [menu, menuButton]() {
//        menu->setMinimumWidth(200);  // force un espace suffisant
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });

    connect(renameAction, &QAction::triggered, [this, label, index]() {
        bool ok = false;
        const QString newName = QInputDialog::getText(nullptr,
                                                      currentLanguage == Language::French ? tr("Renommer la forme") : tr("Rename shape"),
                                                      currentLanguage == Language::French ? tr("Nouveau nom :")       : tr("New name:"),
                                                      QLineEdit::Normal,
                                                      label->text(),
                                                      &ok);
        if (ok && !newName.isEmpty() && index >= 0 && index < m_customShapes.size()) {
            label->setText(newName);
            m_customShapes[index].name = newName;
            saveCustomShapes();
        }
    });

    connect(deleteAction, &QAction::triggered, [this, frame]() {
        bool ok = false;
        const int idx = frame->property("CustomShapeIndex").toInt(&ok);
        if (ok && idx >= 0 && idx < m_customShapes.size()) {
            m_customShapes.removeAt(idx);
            saveCustomShapes();
        }
        displayShapes();
    });

    auto *headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    headerLayout->addWidget(menuButton);

    auto *mainLayout = new QVBoxLayout(frame);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(view, 0, Qt::AlignCenter);
    mainLayout->addWidget(label);

    // Store index for later retrieval (eventFilter)
    frame->setProperty("CustomShapeIndex", index);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);

    return frame;
}

// -----------------------------------------------------------------------------
// Add one custom shape to inventory (called by other parts of the app)
// -----------------------------------------------------------------------------
void Inventaire::addSavedCustomShape(const QList<QPolygonF> &polygons, const QString &name)
{
    if (shapeNameExists(name)) {
        qDebug() << "Forme déjà enregistrée avec ce nom";
        return;
    }

    CustomShapeData newData;
    newData.polygons = polygons;
    newData.name     = name;
    m_customShapes.append(newData);

    saveCustomShapes();
    displayShapes();
}

// -----------------------------------------------------------------------------
// Event filter – detect clicks on thumbnails
// -----------------------------------------------------------------------------
bool Inventaire::eventFilter(QObject *obj, QEvent *event)
{
    if (auto *frame = qobject_cast<QFrame*>(obj)) {
        if (event->type() == QEvent::MouseButtonPress) {
            m_lastPressedFrame = frame;
            m_pressTimer.start();
            m_longPress = false;
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (frame == m_lastPressedFrame) {
                if (m_dragInProgress)
                    m_longPress = false;
                else
                    m_longPress = m_pressTimer.elapsed() > LONG_PRESS_THRESHOLD;
                m_lastPressedFrame = nullptr;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

// -----------------------------------------------------------------------------
// Internationalisation helpers
// -----------------------------------------------------------------------------
void Inventaire::updateTranslations(Language lang)
{
    currentLanguage = lang;
    if (ui->buttonMenu)
        ui->buttonMenu->setText(QString());

    displayShapes(ui->searchBar ? ui->searchBar->text() : QString());
}

void Inventaire::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        displayShapes(ui->searchBar ? ui->searchBar->text() : QString());
    }
    QWidget::changeEvent(event);
}

// -----------------------------------------------------------------------------
// Filesystem helpers
// -----------------------------------------------------------------------------
QString Inventaire::customShapesFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    QDir().mkpath(dir);
    return dir + "/custom_shapes.json";
}

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------
bool Inventaire::shapeNameExists(const QString &name) const
{
    for (const auto &shapeData : m_customShapes) {
        if (shapeData.name.compare(name, Qt::CaseSensitive) == 0)
            return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// Load / Save custom shapes (+ layouts)
// -----------------------------------------------------------------------------
void Inventaire::loadCustomShapes()
{
    m_customShapes.clear();
    m_baseShapeLayouts.clear();
    m_baseShapeFolders.clear();
    m_baseShapeOrder.clear();
    m_folders.clear();

    QFile file(customShapesFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();

    // ------------------------------
    // Folders
    // ------------------------------
    const QJsonArray foldersArr = root.value("folders").toArray();
    for (const QJsonValue &val : foldersArr) {
        if (!val.isObject())
            continue;
        const QJsonObject obj = val.toObject();
        InventaireFolder f;
        f.name = obj.value("name").toString();
        f.parentFolder = obj.value("parentFolder").toString();
        m_folders.append(f);
    }

    // ------------------------------
    // Custom shapes
    // ------------------------------
    const QJsonArray shapesArr = root.value("shapes").toArray();
    for (const QJsonValue &val : shapesArr) {
        if (!val.isObject())
            continue;

        const QJsonObject obj = val.toObject();
        CustomShapeData data;
        data.name = obj.value("name").toString();
        data.folder = obj.value("folder").toString();

        // Polygons
        const QJsonArray polyArr = obj.value("polygons").toArray();
        for (const QJsonValue &polyVal : polyArr) {
            const QJsonArray ptArrList = polyVal.toArray();
            QPolygonF poly;
            for (const QJsonValue &ptVal : ptArrList) {
                const QJsonArray p = ptVal.toArray();
                if (p.size() >= 2)
                    poly.append(QPointF(p.at(0).toDouble(), p.at(1).toDouble()));
            }
            data.polygons.append(poly);
        }

        // Layouts (optional)
        const QJsonArray layoutsArr = obj.value("layouts").toArray();
        for (const QJsonValue &layoutVal : layoutsArr) {
            if (!layoutVal.isObject())
                continue;
            const QJsonObject lo = layoutVal.toObject();
            LayoutData ld;
            ld.name      = lo.value("name").toString();
            ld.largeur   = lo.value("largeur").toInt();
            ld.longueur  = lo.value("longueur").toInt();
            ld.spacing   = lo.value("spacing").toInt();
            const QJsonArray itemsArr = lo.value("items").toArray();
            for (const QJsonValue &itemVal : itemsArr) {
                const QJsonObject io = itemVal.toObject();
                LayoutItem li;
                li.x        = io.value("x").toDouble();
                li.y        = io.value("y").toDouble();
                li.rotation = io.value("rotation").toDouble();
                ld.items.append(li);
            }
            data.layouts.append(ld);
        }

        m_customShapes.append(data);
    }

    // ------------------------------
    // Base shape layouts
    // ------------------------------
    const QJsonObject baseObj = root.value("baseLayouts").toObject();
    for (auto it = baseObj.begin(); it != baseObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        const QJsonArray layoutsArr = it.value().toArray();
        QList<LayoutData> list;
        for (const QJsonValue &layoutVal : layoutsArr) {
            if (!layoutVal.isObject())
                continue;
            const QJsonObject lo = layoutVal.toObject();
            LayoutData ld;
            ld.name     = lo.value("name").toString();
            ld.largeur  = lo.value("largeur").toInt();
            ld.longueur = lo.value("longueur").toInt();
            ld.spacing  = lo.value("spacing").toInt();
            const QJsonArray itemsArr = lo.value("items").toArray();
            for (const QJsonValue &itemVal : itemsArr) {
                const QJsonObject io = itemVal.toObject();
                LayoutItem li;
                li.x        = io.value("x").toDouble();
                li.y        = io.value("y").toDouble();
                li.rotation = io.value("rotation").toDouble();
                ld.items.append(li);
            }
            list.append(ld);
        }
        m_baseShapeLayouts[type] = list;
    }

    const QJsonObject baseFoldersObj = root.value("baseFolders").toObject();
    for (auto it = baseFoldersObj.begin(); it != baseFoldersObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        m_baseShapeFolders[type] = it.value().toString();
    }

    const QJsonArray baseOrderArr = root.value("baseOrder").toArray();
    for (const QJsonValue &v : baseOrderArr) {
        m_baseShapeOrder.append(static_cast<ShapeModel::Type>(v.toInt()));
    }
}

void Inventaire::saveCustomShapes() const
{
    QJsonArray shapesArr;
    for (const CustomShapeData &data : m_customShapes) {
        QJsonObject obj;
        obj["name"] = data.name;
        obj["folder"] = data.folder;

        // Polygons
        QJsonArray polyArr;
        for (const QPolygonF &poly : data.polygons) {
            QJsonArray pointsArr;
            for (const QPointF &pt : poly) {
                QJsonArray ptArr;
                ptArr.append(pt.x());
                ptArr.append(pt.y());
                pointsArr.append(ptArr);
            }
            polyArr.append(pointsArr);
        }
        obj["polygons"] = polyArr;

        // Layouts
        QJsonArray layoutsArr;
        for (const LayoutData &ld : data.layouts) {
            QJsonObject lo;
            lo["name"]     = ld.name;
            lo["largeur"]  = ld.largeur;
            lo["longueur"] = ld.longueur;
            lo["spacing"]  = ld.spacing;
            QJsonArray itemsArr;
            for (const LayoutItem &li : ld.items) {
                QJsonObject io;
                io["x"]        = li.x;
                io["y"]        = li.y;
                io["rotation"] = li.rotation;
                itemsArr.append(io);
            }
            lo["items"] = itemsArr;
            layoutsArr.append(lo);
        }
        obj["layouts"] = layoutsArr;

        shapesArr.append(obj);
    }

    // Base layouts
    QJsonObject baseObj;
    for (auto it = m_baseShapeLayouts.constBegin(); it != m_baseShapeLayouts.constEnd(); ++it) {
        QJsonArray layoutsArr;
        for (const LayoutData &ld : it.value()) {
            QJsonObject lo;
            lo["name"]     = ld.name;
            lo["largeur"]  = ld.largeur;
            lo["longueur"] = ld.longueur;
            lo["spacing"]  = ld.spacing;
            QJsonArray itemsArr;
            for (const LayoutItem &li : ld.items) {
                QJsonObject io;
                io["x"]        = li.x;
                io["y"]        = li.y;
                io["rotation"] = li.rotation;
                itemsArr.append(io);
            }
            lo["items"] = itemsArr;
            layoutsArr.append(lo);
        }
        baseObj[QString::number(static_cast<int>(it.key()))] = layoutsArr;
    }

    QJsonObject baseFoldersObj;
    for (auto it = m_baseShapeFolders.constBegin(); it != m_baseShapeFolders.constEnd(); ++it) {
        baseFoldersObj[QString::number(static_cast<int>(it.key()))] = it.value();
    }
    QJsonArray baseOrderArr;
    for (ShapeModel::Type t : m_baseShapeOrder)
        baseOrderArr.append(static_cast<int>(t));

    QJsonArray foldersArr;
    for (const InventaireFolder &f : m_folders) {
        QJsonObject fo;
        fo["name"] = f.name;
        fo["parentFolder"] = f.parentFolder;
        foldersArr.append(fo);
    }

    QJsonObject rootObj;
    rootObj["shapes"]      = shapesArr;
    rootObj["baseLayouts"] = baseObj;
    rootObj["baseFolders"] = baseFoldersObj;
    rootObj["baseOrder"]   = baseOrderArr;
    rootObj["folders"]     = foldersArr;

    QFile file(customShapesFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(rootObj).toJson());
        file.close();
    }
}

// -----------------------------------------------------------------------------
// Search bar helpers
// -----------------------------------------------------------------------------
void Inventaire::onSearchTextChanged(const QString &text)
{
    if (inFolderView) {
        displayShapesInFolder(currentFolder, text);
    } else {
        displayShapes(text);
    }
}


void Inventaire::onClearSearchClicked()
{
    if (ui->searchBar)
        ui->searchBar->clear();
    displayShapes();
}

void Inventaire::onCreateFolderClicked()
{
    bool ok = false;
    QString name = QInputDialog::getText(this,
                                         tr("Nouveau dossier"),
                                         tr("Nom du dossier :"),
                                         QLineEdit::Normal,
                                         QString(),
                                         &ok);
    if (ok && !name.trimmed().isEmpty()) {
        QString clean = name.trimmed();
        QString parent = inFolderView ? currentFolder : QString();
        m_folders.append({clean, parent});
        saveCustomShapes();

        if (inFolderView)
            displayShapesInFolder(currentFolder, ui->searchBar->text());
        else
            displayShapes(ui->searchBar->text());
    }
}

QStringList Inventaire::getAllShapeNames() const
{
    QStringList names;

    // Built-in shapes
    if (currentLanguage == Language::French) {
        names << "Cercle" << "Rectangle" << "Triangle" << "Étoile" << "Cœur";
    } else {
        names << "Circle" << "Rectangle" << "Triangle" << "Star" << "Heart";
    }

    // Custom shapes
    for (const CustomShapeData &data : m_customShapes)
        names << data.name;

    return names;
}

// -----------------------------------------------------------------------------
// Layout management – Custom shapes
// -----------------------------------------------------------------------------
void Inventaire::addLayoutToShape(const QString &shapeName, const LayoutData &layout)
{
    for (CustomShapeData &data : m_customShapes) {
        if (data.name == shapeName) {
            data.layouts.append(layout);
            saveCustomShapes();
            return;
        }
    }
}

void Inventaire::renameLayout(const QString &shapeName, int index, const QString &newName)
{
    for (CustomShapeData &data : m_customShapes) {
        if (data.name == shapeName && index >= 0 && index < data.layouts.size()) {
            data.layouts[index].name = newName;
            saveCustomShapes();
            return;
        }
    }
}

void Inventaire::deleteLayout(const QString &shapeName, int index)
{
    for (CustomShapeData &data : m_customShapes) {
        if (data.name == shapeName && index >= 0 && index < data.layouts.size()) {
            data.layouts.removeAt(index);
            saveCustomShapes();
            return;
        }
    }
}

QList<LayoutData> Inventaire::getLayoutsForShape(const QString &shapeName) const
{
    for (const CustomShapeData &data : m_customShapes) {
        if (data.name == shapeName)
            return data.layouts;
    }
    return {};
}

// -----------------------------------------------------------------------------
// Layout management – Built-in shapes
// -----------------------------------------------------------------------------
void Inventaire::addLayoutToBaseShape(ShapeModel::Type type, const LayoutData &layout)
{
    m_baseShapeLayouts[type].append(layout);
    saveCustomShapes();
}

void Inventaire::renameBaseLayout(ShapeModel::Type type, int index, const QString &newName)
{
    auto it = m_baseShapeLayouts.find(type);
    if (it != m_baseShapeLayouts.end() && index >= 0 && index < it.value().size()) {
        it.value()[index].name = newName;
        saveCustomShapes();
    }
}

void Inventaire::deleteBaseLayout(ShapeModel::Type type, int index)
{
    auto it = m_baseShapeLayouts.find(type);
    if (it != m_baseShapeLayouts.end() && index >= 0 && index < it.value().size()) {
        it.value().removeAt(index);
        saveCustomShapes();
    }
}

QList<LayoutData> Inventaire::getLayoutsForBaseShape(ShapeModel::Type type) const
{
    return m_baseShapeLayouts.value(type);
}

// -----------------------------------------------------------------------------
// Utility to get localised name for a built-in shape type
// -----------------------------------------------------------------------------
QString Inventaire::baseShapeName(ShapeModel::Type type, Language lang)
{
    switch (type) {
    case ShapeModel::Type::Circle:
        return lang == Language::French ? QStringLiteral("Cercle")    : QStringLiteral("Circle");
    case ShapeModel::Type::Rectangle:
        return lang == Language::French ? QStringLiteral("Rectangle") : QStringLiteral("Rectangle");
    case ShapeModel::Type::Triangle:
        return lang == Language::French ? QStringLiteral("Triangle")  : QStringLiteral("Triangle");
    case ShapeModel::Type::Star:
        return lang == Language::French ? QStringLiteral("Étoile")    : QStringLiteral("Star");
    case ShapeModel::Type::Heart:
        return lang == Language::French ? QStringLiteral("Cœur")      : QStringLiteral("Heart");
    }
    return {};
}

QFrame* Inventaire::createBaseShapeCard(ShapeModel::Type type, const QString &name)
{
    auto *scene = new QGraphicsScene();
    auto *view  = new QGraphicsView(scene);
    view->setFixedSize(120, 120);
    view->setStyleSheet("background-color: white;");
    view->setAlignment(Qt::AlignCenter);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    const QList<QGraphicsItem*> shapes = ShapeModel::generateShapes(type, 70, 70);
    if (!shapes.isEmpty()) {
        QGraphicsItem *shapeItem = shapes.first();
        scene->addItem(shapeItem);
        scene->setSceneRect(shapeItem->boundingRect().adjusted(-5, -5, 5, 5));
        view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    }

    auto *frame = new QFrame();
    frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
    frame->setFixedSize(150, 220);

    auto *label = new QLabel(name);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: black; font-size: 16px;");

    auto *menuButton = new QPushButton("…");
    menuButton->setFixedSize(25, 25);
    menuButton->setStyleSheet("border: none; font-size: 14px;");

    QMenu *menu = new QMenu(menuButton);

    const QString currentFolderName = m_baseShapeFolders.value(type);
    if (!currentFolderName.isEmpty()) {
        QAction *removeFromFolder = menu->addAction("❌ Retirer du dossier");
        connect(removeFromFolder, &QAction::triggered, this, [this, type]() {
            QString oldFolder = m_baseShapeFolders.value(type);
            QString parentFolder;
            for (const InventaireFolder &f : m_folders) {
                if (f.name == oldFolder) { parentFolder = f.parentFolder; break; }
            }
            m_baseShapeFolders[type] = parentFolder;
            saveCustomShapes();
            if (parentFolder.isEmpty())
                displayShapes();
            else
                displayShapesInFolder(parentFolder, ui->searchBar->text());
        });
    }

    // --- Sous‑menu "Ajouter au dossier" ---
    // (créé même s'il n'existe encore aucun dossier)
    QMenu *folderSubMenu = menu->addMenu("Ajouter au dossier");

    // ➕ 1) Action de création d’un nouveau dossier
    QAction *newFolderAction = folderSubMenu->addAction("➕ Créer un nouveau dossier…");
    connect(newFolderAction, &QAction::triggered, this, [this, type]() {
        bool ok;
        QString name = QInputDialog::getText(
            this, "Nouveau dossier", "Nom du dossier :", QLineEdit::Normal, "", &ok);

        if (ok && !name.trimmed().isEmpty()) {
            QString cleanName = name.trimmed();
            QString parent    = inFolderView ? currentFolder : "";
            m_folders.append({cleanName, parent});
            m_baseShapeFolders[type] = cleanName;   // on range la forme dans le nouveau dossier
            saveCustomShapes();

            if (inFolderView)
                displayShapesInFolder(currentFolder, ui->searchBar->text());
            else
                displayShapes(ui->searchBar->text());
        }
    });

    // ➕ 2) Lister ensuite les dossiers existants (s’il y en a)
    for (const InventaireFolder &folder : m_folders) {
    if (inFolderView && folder.parentFolder != currentFolder) continue;
    if (!inFolderView && !folder.parentFolder.isEmpty())      continue;

    QAction *folderAction = folderSubMenu->addAction(folder.name);
    connect(folderAction, &QAction::triggered, this,
            [this, type, folder]() {
                m_baseShapeFolders[type] = folder.name;
                saveCustomShapes();
                displayShapes();
            });
}


    connect(menuButton, &QPushButton::clicked, [menu, menuButton]() {
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });

    auto *headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    headerLayout->addWidget(menuButton);

    auto *mainLayout = new QVBoxLayout(frame);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(view, 0, Qt::AlignCenter);
    mainLayout->addWidget(label);

    frame->setProperty("shapeType", static_cast<int>(type));
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);
    return frame;
}

QFrame* Inventaire::createFolderCard(const QString& folderName)
{
    QLabel *iconLabel = new QLabel();
    QString iconPath = folderIsEmpty(folderName) ? ":/icons/folder.svg" : ":/icons/folderfull.svg";

    QSvgRenderer renderer(iconPath);
    QPixmap icon(120, 120);
    icon.fill(Qt::transparent);

    QPainter painter(&icon);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter);
    painter.end();

    iconLabel->setPixmap(icon);
    iconLabel->setAlignment(Qt::AlignCenter);

    // Nom du dossier
    QLabel *label = new QLabel(folderName);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: black; font-size: 16px;");

    // Bouton "..." en haut à droite
    QPushButton *menuButton = new QPushButton("...");
    menuButton->setFixedSize(25, 25);
    menuButton->setCursor(Qt::PointingHandCursor);
    menuButton->setStyleSheet("border: none;");

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    headerLayout->addWidget(menuButton);

    // Carte contenant l’ensemble
    QFrame *frame = new QFrame();
    frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
    frame->setFixedSize(150, 220);

    QVBoxLayout *mainLayout = new QVBoxLayout(frame);
   // mainLayout->setContentsMargins(5, 5, 5, 5);
   // mainLayout->setSpacing(5);
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(label);

    // Propriétés de la carte dossier
    frame->setProperty("isFolder", true);
    frame->setProperty("folderName", folderName);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);

    // Menu contextuel identique aux formes (renommer / supprimer)
    QMenu *menu = new QMenu(menuButton);
    QAction *renameAction = menu->addAction("Renommer");
    QAction *deleteAction = menu->addAction("Supprimer");

    connect(renameAction, &QAction::triggered, this, [this, label, folderName]() {
        bool ok;
        QString newName = QInputDialog::getText(this, "Renommer le dossier", "Nouveau nom :", QLineEdit::Normal, folderName, &ok);
        if (ok && !newName.trimmed().isEmpty()) {
            for (InventaireFolder &f : m_folders) {
                if (f.name == folderName) {
                    f.name = newName.trimmed();
                    break;
                }
            }
            saveCustomShapes(); // facultatif ici
            displayShapes();
        }
    });

    connect(deleteAction, &QAction::triggered, this, [this, folderName]() {
        m_folders.erase(std::remove_if(m_folders.begin(), m_folders.end(),
                                       [=](const InventaireFolder &f) { return f.name == folderName; }),
                        m_folders.end());

        // Supprime les formes associées au dossier si besoin
        for (CustomShapeData &shape : m_customShapes)
            if (shape.folder == folderName)
                shape.folder.clear();

        saveCustomShapes();
        displayShapes();
    });

    connect(menuButton, &QPushButton::clicked, this, [menu, menuButton]() {
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });

    return frame;
}


void Inventaire::displayShapesInFolder(const QString &folderName, const QString &filter)
{
    QString search = filter.trimmed();

    // Nettoyer l'ancien contenu
    if (!ui->scrollAreaInventaire)
        return;

    QWidget *oldWidget = ui->scrollAreaInventaire->widget();
    if (oldWidget) {
        ui->scrollAreaInventaire->takeWidget();
        oldWidget->deleteLater();
    }

    inFolderView = true;
    currentFolder = folderName;

    DraggableListWidget *listWidget = new DraggableListWidget();
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setMovement(QListView::Snap);
    listWidget->setSpacing(25);

    ui->scrollAreaInventaire->setWidget(listWidget);
    ui->scrollAreaInventaire->setWidgetResizable(true);

    for (const InventaireFolder &folder : m_folders) {
        if (folder.parentFolder != folderName)
            continue;

        QFrame *card = createFolderCard(folder.name);
        QListWidgetItem *it = new QListWidgetItem(listWidget);
        it->setSizeHint(card->size());
        it->setData(Qt::UserRole, 0);
        it->setData(Qt::UserRole + 1, folder.name);
        it->setFlags(it->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        listWidget->addItem(it);
        listWidget->setItemWidget(it, card);
    }

    for (ShapeModel::Type type : m_baseShapeOrder) {
        QString folder = m_baseShapeFolders.value(type);
        if (folder != folderName)
            continue;
        QString name = baseShapeName(type, currentLanguage);
        if (!search.isEmpty() && !name.toLower().contains(search.toLower()))
            continue;
        QFrame *frame = createBaseShapeCard(type, name);
        QListWidgetItem *it = new QListWidgetItem(listWidget);
        it->setSizeHint(frame->size());
        it->setData(Qt::UserRole, 1);
        it->setData(Qt::UserRole + 1, static_cast<int>(type));
        it->setFlags(it->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        listWidget->addItem(it);
        listWidget->setItemWidget(it, frame);
    }

    for (int i = 0; i < m_customShapes.size(); ++i) {
        const CustomShapeData &data = m_customShapes[i];

        if (inFolderView && data.folder != currentFolder)
            continue;
        if (!inFolderView && !data.folder.isEmpty())
            continue;

        if (!search.isEmpty() && !data.name.toLower().contains(search.toLower()))
            continue;

        QFrame* customFrame = addCustomShapeToGrid(i);
        QListWidgetItem *it = new QListWidgetItem(listWidget);
        it->setSizeHint(customFrame->size());
        it->setData(Qt::UserRole, 2);
        it->setData(Qt::UserRole + 1, data.name);
        it->setFlags(it->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        listWidget->addItem(it);
        listWidget->setItemWidget(it, customFrame);
    }


    // 👉 ajouter un bouton retour
    QPushButton *retourButton = new QPushButton("← Retour");
    retourButton->setStyleSheet("color: white; background-color: red; font-size: 14px;");
    retourButton->setFixedSize(120, 40);
    connect(retourButton, &QPushButton::clicked, this, [this]() {
        // Cherche le dossier courant
        QString parent;
        for (const InventaireFolder &f : m_folders) {
            if (f.name == currentFolder) {
                parent = f.parentFolder;
                break;
            }
        }

        if (parent.isEmpty()) {
            // Aucun parent ⇒ revenir à l'inventaire principal
            displayShapes();
        } else {
            // Retourner au dossier parent
            displayShapesInFolder(parent, "");
        }
    });
    // ajouter le bouton en haut à gauche du layout principal
    QListWidgetItem *backItem = new QListWidgetItem(listWidget);
    backItem->setSizeHint(retourButton->size());
    backItem->setFlags(backItem->flags() & ~Qt::ItemIsDragEnabled & ~Qt::ItemIsDropEnabled);
    listWidget->addItem(backItem);
    listWidget->setItemWidget(backItem, retourButton);
    connect(listWidget, &QListWidget::itemClicked, this, &Inventaire::onItemClicked);
    connect(listWidget->model(), &QAbstractItemModel::rowsMoved, this, [this, listWidget](const QModelIndex&, int, int, const QModelIndex&, int){ applyReorderFromList(listWidget); });
    connect(listWidget, &DraggableListWidget::dragStarted, this, [this](){ m_dragInProgress = true; });
    connect(listWidget, &DraggableListWidget::dragFinished, this, [this](){ m_dragInProgress = false; });
    ui->buttonMenu->setVisible(false);

}

bool Inventaire::folderIsEmpty(const QString& folderName) const
{
    // Sous-dossiers
    for (const InventaireFolder& folder : m_folders)
        if (folder.parentFolder == folderName)
            return false;

    // Formes custom
    for (const CustomShapeData& shape : m_customShapes)
        if (shape.folder == folderName)
            return false;

    // Formes de base
    for (auto it = m_baseShapeFolders.constBegin(); it != m_baseShapeFolders.constEnd(); ++it)
        if (it.value() == folderName)
            return false;

    return true;
}

void Inventaire::applyReorderFromList(QListWidget *listWidget)
{
    if (!listWidget) return;

    QString parent = inFolderView ? currentFolder : QString();

    QStringList folderNames;
    QList<ShapeModel::Type> baseTypes;
    QStringList customNames;

    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem *it = listWidget->item(i);
        int t = it->data(Qt::UserRole).toInt();
        if (t == 0) {
            folderNames << it->data(Qt::UserRole + 1).toString();
        } else if (t == 1) {
            baseTypes << static_cast<ShapeModel::Type>(it->data(Qt::UserRole + 1).toInt());
        } else if (t == 2) {
            customNames << it->data(Qt::UserRole + 1).toString();
        }
    }

    // --- Folders ---
    QMap<QString, InventaireFolder> folderMap;
    for (const InventaireFolder &f : m_folders)
        folderMap[f.name] = f;
    QList<int> folderIndices;
    for (int i = 0; i < m_folders.size(); ++i) {
        if (m_folders[i].parentFolder == parent)
            folderIndices.append(i);
    }
    for (int i = 0; i < folderNames.size() && i < folderIndices.size(); ++i) {
        m_folders[folderIndices[i]] = folderMap.value(folderNames[i]);
    }

    // --- Base shapes ---
    QList<int> baseIndices;
    for (int i = 0; i < m_baseShapeOrder.size(); ++i) {
        if (m_baseShapeFolders.value(m_baseShapeOrder[i]) == parent)
            baseIndices.append(i);
    }
    for (int i = 0; i < baseTypes.size() && i < baseIndices.size(); ++i) {
        m_baseShapeOrder[baseIndices[i]] = baseTypes[i];
    }

    // --- Custom shapes ---
    QMap<QString, CustomShapeData> customMap;
    for (const CustomShapeData &c : m_customShapes)
        customMap[c.name] = c;
    QList<int> customIndices;
    for (int i = 0; i < m_customShapes.size(); ++i) {
        if (m_customShapes[i].folder == parent)
            customIndices.append(i);
    }
    for (int i = 0; i < customNames.size() && i < customIndices.size(); ++i) {
        m_customShapes[customIndices[i]] = customMap.value(customNames[i]);
    }

    saveCustomShapes();
    if (inFolderView)
        displayShapesInFolder(currentFolder, ui->searchBar->text());
    else
        displayShapes(ui->searchBar->text());
}

void Inventaire::onItemClicked(QListWidgetItem *item)
{
    if (m_dragInProgress)
        return;
    if (m_longPress) {
        m_longPress = false;
        return; // ignore click after a long press
    }
    if (!item) return;
    int t = item->data(Qt::UserRole).toInt();
    if (t == 0) {
        QString name = item->data(Qt::UserRole + 1).toString();
        displayShapesInFolder(name, ui->searchBar->text());
    } else if (t == 1) {
        ShapeModel::Type type = static_cast<ShapeModel::Type>(item->data(Qt::UserRole + 1).toInt());
        emit shapeSelected(type, 150, 220);
        goToMainWindow();
    } else if (t == 2) {
        QString name = item->data(Qt::UserRole + 1).toString();
        for (const CustomShapeData &c : m_customShapes) {
            if (c.name == name) {
                emit customShapeSelected(c.polygons, c.name);
                goToMainWindow();
                break;
            }
        }
    }
}

