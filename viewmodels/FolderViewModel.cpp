#include "FolderViewModel.h"
#include "ImagePaths.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <algorithm>

FolderViewModel::FolderViewModel(QObject *parent)
    : QObject(parent)
{}

// --- Filters / sort ---

void FolderViewModel::setSortNewestFirst(bool newestFirst)
{
    m_newestFirst = newestFirst;
    applyFilterAndSort();
    emit filesChanged();
}

void FolderViewModel::setSourceFilter(int filterIndex)
{
    m_filterIndex = filterIndex;
    applyFilterAndSort();
    emit filesChanged();
}

void FolderViewModel::setSearchText(const QString &text)
{
    m_searchText = text;
    applyFilterAndSort();
    emit filesChanged();
}

// --- Data ---

void FolderViewModel::refresh()
{
    m_allFiles.clear();
    const QString root = ImagePaths::rootDir();
    QStringList filters{ "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.webp" };
    QDirIterator it(root, filters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        m_allFiles << QFileInfo(it.next());

    applyFilterAndSort();
    emit filesChanged();
}

const QFileInfoList &FolderViewModel::filteredFiles() const
{
    return m_filteredFiles;
}

int FolderViewModel::totalCount() const
{
    return m_filteredFiles.size();
}

// --- Source tag ---

QString FolderViewModel::detectSource(const QString &absPath) const
{
    const QString p = absPath.toLower();
    if (p.contains("/ai/")        || p.contains("\\ai\\"))         return "IA";
    if (p.contains("/wifi/")      || p.contains("\\wifi\\"))       return "Wi\u2011Fi";
    if (p.contains("/bluetooth/") || p.contains("\\bluetooth\\"))  return "Bluetooth";
    return "Autres";
}

// --- File actions ---

bool FolderViewModel::renameFile(const QFileInfo &fi,
                                  const QString &newName,
                                  QString &errorMsg)
{
    const QString newPath = fi.absoluteDir().absoluteFilePath(newName);
    if (QFile::exists(newPath)) {
        errorMsg = tr("Un fichier portant ce nom existe déjà.");
        return false;
    }
    if (!QFile::rename(fi.filePath(), newPath)) {
        errorMsg = tr("Impossible de renommer le fichier.");
        return false;
    }
    errorMsg.clear();
    refresh();
    return true;
}

bool FolderViewModel::deleteFile(const QFileInfo &fi)
{
    const bool ok = QFile::remove(fi.filePath());
    if (ok) refresh();
    return ok;
}

// --- Private ---

void FolderViewModel::applyFilterAndSort()
{
    m_filteredFiles.clear();
    m_filteredFiles.reserve(m_allFiles.size());

    for (const QFileInfo &fi : m_allFiles) {
        const QString src = detectSource(fi.absoluteFilePath());
        bool ok = true;
        switch (m_filterIndex) {
        case 1: ok = (src == "IA");          break;
        case 2: ok = (src == "Wi\u2011Fi");  break;
        case 3: ok = (src == "Bluetooth");   break;
        case 4: ok = (src == "Autres");      break;
        default: break;
        }
        if (!ok) continue;

        if (!m_searchText.isEmpty() &&
            !fi.fileName().contains(m_searchText, Qt::CaseInsensitive))
            continue;

        m_filteredFiles << fi;
    }

    std::sort(m_filteredFiles.begin(), m_filteredFiles.end(),
              [this](const QFileInfo &a, const QFileInfo &b) {
                  return m_newestFirst
                      ? a.lastModified() > b.lastModified()
                      : a.lastModified() < b.lastModified();
              });
}
