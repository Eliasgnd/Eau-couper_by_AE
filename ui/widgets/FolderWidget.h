#ifndef DOSSIERWIDGET_H
#define DOSSIERWIDGET_H

#include <QWidget>
#include <QFileInfoList>
#include "Language.h"

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
    void rebuildFileList();           // (re)scanne le disque + tri + filtre + recherche
    void clearGrid();                 // supprime les widgets du grid
    void loadNextPage();              // charge la page suivante dans le grid
    QWidget* buildCard(const QFileInfo &info); // crée une vignette
    QString detectSource(const QString &absPath) const;
    void renameFile(const QFileInfo &fi);
    void deleteFile(const QFileInfo &fi);
    void viewFile(const QFileInfo &fi);
    void reuseFile(const QFileInfo &fi);
    void openInExplorer(const QFileInfo &fi);

    // UI
    Ui::FolderWidget *ui = nullptr;

    // Widgets créés dynamiquement (barre d’outils)
    QComboBox   *m_sourceFilter = nullptr;
    QLineEdit   *m_searchEdit   = nullptr;
    QSlider     *m_zoomSlider   = nullptr;

    // Données
    Language     m_lang        {Language::French};
    bool         m_newestFirst {true};
    int          m_pageSize    = 40;
    int          m_currentPage = 0;
    int          m_thumbSize   = 150;    // taille côté miniature
    QString      m_searchText;
    int          m_filterIndex = 0;      // 0 = Tous, 1 = IA, 2 = Wi-Fi, 3 = Bluetooth, 4 = Autres

    QFileInfoList m_allFiles;            // tous les fichiers bruts
    QFileInfoList m_filteredFiles;       // après tri/filtre/recherche
    bool          m_loading     = false; // évite double load
};

#endif // DOSSIERWIDGET_H
