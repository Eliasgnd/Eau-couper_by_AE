#include "PageImagesGenerees.h"
#include "ui_PageImagesGenerees.h"
#include "MainWindow.h"
#include "ScreenUtils.h"

#include <QDir>
#include <QLabel>
#include <QPixmap>
#include <QGridLayout>
#include <QApplication>

PageImagesGenerees::PageImagesGenerees(Language lang, QWidget *parent)
    : QWidget(parent), ui(new Ui::PageImagesGenerees), m_lang(lang)
{
    ui->setupUi(this);
    ScreenUtils::placeOnSecondaryScreen(this);

    connect(ui->buttonClose, &QPushButton::clicked, this, &PageImagesGenerees::onCloseClicked);

    loadImages();
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
    // Clear previous thumbnails
    QLayoutItem *child;
    while ((child = ui->gridLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            delete child->widget();
        delete child;
    }

    const QString imagesDirPath = qApp->applicationDirPath() + QDir::separator() + QString::fromUtf8("images_g\xC3\xA9n\xC3\xA9r\xC3\xA9es");
    QDir dir(imagesDirPath);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    int row = 0;
    int col = 0;
    const int columns = 4;
    for (const QFileInfo &info : files) {
        QLabel *lbl = new QLabel;
        lbl->setFixedSize(150, 150);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("background:white; border:1px solid gray;");
        QPixmap pix(info.filePath());
        lbl->setPixmap(pix.scaled(lbl->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->gridLayout->addWidget(lbl, row, col);
        if (++col >= columns) {
            col = 0;
            ++row;
        }
    }
}

void PageImagesGenerees::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        loadImages();
    }
    QWidget::changeEvent(event);
}

