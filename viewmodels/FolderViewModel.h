#pragma once

#include <QObject>
#include <QFileInfoList>
#include <QString>

class FolderViewModel : public QObject
{
    Q_OBJECT

public:
    explicit FolderViewModel(QObject *parent = nullptr);

    // --- Filters / sort ---
    void setSortNewestFirst(bool newestFirst);
    void setSourceFilter(int filterIndex);   // 0=all 1=IA 2=Wi-Fi 3=Bluetooth 4=Autres
    void setSearchText(const QString &text);

    // --- Data ---
    void refresh();                                  // re-scan + re-filter
    const QFileInfoList &filteredFiles() const;
    int totalCount() const;

    // --- Source tag ---
    QString detectSource(const QString &absPath) const;

    // --- File actions ---
    // Returns empty errorMsg on success
    bool renameFile(const QFileInfo &fi, const QString &newName, QString &errorMsg);
    bool deleteFile(const QFileInfo &fi);

signals:
    void filesChanged();

private:
    void applyFilterAndSort();

    QFileInfoList m_allFiles;
    QFileInfoList m_filteredFiles;

    bool    m_newestFirst  = true;
    int     m_filterIndex  = 0;
    QString m_searchText;
};
