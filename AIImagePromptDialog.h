#ifndef AIIMAGEPROMPTDIALOG_H
#define AIIMAGEPROMPTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class AIImagePromptDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AIImagePromptDialog(QWidget *parent = nullptr);

    QString getPrompt() const;
    QString getModel() const;
    QString getQuality() const;
    QString getSize() const;
    bool    isColor() const;

private slots:
    void updatePrice();  // ← DECLARÉ ICI

private:
    void updateOptions();

    QLineEdit   *m_promptEdit;
    QComboBox   *m_modelCombo;
    QComboBox   *m_qualityCombo;
    QComboBox   *m_sizeCombo;
    QComboBox   *m_styleCombo;
    QLabel      *m_priceLabel;      // ← DÉCLARÉ ICI
    QPushButton *m_generateButton;
    QPushButton *m_cancelButton;
};

#endif // AIIMAGEPROMPTDIALOG_H
