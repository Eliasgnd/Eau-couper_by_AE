#include "PageImagesGenerées.h"
#include "ui_PageImagesGenerées.h"
#include "MainWindow.h"

#include <QDir>
#include <QApplication>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QShowEvent>

PageImagesGenerées::PageImagesGenerées(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::PageImagesGenerées)
{
    ui->setupUi(this);

    // access grid layout and store pointer
    m_gridLayout = ui->gridLayout;

    connect(ui->closeButton, &QPushButton::clicked,
            this, &PageImagesGenerées::onCloseClicked);
}

PageImagesGenerées::~PageImagesGenerées()
{
    delete ui;
}

void PageImagesGenerées::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshImages();
}

void PageImagesGenerées::refreshImages()
{
    if (!m_gridLayout)
        return;

    // Clear existing thumbnails
    QLayoutItem *child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            delete child->widget();
        delete child;
    }

    const QString imagesDirPath = qApp->applicationDirPath() + QDir::separator() + "images_generées";
    QDir dir(imagesDirPath);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    dir.setNameFilters(filters);
    dir.setSorting(QDir::Time | QDir::Reversed);

    const QFileInfoList files = dir.entryInfoList();

    int colCount = 4;
    int row = 0;
    int col = 0;
    for (const QFileInfo &info : files) {
        QPixmap pix(info.filePath());
        if (!pix.isNull()) {
            QLabel *label = new QLabel;
            label->setPixmap(pix.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            label->setAlignment(Qt::AlignCenter);
            m_gridLayout->addWidget(label, row, col);
            ++col;
            if (col >= colCount) {
                col = 0;
                ++row;
            }
        }
    }
}

void PageImagesGenerées::onCloseClicked()
{
    hide();
    MainWindow::getInstance()->showFullScreen();
}
