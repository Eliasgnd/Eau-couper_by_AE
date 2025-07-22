#ifndef PAGEIMAGESGENEREES_H
#define PAGEIMAGESGENEREES_H

#include <QWidget>

class QGridLayout;
class QLabel;

namespace Ui {
class PageImagesGenerées;
}

class PageImagesGenerées : public QWidget
{
    Q_OBJECT
public:
    explicit PageImagesGenerées(QWidget *parent = nullptr);
    ~PageImagesGenerées();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onCloseClicked();

private:
    void refreshImages();

    QGridLayout *m_gridLayout {nullptr};
    Ui::PageImagesGenerées *ui {nullptr};
};

#endif // PAGEIMAGESGENEREES_H
