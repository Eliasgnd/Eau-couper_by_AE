#include "Inventory.h"
#include "ui_Inventory.h"
#include "ShapeModel.h"
#include "MainWindow.h"
#include <QApplication>
#include "Language.h"
#include "ScreenUtils.h"
#include "InventoryStorage.h"
#include "InventoryQueryService.h"
#include "InventoryMutationService.h"
#include "InventoryModel.h"
#include "InventoryController.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QPainterPath>

#include <QFile>

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

#define m_customShapes (m_model->customShapes())
#define m_baseShapeLayouts (m_model->baseShapeLayouts())
#define m_baseShapeFolders (m_model->baseShapeFolders())
#define m_baseUsageCount (m_model->baseUsageCount())
#define m_baseLastUsed (m_model->baseLastUsed())
#define m_baseShapeOrder (m_model->baseShapeOrder())
#define m_folders (m_model->folders())
#define currentLanguage (m_model->languageRef())

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
Inventory* Inventory::instance = nullptr;

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------
Inventory::Inventory(QWidget *parent)
    : QWidget(parent), ui(new Ui::Inventory)
{
    ui->setupUi(this);

    m_model = new InventoryModel();
    m_controller = new InventoryController(*m_model);

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
    connect(ui->buttonMenu, &QPushButton::clicked, this, &Inventory::goToMainWindow);

    // Search bar
    connect(ui->searchBar, &QLineEdit::textChanged, this, &Inventory::onSearchTextChanged);
    connect(ui->buttonClearSearch, &QPushButton::clicked, this, &Inventory::onClearSearchClicked);
    connect(ui->buttonNewFolder, &QPushButton::clicked, this, &Inventory::onCreateFolderClicked);

    if (ui->comboSort) {
        ui->comboSort->addItem("Nom (A \342\206\222 Z)");
        ui->comboSort->addItem("Nom (Z \342\206\222 A)");
        ui->comboSort->addItem("Utilisation fr\303\251quente");
        ui->comboSort->addItem("R\303\251cent \342\206\222 Ancien");
        ui->comboSort->addItem("Ancien \342\206\222 R\303\251cent");
        connect(ui->comboSort, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Inventory::onSortChanged);
    }

    if (ui->comboFilter) {
        ui->comboFilter->addItem("Tout");
        ui->comboFilter->addItem("Dossiers uniquement");
        ui->comboFilter->addItem("Formes sans dossier");
        connect(ui->comboFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Inventory::onFilterChanged);
    }

    // Initial display
    displayShapes();

}

Inventory::~Inventory()
{
    qDebug() << "Fermeture de l'Inventory (l'instance reste vivante via le singleton)";
    delete m_controller;
    delete m_model;
    delete ui;
}

// -----------------------------------------------------------------------------
// Singleton accessor
// -----------------------------------------------------------------------------
Inventory* Inventory::getInstance()
{
    if (!instance) {
        qDebug() << "Création de l'instance Inventory";
        instance = new Inventory();
    }
    return instance;
}

QPixmap Inventory::renderColoredSvg(const QString &filePath, const QColor &color, const QSize &size)
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
void Inventory::goToMainWindow()
{
    hide();
    if (auto mw = resolveMainWindow()) mw->showFullScreen();
}

