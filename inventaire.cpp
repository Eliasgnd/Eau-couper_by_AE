#include "inventaire.h"
#include "ui_inventaire.h"
#include "ShapeModel.h"
#include "MainWindow.h"
#include "Language.h"
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
#include <QMenu>
#include <QInputDialog>
#include <QPushButton>
#include <QDebug>
#include <qmessagebox.h>
#include "ScreenUtils.h"

Inventaire* Inventaire::instance = nullptr; // Initialisation du singleton

Inventaire::Inventaire(QWidget *parent)
    : QWidget(parent), ui(new Ui::Inventaire)
{
    ui->setupUi(this);

    loadCustomShapes();

    updateTranslations(currentLanguage);

    ScreenUtils::placeOnSecondaryScreen(this);

    connect(ui->buttonMenu, &QPushButton::clicked, this, &Inventaire::goToMainWindow);
    displayShapes();
}

Inventaire::~Inventaire()
{
    qDebug() << "Fermeture de l'Inventaire (l'instance reste vivante via le singleton)";
    delete ui;
}

Inventaire* Inventaire::getInstance()
{
    if (!instance) {
        qDebug() << "Création de l'instance Inventaire";
        instance = new Inventaire();
    }
    return instance;
}

void Inventaire::goToMainWindow()
{
    this->hide();
    MainWindow::getInstance()->showFullScreen();
}



//
// Affiche les formes prédéfinies suivies des formes custom dans le scrollArea.
//
void Inventaire::displayShapes()
{
    if (!ui->scrollAreaInventaire)
        return;

    // Supprime l'ancien widget d'affichage
    QWidget *oldWidget = ui->scrollAreaInventaire->widget();
    if (oldWidget) {
        ui->scrollAreaInventaire->takeWidget();
        oldWidget->deleteLater();
    }

    QWidget *scrollWidget = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(scrollWidget);
    gridLayout->setSpacing(25);
    gridLayout->setAlignment(Qt::AlignTop);

    ui->scrollAreaInventaire->setWidget(scrollWidget);
    ui->scrollAreaInventaire->setWidgetResizable(true);

    int row = 0, col = 0;

    // Affichage des formes prédéfinies
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
        QGraphicsScene *scene = new QGraphicsScene();
        QGraphicsView *view = new QGraphicsView(scene);
        view->setFixedSize(120, 150);
        view->setStyleSheet("background-color: white;");
        view->setAlignment(Qt::AlignCenter);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        // Prévisualisation avec une forme 70x70
        QList<QGraphicsItem*> shapes = ShapeModel::generateShapes(shapeInfo.second, 70, 70);
        if (!shapes.isEmpty()) {
            QGraphicsItem *shapeItem = shapes.first();
            scene->addItem(shapeItem);
            scene->setSceneRect(shapeItem->boundingRect().adjusted(-5, -5, 5, 5));
            view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
        }

        QFrame *frame = new QFrame();
        frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
        frame->setFixedSize(150, 220);

        QLabel *label = new QLabel(shapeInfo.first);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("color: black; font-size: 16px;");

        QVBoxLayout *frameLayout = new QVBoxLayout(frame);
        frameLayout->setContentsMargins(5, 5, 5, 5);
        frameLayout->setSpacing(5);
        frameLayout->addWidget(view, 0, Qt::AlignCenter);
        frameLayout->addWidget(label);

        frame->setProperty("shapeType", static_cast<int>(shapeInfo.second));
        frame->setCursor(Qt::PointingHandCursor);
        frame->installEventFilter(this);

        gridLayout->addWidget(frame, row, col);
        if (++col >= 7) {
            col = 0;
            row++;
        }
    }

    // Affichage des formes custom sauvegardées
    for (int i = 0; i < m_customShapes.size(); ++i) {
        if (col >= 7) {
            col = 0;
            row++;
        }
        QFrame* customFrame = addCustomShapeToGrid(i);
        gridLayout->addWidget(customFrame, row, col);
        col++;
    }
    this->update();
}

