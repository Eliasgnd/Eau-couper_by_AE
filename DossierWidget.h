#ifndef DOSSIERWIDGET_H
#define DOSSIERWIDGET_H

#include <QWidget>
#include "Language.h"
#include "qfileinfo.h"

namespace Ui {
class DossierWidget;
}

class DossierWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DossierWidget(Language lang = Language::French, QWidget *parent = nullptr);
    ~DossierWidget();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onCloseClicked();
    void onSortChanged(int);

    void clearImages();
private:
    void loadImages();
    Ui::DossierWidget *ui;
    Language m_lang {Language::French};
    bool m_newestFirst {true};
    int m_pageSize = 30;
    int m_currentPage = 0;
    QFileInfoList m_allFiles;

};

#endif // DOSSIERWIDGET_H