// -----------------------------------------------------------------------------
// UI – Build the inventory grid
// -----------------------------------------------------------------------------
void Inventory::displayShapes(const QString &filter /* = QString() */)
{
    if (!ui->scrollAreaInventory)
        return;

    // Clear previous content, if any
    if (QWidget *oldWidget = ui->scrollAreaInventory->widget()) {
        ui->scrollAreaInventory->takeWidget();
        oldWidget->deleteLater();
    }

    inFolderView = false;
    currentFolder.clear();

    QListWidget *listWidget = new QListWidget();
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setMovement(QListView::Snap);
    listWidget->setSpacing(25);

    ui->scrollAreaInventory->setWidget(listWidget);
    ui->scrollAreaInventory->setWidgetResizable(true);

    const QString f = filter.trimmed().toLower();
    int filterMode = ui->comboFilter ? ui->comboFilter->currentIndex() : 0;
    int sortMode = ui->comboSort ? ui->comboSort->currentIndex() : 0;

    struct Item { int type; int index; ShapeModel::Type shape; QString name; int usage; QDateTime last; QFrame* frame; };
    QList<Item> items;

    // Folders
    for (int i=0;i<m_folders.size();++i) {
        const InventoryFolder &folder = m_folders[i];
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
    connect(listWidget, &QListWidget::itemClicked, this, &Inventory::onItemClicked);
    ui->buttonMenu->setVisible(true);
    update();
}

// -----------------------------------------------------------------------------
// Build a single custom shape thumbnail
// -----------------------------------------------------------------------------
QFrame* Inventory::addCustomShapeToGrid(int index)
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
            for (const InventoryFolder &f : m_folders) {
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

        for (const InventoryFolder &folder : m_folders) {
            if (inFolderView && folder.parentFolder != currentFolder)
                continue;  // 👉 dans un dossier, ne proposer que ses sous-dossiers

            if (!inFolderView && !folder.parentFolder.isEmpty())
                continue;  // 👉 dans l'inventory principal, ne proposer que les dossiers racines

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
void Inventory::addSavedCustomShape(const QList<QPolygonF> &polygons, const QString &name)
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
void Inventory::updateTranslations(Language lang)
{
    currentLanguage = lang;
    if (ui->buttonMenu)
        ui->buttonMenu->setText(QString());

    displayShapes(ui->searchBar ? ui->searchBar->text() : QString());
}

void Inventory::changeEvent(QEvent *event)
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
QString Inventory::customShapesFilePath() const
{
    return m_model->customShapesFilePath();
}

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------
bool Inventory::shapeNameExists(const QString &name) const
{
    return InventoryQueryService::shapeNameExists(m_customShapes, name);
}

// -----------------------------------------------------------------------------
// Load / Save custom shapes (+ layouts)
// -----------------------------------------------------------------------------
void Inventory::loadCustomShapes()
{
    m_model->load();
}

void Inventory::saveCustomShapes() const
{
    m_model->save();
}

// -----------------------------------------------------------------------------
// Search bar helpers
// -----------------------------------------------------------------------------
void Inventory::onSearchTextChanged(const QString &text)
{
    if (inFolderView) {
        displayShapesInFolder(currentFolder, text);
    } else {
        displayShapes(text);
    }
}

void Inventory::onClearSearchClicked()
{
    if (ui->searchBar)
        ui->searchBar->clear();
    displayShapes();
}

void Inventory::onCreateFolderClicked()
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
        if (m_controller->createFolder(clean, parent)) {

            if (inFolderView)
                displayShapesInFolder(currentFolder, ui->searchBar->text());
            else
                displayShapes(ui->searchBar->text());
        }
    }
}

void Inventory::onSortChanged(int)
{
    if (inFolderView)
        displayShapesInFolder(currentFolder, ui->searchBar->text());
    else
        displayShapes(ui->searchBar->text());
}

void Inventory::onFilterChanged(int)
{
    if (inFolderView)
        displayShapesInFolder(currentFolder, ui->searchBar->text());
    else
        displayShapes(ui->searchBar->text());
}

QStringList Inventory::getAllShapeNames() const
{
    return InventoryQueryService::getAllShapeNames(m_customShapes, currentLanguage);
}

// -----------------------------------------------------------------------------
// Layout management – Custom shapes
// -----------------------------------------------------------------------------
void Inventory::addLayoutToShape(const QString &shapeName, const LayoutData &layout)
{
    if (InventoryMutationService::addLayoutToShape(m_customShapes, shapeName, layout))
        saveCustomShapes();
}

void Inventory::renameLayout(const QString &shapeName, int index, const QString &newName)
{
    if (InventoryMutationService::renameLayout(m_customShapes, shapeName, index, newName))
        saveCustomShapes();
}

void Inventory::deleteLayout(const QString &shapeName, int index)
{
    if (InventoryMutationService::deleteLayout(m_customShapes, shapeName, index))
        saveCustomShapes();
}

QList<LayoutData> Inventory::getLayoutsForShape(const QString &shapeName) const
{
    return InventoryMutationService::getLayoutsForShape(m_customShapes, shapeName);
}

// -----------------------------------------------------------------------------
// Layout management – Built-in shapes
// -----------------------------------------------------------------------------
void Inventory::addLayoutToBaseShape(ShapeModel::Type type, const LayoutData &layout)
{
    if (InventoryMutationService::addLayoutToBaseShape(m_baseShapeLayouts, type, layout))
        saveCustomShapes();
}

void Inventory::renameBaseLayout(ShapeModel::Type type, int index, const QString &newName)
{
    if (InventoryMutationService::renameBaseLayout(m_baseShapeLayouts, type, index, newName))
        saveCustomShapes();
}

void Inventory::deleteBaseLayout(ShapeModel::Type type, int index)
{
    if (InventoryMutationService::deleteBaseLayout(m_baseShapeLayouts, type, index))
        saveCustomShapes();
}

QList<LayoutData> Inventory::getLayoutsForBaseShape(ShapeModel::Type type) const
{
    return InventoryMutationService::getLayoutsForBaseShape(m_baseShapeLayouts, type);
}

void Inventory::incrementLayoutUsage(const QString &shapeName, int index)
{
    if (InventoryMutationService::incrementLayoutUsage(m_customShapes,
                                                       shapeName,
                                                       index,
                                                       QDateTime::currentDateTime())) {
        saveCustomShapes();
    }
}

void Inventory::incrementBaseLayoutUsage(ShapeModel::Type type, int index)
{
    if (InventoryMutationService::incrementBaseLayoutUsage(m_baseShapeLayouts,
                                                           type,
                                                           index,
                                                           QDateTime::currentDateTime())) {
        saveCustomShapes();
    }
}

// -----------------------------------------------------------------------------
// Utility to get localised name for a built-in shape type
// -----------------------------------------------------------------------------
QString Inventory::baseShapeName(ShapeModel::Type type, Language lang)
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

QFrame* Inventory::createBaseShapeCard(ShapeModel::Type type, const QString &name)
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
            for (const InventoryFolder &f : m_folders) {
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
    for (const InventoryFolder &folder : m_folders) {
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

QFrame* Inventory::createFolderCard(const QString& folderName)
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
            if (InventoryMutationService::renameFolder(m_folders, folderName, newName.trimmed())) {
                saveCustomShapes();
                displayShapes();
            }
        }
    });

    connect(deleteAction, &QAction::triggered, this, [this, folderName]() {
        if (InventoryMutationService::deleteFolder(m_folders, m_customShapes, folderName)) {
            saveCustomShapes();
            displayShapes();
        }
    });

    connect(menuButton, &QPushButton::clicked, this, [menu, menuButton]() {
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });

    return frame;
}

