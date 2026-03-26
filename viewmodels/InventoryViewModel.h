#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QPolygonF>

#include "InventoryViewState.h"
#include "ShapeModel.h"

class InventoryController;

// ViewModel de l'inventaire : expose l'état de présentation à la View (Inventory widget).
// La View ne touche jamais InventoryController directement.
class InventoryViewModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(InventorySortMode sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(InventoryFilterMode filterMode READ filterMode WRITE setFilterMode NOTIFY filterModeChanged)
    Q_PROPERTY(bool inFolderView READ inFolderView NOTIFY stateChanged)
    Q_PROPERTY(QString currentFolder READ currentFolder NOTIFY stateChanged)

public:
    explicit InventoryViewModel(InventoryController &controller, QObject *parent = nullptr);

    // --- Accesseurs d'état (lus par la View) ---
    QString searchText() const { return m_searchText; }
    InventorySortMode sortMode() const { return m_sortMode; }
    InventoryFilterMode filterMode() const { return m_filterMode; }
    bool inFolderView() const { return m_state.inFolderView; }
    QString currentFolder() const { return m_state.currentFolder; }
    InventoryViewState currentState() const { return m_state; }

    // --- Setters (appelés par la View via binding ou signals) ---
    void setSearchText(const QString &text);
    void setSortMode(InventorySortMode mode);
    void setFilterMode(InventoryFilterMode mode);

public slots:
    // --- Actions utilisateur relayées depuis la View ---
    void addCustomShape(const QList<QPolygonF> &polygons, const QString &name);
    void selectFolder(const QString &folderName);
    void selectBaseShape(ShapeModel::Type type);
    bool selectCustomShape(const QString &shapeName,
                           QList<QPolygonF> &polygonsOut,
                           QString &resolvedNameOut);

    bool createFolder(const QString &name, const QString &parentFolder = {});
    bool renameFolder(const QString &oldName, const QString &newName);
    bool deleteFolder(const QString &folderName);

    bool renameCustomShape(int index, const QString &newName);
    bool deleteCustomShape(int index);

    void navigateBack();
    void refresh();

signals:
    // --- Notifications vers la View ---
    void stateChanged(const InventoryViewState &state);
    void searchTextChanged(const QString &text);
    void sortModeChanged(InventorySortMode mode);
    void filterModeChanged(InventoryFilterMode mode);

private:
    void rebuildState();

    InventoryController &m_controller;
    InventoryViewState m_state;
    QString m_searchText;
    InventorySortMode m_sortMode{InventorySortMode::NameAsc};
    InventoryFilterMode m_filterMode{InventoryFilterMode::All};
};
