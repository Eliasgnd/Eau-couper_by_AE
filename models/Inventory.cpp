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

    m_controller->initialize();

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
    emit searchRequested(filter);
    const InventoryViewState state = m_controller->buildRootState(filter,
                                                                  currentSortMode(),
                                                                  currentFilterMode());
    renderState(state);
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
            if (!m_controller->removeCustomShapeFromFolderToParent(index))
                return;
            if (inFolderView)
                displayShapesInFolder(currentFolder, ui->searchBar->text());
            else
                displayShapes(ui->searchBar->text());
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
            if (!m_controller->createFolder(cleanName, parent))
                return;
            if (!m_controller->moveCustomShapeToFolder(index, cleanName))
                return;
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
                        if (!ok)
                            return;
                        if (!m_controller->moveCustomShapeToFolder(idx, folder.name))
                            return;
                        displayShapes();
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
            m_controller->renameCustomShape(index, newName);
        }
    });

    connect(deleteAction, &QAction::triggered, [this, frame]() {
        bool ok = false;
        const int idx = frame->property("CustomShapeIndex").toInt(&ok);
        if (ok)
            m_controller->deleteCustomShape(idx);
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

    m_controller->addCustomShape(polygons, name);
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
// Utilities
// -----------------------------------------------------------------------------
bool Inventory::shapeNameExists(const QString &name) const
{
    return InventoryQueryService::shapeNameExists(m_customShapes, name);
}

// -----------------------------------------------------------------------------
// Search bar helpers
// -----------------------------------------------------------------------------
void Inventory::onSearchTextChanged(const QString &text)
{
    emit searchRequested(text);
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
    emit sortModeRequested(currentSortMode());
    if (inFolderView)
        displayShapesInFolder(currentFolder, ui->searchBar->text());
    else
        displayShapes(ui->searchBar->text());
}

void Inventory::onFilterChanged(int)
{
    emit filterModeRequested(currentFilterMode());
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
    m_controller->addLayoutToCustomShape(shapeName, layout);
}

void Inventory::renameLayout(const QString &shapeName, int index, const QString &newName)
{
    m_controller->renameLayoutForCustomShape(shapeName, index, newName);
}

void Inventory::deleteLayout(const QString &shapeName, int index)
{
    m_controller->deleteLayoutForCustomShape(shapeName, index);
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
    m_controller->addLayoutToBaseShape(type, layout);
}

void Inventory::renameBaseLayout(ShapeModel::Type type, int index, const QString &newName)
{
    m_controller->renameLayoutForBaseShape(type, index, newName);
}

void Inventory::deleteBaseLayout(ShapeModel::Type type, int index)
{
    m_controller->deleteLayoutForBaseShape(type, index);
}

QList<LayoutData> Inventory::getLayoutsForBaseShape(ShapeModel::Type type) const
{
    return InventoryMutationService::getLayoutsForBaseShape(m_baseShapeLayouts, type);
}

void Inventory::incrementLayoutUsage(const QString &shapeName, int index)
{
    m_controller->incrementLayoutUsageForCustomShape(shapeName, index);
}

void Inventory::incrementBaseLayoutUsage(ShapeModel::Type type, int index)
{
    m_controller->incrementLayoutUsageForBaseShape(type, index);
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
            if (!m_controller->removeBaseShapeFromFolderToParent(type))
                return;
            if (inFolderView)
                displayShapesInFolder(currentFolder, ui->searchBar->text());
            else
                displayShapes();
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
            if (!m_controller->createFolder(cleanName, parent))
                return;
            if (!m_controller->moveBaseShapeToFolder(type, cleanName))
                return;

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
                if (!m_controller->moveBaseShapeToFolder(type, folder.name))
                    return;
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
            if (!m_controller->renameFolder(folderName, newName.trimmed()))
                return;
            label->setText(newName.trimmed());
            displayShapes();
        }
    });

    connect(deleteAction, &QAction::triggered, this, [this, folderName]() {
        if (!m_controller->deleteFolder(folderName))
            return;
        displayShapes();
    });

    connect(menuButton, &QPushButton::clicked, this, [menu, menuButton]() {
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });

    return frame;
}

void Inventory::displayShapesInFolder(const QString &folderName, const QString &filter)
{
    emit folderOpenRequested(folderName);
    const InventoryViewState state = m_controller->buildFolderState(folderName,
                                                                    filter,
                                                                    currentSortMode(),
                                                                    currentFilterMode());
    renderState(state);
}

void Inventory::renderState(const InventoryViewState &state)
{
    if (!ui->scrollAreaInventory)
        return;

    if (QWidget *oldWidget = ui->scrollAreaInventory->widget()) {
        ui->scrollAreaInventory->takeWidget();
        oldWidget->deleteLater();
    }

    inFolderView = state.inFolderView;
    currentFolder = state.currentFolder;

    auto *listWidget = new QListWidget();
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setResizeMode(QListView::Adjust);
    listWidget->setMovement(QListView::Snap);
    listWidget->setSpacing(25);

    ui->scrollAreaInventory->setWidget(listWidget);
    ui->scrollAreaInventory->setWidgetResizable(true);

    for (const InventoryViewItem &entry : state.items) {
        QFrame *frame = nullptr;
        int type = -1;
        QVariant payload;

        if (entry.kind == InventoryViewItem::Kind::Folder) {
            frame = createFolderCard(entry.displayName);
            type = 0;
            payload = entry.payload;
        } else if (entry.kind == InventoryViewItem::Kind::BaseShape) {
            const auto shapeType = static_cast<ShapeModel::Type>(entry.payload.toInt());
            frame = createBaseShapeCard(shapeType, entry.displayName);
            type = 1;
            payload = entry.payload;
        } else if (entry.kind == InventoryViewItem::Kind::CustomShape) {
            frame = addCustomShapeToGrid(entry.payload.toInt());
            type = 2;
            payload = entry.displayName;
        }

        if (!frame)
            continue;

        auto *item = new QListWidgetItem(listWidget);
        item->setSizeHint(frame->size());
        item->setData(Qt::UserRole, type);
        item->setData(Qt::UserRole + 1, payload);
        listWidget->addItem(item);
        listWidget->setItemWidget(item, frame);
    }

    if (state.inFolderView) {
        auto *retourButton = new QPushButton("← Retour");
        retourButton->setStyleSheet("color: white; background-color: red; font-size: 14px;");
        retourButton->setFixedSize(120, 40);
        connect(retourButton, &QPushButton::clicked, this, [this]() {
            emit returnRequested();
            QString parent = InventoryQueryService::parentFolderOf(m_folders, currentFolder);
            if (parent.isEmpty())
                displayShapes();
            else
                displayShapesInFolder(parent, "");
        });
        auto *backItem = new QListWidgetItem(listWidget);
        backItem->setSizeHint(retourButton->size());
        listWidget->addItem(backItem);
        listWidget->setItemWidget(backItem, retourButton);
    }

    connect(listWidget, &QListWidget::itemClicked, this, &Inventory::onItemClicked);
    ui->buttonClearSearch->setVisible(!state.inFolderView);
    ui->buttonMenu->setVisible(!state.inFolderView);
    update();
}

InventorySortMode Inventory::currentSortMode() const
{
    const int mode = ui->comboSort ? ui->comboSort->currentIndex() : 0;
    return static_cast<InventorySortMode>(mode);
}

InventoryFilterMode Inventory::currentFilterMode() const
{
    const int mode = ui->comboFilter ? ui->comboFilter->currentIndex() : 0;
    return static_cast<InventoryFilterMode>(mode);
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
