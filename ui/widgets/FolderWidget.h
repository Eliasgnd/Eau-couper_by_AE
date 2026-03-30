#ifndef DOSSIERWIDGET_H
#define DOSSIERWIDGET_H

#include <QWidget>
#include "Language.h"
#include "FolderViewModel.h"

class QLineEdit;
class QComboBox;
class QSlider;
class QScrollBar;
class QLayout;
class QListWidgetItem;

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

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onCloseClicked();
    void onSortChanged(int idx);
    void onFilterChanged(int idx);
    void onSearchChanged(const QString &txt);
    void onZoomChanged(int value);

private:
    // Helpers
    void refreshAndDisplay();       // refresh VM + reset pagination + load first page
    void clearGrid();
    void loadNextPage();
    QWidget* buildCard(const QFileInfo &info);
    void renameFile(const QFileInfo &fi);
    void deleteFile(const QFileInfo &fi);
    void viewFile(const QFileInfo &fi);
    void reuseFile(const QFileInfo &fi);
    void openInExplorer(const QFileInfo &fi);

    // UI
    Ui::FolderWidget *ui = nullptr;

    // Widgets créés dynamiquement (barre d'outils)
    QComboBox   *m_sourceFilter = nullptr;
    QLineEdit   *m_searchEdit   = nullptr;
    QSlider     *m_zoomSlider   = nullptr;

    // ViewModel
    FolderViewModel *m_vm = nullptr;

    // Display state
    Language m_lang        {Language::French};
    int      m_pageSize    = 40;
    int      m_currentPage = 0;
    int      m_thumbSize   = 150;
    bool     m_loading     = false;
};

#endif // DOSSIERWIDGET_H
