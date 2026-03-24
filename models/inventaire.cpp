#include "inventaire.h"
#include "ui_inventaire.h"
#include "ShapeModel.h"
#include "MainWindow.h"
#include <QApplication>
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
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
#include <QSvgRenderer>
#include <algorithm>

namespace {
MainWindow* resolveMainWindow()
{
    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget *w : widgets) {
        if (auto *mw = qobject_cast<MainWindow*>(w)) {
            return mw;
        }
    }
    return nullptr;
}
}

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

    if (ui->comboSort) {
        ui->comboSort->addItem("Nom (A \342\206\222 Z)");
        ui->comboSort->addItem("Nom (Z \342\206\222 A)");
        ui->comboSort->addItem("Utilisation fr\303\251quente");
        ui->comboSort->addItem("R\303\251cent \342\206\222 Ancien");
        ui->comboSort->addItem("Ancien \342\206\222 R\303\251cent");
        connect(ui->comboSort, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Inventaire::onSortChanged);
    }

    if (ui->comboFilter) {
        ui->comboFilter->addItem("Tout");
        ui->comboFilter->addItem("Dossiers uniquement");
        ui->comboFilter->addItem("Formes sans dossier");
        connect(ui->comboFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Inventaire::onFilterChanged);
    }

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
    if (auto mw = resolveMainWindow()) mw->showFullScreen();
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

    QListWidget *listWidget = new QListWidget();
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setMovement(QListView::Snap);
    listWidget->setSpacing(25);

    ui->scrollAreaInventaire->setWidget(listWidget);
    ui->scrollAreaInventaire->setWidgetResizable(true);

    const QString f = filter.trimmed().toLower();
    int filterMode = ui->comboFilter ? ui->comboFilter->currentIndex() : 0;
    int sortMode = ui->comboSort ? ui->comboSort->currentIndex() : 0;

    struct Item { int type; int index; ShapeModel::Type shape; QString name; int usage; QDateTime last; QFrame* frame; };
    QList<Item> items;

    // Folders
    for (int i=0;i<m_folders.size();++i) {
        const InventaireFolder &folder = m_folders[i];
        if (!folder.parentFolder.isEmpty())
            continue;
        if (!f.isEmpty() && !folder.name.toLower().contains(f) &&
            !folderContainsMatchingShape(folder.name, f))
            continue;
        if (filterMode==2) // shapes only
            continue;
        Item it{0, i, ShapeModel::Type::Circle, folder.name, folder.usageCount, folder.lastUsed, createFolderCard(folder.name)};
        items.append(it);
    }

    // Built-in shapes
    if (filterMode!=1) {
        for (ShapeModel::Type type : m_baseShapeOrder) {
            QString name = baseShapeName(type, currentLanguage);
            if (!f.isEmpty() && !name.toLower().contains(f))
                continue;
            const QString folder = m_baseShapeFolders.value(type);
            if (!folder.isEmpty())
                continue; // only root shapes

            Item it; it.type=1; it.index=0; it.shape=type; it.name=name; it.usage=m_baseUsageCount.value(type); it.last=m_baseLastUsed.value(type); it.frame=createBaseShapeCard(type,name);
            items.append(it);
        }
    }

    // Custom shapes
    if (filterMode!=1) {
        for (int i = 0; i < m_customShapes.size(); ++i) {
            const CustomShapeData &data = m_customShapes[i];
            if (!data.folder.isEmpty())
                continue;
            if (!f.isEmpty() && !data.name.toLower().contains(f))
                continue;
            Item it{2, i, ShapeModel::Type::Circle, data.name, data.usageCount, data.lastUsed, addCustomShapeToGrid(i)};
            items.append(it);
        }
    }

    // sorting
    std::sort(items.begin(), items.end(), [sortMode](const Item &a, const Item &b){
        // Folders always come first
        if (a.type == 0 && b.type != 0)
            return true;
        if (a.type != 0 && b.type == 0)
            return false;

        switch(sortMode){
        case 0: return a.name.toLower() < b.name.toLower();
        case 1: return a.name.toLower() > b.name.toLower();
        case 2: return a.usage > b.usage;
        case 3: return a.last > b.last; // recent first
        case 4: return a.last < b.last;
        }
        return false;
    });

    for (const Item &it : items) {
        QListWidgetItem *item = new QListWidgetItem(listWidget);
        item->setSizeHint(it.frame->size());
        item->setData(Qt::UserRole, it.type);
        if (it.type==0) item->setData(Qt::UserRole+1, it.name);
        else if (it.type==1) item->setData(Qt::UserRole+1, static_cast<int>(it.shape));
        else if (it.type==2) item->setData(Qt::UserRole+1, it.name);
        listWidget->addItem(item);
        listWidget->setItemWidget(item, it.frame);
    }
    ui->buttonClearSearch->setVisible(!inFolderView);
    inFolderView = false;
    currentFolder.clear();
    connect(listWidget, &QListWidget::itemClicked, this, &Inventaire::onItemClicked);
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
    view->setAttribute(Qt::WA_TransparentForMouseEvents);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    auto *frame = new QFrame();
    frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
    frame->setFixedSize(150, 220);
    //frame->setAttribute(Qt::WA_TransparentForMouseEvents);    <--- Fais bugger les 3 petits points


    auto *label = new QLabel(data.name);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: black; font-size: 16px;");
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

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

    frame->setProperty("CustomShapeIndex", index);
    frame->setCursor(Qt::PointingHandCursor);

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
        f.usageCount = obj.value("usageCount").toInt();
        qint64 ts = static_cast<qint64>(obj.value("lastUsed").toDouble());
        if (ts > 0)
            f.lastUsed = QDateTime::fromSecsSinceEpoch(ts);
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
        data.usageCount = obj.value("usageCount").toInt();
        qint64 tsShape = static_cast<qint64>(obj.value("lastUsed").toDouble());
        if (tsShape > 0)
            data.lastUsed = QDateTime::fromSecsSinceEpoch(tsShape);

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
            ld.usageCount = lo.value("usageCount").toInt();
            qint64 tsLayout = static_cast<qint64>(lo.value("lastUsed").toDouble());
            if (tsLayout > 0)
                ld.lastUsed = QDateTime::fromSecsSinceEpoch(tsLayout);
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
            ld.usageCount = lo.value("usageCount").toInt();
            qint64 tsLayout = static_cast<qint64>(lo.value("lastUsed").toDouble());
            if (tsLayout > 0)
                ld.lastUsed = QDateTime::fromSecsSinceEpoch(tsLayout);
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

    const QJsonObject usageObj = root.value("baseUsageCount").toObject();
    for (auto it = usageObj.begin(); it != usageObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        m_baseUsageCount[type] = it.value().toInt();
    }

    const QJsonObject lastUsedObj = root.value("baseLastUsed").toObject();
    for (auto it = lastUsedObj.begin(); it != lastUsedObj.end(); ++it) {
        const ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        qint64 ts = static_cast<qint64>(it.value().toDouble());
        if (ts > 0)
            m_baseLastUsed[type] = QDateTime::fromSecsSinceEpoch(ts);
    }

    // Preserve default order of built-in shapes
}

