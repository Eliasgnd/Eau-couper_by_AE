#include "FolderWidget.h"
#include "ui_FolderWidget.h"

#include "ScreenUtils.h"
#include "ImagePaths.h"
#include <QDir>
#include <QDirIterator>
#include <QScrollBar>
#include <QLabel>
#include <QPixmap>
#include <QGridLayout>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>
#include <QMenu>
#include <QDialog>
#include <QVBoxLayout>
#include <QSlider>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QScreen>
#include <QImageReader>
#include <QtGlobal>


FolderWidget::FolderWidget(Language lang, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FolderWidget)
    , m_lang(lang)
{
    ui->setupUi(this);
    ScreenUtils::placeOnSecondaryScreen(this);

    // ===== BARRE OUTILS (ajout dynamique) =====
    // On insère une barre juste après ton bouton "Close" (ou tout en haut si tu veux)
    // Layout parent (main vertical)
    auto *mainLay = qobject_cast<QVBoxLayout*>(layout());
    if (mainLay) {
        QWidget *toolBar = new QWidget(this);
        QHBoxLayout *toolLay = new QHBoxLayout(toolBar);
        toolLay->setContentsMargins(0,0,0,0);
        toolLay->setSpacing(8);

        // Filtre source
        m_sourceFilter = new QComboBox(toolBar);
        m_sourceFilter->addItem(tr("Toutes les sources")); // 0
        m_sourceFilter->addItem(tr("IA"));                 // 1
        m_sourceFilter->addItem(tr("Wi‑Fi"));              // 2
        m_sourceFilter->addItem(tr("Bluetooth"));          // 3
        m_sourceFilter->addItem(tr("Autres"));             // 4
        toolLay->addWidget(m_sourceFilter);
        connect(m_sourceFilter, &QComboBox::currentIndexChanged,
                this, &FolderWidget::onFilterChanged);

        // Recherche
        m_searchEdit = new QLineEdit(toolBar);
        m_searchEdit->setPlaceholderText(tr("Rechercher..."));
        toolLay->addWidget(m_searchEdit, 1);
        connect(m_searchEdit, &QLineEdit::textChanged,
                this, &FolderWidget::onSearchChanged);

        // Tri (ui->comboSort existe déjà)
        if (ui->comboSort) {
            ui->comboSort->clear();
            ui->comboSort->addItem(tr("Récent → Ancien"));
            ui->comboSort->addItem(tr("Ancien → Récent"));
            connect(ui->comboSort, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this, &FolderWidget::onSortChanged);
            toolLay->addWidget(ui->comboSort);
        }

        // Zoom slider
        m_zoomSlider = new QSlider(Qt::Horizontal, toolBar);
        m_zoomSlider->setRange(80, 260); // taille miniatures
        m_zoomSlider->setValue(m_thumbSize);
        m_zoomSlider->setToolTip(tr("Taille des vignettes"));
        m_zoomSlider->setFixedWidth(160);
        toolLay->addWidget(m_zoomSlider);
        connect(m_zoomSlider, &QSlider::valueChanged,
                this, &FolderWidget::onZoomChanged);

        // Injecte la barre tout de suite après le bouton close
        int idx = mainLay->indexOf(ui->buttonClose);
        if (idx < 0) idx = 0;
        mainLay->insertWidget(idx+1, toolBar);
    }

    // ===== Connexions de base =====
    connect(ui->buttonClose, &QPushButton::clicked, this, &FolderWidget::onCloseClicked);

    // Scroll infini
    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this,
            [this](int value){
                if (!m_loading &&
                    value >= ui->scrollArea->verticalScrollBar()->maximum() - 5) {
                    loadNextPage();
                }
            });

    // ===== CHARGEMENT INITIAL =====
    rebuildFileList();   // scan + filtre + tri
    loadNextPage();      // première page
}

FolderWidget::~FolderWidget()
{
    delete ui;
}

/*---------------------------- UI -----------------------------*/
void FolderWidget::onCloseClicked()
{
    close();
}

void FolderWidget::onSortChanged(int idx)
{
    m_newestFirst = (idx == 0);
    rebuildFileList();
    loadNextPage();
}

void FolderWidget::onFilterChanged(int idx)
{
    m_filterIndex = idx;
    rebuildFileList();
    loadNextPage();
}

void FolderWidget::onSearchChanged(const QString &txt)
{
    m_searchText = txt;
    rebuildFileList();
    loadNextPage();
}

