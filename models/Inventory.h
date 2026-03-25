#ifndef INVENTAIRE_H
#define INVENTAIRE_H

#include <QWidget>
#include <QPolygonF>
#include <QList>
#include <QMap>
#include <QFrame>
#include <QString>
#include <QStringList>
#include <QListWidget>
#include <QDateTime>
#include "ShapeModel.h"
#include "Language.h"
#include "InventoryViewState.h"
#include "InventoryDomainTypes.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Inventory; }
QT_END_NAMESPACE

// -----------------------------------------------------------------------------
// Inventory widget
// -----------------------------------------------------------------------------
class InventoryModel;
class InventoryController;

class Inventory : public QWidget
{
    Q_OBJECT

public:
    explicit Inventory(QWidget *parent = nullptr);
    ~Inventory() override;

    // Singleton accessor
    static Inventory* getInstance();

    // Public attributes -------------------------------------------------------
    bool gros_cavu {false};

    // -------------------------------------------------------------------------
    // API – Custom shapes
    // -------------------------------------------------------------------------
    void addSavedCustomShape(const QList<QPolygonF> &polygons, const QString &name);
    QStringList getAllShapeNames() const;

    // Layouts for custom shapes
    void addLayoutToShape(const QString &shapeName, const LayoutData &layout);
    void renameLayout(const QString &shapeName, int index, const QString &newName);
    void deleteLayout(const QString &shapeName, int index);
    QList<LayoutData> getLayoutsForShape(const QString &shapeName) const;

    // Layouts for base (built‑in) shapes
    void addLayoutToBaseShape(ShapeModel::Type type, const LayoutData &layout);
    void renameBaseLayout(ShapeModel::Type type, int index, const QString &newName);
    void deleteBaseLayout(ShapeModel::Type type, int index);
    QList<LayoutData> getLayoutsForBaseShape(ShapeModel::Type type) const;

    // Usage statistics
    void incrementLayoutUsage(const QString &shapeName, int index);
    void incrementBaseLayoutUsage(ShapeModel::Type type, int index);

    // Utility helpers
    bool shapeNameExists(const QString &name) const;

    // Internationalisation
    void updateTranslations(Language lang);

    QPixmap renderColoredSvg(const QString &filePath, const QColor &color, const QSize &size);
signals:
    void shapeSelected(ShapeModel::Type type, int width, int height);
    void customShapeSelected(const QList<QPolygonF> &polygons, const QString &name);
    void searchRequested(const QString &text);
    void sortModeRequested(InventorySortMode mode);
    void filterModeRequested(InventoryFilterMode mode);
    void folderOpenRequested(const QString &folderName);
    void returnRequested();

protected:
    // React to palette / language changes
    void changeEvent(QEvent *event) override;

private slots:
    void goToMainWindow();
    void onSearchTextChanged(const QString &text);
    void onClearSearchClicked();
    void onCreateFolderClicked();
    void onItemClicked(QListWidgetItem *item);
    void onSortChanged(int index);
    void onFilterChanged(int index);

private:
    // ---------------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------------
    // Display all shapes (built‑in + custom). Optional filter on name.
    void displayShapes(const QString &filter = QString());
    void renderState(const InventoryViewState &state);

    // Build and return the QFrame representing the custom shape at index
    QFrame* addCustomShapeToGrid(int index);

    // ---------------------------------------------------------------------
    // Data members
    // ---------------------------------------------------------------------
    Ui::Inventory *ui {nullptr};
    static Inventory *instance;

    InventoryModel *m_model {nullptr};
    InventoryController *m_controller {nullptr};

    QFrame *createFolderCard(const QString &folderName);
    QFrame *createBaseShapeCard(ShapeModel::Type type, const QString &name);
    bool inFolderView = false;
    QString currentFolder;

    void displayShapesInFolder(const QString &folderName, const QString &filter);
    InventorySortMode currentSortMode() const;
    InventoryFilterMode currentFilterMode() const;
    bool folderIsEmpty(const QString &folderName) const;
    bool folderContainsMatchingShape(const QString &folderName, const QString &text) const;
};

#endif // INVENTAIRE_H
