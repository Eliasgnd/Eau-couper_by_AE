#include "LayoutsDialog.h"
#include "ui_LayoutsDialog.h"

#include <QApplication>
#include <QGridLayout>
#include "ThemeManager.h"
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
#include <QFile>
#include <QTextStream>
#include <QSettings>

#include <algorithm>
#include "InventoryViewModel.h"
#include "ShapeModel.h"

LayoutsDialog::LayoutsDialog(const QString &shapeName,
                           const QList<LayoutData> &layouts,
                           const QList<QPolygonF> &shapePolygons,
                           Language lang,
                           InventoryViewModel *vm,
                           bool isBaseShape,
                           int baseType,
                           QWidget *parent)
    : QWidget(parent),
    ui(new Ui::LayoutsDialog),
    m_layouts(layouts),
    m_polygons(shapePolygons),
    m_lang(lang),
    m_shapeName(shapeName),
    m_isBaseShape(isBaseShape),
    m_baseType(baseType),
    m_vm(vm)
{
    ui->setupUi(this);

    m_isDarkTheme = ThemeManager::instance()->isDark();
    updateThemeButton();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](bool dark){ m_isDarkTheme = dark; updateThemeButton(); });
    connect(ui->buttonTheme, &QPushButton::clicked, this, &LayoutsDialog::toggleTheme);

    setWindowTitle(m_lang == Language::French ? tr("LayoutsDialog") : tr("Layouts"));
    resize(800, 600);
    setWindowState(Qt::WindowFullScreen);
    QTimer::singleShot(0, this, &QWidget::showFullScreen);

    connect(ui->buttonMenu, &QPushButton::clicked,
            this, &LayoutsDialog::onMenuButtonClicked);
    connect(ui->searchBar, &QLineEdit::textChanged,
            this, &LayoutsDialog::onSearchTextChanged);
    connect(ui->buttonClearSearch, &QPushButton::clicked,
            this, &LayoutsDialog::onClearSearchClicked);

    if (ui->comboSort) {
        ui->comboSort->addItem("Nom (A \342\206\222 Z)");
        ui->comboSort->addItem("Nom (Z \342\206\222 A)");
        ui->comboSort->addItem("Utilisation fr\303\251quente");
        ui->comboSort->addItem("R\303\251cent \342\206\222 Ancien");
        ui->comboSort->addItem("Ancien \342\206\222 R\303\251cent");

        connect(ui->comboSort, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &LayoutsDialog::onSortChanged);
    }


    /* --------------------- grille --------------------- */
    if (ui->gridLayout) {
        ui->gridLayout->setSpacing(20);
        ui->gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        displayLayouts();
    }

}

LayoutsDialog::~LayoutsDialog()
{
    delete ui;
}

/* --------------------- slots --------------------- */
void LayoutsDialog::onMenuButtonClicked()
{
    emit requestOpenInventory();
    close();
}

void LayoutsDialog::onCloseButtonClicked()
{
    emit closed();
    close();
}

/* --------------------- création d’une carte --------------------- */
QFrame *LayoutsDialog::createLayoutFrame(int index)
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
    view->setFixedSize(334, 210);
    view->setRenderHint(QPainter::Antialiasing);
    view->setStyleSheet(m_isDarkTheme
        ? "background-color:white; border:1px solid #2D3139; border-radius:6px;"
        : "background-color:white; border:1px solid #DDE3EC; border-radius:6px;");
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    /* --- cadre extérieur --- */
    QFrame *frame = new QFrame();
    if (m_isDarkTheme)
        frame->setStyleSheet("QFrame{background-color:#1C1F24; border:1px solid #2D3139; border-radius:10px;}");
    else
        frame->setStyleSheet("QFrame{background-color:#F8FAFC; border:1px solid #DDE3EC; border-radius:10px;}");
    frame->setFixedSize(360, 300);
    frame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel *label = new QLabel(ld.name);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setStyleSheet(m_isDarkTheme
        ? "color:#CBD5E1; font-size:13px; font-weight:500; background:transparent; border:none; padding:2px 8px;"
        : "color:#334155; font-size:13px; font-weight:500; background:transparent; border:none; padding:2px 8px;");

    QPushButton *menuButton = new QPushButton("...");
    menuButton->setFixedSize(34, 20);
    menuButton->setCursor(Qt::PointingHandCursor);
    menuButton->setStyleSheet(m_isDarkTheme
        ? "QPushButton{background:#272A30;border:1px solid #363A42;border-radius:4px;color:#94A3B8;"
          "font-size:11px;font-weight:700;min-height:20px;max-height:20px;padding:0 4px;}"
          "QPushButton:hover{color:#F1F5F9;border-color:#0EA5E9;background:#363A42;}"
        : "QPushButton{background:#E8EDF4;border:1px solid #C8D0DC;border-radius:4px;color:#64748B;"
          "font-size:11px;font-weight:700;min-height:20px;max-height:20px;padding:0 4px;}"
          "QPushButton:hover{color:#0F172A;border-color:#0EA5E9;background:#CDD4DF;}");

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
                    m_vm->renameLayoutForBaseShape(static_cast<ShapeModel::Type>(m_baseType), index, newName);
                else
                    m_vm->renameLayoutForCustomShape(m_shapeName, index, newName);
            }
        }
    });

    connect(deleteAction, &QAction::triggered, [this, index]() {
        if (index >= 0 && index < m_layouts.size()) {
            m_layouts.removeAt(index);
            if (m_isBaseShape)
                m_vm->deleteLayoutForBaseShape(static_cast<ShapeModel::Type>(m_baseType), index);
            else
                m_vm->deleteLayoutForCustomShape(m_shapeName, index);
            displayLayouts();
        }
    });

    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->addStretch();
    headerLayout->addWidget(menuButton);

    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFixedHeight(1);
    sep->setStyleSheet(m_isDarkTheme ? "background:#2D3139; border:none;" : "background:#E2E8F0; border:none;");

    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 8, 10, 10);
    layout->setSpacing(6);
    layout->addLayout(headerLayout);
    layout->addWidget(view, 0, Qt::AlignCenter);
    layout->addWidget(sep);
    layout->addWidget(label);

    /* propriétés/filtre d’évènement pour le clic */
    frame->setProperty("layoutIndex", index);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);

    return frame;
}

