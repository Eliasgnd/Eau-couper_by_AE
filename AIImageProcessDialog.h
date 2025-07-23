#ifndef AIIMAGEPROCESSDIALOG_H
#define AIIMAGEPROCESSDIALOG_H

#include <QDialog>
#include <QImage>

class QLabel;
class QPushButton;

class AIImageProcessDialog : public QDialog
{
    Q_OBJECT
public:
    enum Method {
        LogoNoInternal,
        LogoWithInternal,
        ColorEdges
    };

    explicit AIImageProcessDialog(const QImage &img, QWidget *parent = nullptr);
    Method selectedMethod() const { return m_selected; }

private:
    Method m_selected = LogoNoInternal;
    QLabel *m_imageLabel;
    QPushButton *m_noInternalBtn;
    QPushButton *m_withInternalBtn;
    QPushButton *m_colorBtn;
};

#endif // AIIMAGEPROCESSDIALOG_H

