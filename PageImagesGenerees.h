#ifndef PAGEIMAGESGENEREES_H
#define PAGEIMAGESGENEREES_H

#include <QWidget>
#include "Language.h"
#include "qfileinfo.h"

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

    void clearImages();
private:
    void loadImages();
    Ui::PageImagesGenerees *ui;
    Language m_lang {Language::French};
    bool m_newestFirst {true};
    int m_pageSize = 30;
    int m_currentPage = 0;
    QFileInfoList m_allFiles;

};

#endif // PAGEIMAGESGENEREES_H
