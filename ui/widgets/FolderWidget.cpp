#include "FolderWidget.h"
#include "ui_FolderWidget.h"

#include "ScreenUtils.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImageReader>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QScrollBar>
#include <QScreen>
#include <QToolButton>
#include <QUrl>

FolderWidget::FolderWidget(Language lang, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FolderWidget)
    , m_vm(new FolderViewModel(this))
    , m_lang(lang)
{
    ui->setupUi(this);
    ScreenUtils::placeOnSecondaryScreen(this);

    m_isDarkTheme = ThemeManager::instance()->isDark();
    updateThemeButton();
    connect(ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](bool dark) {
        m_isDarkTheme = dark;
        updateThemeButton();
        refreshAndDisplay();
    });

    if (ui->comboSort) {
        ui->comboSort->clear();
        ui->comboSort->addItem(tr("Récent → Ancien"));
        ui->comboSort->addItem(tr("Ancien → Récent"));
        connect(ui->comboSort,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                &FolderWidget::onSortChanged);
    }

    if (ui->comboFilter) {
        ui->comboFilter->clear();
        ui->comboFilter->addItem(tr("Toutes les sources"));
        ui->comboFilter->addItem(tr("IA"));
        ui->comboFilter->addItem(tr("Wi-Fi"));
        ui->comboFilter->addItem(tr("Bluetooth"));
        ui->comboFilter->addItem(tr("Autres"));
        connect(ui->comboFilter,
                QOverload<int>::of(&QComboBox::currentIndexChanged),
                this,
                &FolderWidget::onFilterChanged);
    }

    if (ui->sliderZoom) {
        ui->sliderZoom->setRange(80, 260);
        ui->sliderZoom->setValue(m_thumbSize);
        ui->sliderZoom->setToolTip(tr("Taille des vignettes"));
        connect(ui->sliderZoom, &QSlider::valueChanged, this, &FolderWidget::onZoomChanged);
    }

    if (ui->searchBar) {
        connect(ui->searchBar, &QLineEdit::textChanged, this, &FolderWidget::onSearchChanged);
    }

    connect(ui->buttonClearSearch, &QPushButton::clicked, this, &FolderWidget::onClearSearchClicked);
    connect(ui->buttonMenu, &QPushButton::clicked, this, &FolderWidget::onCloseClicked);
    connect(ui->buttonTheme, &QPushButton::clicked, this, &FolderWidget::toggleTheme);

    connect(ui->scrollAreaInventory->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int value) {
                if (!m_loading &&
                    value >= ui->scrollAreaInventory->verticalScrollBar()->maximum() - 5) {
                    loadNextPage();
                }
            });

    m_vm->refresh();
    loadNextPage();
}

FolderWidget::~FolderWidget()
{
    delete ui;
}

void FolderWidget::updateThemeButton()
{
    if (!ui->buttonTheme) {
        return;
    }

    const QString iconPath = m_isDarkTheme ? ":/icons/moon.svg" : ":/icons/sun.svg";
    ui->buttonTheme->setIcon(QIcon(iconPath));
}

QString FolderWidget::sourceFilterLabel(const QString &source) const
{
    if (source == "IA") {
        return tr("IA");
    }
    if (source == "Wi-Fi") {
        return tr("Wi-Fi");
    }
    if (source == "Bluetooth") {
        return tr("Bluetooth");
    }
    return tr("Autres");
}

void FolderWidget::toggleTheme()
{
    ThemeManager::instance()->toggle();
}

void FolderWidget::onCloseClicked()
{
    close();
    emit navigationBackRequested();
}

void FolderWidget::onClearSearchClicked()
{
    if (ui->searchBar) {
        ui->searchBar->clear();
    }
}

void FolderWidget::onSortChanged(int idx)
{
    m_vm->setSortNewestFirst(idx == 0);
    clearGrid();
    m_currentPage = 0;
    loadNextPage();
}

void FolderWidget::onFilterChanged(int idx)
{
    m_vm->setSourceFilter(idx);
    clearGrid();
    m_currentPage = 0;
    loadNextPage();
}

void FolderWidget::onSearchChanged(const QString &txt)
{
    m_vm->setSearchText(txt);
    clearGrid();
    m_currentPage = 0;
    loadNextPage();
}

void FolderWidget::onZoomChanged(int value)
{
    m_thumbSize = value;
    clearGrid();
    m_currentPage = 0;
    loadNextPage();
}

void FolderWidget::refreshAndDisplay()
{
    m_vm->refresh();
    clearGrid();
    m_currentPage = 0;
    loadNextPage();
}

