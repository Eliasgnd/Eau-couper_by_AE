#include "Inventory.h"
#include "ui_Inventory.h"
#include "ShapeModel.h"
#include <QApplication>
#include "Language.h"
#include "ScreenUtils.h"
#include "InventoryViewModel.h"
#include "InventoryDomainTypes.h"

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



// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------
Inventory::Inventory(InventoryViewModel *vm, QWidget *parent)
    : QWidget(parent), ui(new Ui::Inventory)
{
    ui->setupUi(this);

    m_viewModel = vm;
    m_viewModel->setParent(this);
    connect(m_viewModel, &InventoryViewModel::stateChanged,
            this, &Inventory::renderState);

    updateTranslations(m_viewModel->language());

    // Place window on secondary screen if available
    ScreenUtils::placeOnSecondaryScreen(this);

    // Navigation
    connect(ui->buttonMenu, &QPushButton::clicked, this, &Inventory::goToMainWindow);

    // Search bar → ViewModel (setSearchText émet stateChanged → renderState)
    connect(ui->searchBar, &QLineEdit::textChanged,
            m_viewModel, &InventoryViewModel::setSearchText);
    connect(ui->buttonClearSearch, &QPushButton::clicked,
            this, &Inventory::onClearSearchClicked);
    connect(ui->buttonNewFolder, &QPushButton::clicked,
            this, &Inventory::onCreateFolderClicked);

    if (ui->comboSort) {
        ui->comboSort->addItem("Nom (A \342\206\222 Z)");
        ui->comboSort->addItem("Nom (Z \342\206\222 A)");
        ui->comboSort->addItem("Utilisation fr\303\251quente");
        ui->comboSort->addItem("R\303\251cent \342\206\222 Ancien");
        ui->comboSort->addItem("Ancien \342\206\222 R\303\251cent");
        // Sort → ViewModel
        connect(ui->comboSort, QOverload<int>::of(&QComboBox::currentIndexChanged),
                m_viewModel, [this](int idx) {
                    m_viewModel->setSortMode(static_cast<InventorySortMode>(idx));
                });
    }

    if (ui->comboFilter) {
        ui->comboFilter->addItem("Tout");
        ui->comboFilter->addItem("Dossiers uniquement");
        ui->comboFilter->addItem("Formes sans dossier");
        // Filter → ViewModel
        connect(ui->comboFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
                m_viewModel, [this](int idx) {
                    m_viewModel->setFilterMode(static_cast<InventoryFilterMode>(idx));
                });
    }

    // Affichage initial via le ViewModel
    m_viewModel->refresh();

}

Inventory::~Inventory()
{
    delete ui;
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
    emit navigationBackRequested();
}

// -----------------------------------------------------------------------------
// UI – Build the inventory grid
// -----------------------------------------------------------------------------
void Inventory::displayShapes(const QString &filter /* = QString() */)
{
    emit searchRequested(filter);
    m_viewModel->setFilterMode(currentFilterMode());
    m_viewModel->setSortMode(currentSortMode());
    m_viewModel->setSearchText(filter);
}

// -----------------------------------------------------------------------------
// Build a single custom shape thumbnail
// -----------------------------------------------------------------------------
QFrame* Inventory::addCustomShapeToGrid(int index)
{
    if (index < 0 || index >= m_viewModel->customShapes().size())
        return nullptr;

    const CustomShapeData &data = m_viewModel->customShapes().at(index);

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
    QAction *renameAction = menu->addAction(m_viewModel->language() == Language::French ? "Renommer"  : "Rename");
    QAction *deleteAction = menu->addAction(m_viewModel->language() == Language::French ? "Supprimer" : "Delete");

    const QString currentFolderName = m_viewModel->customShapes().at(index).folder;

    if (!currentFolderName.isEmpty()) {
        QAction *removeFromFolder = menu->addAction("❌ Retirer du dossier");
        connect(removeFromFolder, &QAction::triggered, this, [this, index]() {
            m_viewModel->removeCustomShapeFromFolderToParent(index);
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
            QString parent = m_viewModel->inFolderView() ? m_viewModel->currentFolder() : "";
            if (!m_viewModel->createFolder(cleanName, parent))
                return;
            m_viewModel->moveCustomShapeToFolder(index, cleanName);
        }
    });

        for (const InventoryFolder &folder : m_viewModel->folders()) {
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
                        m_viewModel->moveCustomShapeToFolder(idx, folder.name);
                    });
        }

    connect(menuButton, &QPushButton::clicked, [menu, menuButton]() {
//        menu->setMinimumWidth(200);  // force un espace suffisant
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });

    connect(renameAction, &QAction::triggered, [this, label, index]() {
        bool ok = false;
        const QString newName = QInputDialog::getText(nullptr,
                                                      m_viewModel->language() == Language::French ? tr("Renommer la forme") : tr("Rename shape"),
                                                      m_viewModel->language() == Language::French ? tr("Nouveau nom :")       : tr("New name:"),
                                                      QLineEdit::Normal,
                                                      label->text(),
                                                      &ok);
        if (ok && !newName.isEmpty() && index >= 0 && index < m_viewModel->customShapes().size()) {
            label->setText(newName);
            m_viewModel->renameCustomShape(index, newName);
        }
    });

    connect(deleteAction, &QAction::triggered, [this, frame]() {
        bool ok = false;
        const int idx = frame->property("CustomShapeIndex").toInt(&ok);
        if (ok)
            m_viewModel->deleteCustomShape(idx);
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
    if (m_viewModel->shapeNameExists(name)) {
        qDebug() << "Forme déjà enregistrée avec ce nom";
        return;
    }

    m_viewModel->addCustomShape(polygons, name);
}

// -----------------------------------------------------------------------------
// Internationalisation helpers
// -----------------------------------------------------------------------------
void Inventory::updateTranslations(Language lang)
{
    m_viewModel->setLanguage(lang);
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
    return m_viewModel->shapeNameExists(name);
}

// -----------------------------------------------------------------------------
// Search bar helpers
// -----------------------------------------------------------------------------
void Inventory::onSearchTextChanged(const QString &text)
{
    emit searchRequested(text);
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
        QString parent = m_viewModel->inFolderView() ? m_viewModel->currentFolder() : QString();
        m_viewModel->createFolder(clean, parent);
    }
}

