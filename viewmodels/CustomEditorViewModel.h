#pragma once

#include <QObject>
#include <QList>
#include <QPolygonF>
#include <QPainterPath>
#include <QSizeF>
#include <QString>
#include "ImageEdgeImporter.h"

class InventoryViewModel;

// ViewModel for CustomEditor widget.
// Handles all business logic: shape persistence checks, logo import,
// color-image import, geometry scaling/centering.
// CustomEditor only handles UI interactions and delegates here.
class CustomEditorViewModel : public QObject
{
    Q_OBJECT

public:
    explicit CustomEditorViewModel(InventoryViewModel &inventoryVm,
                                   QObject *parent = nullptr);

    // --- Query (called from View before triggering save) ---
    bool shapeNameExists(const QString &name) const;

public slots:
    // --- Commands ---
    void saveShape(const QList<QPolygonF> &shapes, const QString &name);
    void importLogo(const QString &filePath, bool includeInternal, QSizeF drawAreaSize);
    void importImageColor(const QString &filePath, QSizeF drawAreaSize);

signals:
    void subpathsReady(const QList<QPainterPath> &subpaths);
    void importFailed(const QString &message);
    void shapeSaved();

private:
    static QList<QPainterPath> scaledAndCentered(const QPainterPath &outline,
                                                  QSizeF drawAreaSize);

    InventoryViewModel &m_inventoryVm;
    ImageEdgeImporter   m_imageImporter;
};
