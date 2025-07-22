#include "AIImagePromptDialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>

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
    layout->addWidget(m_modelCombo);

    m_qualityCombo = new QComboBox(this);
    layout->addWidget(m_qualityCombo);

    m_sizeCombo = new QComboBox(this);
    layout->addWidget(m_sizeCombo);

    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_generateButton = box->button(QDialogButtonBox::Ok);
    m_cancelButton = box->button(QDialogButtonBox::Cancel);
    m_generateButton->setText(tr("Générer"));
    m_cancelButton->setText(tr("Annuler"));
    layout->addWidget(box);

    connect(box, &QDialogButtonBox::accepted, this, &AIImagePromptDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &AIImagePromptDialog::reject);
    connect(m_modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AIImagePromptDialog::onModelChanged);

    m_modelCombo->setCurrentIndex(0);
    updateOptions();
}

void AIImagePromptDialog::onModelChanged(int)
{
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
}

QString AIImagePromptDialog::getPrompt() const
{
    return m_promptEdit->text().trimmed();
}

QString AIImagePromptDialog::getModel() const
{
    return m_modelCombo->currentText();
}

QString AIImagePromptDialog::getQuality() const
{
    return m_qualityCombo->currentText();
}

QString AIImagePromptDialog::getSize() const
{
    return m_sizeCombo->currentText();
}