void Inventaire::saveCustomShapes() const
{
    QJsonArray shapesArr;
    for (const CustomShapeData &data : m_customShapes) {
        QJsonObject obj;
        obj["name"] = data.name;
        obj["folder"] = data.folder;
        obj["usageCount"] = data.usageCount;
        obj["lastUsed"] = data.lastUsed.isValid() ? static_cast<qint64>(data.lastUsed.toSecsSinceEpoch()) : 0;

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
            lo["usageCount"] = ld.usageCount;
            lo["lastUsed"] = ld.lastUsed.isValid() ? static_cast<qint64>(ld.lastUsed.toSecsSinceEpoch()) : 0;
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
            lo["usageCount"] = ld.usageCount;
            lo["lastUsed"] = ld.lastUsed.isValid() ? static_cast<qint64>(ld.lastUsed.toSecsSinceEpoch()) : 0;
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
    QJsonObject usageObj;
    for (auto it = m_baseUsageCount.constBegin(); it != m_baseUsageCount.constEnd(); ++it) {
        usageObj[QString::number(static_cast<int>(it.key()))] = it.value();
    }
    QJsonObject lastUsedObj;
    for (auto it = m_baseLastUsed.constBegin(); it != m_baseLastUsed.constEnd(); ++it) {
        lastUsedObj[QString::number(static_cast<int>(it.key()))] = static_cast<qint64>(it.value().toSecsSinceEpoch());
    }
    QJsonArray foldersArr;
    for (const InventaireFolder &f : m_folders) {
        QJsonObject fo;
        fo["name"] = f.name;
        fo["parentFolder"] = f.parentFolder;
        fo["usageCount"] = f.usageCount;
        fo["lastUsed"] = f.lastUsed.isValid() ? static_cast<qint64>(f.lastUsed.toSecsSinceEpoch()) : 0;
        foldersArr.append(fo);
    }

    QJsonObject rootObj;
    rootObj["shapes"]      = shapesArr;
    rootObj["baseLayouts"] = baseObj;
    rootObj["baseFolders"] = baseFoldersObj;
    rootObj["baseUsageCount"] = usageObj;
    rootObj["baseLastUsed"] = lastUsedObj;
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

void Inventaire::onSortChanged(int)
{
    if (inFolderView)
        displayShapesInFolder(currentFolder, ui->searchBar->text());
    else
        displayShapes(ui->searchBar->text());
}

void Inventaire::onFilterChanged(int)
{
    if (inFolderView)
        displayShapesInFolder(currentFolder, ui->searchBar->text());
    else
        displayShapes(ui->searchBar->text());
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

void Inventaire::incrementLayoutUsage(const QString &shapeName, int index)
{
    for (CustomShapeData &data : m_customShapes) {
        if (data.name == shapeName && index >= 0 && index < data.layouts.size()) {
            data.layouts[index].usageCount++;
            data.layouts[index].lastUsed = QDateTime::currentDateTime();
            saveCustomShapes();
            return;
        }
    }
}

void Inventaire::incrementBaseLayoutUsage(ShapeModel::Type type, int index)
{
    auto it = m_baseShapeLayouts.find(type);
    if (it != m_baseShapeLayouts.end() && index >= 0 && index < it.value().size()) {
        it.value()[index].usageCount++;
        it.value()[index].lastUsed = QDateTime::currentDateTime();
        saveCustomShapes();
    }
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
    //frame->setAttribute(Qt::WA_TransparentForMouseEvents);    <--- Fais bugger les 3 petits points

    auto *label = new QLabel(name);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: black; font-size: 16px;");
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

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
    iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

    // Nom du dossier
    QLabel *label = new QLabel(folderName);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: black; font-size: 16px;");
    label->setAttribute(Qt::WA_TransparentForMouseEvents);

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
    //frame->setAttribute(Qt::WA_TransparentForMouseEvents);    <--- Fais bugger les 3 petits points


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

    QListWidget *listWidget = new QListWidget();
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setMovement(QListView::Snap);
    listWidget->setSpacing(25);

    ui->scrollAreaInventaire->setWidget(listWidget);
    ui->scrollAreaInventaire->setWidgetResizable(true);

    int filterMode = ui->comboFilter ? ui->comboFilter->currentIndex() : 0;
    int sortMode = ui->comboSort ? ui->comboSort->currentIndex() : 0;

    struct Item { int type; int index; ShapeModel::Type shape; QString name; int usage; QDateTime last; QFrame* frame; };
    QList<Item> items;

    for (int i=0;i<m_folders.size();++i) {
        const InventaireFolder &folder = m_folders[i];
        if (folder.parentFolder != folderName)
            continue;
        if (!search.isEmpty() && !folder.name.toLower().contains(search.toLower()) &&
            !folderContainsMatchingShape(folder.name, search))
            continue;
        if (filterMode==2) continue; // shapes only
        Item it{0,i,ShapeModel::Type::Circle,folder.name,folder.usageCount,folder.lastUsed,createFolderCard(folder.name)};
        items.append(it);
    }

    if (filterMode!=1) {
        for (ShapeModel::Type type : m_baseShapeOrder) {
            QString folder = m_baseShapeFolders.value(type);
            if (folder != folderName)
                continue;
            QString name = baseShapeName(type, currentLanguage);
            if (!search.isEmpty() && !name.toLower().contains(search.toLower()))
                continue;
            Item it{1,0,type,name,m_baseUsageCount.value(type),m_baseLastUsed.value(type),createBaseShapeCard(type,name)};
            items.append(it);
        }

        for (int i = 0; i < m_customShapes.size(); ++i) {
            const CustomShapeData &data = m_customShapes[i];
            if (data.folder != folderName)
                continue;
            if (!search.isEmpty() && !data.name.toLower().contains(search.toLower()))
                continue;
            Item it{2,i,ShapeModel::Type::Circle,data.name,data.usageCount,data.lastUsed,addCustomShapeToGrid(i)};
            items.append(it);
        }
    }

    std::sort(items.begin(), items.end(), [sortMode](const Item&a,const Item&b){
        // Folders always come first
        if (a.type == 0 && b.type != 0)
            return true;
        if (a.type != 0 && b.type == 0)
            return false;

        switch(sortMode){
        case 0: return a.name.toLower()<b.name.toLower();
        case 1: return a.name.toLower()>b.name.toLower();
        case 2: return a.usage>b.usage;
        case 3: return a.last>b.last;
        case 4: return a.last<b.last;
        }
        return false;
    });

    for (const Item& it : items) {
        QListWidgetItem *qi = new QListWidgetItem(listWidget);
        qi->setSizeHint(it.frame->size());
        qi->setData(Qt::UserRole,it.type);
        if(it.type==0) qi->setData(Qt::UserRole+1,it.name);
        else if(it.type==1) qi->setData(Qt::UserRole+1,static_cast<int>(it.shape));
        else qi->setData(Qt::UserRole+1,it.name);
        listWidget->addItem(qi);
        listWidget->setItemWidget(qi,it.frame);
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
    listWidget->addItem(backItem);
    listWidget->setItemWidget(backItem, retourButton);
    connect(listWidget, &QListWidget::itemClicked, this, &Inventaire::onItemClicked);
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

bool Inventaire::folderContainsMatchingShape(const QString &folderName, const QString &text) const
{
    const QString search = text.trimmed().toLower();
    if (search.isEmpty())
        return false;

    // Check subfolders recursively
    for (const InventaireFolder &folder : m_folders) {
        if (folder.parentFolder == folderName) {
            if (folder.name.toLower().contains(search))
                return true;
            if (folderContainsMatchingShape(folder.name, search))
                return true;
        }
    }

    // Custom shapes in this folder
    for (const CustomShapeData &shape : m_customShapes)
        if (shape.folder == folderName && shape.name.toLower().contains(search))
            return true;

    // Built-in shapes assigned to this folder
    for (auto it = m_baseShapeFolders.constBegin(); it != m_baseShapeFolders.constEnd(); ++it) {
        if (it.value() == folderName) {
            QString name = baseShapeName(it.key(), currentLanguage);
            if (name.toLower().contains(search))
                return true;
        }
    }

    return false;
}

void Inventaire::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    int t = item->data(Qt::UserRole).toInt();
    if (t == 0) {
        QString name = item->data(Qt::UserRole + 1).toString();
        for (InventaireFolder &f : m_folders) {
            if (f.name == name) {
                f.usageCount++;
                f.lastUsed = QDateTime::currentDateTime();
                break;
            }
        }
        saveCustomShapes();
        displayShapesInFolder(name, ui->searchBar->text());
    } else if (t == 1) {
        ShapeModel::Type type = static_cast<ShapeModel::Type>(item->data(Qt::UserRole + 1).toInt());
        m_baseUsageCount[type]++;
        m_baseLastUsed[type] = QDateTime::currentDateTime();
        saveCustomShapes();
        emit shapeSelected(type, 150, 220);
        goToMainWindow();
    } else if (t == 2) {
        QString name = item->data(Qt::UserRole + 1).toString();
        for (CustomShapeData &c : m_customShapes) {
            if (c.name == name) {
                c.usageCount++;
                c.lastUsed = QDateTime::currentDateTime();
                saveCustomShapes();
                emit customShapeSelected(c.polygons, c.name);
                goToMainWindow();
                break;
            }
        }
    }
}

