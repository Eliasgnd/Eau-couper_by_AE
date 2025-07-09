#include "Dispositions.h"
#include "ui_Dispositions.h"

#include <QGridLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QEvent>
#include <QPainterPath>
#include <QTransform>
#include <QSizePolicy>

Dispositions::Dispositions(const QList<LayoutData> &layouts,
                           const QList<QPolygonF> &shapePolygons,
                           Language lang,
                           QWidget *parent)
    : QWidget(parent),
    ui(new Ui::Dispositions),
    m_layouts(layouts),
    m_polygons(shapePolygons),
    m_lang(lang)
{
    ui->setupUi(this);

    setWindowTitle(m_lang == Language::French ? tr("Dispositions") : tr("Layouts"));
    resize(800, 600);
    setWindowState(Qt::WindowFullScreen);
    QTimer::singleShot(0, this, &QWidget::showFullScreen);

    connect(ui->buttonMenu, &QPushButton::clicked,
            this, &Dispositions::onMenuButtonClicked);
    connect(ui->closeBtn,  &QPushButton::clicked,
            this, &Dispositions::onCloseButtonClicked);

    /* --------------------- grille --------------------- */
    if (ui->gridLayout) {
        ui->gridLayout->setSpacing(20);
        /* Aligne tout en haut-gauche */
        ui->gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        /* 1) carte « forme seule » */
        QFrame *shapeFrame = createBaseShapeFrame();
        ui->gridLayout->addWidget(shapeFrame, 0, 0);

        /* 2) cartes des dispositions */
        for (int i = 0; i < m_layouts.size(); ++i) {
            QFrame *frame = createLayoutFrame(i);
            int pos = i + 1;                     // décalage d’une colonne
            ui->gridLayout->addWidget(frame, pos / 4, pos % 4);
        }
    }

    ui->closeBtn->setText(m_lang == Language::French ? tr("Fermer")
                                                     : tr("Cancel"));
}

Dispositions::~Dispositions()
{
    delete ui;
}

/* --------------------- slots --------------------- */
void Dispositions::onMenuButtonClicked()
{
    emit requestOpenInventaire();
    close();
}

void Dispositions::onCloseButtonClicked()
{
    emit closed();
    close();
}

/* --------------------- création d’une carte --------------------- */
QFrame *Dispositions::createLayoutFrame(int index)
{
    if (index < 0 || index >= m_layouts.size())
        return nullptr;

    const LayoutData &ld = m_layouts.at(index);

    /* --- scène miniature --- */
    QGraphicsScene *scene = new QGraphicsScene();
    scene->addRect(0, 0, ld.largeur, ld.longueur,
                   QPen(Qt::black), Qt::NoBrush);

    QPainterPath combinedPath;
    for (const QPolygonF &poly : m_polygons)
        combinedPath.addPolygon(poly);

    /* mise à l’échelle pour s’adapter au rectangle de la disposition */
    QRectF bounds = combinedPath.boundingRect();
    const qreal scaleX = bounds.width()  > 0 ? qreal(ld.largeur)  / bounds.width()  : 1.0;
    const qreal scaleY = bounds.height() > 0 ? qreal(ld.longueur) / bounds.height() : 1.0;

    QTransform scale;
    scale.scale(scaleX, scaleY);
    QPainterPath scaledPath = scale.map(combinedPath);

    for (const LayoutItem &li : ld.items) {
        QGraphicsPathItem *item = new QGraphicsPathItem(scaledPath);
        item->setPen(QPen(Qt::black, 1));
        item->setBrush(Qt::NoBrush);
        item->setTransformOriginPoint(item->boundingRect().center());
        item->setRotation(li.rotation);

        QRectF br = item->boundingRect();
        QPointF offset(-br.x(), -br.y());
        item->setPos(li.x + offset.x(), li.y + offset.y());

        scene->addItem(item);
    }

    scene->setSceneRect(scene->itemsBoundingRect().adjusted(-5, -5, 5, 5));

    /* --- vue miniature --- */
    QGraphicsView *view = new QGraphicsView(scene);
    view->setFixedSize(300, 200);
    view->setRenderHint(QPainter::Antialiasing);
    view->setStyleSheet("background-color: white;");
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    /* --- cadre extérieur --- */
    QFrame *frame = new QFrame();
    frame->setStyleSheet("background-color: white; "
                         "border: 2px solid black; "
                         "border-radius: 15px;");
    /* taille fixe → pas d’étirement */
    frame->setFixedSize(350, 250);
    frame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel *label = new QLabel(ld.name);
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    layout->addWidget(view, 0, Qt::AlignCenter);
    layout->addWidget(label);

    /* propriétés/filtre d’évènement pour le clic */
    frame->setProperty("layoutIndex", index);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);

    return frame;
}

/* --------------------- carte « forme seule » --------------------- */
QFrame *Dispositions::createBaseShapeFrame()
{
    QGraphicsScene *scene = new QGraphicsScene();

    QPainterPath combinedPath;
    for (const QPolygonF &poly : m_polygons)
        combinedPath.addPolygon(poly);

    scene->addPath(combinedPath, QPen(Qt::black, 1), Qt::NoBrush);
    scene->setSceneRect(combinedPath.boundingRect().adjusted(-5, -5, 5, 5));

    QGraphicsView *view = new QGraphicsView(scene);
    view->setFixedSize(300, 200);
    view->setRenderHint(QPainter::Antialiasing);
    view->setStyleSheet("background-color: white;");
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QFrame *frame = new QFrame();
    frame->setStyleSheet("background-color: white; "
                         "border: 2px solid black; "
                         "border-radius: 15px;");
    frame->setFixedSize(325, 250);
    frame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel *label = new QLabel(m_lang == Language::French
                                   ? "Forme seule"
                                   : "Shape only");
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    layout->addWidget(view, 0, Qt::AlignCenter);
    layout->addWidget(label);

    frame->setProperty("layoutIndex", -1);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);

    return frame;
}

/* --------------------- filtrage du clic sur une carte --------------------- */
bool Dispositions::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto *frame = qobject_cast<QFrame *>(obj);
        if (frame && frame->property("layoutIndex").isValid()) {
            int idx = frame->property("layoutIndex").toInt();
            if (idx == -1) {                 // « forme seule »
                emit shapeOnlySelected();
            } else if (idx >= 0 && idx < m_layouts.size()) {
                emit layoutSelected(m_layouts.at(idx));
            }
            emit closed();
            close();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
