#ifndef INVENTAIRE_H
#define INVENTAIRE_H

#include <QWidget>
#include <QPolygonF>
#include <QList>
#include <QMap>
#include <QFrame>
#include <QString>
#include <QStringList>
#include "ShapeModel.h"
#include "Language.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Inventaire; }
QT_END_NAMESPACE

// -----------------------------------------------------------------------------
// Data structures
// -----------------------------------------------------------------------------

// Single item in a saved layout
struct LayoutItem {
    double x {0};
    double y {0};
    double rotation {0};
};

// Complete layout for a custom shape
struct LayoutData {
    QString name;
    int largeur  {0};
    int longueur {0};
    int spacing  {0};
    QList<LayoutItem> items;
};

// Custom shape definition (can contain multiple polygons) and its saved layouts
struct CustomShapeData {
    QList<QPolygonF> polygons;
    QString name;
    QList<LayoutData> layouts;
};

// -----------------------------------------------------------------------------
// Inventaire widget
// -----------------------------------------------------------------------------
class Inventaire : public QWidget
{
    Q_OBJECT

public:
    explicit Inventaire(QWidget *parent = nullptr);
    ~Inventaire() override;

    // Singleton accessor
    static Inventaire* getInstance();

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

    // Utility helpers
    bool shapeNameExists(const QString &name) const;
    static QString baseShapeName(ShapeModel::Type type, Language lang);

    // Internationalisation
    void updateTranslations(Language lang);

signals:
    void shapeSelected(ShapeModel::Type type, int width, int height);
    void customShapeSelected(const QList<QPolygonF> &polygons, const QString &name);

protected:
    // React to palette / language changes
    void changeEvent(QEvent *event) override;

    // Intercept clicks on thumbnails
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void goToMainWindow();
    void onSearchTextChanged(const QString &text);
    void onClearSearchClicked();

private:
    // ---------------------------------------------------------------------
    // Internal helpers
    // ---------------------------------------------------------------------
    QString customShapesFilePath() const;
    void loadCustomShapes();
    void saveCustomShapes() const;

    // Display all shapes (built‑in + custom). Optional filter on name.
    void displayShapes(const QString &filter = QString());

    // Build and return the QFrame representing the custom shape at index
    QFrame* addCustomShapeToGrid(int index);

    // ---------------------------------------------------------------------
    // Data members
    // ---------------------------------------------------------------------
    Ui::Inventaire *ui {nullptr};
    static Inventaire *instance;

    QList<CustomShapeData> m_customShapes;
    QMap<ShapeModel::Type, QList<LayoutData>> m_baseShapeLayouts;
    Language currentLanguage {Language::French};
};

#endif // INVENTAIRE_H
