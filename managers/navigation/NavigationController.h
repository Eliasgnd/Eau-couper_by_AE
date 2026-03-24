#ifndef NAVIGATIONCONTROLLER_H
#define NAVIGATIONCONTROLLER_H

#include <QObject>
#include <QList>
#include <QPolygonF>
#include <QString>
#include <QPainterPath>
#include "Language.h"
#include "Inventory.h"
#include "ShapeModel.h"

class QWidget;
class CustomEditor;
class WifiTransferWidget;
class WifiConfigDialog;
class BluetoothReceiverDialog;
class TestGpio;
class FolderWidget;
class LayoutsDialog;

class NavigationController : public QObject
{
    Q_OBJECT
public:
    explicit NavigationController(QObject *parent = nullptr);

    void showInventory(QWidget *from, QWidget *inventory);
    void openCustomEditor(QWidget *from, Language language);
    void openCustomEditorWithImportedPath(QWidget *from, Language language, const QPainterPath &outline);
    void openWifiTransfer(QWidget *from);
    void openWifiSettings(QWidget *from);
    void openBluetoothReceiver(QWidget *from);
    void openTestGpio(QWidget *from);
    void openFolder(QWidget *from, Language language);
    void openLayoutsDialog(QWidget *from,
                           const QString &shapeName,
                           const QList<LayoutData> &layouts,
                           const QList<QPolygonF> &shapePolygons,
                           Language language,
                           bool isBaseShape = false,
                           ShapeModel::Type baseType = ShapeModel::Type::Circle);
    QString promptForName(QWidget *parent,
                          const QString &title,
                          const QString &label,
                          bool *ok = nullptr) const;

signals:
    void customShapeApplied(QList<QPolygonF> shapes);
    void customDrawingReset();
    void layoutSelected(const LayoutData &layout);
    void baseShapeOnlySelected(ShapeModel::Type type);
    void baseShapeLayoutSelected(ShapeModel::Type type, const LayoutData &layout);
    void requestOpenInventory();
    void requestReturnToFullScreen();
};

#endif // NAVIGATIONCONTROLLER_H
