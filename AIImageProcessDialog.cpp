#include "AIImageProcessDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>

AIImageProcessDialog::AIImageProcessDialog(const QImage &img, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Choisir le traitement"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_imageLabel = new QLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setPixmap(QPixmap::fromImage(img).scaled(400, 400,
                                                        Qt::KeepAspectRatio,
                                                        Qt::SmoothTransformation));
    mainLayout->addWidget(m_imageLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_noInternalBtn = new QPushButton(tr("Logo ext."), this);
    m_withInternalBtn = new QPushButton(tr("Logo + int."), this);
    m_colorBtn = new QPushButton(tr("Image couleur"), this);
    btnLayout->addWidget(m_noInternalBtn);
    btnLayout->addWidget(m_withInternalBtn);
    btnLayout->addWidget(m_colorBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_noInternalBtn, &QPushButton::clicked, this, [this]() {
        m_selected = LogoNoInternal;
        accept();
    });
    connect(m_withInternalBtn, &QPushButton::clicked, this, [this]() {
        m_selected = LogoWithInternal;
        accept();
    });
    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        m_selected = ColorEdges;
        accept();
    });
}