void FolderWidget::onZoomChanged(int value)
{
    m_thumbSize = value;
    // Re-bâtir juste l'affichage (pas de re-scan)
    clearGrid();
    m_currentPage = 0;
    loadNextPage();
}

/*---------------------------- DATA -----------------------------*/
void FolderWidget::rebuildFileList()
{
    // 1) Scanner (si pas déjà scanné ou si tri/filtre impose un re-scan)
    // ici on rescannera toujours pour simpli. Tu peux garder un cache et check m_allFiles.empty() pour optimiser.
    m_allFiles.clear();

    const QString root = ImagePaths::rootDir();
    QStringList filters{ "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.webp" };
    QDirIterator it(root, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        m_allFiles << QFileInfo(it.next());

    // 2) Filtre source + recherche
    m_filteredFiles.clear();
    m_filteredFiles.reserve(m_allFiles.size());

    for (const QFileInfo &fi : m_allFiles) {
        // filtre source
        QString src = detectSource(fi.absoluteFilePath());
        bool ok = true;
        switch (m_filterIndex) {
        case 1: ok = (src == "IA"); break;
        case 2: ok = (src == "Wi‑Fi"); break;
        case 3: ok = (src == "Bluetooth"); break;
        case 4: ok = (src == "Autres"); break;
        default: break; // Tous
        }
        if (!ok) continue;

        // recherche
        if (!m_searchText.isEmpty() &&
            !fi.fileName().contains(m_searchText, Qt::CaseInsensitive))
            continue;

        m_filteredFiles << fi;
    }

    // 3) Tri
    std::sort(m_filteredFiles.begin(), m_filteredFiles.end(),
              [this](const QFileInfo &a, const QFileInfo &b){
                  return m_newestFirst
                             ? a.lastModified() > b.lastModified()
                             : a.lastModified() < b.lastModified();
              });

    // 4) Reset pagination & affichage
    clearGrid();
    m_currentPage = 0;
}

void FolderWidget::clearGrid()
{
    QLayoutItem *child;
    while ((child = ui->gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    ui->scrollArea->verticalScrollBar()->setValue(0);
}

void FolderWidget::loadNextPage()
{
    if (m_loading) return;
    m_loading = true;

    int start = m_currentPage * m_pageSize;
    if (start >= m_filteredFiles.size()) {
        m_loading = false;
        return;
    }
    int end = qMin(start + m_pageSize, m_filteredFiles.size());

    // Position courante dans le grid
    int row = ui->gridLayout->rowCount();
    int col = 0;
    const int columns = 4; // tu peux rendre dynamique selon la largeur

    for (int i = start; i < end; ++i) {
        QWidget *card = buildCard(m_filteredFiles[i]);
        ui->gridLayout->addWidget(card, row, col);
        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }

    m_currentPage++;
    m_loading = false;
}

/*---------------------------- CARTE (vignette) -----------------------------*/
QWidget* FolderWidget::buildCard(const QFileInfo &info)
{
    QWidget *frame = new QWidget;
    frame->setFixedSize(m_thumbSize + 40, m_thumbSize + 70);
    frame->setStyleSheet("background:white; border:1px solid #bdbdbd; border-radius:6px;");

    // menu
    auto *btnMenu = new QToolButton(frame);
    btnMenu->setText("⋮");
    btnMenu->setStyleSheet("border:none; font-size:18px;");
    btnMenu->setCursor(Qt::PointingHandCursor);

    QMenu *menu = new QMenu(btnMenu);
    QAction *actView   = menu->addAction(tr("Afficher"));
    QAction *actReuse  = menu->addAction(tr("Réutiliser"));
    QAction *actRename = menu->addAction(tr("Renommer"));
    QAction *actDelete = menu->addAction(tr("Supprimer"));
    QAction *actOpenFs = menu->addAction(tr("Ouvrir dans le dossier"));

    btnMenu->setMenu(menu);
    btnMenu->setPopupMode(QToolButton::InstantPopup);

    // vignette
    QLabel *thumb = new QLabel(frame);
    thumb->setFixedSize(m_thumbSize, m_thumbSize);
    thumb->setAlignment(Qt::AlignCenter);
    thumb->setStyleSheet("background:#fafafa;");

    QImageReader reader(info.filePath());
    reader.setScaledSize(QSize(m_thumbSize, m_thumbSize));
    QPixmap pix = QPixmap::fromImage(reader.read());
    thumb->setPixmap(pix);

    // nom
    QLabel *name = new QLabel(info.fileName(), frame);
    name->setAlignment(Qt::AlignCenter);
    name->setWordWrap(true);
    name->setStyleSheet("font-size:9pt;");

    // source tag
    QString src = detectSource(info.absoluteFilePath());
    QLabel *tag = new QLabel(src, frame);
    tag->setAlignment(Qt::AlignCenter);
    tag->setStyleSheet("background:#eeeeee; border-radius:8px; padding:2px 6px; font-size:8pt;");

    // Layouts
    QHBoxLayout *top = new QHBoxLayout;
    top->setContentsMargins(0,0,0,0);
    top->addWidget(tag);
    top->addStretch();
    top->addWidget(btnMenu);

    QVBoxLayout *v = new QVBoxLayout(frame);
    v->setContentsMargins(6,6,6,6);
    v->setSpacing(4);
    v->addLayout(top);
    v->addWidget(thumb);
    v->addWidget(name);

    // Connections actions
    connect(actDelete, &QAction::triggered, this, [this, info](){ deleteFile(info); });
    connect(actRename, &QAction::triggered, this, [this, info](){ renameFile(info); });
    connect(actReuse , &QAction::triggered, this, [this, info](){ reuseFile(info); });
    connect(actView  , &QAction::triggered, this, [this, info](){ viewFile(info); });
    connect(actOpenFs, &QAction::triggered, this, [this, info](){ openInExplorer(info); });

    return frame;
}

/*---------------------------- ACTIONS -----------------------------*/
void FolderWidget::renameFile(const QFileInfo &fi)
{
    bool ok = false;
    QString newName = QInputDialog::getText(this, tr("Renommer"),
                                            tr("Nouveau nom (sans chemin) :"),
                                            QLineEdit::Normal, fi.fileName(), &ok);
    if (!ok || newName.isEmpty()) return;

    QString newPath = fi.absoluteDir().absoluteFilePath(newName);
    if (QFile::exists(newPath)) {
        QMessageBox::warning(this, tr("Erreur"), tr("Un fichier portant ce nom existe déjà."));
        return;
    }
    if (QFile::rename(fi.filePath(), newPath)) {
        rebuildFileList();
        loadNextPage();
    }
}

void FolderWidget::deleteFile(const QFileInfo &fi)
{
    if (QMessageBox::question(this, tr("Supprimer"),
                              tr("Supprimer %1 ?").arg(fi.fileName()))
        != QMessageBox::Yes)
        return;

    QFile::remove(fi.filePath());
    rebuildFileList();
    loadNextPage();
}

void FolderWidget::viewFile(const QFileInfo &fi)
{
    QDialog dlg(this);
    dlg.setWindowTitle(fi.fileName());
    QVBoxLayout layout(&dlg);
    QLabel lblBig;
    QPixmap pix(fi.filePath());
    QSize maxSize = qApp->primaryScreen()->availableGeometry().size() * 0.9;
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

/*---------------------------- UTIL -----------------------------*/
QString FolderWidget::detectSource(const QString &absPath) const
{
    const QString p = absPath.toLower();
    if (p.contains("/ai/") || p.contains("\\ai\\"))          return "IA";
    if (p.contains("/wifi/") || p.contains("\\wifi\\"))      return "Wi‑Fi";
    if (p.contains("/bluetooth/") || p.contains("\\bluetooth\\")) return "Bluetooth";
    return "Autres";
}

/*---------------------------- QT events -----------------------------*/
void FolderWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        // remettre les textes des combos
        if (ui->comboSort) {
            ui->comboSort->clear();
            ui->comboSort->addItem(tr("Récent → Ancien"));
            ui->comboSort->addItem(tr("Ancien → Récent"));
        }
        if (m_sourceFilter) {
            m_sourceFilter->clear();
            m_sourceFilter->addItem(tr("Toutes les sources"));
            m_sourceFilter->addItem(tr("IA"));
            m_sourceFilter->addItem(tr("Wi‑Fi"));
            m_sourceFilter->addItem(tr("Bluetooth"));
            m_sourceFilter->addItem(tr("Autres"));
        }
        if (m_searchEdit)
            m_searchEdit->setPlaceholderText(tr("Rechercher..."));
        rebuildFileList();
        loadNextPage();
    }
    QWidget::changeEvent(event);
}
