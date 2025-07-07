#include "inventaire.h"
#include "ui_inventaire.h"
#include "ShapeModel.h"
#include "MainWindow.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
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
    MainWindow::getInstance()->show();
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
    const QList<QPair<QString, ShapeModel::Type>> shapeList = {
        {"Cercle",    ShapeModel::Type::Circle},
        {"Rectangle", ShapeModel::Type::Rectangle},
        {"Triangle",  ShapeModel::Type::Triangle},
        {"Étoile",    ShapeModel::Type::Star},
        {"Cœur",      ShapeModel::Type::Heart}
    };

    for (const auto &shapeInfo : shapeList) {
        QGraphicsScene *scene = new QGraphicsScene();
        QGraphicsView *view = new QGraphicsView(scene);
        view->setFixedSize(120, 120);
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
    QAction *renameAction = new QAction("Renommer", menu);
    QAction *deleteAction = new QAction("Supprimer", menu);
    menu->addAction(renameAction);
    menu->addAction(deleteAction);

    connect(menuButton, &QPushButton::clicked, [menu, menuButton]() {
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });
    connect(renameAction, &QAction::triggered, [this, label, index]() {
        bool ok;
        QString newName = QInputDialog::getText(nullptr, "Renommer la forme",
                                                "Nouveau nom :", QLineEdit::Normal,
                                                label->text(), &ok);
        if (ok && !newName.isEmpty()) {
            label->setText(newName);
            if (index >= 0 && index < m_customShapes.size()) {
                m_customShapes[index].name = newName;
            }
        }
    });
    connect(deleteAction, &QAction::triggered, [this, frame]() {
    // Récupère l'index stocké dans la frame au moment du build
    bool ok;
    int idx = frame->property("CustomShapeIndex").toInt(&ok);
    if (ok && idx >= 0 && idx < m_customShapes.size()) {
        m_customShapes.removeAt(idx);
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
    // Optionnel : vérifier si une forme avec ce nom existe déjà
    for (const auto &shapeData : m_customShapes) {
        if (shapeData.name == name) {
            qDebug() << "Forme déjà enregistrée avec ce nom";
            return;
        }
    }
    CustomShapeData newData;
    newData.polygons = polygons;
    newData.name = name;
    m_customShapes.append(newData);

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
            // Tenter de récupérer le label contenu dans le cadre
            QLabel *label = frame->findChild<QLabel*>();
            if (label) {
                QString shapeName = label->text().trimmed();
                // Debug pour vérifier la valeur lue
                qDebug() << "[DEBUG] shapeName =" << shapeName;

                bool isPredef = false;
                ShapeModel::Type type = ShapeModel::Type::Circle; // initialisation par défaut

                // Comparaison insensible à la casse, en acceptant les versions avec ou sans accent.
                if (shapeName.compare("Cercle", Qt::CaseInsensitive) == 0) {
                    type = ShapeModel::Type::Circle;
                    isPredef = true;
                }
                else if (shapeName.compare("Rectangle", Qt::CaseInsensitive) == 0) {
                    type = ShapeModel::Type::Rectangle;
                    isPredef = true;
                }
                else if (shapeName.compare("Triangle", Qt::CaseInsensitive) == 0) {
                    type = ShapeModel::Type::Triangle;
                    isPredef = true;
                }
                else if (shapeName.compare("Étoile", Qt::CaseInsensitive) == 0 ||
                         shapeName.compare("Etoile", Qt::CaseInsensitive) == 0) {
                    type = ShapeModel::Type::Star;
                    isPredef = true;
                }
                else if (shapeName.compare("Cœur", Qt::CaseInsensitive) == 0 ||
                         shapeName.compare("Coeur", Qt::CaseInsensitive) == 0) {
                    type = ShapeModel::Type::Heart;
                    isPredef = true;
                }

                if (isPredef) {
                    qDebug() << "[EVENT] Sélection d'une forme prédéfinie:" << shapeName;
                    emit shapeSelected(type, frame->width(), frame->height());
                    goToMainWindow();
                    return true;
                }
            }
//        checkCustom:
            // Si ce n'est pas une forme prédéfinie, vérifier la propriété custom.
            if (frame->property("CustomShapeIndex").isValid()) {
                int index = frame->property("CustomShapeIndex").toInt();
                if (index >= 0 && index < m_customShapes.size()) {
                    const CustomShapeData &data = m_customShapes.at(index);
                    qDebug() << "[EVENT] Sélection d'une forme custom:" << data.name;
                    emit customShapeSelected(data.polygons);
                    goToMainWindow();
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