void Inventory::displayShapesInFolder(const QString &folderName, const QString &filter)
{
    QString search = filter.trimmed();

    // Nettoyer l'ancien contenu
    if (!ui->scrollAreaInventory)
        return;

    QWidget *oldWidget = ui->scrollAreaInventory->widget();
    if (oldWidget) {
        ui->scrollAreaInventory->takeWidget();
        oldWidget->deleteLater();
    }

    inFolderView = true;
    currentFolder = folderName;

    QListWidget *listWidget = new QListWidget();
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setMovement(QListView::Snap);
    listWidget->setSpacing(25);

    ui->scrollAreaInventory->setWidget(listWidget);
    ui->scrollAreaInventory->setWidgetResizable(true);

    int filterMode = ui->comboFilter ? ui->comboFilter->currentIndex() : 0;
    int sortMode = ui->comboSort ? ui->comboSort->currentIndex() : 0;

    struct Item { int type; int index; ShapeModel::Type shape; QString name; int usage; QDateTime last; QFrame* frame; };
    QList<Item> items;

    for (int i=0;i<m_folders.size();++i) {
        const InventoryFolder &folder = m_folders[i];
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
        QString parent = InventoryQueryService::parentFolderOf(m_folders, currentFolder);

        if (parent.isEmpty()) {
            // Aucun parent ⇒ revenir à l'inventory principal
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
    connect(listWidget, &QListWidget::itemClicked, this, &Inventory::onItemClicked);
    ui->buttonMenu->setVisible(false);

}

bool Inventory::folderIsEmpty(const QString& folderName) const
{
    return InventoryQueryService::folderIsEmpty(folderName,
                                                m_folders,
                                                m_customShapes,
                                                m_baseShapeFolders);
}

bool Inventory::folderContainsMatchingShape(const QString &folderName, const QString &text) const
{
    return InventoryQueryService::folderContainsMatchingShape(folderName,
                                                              text,
                                                              m_folders,
                                                              m_customShapes,
                                                              m_baseShapeFolders,
                                                              currentLanguage);
}

void Inventory::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    int t = item->data(Qt::UserRole).toInt();
    if (t == 0) {
        QString name = item->data(Qt::UserRole + 1).toString();
        m_controller->onFolderSelected(name);
        displayShapesInFolder(name, ui->searchBar->text());
    } else if (t == 1) {
        ShapeModel::Type type = static_cast<ShapeModel::Type>(item->data(Qt::UserRole + 1).toInt());
        m_controller->onBaseShapeSelected(type);
        emit shapeSelected(type, 150, 220);
        goToMainWindow();
    } else if (t == 2) {
        QString name = item->data(Qt::UserRole + 1).toString();
        QList<QPolygonF> polygons;
        QString resolvedName;
        if (m_controller->onCustomShapeSelected(name, polygons, resolvedName)) {
            emit customShapeSelected(polygons, resolvedName);
            goToMainWindow();
        }
    }
}

#undef m_customShapes
#undef m_baseShapeLayouts
#undef m_baseShapeFolders
#undef m_baseUsageCount
#undef m_baseLastUsed
#undef m_baseShapeOrder
#undef m_folders
#undef currentLanguage
