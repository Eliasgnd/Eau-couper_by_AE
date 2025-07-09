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
    : QWidget(parent), ui(new Ui::Dispositions),
      m_layouts(layouts), m_polygons(shapePolygons), m_lang(lang)
{
    ui->setupUi(this);
    setWindowTitle(m_lang == Language::French ? tr("Dispositions") : tr("Layouts"));
    resize(800, 600);
    setWindowState(Qt::WindowFullScreen);
    QTimer::singleShot(0, this, &QWidget::showFullScreen);

    connect(ui->buttonMenu, &QPushButton::clicked, this, &Dispositions::onMenuButtonClicked);
    connect(ui->closeBtn, &QPushButton::clicked, this, &Dispositions::onCloseButtonClicked);

    if (ui->gridLayout) {
        ui->gridLayout->setSpacing(20);
        ui->gridLayout->setAlignment(Qt::AlignTop);

        QFrame *shapeFrame = createBaseShapeFrame();
        ui->gridLayout->addWidget(shapeFrame, 0, 0);

        for (int i = 0; i < m_layouts.size(); ++i) {
            QFrame *frame = createLayoutFrame(i);
            int pos = i + 1; // shift because of base frame
            ui->gridLayout->addWidget(frame, pos / 4, pos % 4);
        }
    }

    if (m_lang == Language::French)
        ui->closeBtn->setText(tr("Fermer"));
    else
        ui->closeBtn->setText(tr("Cancel"));
}

Dispositions::~Dispositions()
{
    delete ui;
}

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

QFrame* Dispositions::createLayoutFrame(int index)
{
    if (index < 0 || index >= m_layouts.size())
        return nullptr;
    const LayoutData &ld = m_layouts.at(index);

    QGraphicsScene *scene = new QGraphicsScene();
    scene->addRect(0, 0, ld.largeur, ld.longueur, QPen(Qt::black), Qt::NoBrush);

    QPainterPath combinedPath;
    for (const QPolygonF &poly : m_polygons)
        combinedPath.addPolygon(poly);

    QRectF bounds = combinedPath.boundingRect();
    qreal scaleX = (bounds.width() > 0) ? static_cast<qreal>(ld.largeur) / bounds.width() : 1.0;
    qreal scaleY = (bounds.height() > 0) ? static_cast<qreal>(ld.longueur) / bounds.height() : 1.0;

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

    QGraphicsView *view = new QGraphicsView(scene);
    view->setFixedSize(300, 200);
    view->setRenderHint(QPainter::Antialiasing);
    view->setStyleSheet("background-color: white;");
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    QFrame *frame = new QFrame();
    frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
    frame->setMinimumSize(325, 250);
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    QLabel *label = new QLabel(ld.name);
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    layout->addWidget(view, 0, Qt::AlignCenter);
    layout->addWidget(label);

    frame->setProperty("layoutIndex", index);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);
    return frame;
}

QFrame* Dispositions::createBaseShapeFrame()
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

    QFrame *frame = new QFrame();
    frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
    frame->setMinimumSize(325, 250);
    frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    QLabel *label = new QLabel(m_lang == Language::French ? "Forme seule" : "Shape only");
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

bool Dispositions::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *frame = qobject_cast<QFrame*>(obj);
        if (frame && frame->property("layoutIndex").isValid()) {
            int idx = frame->property("layoutIndex").toInt();
            if (idx == -1) {
                emit shapeOnlySelected();
                emit closed();
                close();
                return true;
            }
            if (idx >= 0 && idx < m_layouts.size()) {
                emit layoutSelected(m_layouts.at(idx));
                emit closed();
                close();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

