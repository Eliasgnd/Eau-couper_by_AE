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
#include "Language.h"
#include "InventoryViewState.h"
#include "InventoryDomainTypes.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Inventory; }
QT_END_NAMESPACE

// -----------------------------------------------------------------------------
// Inventory widget
// -----------------------------------------------------------------------------
class InventoryViewModel;

class Inventory : public QWidget
{
    Q_OBJECT

public:
    explicit Inventory(InventoryViewModel *viewModel, QWidget *parent = nullptr);
    ~Inventory() override;


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

    // Usage statistics
    void incrementLayoutUsage(const QString &shapeName, int index);

    // Utility helpers
    bool shapeNameExists(const QString &name) const;
    InventoryViewModel *viewModel() const { return m_viewModel; }

    // Internationalisation
    void updateTranslations(Language lang);

    QPixmap renderColoredSvg(const QString &filePath, const QColor &color, const QSize &size);
signals:
    void customShapeSelected(const QList<QPolygonF> &polygons, const QString &name);
    void searchRequested(const QString &text);
    void sortModeRequested(InventorySortMode mode);
    void filterModeRequested(InventoryFilterMode mode);
    void folderOpenRequested(const QString &folderName);
    void returnRequested();
    void navigationBackRequested();

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

    InventoryViewModel  *m_viewModel  {nullptr};

    QFrame *createFolderCard(const QString &folderName);
    // type est stocké comme int (ShapeModel::Type) dans l'InventoryViewItem::payload
    QFrame *createBaseShapeCard(int shapeTypeInt, const QString &name);
    bool inFolderView = false;
    QString currentFolder;

    void displayShapesInFolder(const QString &folderName, const QString &filter);
    InventorySortMode currentSortMode() const;
    InventoryFilterMode currentFilterMode() const;
    bool folderIsEmpty(const QString &folderName) const;
};

#endif // INVENTAIRE_H
