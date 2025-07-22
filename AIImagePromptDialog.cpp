#include "AIImagePromptDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>

AIImagePromptDialog::AIImagePromptDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Génération d'image IA"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    m_promptEdit = new QLineEdit(this);
    m_promptEdit->setPlaceholderText(tr("Décrivez l'image..."));
    layout->addWidget(m_promptEdit);

    m_modelCombo = new QComboBox(this);
    m_modelCombo->addItems(QStringList() << "gpt-image-1" << "dall-e-3" << "dall-e-2");
    layout->addWidget(new QLabel("Modèle :"));
    layout->addWidget(m_modelCombo);

    m_qualityCombo = new QComboBox(this);
    layout->addWidget(new QLabel("Qualité :"));
    layout->addWidget(m_qualityCombo);

    m_sizeCombo = new QComboBox(this);
    layout->addWidget(new QLabel("Taille :"));
    layout->addWidget(m_sizeCombo);

    m_priceLabel = new QLabel(this);
    layout->addWidget(new QLabel("Prix estimé :"));
    layout->addWidget(m_priceLabel);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_generateButton = box->button(QDialogButtonBox::Ok);
    m_cancelButton = box->button(QDialogButtonBox::Cancel);
    m_generateButton->setText(tr("Générer"));
    m_cancelButton->setText(tr("Annuler"));
    layout->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, &AIImagePromptDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &AIImagePromptDialog::reject);
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIImagePromptDialog::updateOptions);
    connect(m_qualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIImagePromptDialog::updatePrice);
    connect(m_sizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIImagePromptDialog::updatePrice);

    m_modelCombo->setCurrentIndex(0);
    updateOptions();
}

void AIImagePromptDialog::updateOptions()
{
    QString model = m_modelCombo->currentText();

    m_qualityCombo->clear();
    m_sizeCombo->clear();

    if (model == "gpt-image-1") {
        m_qualityCombo->addItems(QStringList() << "low" << "medium" << "high");
        m_sizeCombo->addItems(QStringList() << "1024x1024" << "1024x1536" << "1536x1024");
    } else if (model == "dall-e-3") {
        m_qualityCombo->addItems(QStringList() << "standard" << "hd");
        m_sizeCombo->addItems(QStringList() << "1024x1024" << "1024x1792" << "1792x1024");
    } else { // dall-e-2
        m_qualityCombo->addItem("standard");
        m_sizeCombo->addItems(QStringList() << "256x256" << "512x512" << "1024x1024");
    }
    updatePrice();
}

void AIImagePromptDialog::updatePrice()
{
    QString model = m_modelCombo->currentText();
    QString quality = m_qualityCombo->currentText();
    QString size = m_sizeCombo->currentText();

    double price = 0.0;
    if (model == "gpt-image-1") {
        if (quality == "low") price = (size == "1024x1024") ? 0.011 : 0.016;
        else if (quality == "medium") price = (size == "1024x1024") ? 0.042 : 0.063;
        else if (quality == "high") price = (size == "1024x1024") ? 0.167 : 0.25;
    } else if (model == "dall-e-3") {
        if (quality == "standard") price = (size == "1024x1024") ? 0.04 : 0.08;
        else if (quality == "hd") price = (size == "1024x1024") ? 0.08 : 0.12;
    } else if (model == "dall-e-2") {
        if (size == "256x256") price = 0.016;
        else if (size == "512x512") price = 0.018;
        else if (size == "1024x1024") price = 0.02;
    }

    m_priceLabel->setText(QString("$%1").arg(QString::number(price, 'f', 3)));
}

QString AIImagePromptDialog::getPrompt() const { return m_promptEdit->text().trimmed(); }
QString AIImagePromptDialog::getModel() const { return m_modelCombo->currentText(); }
QString AIImagePromptDialog::getQuality() const { return m_qualityCombo->currentText(); }
QString AIImagePromptDialog::getSize() const { return m_sizeCombo->currentText(); }