/* --------------------- réaffichage des cartes --------------------- */
void LayoutsDialog::displayLayouts(const QString &filter)
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
QFrame *LayoutsDialog::createBaseShapeFrame()
{
    QGraphicsScene *scene = new QGraphicsScene();

    QPainterPath combinedPath;
    for (const QPolygonF &poly : m_polygons)
        combinedPath.addPolygon(poly);

    scene->addPath(combinedPath, QPen(Qt::black, 1), Qt::NoBrush);
    scene->setSceneRect(combinedPath.boundingRect().adjusted(-5, -5, 5, 5));

    QGraphicsView *view = new QGraphicsView(scene);
    view->setFixedSize(334, 230);
    view->setRenderHint(QPainter::Antialiasing);
    view->setStyleSheet(m_isDarkTheme
        ? "background-color:white; border:1px solid #2D3139; border-radius:6px;"
        : "background-color:white; border:1px solid #DDE3EC; border-radius:6px;");
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    view->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QFrame *frame = new QFrame();
    if (m_isDarkTheme)
        frame->setStyleSheet("QFrame{background-color:#1C1F24; border:1px solid #2D3139; border-radius:10px;}");
    else
        frame->setStyleSheet("QFrame{background-color:#F8FAFC; border:1px solid #DDE3EC; border-radius:10px;}");
    frame->setFixedSize(360, 300);
    frame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QLabel *label = new QLabel(m_lang == Language::French ? "Forme seule" : "Shape only");
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(m_isDarkTheme
        ? "color:#94A3B8; font-size:13px; font-style:italic; background:transparent; border:none; padding:2px 8px;"
        : "color:#64748B; font-size:13px; font-style:italic; background:transparent; border:none; padding:2px 8px;");

    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setFixedHeight(1);
    sep->setStyleSheet(m_isDarkTheme ? "background:#2D3139; border:none;" : "background:#E2E8F0; border:none;");

    QVBoxLayout *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);
    layout->addWidget(view, 0, Qt::AlignCenter);
    layout->addWidget(sep);
    layout->addWidget(label);

    frame->setProperty("layoutIndex", -1);
    frame->setCursor(Qt::PointingHandCursor);
    frame->installEventFilter(this);

    return frame;
}

/* --------------------- filtrage du clic sur une carte --------------------- */
bool LayoutsDialog::eventFilter(QObject *obj, QEvent *event)
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
                    m_vm->incrementLayoutUsageForBaseShape(static_cast<ShapeModel::Type>(m_baseType), idx);
                else
                    m_vm->incrementLayoutUsageForCustomShape(m_shapeName, idx);
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

void LayoutsDialog::onSearchTextChanged(const QString &text)
{
    displayLayouts(text);
}

void LayoutsDialog::onClearSearchClicked()
{
    if (ui->searchBar)
        ui->searchBar->clear();
    displayLayouts();
}

void LayoutsDialog::onSortChanged(int)
{
    displayLayouts(ui->searchBar ? ui->searchBar->text() : QString());
}

void LayoutsDialog::applyStyleSheets()
{
    updateThemeButton();
}

void LayoutsDialog::updateThemeButton()
{
    if (!ui->buttonTheme) return;
    ui->buttonTheme->setIcon(QIcon(m_isDarkTheme ? ":/icons/moon.svg" : ":/icons/sun.svg"));
}

void LayoutsDialog::toggleTheme()
{
    ThemeManager::instance()->toggle();
    displayLayouts(ui->searchBar ? ui->searchBar->text() : QString());
}

void LayoutsDialog::applyTheme(bool isDark)
{
    m_isDarkTheme = isDark;
    applyStyleSheets();
    displayLayouts(ui->searchBar ? ui->searchBar->text() : QString());
}