void Inventory::onSortChanged(int)
{
    emit sortModeRequested(currentSortMode());
}

void Inventory::onFilterChanged(int)
{
    emit filterModeRequested(currentFilterMode());
}

QStringList Inventory::getAllShapeNames() const
{
    return m_viewModel->getAllShapeNames(m_viewModel->language());
}

// -----------------------------------------------------------------------------
// Layout management – Custom shapes
// -----------------------------------------------------------------------------
void Inventory::addLayoutToShape(const QString &shapeName, const LayoutData &layout)
{
    m_viewModel->addLayoutToCustomShape(shapeName, layout);
}

void Inventory::renameLayout(const QString &shapeName, int index, const QString &newName)
{
    m_viewModel->renameLayoutForCustomShape(shapeName, index, newName);
}

void Inventory::deleteLayout(const QString &shapeName, int index)
{
    m_viewModel->deleteLayoutForCustomShape(shapeName, index);
}

QList<LayoutData> Inventory::getLayoutsForShape(const QString &shapeName) const
{
    return m_viewModel->getLayoutsForShape(shapeName);
}

// -----------------------------------------------------------------------------
// Layout management – Built-in shapes
// -----------------------------------------------------------------------------
void Inventory::addLayoutToBaseShape(ShapeModel::Type type, const LayoutData &layout)
{
    m_viewModel->addLayoutToBaseShape(type, layout);
}

void Inventory::renameBaseLayout(ShapeModel::Type type, int index, const QString &newName)
{
    m_viewModel->renameLayoutForBaseShape(type, index, newName);
}

void Inventory::deleteBaseLayout(ShapeModel::Type type, int index)
{
    m_viewModel->deleteLayoutForBaseShape(type, index);
}

QList<LayoutData> Inventory::getLayoutsForBaseShape(ShapeModel::Type type) const
{
    return m_viewModel->getLayoutsForBaseShape(type);
}

void Inventory::incrementLayoutUsage(const QString &shapeName, int index)
{
    m_viewModel->incrementLayoutUsageForCustomShape(shapeName, index);
}

void Inventory::incrementBaseLayoutUsage(ShapeModel::Type type, int index)
{
    m_viewModel->incrementLayoutUsageForBaseShape(type, index);
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

    const QString currentFolderName = m_viewModel->baseShapeFolders().value(type);
    if (!currentFolderName.isEmpty()) {
        QAction *removeFromFolder = menu->addAction("❌ Retirer du dossier");
        connect(removeFromFolder, &QAction::triggered, this, [this, type]() {
            m_viewModel->removeBaseShapeFromFolderToParent(type);
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
            QString parent    = m_viewModel->inFolderView() ? m_viewModel->currentFolder() : "";
            if (!m_viewModel->createFolder(cleanName, parent))
                return;
            m_viewModel->moveBaseShapeToFolder(type, cleanName);
        }
    });

    // ➕ 2) Lister ensuite les dossiers existants (s’il y en a)
    for (const InventoryFolder &folder : m_viewModel->folders()) {
    if (inFolderView && folder.parentFolder != currentFolder) continue;
    if (!inFolderView && !folder.parentFolder.isEmpty())      continue;

    QAction *folderAction = folderSubMenu->addAction(folder.name);
    connect(folderAction, &QAction::triggered, this,
            [this, type, folder]() {
                m_viewModel->moveBaseShapeToFolder(type, folder.name);
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
            if (!m_viewModel->renameFolder(folderName, newName.trimmed()))
                return;
            label->setText(newName.trimmed());
        }
    });

    connect(deleteAction, &QAction::triggered, this, [this, folderName]() {
        m_viewModel->deleteFolder(folderName);
    });

    connect(menuButton, &QPushButton::clicked, this, [menu, menuButton]() {
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });

    return frame;
}

void Inventory::displayShapesInFolder(const QString &folderName, const QString &filter)
{
    Q_UNUSED(filter)
    emit folderOpenRequested(folderName);
    m_viewModel->navigateToFolder(folderName);
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
            m_viewModel->navigateBack();
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
    return m_viewModel->folderIsEmpty(folderName);
}


void Inventory::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;

    int t = item->data(Qt::UserRole).toInt();
    if (t == 0) {
        QString name = item->data(Qt::UserRole + 1).toString();
        m_viewModel->selectFolder(name);
    } else if (t == 1) {
        ShapeModel::Type type = static_cast<ShapeModel::Type>(item->data(Qt::UserRole + 1).toInt());
        m_viewModel->selectBaseShape(type);
        emit shapeSelected(type, 150, 220);
        goToMainWindow();
    } else if (t == 2) {
        QString name = item->data(Qt::UserRole + 1).toString();
        QList<QPolygonF> polygons;
        QString resolvedName;
        if (m_viewModel->selectCustomShape(name, polygons, resolvedName)) {
            emit customShapeSelected(polygons, resolvedName);
            goToMainWindow();
        }
    }
}

