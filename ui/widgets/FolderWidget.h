#ifndef DOSSIERWIDGET_H
#define DOSSIERWIDGET_H

#include <QWidget>

#include "FolderViewModel.h"
#include "Language.h"

namespace Ui {
class FolderWidget;
}

class FolderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FolderWidget(Language lang = Language::French, QWidget *parent = nullptr);
    ~FolderWidget() override;

signals:
    void imageReuseRequested(const QString &filePath, bool internalContours, bool colorEdges);
    void navigationBackRequested();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onCloseClicked();
    void onClearSearchClicked();
    void onSortChanged(int idx);
    void onFilterChanged(int idx);
    void onSearchChanged(const QString &txt);
    void onZoomChanged(int value);
    void toggleTheme();

private:
    void refreshAndDisplay();
    void clearGrid();
    void loadNextPage();
    QWidget *buildCard(const QFileInfo &info);
    void renameFile(const QFileInfo &fi);
    void deleteFile(const QFileInfo &fi);
    void viewFile(const QFileInfo &fi);
    void reuseFile(const QFileInfo &fi);
    void openInExplorer(const QFileInfo &fi);
    void updateThemeButton();
    QString sourceFilterLabel(const QString &source) const;

    Ui::FolderWidget *ui = nullptr;
    FolderViewModel *m_vm = nullptr;

    Language m_lang {Language::French};
    int m_pageSize = 40;
    int m_currentPage = 0;
    int m_thumbSize = 150;
    bool m_loading = false;
    bool m_isDarkTheme = false;
};

#endif // DOSSIERWIDGET_H
