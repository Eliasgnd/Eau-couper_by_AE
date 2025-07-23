#include "PageImagesGenerees.h"
#include "ui_PageImagesGenerees.h"
#include "MainWindow.h"
#include "ScreenUtils.h"

#include <QDir>
#include <QLabel>
#include <QPixmap>
#include <QGridLayout>
#include <QApplication>
#include <QComboBox>
#include <QScreen>
#include <QToolButton>
#include <QMenu>
#include <QDialog>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QtGlobal>
#include <QImageReader>

PageImagesGenerees::PageImagesGenerees(Language lang, QWidget *parent)
    : QWidget(parent), ui(new Ui::PageImagesGenerees), m_lang(lang)
{
    ui->setupUi(this);
    ScreenUtils::placeOnSecondaryScreen(this);

    connect(ui->buttonClose, &QPushButton::clicked, this, &PageImagesGenerees::onCloseClicked);

    if (ui->comboSort) {
        ui->comboSort->addItem(tr("Récent → Ancien"));
        ui->comboSort->addItem(tr("Ancien → Récent"));
        connect(ui->comboSort, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &PageImagesGenerees::onSortChanged);
    }

    loadImages();

    connect(ui->scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value){
        if (value == ui->scrollArea->verticalScrollBar()->maximum()) {
            loadImages(); // Charger la page suivante
        }
    });

}

PageImagesGenerees::~PageImagesGenerees()
{
    delete ui;
}

void PageImagesGenerees::onCloseClicked()
{
    this->close();
    MainWindow::getInstance()->showFullScreen();
}

void PageImagesGenerees::loadImages()
{
    // Chargement initial de tous les fichiers si nécessaire
    if (m_allFiles.isEmpty()) {
        const QString imagesDirPath = qApp->applicationDirPath() + QDir::separator() + QString::fromUtf8("images_generees");
        QDir dir(imagesDirPath);
        QStringList filters;
        filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
        m_allFiles = dir.entryInfoList(filters, QDir::Files);

        std::sort(m_allFiles.begin(), m_allFiles.end(), [this](const QFileInfo &a, const QFileInfo &b){
            return m_newestFirst ? a.lastModified() > b.lastModified() : a.lastModified() < b.lastModified();
        });
    }

    int row = ui->gridLayout->rowCount();
    int col = 0;
    const int columns = 4;

    int start = m_currentPage * m_pageSize;
    int end = qMin(start + m_pageSize, m_allFiles.size());

    for (int i = start; i < end; ++i) {
        const QFileInfo &info = m_allFiles[i];
        QWidget *frame = new QWidget;
        frame->setFixedSize(170, 220);
        frame->setStyleSheet("background:white; border:1px solid gray; border-radius:5px;");

        auto *btnMenu = new QToolButton;
        btnMenu->setText("\u22ee");
        btnMenu->setStyleSheet("border:none; font-size:18px;");

        QMenu *menu = new QMenu(btnMenu);
        QAction *actView = menu->addAction(tr("Afficher"));
        QAction *actReuse = menu->addAction(tr("Réutiliser"));
        QAction *actDelete = menu->addAction(tr("Supprimer"));
        btnMenu->setMenu(menu);
        btnMenu->setPopupMode(QToolButton::InstantPopup);

        QLabel *lbl = new QLabel;
        lbl->setFixedSize(150, 150);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("background:white;");
        QImageReader reader(info.filePath());
        reader.setScaledSize(lbl->size());
        QPixmap pix = QPixmap::fromImage(reader.read());
        lbl->setPixmap(pix);

        QLabel *name = new QLabel(info.fileName());
        name->setAlignment(Qt::AlignCenter);
        name->setWordWrap(true);

        QHBoxLayout *top = new QHBoxLayout;
        top->addStretch();
        top->addWidget(btnMenu);

        QVBoxLayout *v = new QVBoxLayout(frame);
        v->setContentsMargins(5, 5, 5, 5);
        v->addLayout(top);
        v->addWidget(lbl);
        v->addWidget(name);

        connect(actDelete, &QAction::triggered, this, [this, info]() {
            QFile::remove(info.filePath());
            m_allFiles.clear();
            clearImages();
            m_currentPage = 0;
            loadImages();
        });

        connect(actReuse, &QAction::triggered, this, [info, this]() {
            this->close();
            MainWindow::getInstance()->openImageInCustom(info.filePath());
        });

        connect(actView, &QAction::triggered, this, [info, this]() {
            QDialog dlg(this);
            QVBoxLayout layout(&dlg);
            QLabel lblBig;
            QPixmap pix(info.filePath());
            QSize maxSize = qApp->primaryScreen()->availableGeometry().size() * 0.9;
            lblBig.setPixmap(pix.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            layout.addWidget(&lblBig);
            dlg.exec();
        });

        ui->gridLayout->addWidget(frame, row, col);
        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }

    m_currentPage++;
}

void PageImagesGenerees::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        if (ui->comboSort) {
            ui->comboSort->clear();
            ui->comboSort->addItem(tr("Récent → Ancien"));
            ui->comboSort->addItem(tr("Ancien → Récent"));
        }
        loadImages();
    }
    QWidget::changeEvent(event);
}

void PageImagesGenerees::onSortChanged(int index)
{
    m_newestFirst = (index == 0);
    loadImages();
}

void PageImagesGenerees::clearImages()
{
    QLayoutItem *child;
    while ((child = ui->gridLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            delete child->widget();
        delete child;
    }
}
