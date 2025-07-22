#ifndef AIIMAGEPROMPTDIALOG_H
#define AIIMAGEPROMPTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

class AIImagePromptDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AIImagePromptDialog(QWidget *parent = nullptr);

    QString getPrompt() const;
    QString getModel() const;
    QString getQuality() const;
    QString getSize() const;

private slots:
    void onModelChanged(int index);

private:
    void updateOptions();

    QLineEdit  *m_promptEdit;
    QComboBox  *m_modelCombo;
    QComboBox  *m_qualityCombo;
    QComboBox  *m_sizeCombo;
    QPushButton *m_generateButton;
    QPushButton *m_cancelButton;
};

#endif // AIIMAGEPROMPTDIALOG_H
