#include "LayoutSelector.h"
#include <QScrollArea>
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

LayoutSelector::LayoutSelector(const QList<LayoutData>& layouts,
                               const QList<QPolygonF>& shapePolygons,
                               Language lang,
                               QWidget *parent)
    : QDialog(parent), m_layouts(layouts), m_polygons(shapePolygons), m_lang(lang)
{
    setWindowTitle(m_lang == Language::French ? "Dispositions" : "Layouts");
    resize(800, 600);
    setWindowState(Qt::WindowFullScreen);
    QTimer::singleShot(0, this, &QWidget::showFullScreen);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Top-right close button styled like the inventory one
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QPushButton *closeHeaderBtn = new QPushButton(this);
    closeHeaderBtn->setIcon(QIcon(":/icons/cross.svg"));
    closeHeaderBtn->setIconSize(QSize(40, 40));
    closeHeaderBtn->setFixedSize(61, 51);
    closeHeaderBtn->setStyleSheet(
        "QPushButton {"
        "background-color: #F44336;"
        "color: white;"
        "font-size: 14px;"
        "border: 2px solid #D32F2F;"
        "border-radius: 5px;"
        "padding: 6px 12px;"
        "text-align: left;"
        "padding-left: 8px;"
        "padding-right: 8px;"
        "}"
        "QPushButton:hover {"
        "background-color: #E57373;"
        "}"
        "QPushButton:pressed {"
        "background-color: #C62828;"
        "}");
    connect(closeHeaderBtn, &QPushButton::clicked, [this]() {
        m_openInventaire = true;
        reject();
    });
    headerLayout->addStretch();
    headerLayout->addWidget(closeHeaderBtn);
    mainLayout->addLayout(headerLayout);

    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    QWidget *container = new QWidget();
    QGridLayout *grid = new QGridLayout(container);
    grid->setSpacing(20);
    grid->setAlignment(Qt::AlignTop);

    // First card : only apply the base shape
    QFrame *shapeFrame = createBaseShapeFrame();
    grid->addWidget(shapeFrame, 0, 0);

    for (int i = 0; i < m_layouts.size(); ++i) {
        QFrame *frame = createLayoutFrame(i);
        int pos = i + 1; // shift because of base frame
        grid->addWidget(frame, pos / 4, pos % 4);
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

    // Add a rectangle representing the board to mimic the cutting preview widget
    scene->addRect(0, 0, ld.largeur, ld.longueur, QPen(Qt::black), Qt::NoBrush);

    // Prepare the prototype shape path scaled to the preset dimensions
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
    frame->setFixedSize(325, 250);

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

QFrame* LayoutSelector::createBaseShapeFrame()
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
    frame->setFixedSize(325, 250);

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

bool LayoutSelector::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame *frame = qobject_cast<QFrame*>(obj);
        if (frame && frame->property("layoutIndex").isValid()) {
            int idx = frame->property("layoutIndex").toInt();
            if (idx == -1) {
                // Only apply the shape without changing layout
                m_hasSelection = false;
                accept();
                return true;
            }
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

