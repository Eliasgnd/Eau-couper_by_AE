#include "Dispositions.h"
#include "ui_Dispositions.h"

#include <QGridLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QTimer>
#include <QEvent>
#include <QPainterPath>
#include <QTransform>
#include <QSizePolicy>
#include <QDateTime>

#include <algorithm>
#include "inventaire.h"

Dispositions::Dispositions(const QString &shapeName,
                           const QList<LayoutData> &layouts,
                           const QList<QPolygonF> &shapePolygons,
                           Language lang,
                           bool isBaseShape,
                           ShapeModel::Type baseType,
                           QWidget *parent)
    : QWidget(parent),
    ui(new Ui::Dispositions),
    m_layouts(layouts),
    m_polygons(shapePolygons),
    m_lang(lang),
    m_shapeName(shapeName),
    m_isBaseShape(isBaseShape),
    m_baseType(baseType)
{
    ui->setupUi(this);

    setWindowTitle(m_lang == Language::French ? tr("Dispositions") : tr("Layouts"));
    resize(800, 600);
    setWindowState(Qt::WindowFullScreen);
    QTimer::singleShot(0, this, &QWidget::showFullScreen);

    connect(ui->buttonMenu, &QPushButton::clicked,
            this, &Dispositions::onMenuButtonClicked);
    connect(ui->searchBar, &QLineEdit::textChanged,
            this, &Dispositions::onSearchTextChanged);
    connect(ui->buttonClearSearch, &QPushButton::clicked,
            this, &Dispositions::onClearSearchClicked);

    if (ui->comboSort) {
        ui->comboSort->addItem("Nom (A \342\206\222 Z)");
        ui->comboSort->addItem("Nom (Z \342\206\222 A)");
        ui->comboSort->addItem("Utilisation fr\303\251quente");
        ui->comboSort->addItem("R\303\251cent \342\206\222 Ancien");
        ui->comboSort->addItem("Ancien \342\206\222 R\303\251cent");

        connect(ui->comboSort, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &Dispositions::onSortChanged);
    }


    /* --------------------- grille --------------------- */
    if (ui->gridLayout) {
        ui->gridLayout->setSpacing(20);
        ui->gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        displayLayouts();
    }

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
    frame->setFixedSize(350, 285);
    frame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel *label = new QLabel(ld.name);
    label->setAlignment(Qt::AlignCenter);

    QPushButton *menuButton = new QPushButton("...");
    menuButton->setFixedSize(25, 25);
    menuButton->setStyleSheet("border: none; font-size: 14px;");

    QMenu *menu = new QMenu(menuButton);
    QAction *renameAction = new QAction(m_lang == Language::French ? "Renommer" : "Rename", menu);
    QAction *deleteAction = new QAction(m_lang == Language::French ? "Supprimer" : "Delete", menu);
    menu->addAction(renameAction);
    menu->addAction(deleteAction);

    connect(menuButton, &QPushButton::clicked, [menu, menuButton]() {
        menu->exec(menuButton->mapToGlobal(QPoint(0, menuButton->height())));
    });

    connect(renameAction, &QAction::triggered, [this, label, index]() {
        bool ok;
        QString newName = QInputDialog::getText(nullptr,
                                                m_lang == Language::French ? "Renommer la disposition" : "Rename layout",
                                                m_lang == Language::French ? "Nouveau nom :" : "New name:",
                                                QLineEdit::Normal,
                                                label->text(), &ok);
        if (ok && !newName.isEmpty()) {
            label->setText(newName);
            if (index >= 0 && index < m_layouts.size()) {
                m_layouts[index].name = newName;
                if (m_isBaseShape)
                    Inventaire::getInstance()->renameBaseLayout(m_baseType, index, newName);
                else
                    Inventaire::getInstance()->renameLayout(m_shapeName, index, newName);
            }
        }
    });

    connect(deleteAction, &QAction::triggered, [this, index]() {
        if (index >= 0 && index < m_layouts.size()) {
            m_layouts.removeAt(index);
            if (m_isBaseShape)
                Inventaire::getInstance()->deleteBaseLayout(m_baseType, index);
            else
                Inventaire::getInstance()->deleteLayout(m_shapeName, index);
            displayLayouts();
        }
    });

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->addStretch();
    headerLayout->addWidget(menuButton);

    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);
    layout->addLayout(headerLayout);
    layout->addWidget(view, 0, Qt::AlignCenter);
    layout->addWidget(label);

    /* propriétés/filtre d’évènement pour le clic */
    frame->setProperty("layoutIndex", index);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);

    return frame;
}

/* --------------------- réaffichage des cartes --------------------- */
void Dispositions::displayLayouts(const QString &filter)
{
    if (!ui->gridLayout)
        return;

    QLayoutItem *child;
    while ((child = ui->gridLayout->takeAt(0)) != nullptr) {
        if (QWidget *w = child->widget())
            w->deleteLater();
        delete child;
    }

    const QString f = filter.trimmed().toLower();

    int sortMode = ui->comboSort ? ui->comboSort->currentIndex() : 0;

    struct Item { int index; QString name; int usage; QDateTime last; };

    QList<Item> items;

    const QString baseName = m_lang == Language::French ? "Forme seule" : "Shape only";
    bool showBase = f.isEmpty() || baseName.toLower().contains(f);

    for (int i = 0; i < m_layouts.size(); ++i) {
        if (!f.isEmpty() && !m_layouts.at(i).name.toLower().contains(f))
            continue;
        const LayoutData &ld = m_layouts.at(i);
        items.append({i, ld.name, ld.usageCount, ld.lastUsed});
    }

    std::sort(items.begin(), items.end(), [sortMode](const Item &a, const Item &b){
        switch(sortMode){
        case 0: return a.name.toLower() < b.name.toLower();
        case 1: return a.name.toLower() > b.name.toLower();
        case 2: return a.usage > b.usage;
        case 3: return a.last > b.last;
        case 4: return a.last < b.last;
        }
        return false;
    });

    int index = 0;
    if (showBase) {
        QFrame *shapeFrame = createBaseShapeFrame();
        ui->gridLayout->addWidget(shapeFrame, index / 4, index % 4);
        ++index;
    }


    for (const Item &it : items) {
        QFrame *frame = createLayoutFrame(it.index);
        ui->gridLayout->addWidget(frame, index / 4, index % 4);
        ++index;
    }
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
    view->setFixedSize(300, 230);
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
    frame->setFixedSize(350, 285);
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
                LayoutData ld = m_layouts.at(idx);
                if (m_isBaseShape)
                    Inventaire::getInstance()->incrementBaseLayoutUsage(m_baseType, idx);
                else
                    Inventaire::getInstance()->incrementLayoutUsage(m_shapeName, idx);
                ld.usageCount++;
                ld.lastUsed = QDateTime::currentDateTime();
                m_layouts[idx].usageCount = ld.usageCount;
                m_layouts[idx].lastUsed = ld.lastUsed;
                emit layoutSelected(ld);
            }
            emit closed();
            close();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void Dispositions::onSearchTextChanged(const QString &text)
{
    displayLayouts(text);
}

void Dispositions::onClearSearchClicked()
{
    if (ui->searchBar)
        ui->searchBar->clear();
    displayLayouts();
}

void Dispositions::onSortChanged(int)
{
    displayLayouts(ui->searchBar ? ui->searchBar->text() : QString());
}