//
// Construit et retourne la vignette (QFrame) correspondant à la forme custom à l'index donné.
//
QFrame* Inventaire::addCustomShapeToGrid(int index)
{
    if (index < 0 || index >= m_customShapes.size())
        return nullptr;

    const CustomShapeData &data = m_customShapes.at(index);

    QGraphicsScene *scene = new QGraphicsScene();
    QPainterPath path;
    for (const QPolygonF &poly : data.polygons) {
        path.addPolygon(poly);
    }
    QGraphicsPathItem *item = new QGraphicsPathItem(path);
    item->setPen(QPen(Qt::black, 4));
    item->setBrush(Qt::NoBrush);
    scene->addItem(item);
    scene->setSceneRect(item->boundingRect().adjusted(-5, -5, 5, 5));

    QGraphicsView *view = new QGraphicsView(scene);
    view->setFixedSize(120, 120);
    view->setStyleSheet("background-color: white;");
    view->setAlignment(Qt::AlignCenter);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    QFrame *frame = new QFrame();
    frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
    frame->setFixedSize(150, 220);

    QLabel *label = new QLabel(data.name);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: black; font-size: 16px;");

    // Bouton de menu pour renommer/supprimer
    QPushButton *menuButton = new QPushButton("...");
    menuButton->setFixedSize(25, 25);
    menuButton->setStyleSheet("border: none; font-size: 14px;");

    QMenu *menu = new QMenu(menuButton);
    QAction *renameAction = new QAction(currentLanguage == Language::French ? "Renommer" : "Rename", menu);
    QAction *deleteAction = new QAction(currentLanguage == Language::French ? "Supprimer" : "Delete", menu);
    menu->addAction(renameAction);
    menu->addAction(deleteAction);

    connect(menuButton, &QPushButton::clicked, [menu, menuButton]() {
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });
    connect(renameAction, &QAction::triggered, [this, label, index]() {
        bool ok;
        QString newName = QInputDialog::getText(nullptr,
                                                currentLanguage == Language::French ? "Renommer la forme" : "Rename shape",
                                                currentLanguage == Language::French ? "Nouveau nom :" : "New name:",
                                                QLineEdit::Normal,
                                                label->text(), &ok);
        if (ok && !newName.isEmpty()) {
            label->setText(newName);
            if (index >= 0 && index < m_customShapes.size()) {
                m_customShapes[index].name = newName;
                saveCustomShapes();
            }
        }
    });
    connect(deleteAction, &QAction::triggered, [this, frame]() {
    // Récupère l'index stocké dans la frame au moment du build
    bool ok;
    int idx = frame->property("CustomShapeIndex").toInt(&ok);
    if (ok && idx >= 0 && idx < m_customShapes.size()) {
        m_customShapes.removeAt(idx);
        saveCustomShapes();
        }
    // Reconstruit tout l'inventaire :
    displayShapes();
    });

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    headerLayout->addWidget(menuButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(frame);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(view, 0, Qt::AlignCenter);
    mainLayout->addWidget(label);

    // Stocke l'index de la forme custom pour identifier le clic dans eventFilter
    frame->setProperty("CustomShapeIndex", index);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);
    return frame;
}

//
// Permet de sauvegarder une forme custom (liste de QPolygonF) avec un nom donné.
//
void Inventaire::addSavedCustomShape(const QList<QPolygonF> &polygons, const QString &name)
{
    if (shapeNameExists(name)) {
        qDebug() << "Forme déjà enregistrée avec ce nom";
        return;
    }
    CustomShapeData newData;
    newData.polygons = polygons;
    newData.name = name;
    m_customShapes.append(newData);

    saveCustomShapes();

    displayShapes();
}

//
// Filtre les événements (clic) sur les QFrame des vignettes
//
bool Inventaire::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *frame = qobject_cast<QFrame*>(obj);
        if (frame) {
            if (frame->property("shapeType").isValid()) {
                int val = frame->property("shapeType").toInt();
                ShapeModel::Type type = static_cast<ShapeModel::Type>(val);
                emit shapeSelected(type, frame->width(), frame->height());
                goToMainWindow();
                return true;
            }

//        checkCustom:
            // Si ce n'est pas une forme prédéfinie, vérifier la propriété custom.
            if (frame->property("CustomShapeIndex").isValid()) {
                int index = frame->property("CustomShapeIndex").toInt();
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

void Inventaire::updateTranslations(Language lang)
{
    currentLanguage = lang;
    if (ui->buttonMenu) {
        ui->buttonMenu->setText("");
    }
    displayShapes();
}

void Inventaire::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        displayShapes();
    }
    QWidget::changeEvent(event);
}

QString Inventaire::customShapesFilePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir().mkpath(dir);
    return dir + "/custom_shapes.json";
}

bool Inventaire::shapeNameExists(const QString &name) const
{
    for (const auto &shapeData : m_customShapes) {
        if (shapeData.name == name)
            return true;
    }
    return false;
}

