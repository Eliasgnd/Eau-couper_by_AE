#include "LayoutSelector.h"
#include <QScrollArea>
#include <QGridLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QEvent>
#include <QPainterPath>
#include <QTransform>

LayoutSelector::LayoutSelector(const QList<LayoutData>& layouts,
                               const QList<QPolygonF>& shapePolygons,
                               Language lang,
                               QWidget *parent)
    : QDialog(parent), m_layouts(layouts), m_polygons(shapePolygons), m_lang(lang)
{
    setWindowTitle(m_lang == Language::French ? "Dispositions" : "Layouts");
    resize(800, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    QWidget *container = new QWidget();
    QGridLayout *grid = new QGridLayout(container);
    grid->setSpacing(20);
    grid->setAlignment(Qt::AlignTop);

    for (int i = 0; i < m_layouts.size(); ++i) {
        QFrame *frame = createLayoutFrame(i);
        grid->addWidget(frame, i / 4, i % 4);
    }

    scroll->setWidget(container);
    mainLayout->addWidget(scroll);

    QPushButton *closeBtn = new QPushButton(tr("Cancel"), this);
    if (m_lang == Language::French)
        closeBtn->setText("Fermer");
    connect(closeBtn, &QPushButton::clicked, this, &LayoutSelector::reject);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignCenter);
}

QFrame* LayoutSelector::createLayoutFrame(int index)
{
    if (index < 0 || index >= m_layouts.size())
        return nullptr;
    const LayoutData &ld = m_layouts.at(index);

    QGraphicsScene *scene = new QGraphicsScene();
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
    view->setFixedSize(120, 120);
    view->setStyleSheet("background-color: white;");
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    QFrame *frame = new QFrame();
    frame->setStyleSheet("background-color: white; border: 2px solid black; border-radius: 15px;");
    frame->setFixedSize(150, 200);

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

bool LayoutSelector::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *frame = qobject_cast<QFrame*>(obj);
        if (frame && frame->property("layoutIndex").isValid()) {
            int idx = frame->property("layoutIndex").toInt();
            if (idx >= 0 && idx < m_layouts.size()) {
                m_selectedLayout = m_layouts.at(idx);
                m_hasSelection = true;
                accept();
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}

