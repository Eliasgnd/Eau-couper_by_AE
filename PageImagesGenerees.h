#ifndef PAGEIMAGESGENEREES_H
#define PAGEIMAGESGENEREES_H

#include <QWidget>
#include "Language.h"

namespace Ui {
class PageImagesGenerees;
}

class PageImagesGenerees : public QWidget
{
    Q_OBJECT
public:
    explicit PageImagesGenerees(Language lang = Language::French, QWidget *parent = nullptr);
    ~PageImagesGenerees();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onCloseClicked();
    void onSortChanged(int);

private:
    void loadImages();
    Ui::PageImagesGenerees *ui;
    Language m_lang {Language::French};
    bool m_newestFirst {true};
};

#endif // PAGEIMAGESGENEREES_H
