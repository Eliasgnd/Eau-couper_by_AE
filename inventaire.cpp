#include "Inventaire.h"
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
#include <QCompleter>
#include <QStringListModel>
#include <QDateTime>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QPushButton>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>

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

    updateTranslations(currentLanguage);

    // Place window on secondary screen if available
    ScreenUtils::placeOnSecondaryScreen(this);

    // Navigation
    connect(ui->buttonMenu, &QPushButton::clicked, this, &Inventaire::goToMainWindow);

    // Search bar
    connect(ui->searchBar, &QLineEdit::textChanged, this, &Inventaire::onSearchTextChanged);
    connect(ui->buttonClearSearch, &QPushButton::clicked, this, &Inventaire::onClearSearchClicked);

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

    QWidget *scrollWidget = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(scrollWidget);
    gridLayout->setSpacing(25);
    gridLayout->setAlignment(Qt::AlignTop);

    ui->scrollAreaInventaire->setWidget(scrollWidget);
    ui->scrollAreaInventaire->setWidgetResizable(true);

    int row = 0;
    int col = 0;
    const QString f = filter.trimmed().toLower();

    // ---------------------------------------------------------------------
    // Built-in shapes
    // ---------------------------------------------------------------------
    QList<QPair<QString, ShapeModel::Type>> shapeList;
    if (currentLanguage == Language::French) {
        shapeList = {
            {"Cercle",    ShapeModel::Type::Circle},
            {"Rectangle", ShapeModel::Type::Rectangle},
            {"Triangle",  ShapeModel::Type::Triangle},
            {"Étoile",    ShapeModel::Type::Star},
            {"Cœur",      ShapeModel::Type::Heart}
        };
    } else {
        shapeList = {
            {"Circle",    ShapeModel::Type::Circle},
            {"Rectangle", ShapeModel::Type::Rectangle},
            {"Triangle",  ShapeModel::Type::Triangle},
            {"Star",      ShapeModel::Type::Star},
            {"Heart",     ShapeModel::Type::Heart}
        };
    }

    for (const auto &shapeInfo : shapeList) {
        if (!f.isEmpty() && !shapeInfo.first.toLower().contains(f))
            continue; // name does not match filter

        // Thumbnail scene
        auto *scene = new QGraphicsScene();
        auto *view  = new QGraphicsView(scene);
        view->setFixedSize(120, 150);
        view->setStyleSheet("background-color: white;");
        view->setAlignment(Qt::AlignCenter);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        const QList<QGraphicsItem*> shapes = ShapeModel::generateShapes(shapeInfo.second, 70, 70);
        if (!shapes.isEmpty()) {
            QGraphicsItem *shapeItem = shapes.first();
            scene->addItem(shapeItem);
            scene->setSceneRect(shapeItem->boundingRect().adjusted(-5, -5, 5, 5));
            view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
        }

        auto *frame = new QFrame();
        frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
        frame->setFixedSize(150, 220);

        auto *label = new QLabel(shapeInfo.first);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: black; font-size: 16px;");

        auto *frameLayout = new QVBoxLayout(frame);
        frameLayout->setContentsMargins(5, 5, 5, 5);
        frameLayout->setSpacing(5);
        frameLayout->addWidget(view, 0, Qt::AlignCenter);
        frameLayout->addWidget(label);

        // Store built-in shape type
        frame->setProperty("shapeType", static_cast<int>(shapeInfo.second));
        frame->setCursor(Qt::PointingHandCursor);
        frame->installEventFilter(this);

        gridLayout->addWidget(frame, row, col);
        if (++col >= 7) {
            col = 0;
            ++row;
        }
    }

    // ---------------------------------------------------------------------
    // Custom shapes (filtered)
    // ---------------------------------------------------------------------
    for (int i = 0; i < m_customShapes.size(); ++i) {
        const CustomShapeData &data = m_customShapes.at(i);

        if (!f.isEmpty() && !data.name.toLower().contains(f))
            continue;

        if (col >= 7) {
            col = 0;
            ++row;
        }

        QFrame *customFrame = addCustomShapeToGrid(i);
        gridLayout->addWidget(customFrame, row, col);
        ++col;
    }

    update();

    updateCompleter();
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

    connect(menuButton, &QPushButton::clicked, [menu, menuButton]() {
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
            updateCompleter();
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
        updateCompleter();
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
    updateCompleter();
}

// -----------------------------------------------------------------------------
// Event filter – detect clicks on thumbnails
// -----------------------------------------------------------------------------
bool Inventaire::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (auto *frame = qobject_cast<QFrame*>(obj)) {
            // Built-in shape ?
            if (frame->property("shapeType").isValid()) {
                const int val = frame->property("shapeType").toInt();
                ShapeModel::Type type = static_cast<ShapeModel::Type>(val);
                emit shapeSelected(type, frame->width(), frame->height());
                goToMainWindow();
                return true;
            }
            // Custom shape ?
            if (frame->property("CustomShapeIndex").isValid()) {
                const int index = frame->property("CustomShapeIndex").toInt();
                if (index >= 0 && index < m_customShapes.size()) {
                    const CustomShapeData &data = m_customShapes.at(index);
                    qDebug() << "[EVENT] Sélection d'une forme custom:" << data.name;
                    emit customShapeSelected(data.polygons, data.name);
                    goToMainWindow();
                }
                return true;
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

    QFile file(customShapesFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;

    const QJsonObject root = doc.object();

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
            ld.lastUsed  = lo.value("lastUsed").toVariant().toLongLong();
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
            ld.lastUsed = lo.value("lastUsed").toVariant().toLongLong();
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

    updateCompleter();
}

void Inventaire::saveCustomShapes() const
{
    QJsonArray shapesArr;
    for (const CustomShapeData &data : m_customShapes) {
        QJsonObject obj;
        obj["name"] = data.name;

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
            lo["lastUsed"] = static_cast<qint64>(ld.lastUsed);
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
            lo["lastUsed"] = static_cast<qint64>(ld.lastUsed);
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

    QJsonObject rootObj;
    rootObj["shapes"]      = shapesArr;
    rootObj["baseLayouts"] = baseObj;

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
    displayShapes(text);
}

void Inventaire::onClearSearchClicked()
{
    if (ui->searchBar)
        ui->searchBar->clear();
    displayShapes();
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

void Inventaire::updateCompleter()
{
    if (!ui->searchBar)
        return;

    if (!m_completer) {
        m_completer = new QCompleter(this);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        ui->searchBar->setCompleter(m_completer);
    }

    QStringList names = getAllShapeNames();
    auto *model = new QStringListModel(names, m_completer);
    m_completer->setModel(model);
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

void Inventaire::updateLayoutLastUsed(const QString &shapeName, int index)
{
    for (CustomShapeData &data : m_customShapes) {
        if (data.name == shapeName && index >= 0 && index < data.layouts.size()) {
            data.layouts[index].lastUsed = QDateTime::currentSecsSinceEpoch();
            saveCustomShapes();
            return;
        }
    }
}

void Inventaire::updateBaseLayoutLastUsed(ShapeModel::Type type, int index)
{
    auto it = m_baseShapeLayouts.find(type);
    if (it != m_baseShapeLayouts.end() && index >= 0 && index < it.value().size()) {
        it.value()[index].lastUsed = QDateTime::currentSecsSinceEpoch();
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