void FolderWidget::clearGrid()
{
    QLayoutItem *child;
    while ((child = ui->gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }
    ui->scrollAreaInventory->verticalScrollBar()->setValue(0);
}

void FolderWidget::loadNextPage()
{
    if (m_loading) {
        return;
    }
    m_loading = true;

    const QFileInfoList &files = m_vm->filteredFiles();
    const int start = m_currentPage * m_pageSize;
    if (start >= files.size()) {
        m_loading = false;
        return;
    }

    const int end = qMin(start + m_pageSize, files.size());
    const int columns = 4;

    for (int i = start; i < end; ++i) {
        QWidget *card = buildCard(files[i]);
        const int relativeIndex = i - start;
        const int row = (start / columns) + (relativeIndex / columns);
        const int col = relativeIndex % columns;
        ui->gridLayout->addWidget(card, row, col);
    }

    m_currentPage++;
    m_loading = false;
}

QWidget *FolderWidget::buildCard(const QFileInfo &info)
{
    QWidget *frame = new QWidget;
    frame->setFixedSize(m_thumbSize + 48, m_thumbSize + 92);
    frame->setStyleSheet(m_isDarkTheme
        ? "background:#1C1F24; border:1px solid #2D3139; border-radius:10px;"
        : "background:#F8FAFC; border:1px solid #DDE3EC; border-radius:10px;");

    auto *btnMenu = new QToolButton(frame);
    btnMenu->setText("⋮");
    btnMenu->setStyleSheet("border:none; font-size:18px; padding:0 2px;");
    btnMenu->setCursor(Qt::PointingHandCursor);

    QMenu *menu = new QMenu(btnMenu);
    QAction *actView   = menu->addAction(tr("Afficher"));
    QAction *actReuse  = menu->addAction(tr("Réutiliser"));
    QAction *actRename = menu->addAction(tr("Renommer"));
    QAction *actDelete = menu->addAction(tr("Supprimer"));
    QAction *actOpenFs = menu->addAction(tr("Ouvrir dans le dossier"));
    btnMenu->setMenu(menu);
    btnMenu->setPopupMode(QToolButton::InstantPopup);

    QLabel *thumb = new QLabel(frame);
    thumb->setFixedSize(m_thumbSize, m_thumbSize);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet(m_isDarkTheme
        ? "background:#FFFFFF; border:1px solid #2D3139; border-radius:6px;"
        : "background:#FFFFFF; border:1px solid #DDE3EC; border-radius:6px;");
    QImageReader reader(info.filePath());
    reader.setScaledSize(QSize(m_thumbSize, m_thumbSize));
    thumb->setPixmap(QPixmap::fromImage(reader.read()));

    QLabel *name = new QLabel(info.fileName(), frame);
    name->setAlignment(Qt::AlignCenter);
    name->setWordWrap(true);
    name->setStyleSheet(m_isDarkTheme
        ? "font-size:9pt; color:#CBD5E1; background:transparent;"
        : "font-size:9pt; color:#334155; background:transparent;");

    const QString src = sourceFilterLabel(m_vm->detectSource(info.absoluteFilePath()));
    QLabel *tag = new QLabel(src, frame);
    tag->setAlignment(Qt::AlignCenter);
    tag->setStyleSheet(m_isDarkTheme
        ? "background:#272A30; color:#94A3B8; border:1px solid #363A42; border-radius:10px; padding:2px 8px; font-size:8pt;"
        : "background:#E8EDF4; color:#64748B; border:1px solid #C8D0DC; border-radius:10px; padding:2px 8px; font-size:8pt;");

    QHBoxLayout *top = new QHBoxLayout;
    top->setContentsMargins(0, 0, 0, 0);
    top->addWidget(tag);
    top->addStretch();
    top->addWidget(btnMenu);

    QVBoxLayout *v = new QVBoxLayout(frame);
    v->setContentsMargins(8, 8, 8, 8);
    v->setSpacing(6);
    v->addLayout(top);
    v->addWidget(thumb, 0, Qt::AlignCenter);
    v->addWidget(name);

    connect(actDelete, &QAction::triggered, this, [this, info]() { deleteFile(info); });
    connect(actRename, &QAction::triggered, this, [this, info]() { renameFile(info); });
    connect(actReuse,  &QAction::triggered, this, [this, info]() { reuseFile(info); });
    connect(actView,   &QAction::triggered, this, [this, info]() { viewFile(info); });
    connect(actOpenFs, &QAction::triggered, this, [this, info]() { openInExplorer(info); });

    return frame;
}

void FolderWidget::renameFile(const QFileInfo &fi)
{
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("Renommer"),
                                                  tr("Nouveau nom (sans chemin) :"),
                                                  QLineEdit::Normal, fi.fileName(), &ok);
    if (!ok || newName.isEmpty()) {
        return;
    }

    QString errorMsg;
    if (!m_vm->renameFile(fi, newName, errorMsg)) {
        QMessageBox::warning(this, tr("Erreur"), errorMsg);
        return;
    }

    clearGrid();
    m_currentPage = 0;
    loadNextPage();
}

void FolderWidget::deleteFile(const QFileInfo &fi)
{
    if (QMessageBox::question(this, tr("Supprimer"),
                              tr("Supprimer %1 ?").arg(fi.fileName()))
        != QMessageBox::Yes) {
        return;
    }

    m_vm->deleteFile(fi);
    clearGrid();
    m_currentPage = 0;
    loadNextPage();
}

void FolderWidget::viewFile(const QFileInfo &fi)
{
    QDialog dlg(this);
    dlg.setWindowTitle(fi.fileName());
    QVBoxLayout layout(&dlg);
    QLabel lblBig;
    QPixmap pix(fi.filePath());
    const QSize maxSize = qApp->primaryScreen()->availableGeometry().size() * 0.9;
    lblBig.setPixmap(pix.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout.addWidget(&lblBig);
    dlg.exec();
}

void FolderWidget::reuseFile(const QFileInfo &fi)
{
    emit imageReuseRequested(fi.filePath(), true, false);
    close();
}

void FolderWidget::openInExplorer(const QFileInfo &fi)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.absoluteFilePath()));
}

void FolderWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);

        if (ui->comboSort) {
            ui->comboSort->clear();
            ui->comboSort->addItem(tr("Récent → Ancien"));
            ui->comboSort->addItem(tr("Ancien → Récent"));
        }

        if (ui->comboFilter) {
            ui->comboFilter->clear();
            ui->comboFilter->addItem(tr("Toutes les sources"));
            ui->comboFilter->addItem(tr("IA"));
            ui->comboFilter->addItem(tr("Wi-Fi"));
            ui->comboFilter->addItem(tr("Bluetooth"));
            ui->comboFilter->addItem(tr("Autres"));
        }

        refreshAndDisplay();
    }

    QWidget::changeEvent(event);
}