void Inventaire::loadCustomShapes()
{
    QFile file(customShapesFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;
    QJsonArray arr = doc.object().value("shapes").toArray();
    for (const QJsonValue &val : arr) {
        if (!val.isObject())
            continue;
        QJsonObject obj = val.toObject();
        CustomShapeData data;
        data.name = obj.value("name").toString();
        QJsonArray polyArr = obj.value("polygons").toArray();
        for (const QJsonValue &polyVal : polyArr) {
            QJsonArray ptArrList = polyVal.toArray();
            QPolygonF poly;
            for (const QJsonValue &ptVal : ptArrList) {
                QJsonArray p = ptVal.toArray();
                if (p.size() >= 2)
                    poly.append(QPointF(p.at(0).toDouble(), p.at(1).toDouble()));
            }
            data.polygons.append(poly);
        }
        QJsonArray layoutsArr = obj.value("layouts").toArray();
        for (const QJsonValue &layoutVal : layoutsArr) {
            if (!layoutVal.isObject())
                continue;
            QJsonObject lo = layoutVal.toObject();
            LayoutData ld;
            ld.name = lo.value("name").toString();
            ld.largeur = lo.value("largeur").toInt();
            ld.longueur = lo.value("longueur").toInt();
            ld.spacing = lo.value("spacing").toInt();
            QJsonArray itemsArr = lo.value("items").toArray();
            for (const QJsonValue &itVal : itemsArr) {
                QJsonObject io = itVal.toObject();
                LayoutItem li;
                li.x = io.value("x").toDouble();
                li.y = io.value("y").toDouble();
                li.rotation = io.value("rotation").toDouble();
                ld.items.append(li);
            }
            data.layouts.append(ld);
        }
        m_customShapes.append(data);
    }

    QJsonObject baseObj = doc.object().value("baseLayouts").toObject();
    for (auto it = baseObj.begin(); it != baseObj.end(); ++it) {
        ShapeModel::Type type = static_cast<ShapeModel::Type>(it.key().toInt());
        QJsonArray layoutsArr = it.value().toArray();
        QList<LayoutData> list;
        for (const QJsonValue &layoutVal : layoutsArr) {
            if (!layoutVal.isObject())
                continue;
            QJsonObject lo = layoutVal.toObject();
            LayoutData ld;
            ld.name = lo.value("name").toString();
            ld.largeur = lo.value("largeur").toInt();
            ld.longueur = lo.value("longueur").toInt();
            ld.spacing = lo.value("spacing").toInt();
            QJsonArray itemsArr = lo.value("items").toArray();
            for (const QJsonValue &itVal : itemsArr) {
                QJsonObject io = itVal.toObject();
                LayoutItem li;
                li.x = io.value("x").toDouble();
                li.y = io.value("y").toDouble();
                li.rotation = io.value("rotation").toDouble();
                ld.items.append(li);
            }
            list.append(ld);
        }
        m_baseShapeLayouts[type] = list;
    }
}

void Inventaire::saveCustomShapes() const
{
    QJsonArray arr;
    for (const CustomShapeData &data : m_customShapes) {
        QJsonObject obj;
        obj["name"] = data.name;
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
        QJsonArray layoutsArr;
        for (const LayoutData &ld : data.layouts) {
            QJsonObject lo;
            lo["name"] = ld.name;
            lo["largeur"] = ld.largeur;
            lo["longueur"] = ld.longueur;
            lo["spacing"] = ld.spacing;
            QJsonArray itemsArr;
            for (const LayoutItem &li : ld.items) {
                QJsonObject io;
                io["x"] = li.x;
                io["y"] = li.y;
                io["rotation"] = li.rotation;
                itemsArr.append(io);
            }
            lo["items"] = itemsArr;
            layoutsArr.append(lo);
        }
        obj["layouts"] = layoutsArr;
        arr.append(obj);
    }
    QJsonObject rootObj;
    rootObj["shapes"] = arr;

    QJsonObject baseObj;
    for (auto it = m_baseShapeLayouts.constBegin(); it != m_baseShapeLayouts.constEnd(); ++it) {
        QJsonArray layoutsArr;
        for (const LayoutData &ld : it.value()) {
            QJsonObject lo;
            lo["name"] = ld.name;
            lo["largeur"] = ld.largeur;
            lo["longueur"] = ld.longueur;
            lo["spacing"] = ld.spacing;
            QJsonArray itemsArr;
            for (const LayoutItem &li : ld.items) {
                QJsonObject io;
                io["x"] = li.x;
                io["y"] = li.y;
                io["rotation"] = li.rotation;
                itemsArr.append(io);
            }
            lo["items"] = itemsArr;
            layoutsArr.append(lo);
        }
        baseObj[QString::number(static_cast<int>(it.key()))] = layoutsArr;
    }
    rootObj["baseLayouts"] = baseObj;
    QJsonDocument doc(rootObj);
    QFile file(customShapesFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

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
        if (data.name == shapeName) {
            if (index >= 0 && index < data.layouts.size()) {
                data.layouts[index].name = newName;
                saveCustomShapes();
            }
            return;
        }
    }
}

void Inventaire::deleteLayout(const QString &shapeName, int index)
{
    for (CustomShapeData &data : m_customShapes) {
        if (data.name == shapeName) {
            if (index >= 0 && index < data.layouts.size()) {
                data.layouts.removeAt(index);
                saveCustomShapes();
            }
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

QString Inventaire::baseShapeName(ShapeModel::Type type, Language lang)
{
    switch (type) {
    case ShapeModel::Type::Circle:
        return lang == Language::French ? QStringLiteral("Cercle") : QStringLiteral("Circle");
    case ShapeModel::Type::Rectangle:
        return lang == Language::French ? QStringLiteral("Rectangle") : QStringLiteral("Rectangle");
    case ShapeModel::Type::Triangle:
        return lang == Language::French ? QStringLiteral("Triangle") : QStringLiteral("Triangle");
    case ShapeModel::Type::Star:
        return lang == Language::French ? QStringLiteral("Étoile") : QStringLiteral("Star");
    case ShapeModel::Type::Heart:
        return lang == Language::French ? QStringLiteral("Cœur") : QStringLiteral("Heart");
    }
    return {};
}
