#include "NavigationController.h"

#include "CustomEditor.h"
#include "WifiTransferWidget.h"
#include "WifiConfigDialog.h"
#include "BluetoothReceiverDialog.h"
#include "TestGpio.h"
#include "FolderWidget.h"
#include "LayoutsDialog.h"
#include "ShapeVisualization.h"
#include "drawing/tools/ImportedImageGeometryHelper.h"
#include "drawing/PathGenerator.h"

#include <QWidget>
#include <QInputDialog>
#include <QLineEdit>
#include <QTimer>
#include <QMessageBox>

NavigationController::NavigationController(QObject *parent)
    : QObject(parent)
{
}

void NavigationController::showInventory(QWidget *from, QWidget *inventory)
{
    if (from)
        from->hide();
    if (inventory)
        inventory->showFullScreen();
}

void NavigationController::openCustomEditor(QWidget *from, Language language)
{
    if (from)
        from->hide();

    CustomEditor *customWindow = new CustomEditor(language);
    customWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(customWindow, &CustomEditor::applyCustomShapeSignal,
            this, &NavigationController::customShapeApplied);
    connect(customWindow, &CustomEditor::resetDrawingSignal,
            this, &NavigationController::customDrawingReset);
    connect(customWindow, &QObject::destroyed, this, [this]() {
        emit requestReturnToFullScreen();
    });
    customWindow->showFullScreen();
}

void NavigationController::openCustomEditorWithImportedPath(QWidget *from,
                                                            Language language,
                                                            const QPainterPath &outline)
{
    if (from)
        from->hide();

    CustomEditor *customWindow = new CustomEditor(language);
    customWindow->setAttribute(Qt::WA_DeleteOnClose);
    connect(customWindow, &CustomEditor::applyCustomShapeSignal,
            this, &NavigationController::customShapeApplied);
    connect(customWindow, &CustomEditor::resetDrawingSignal,
            this, &NavigationController::customDrawingReset);
    connect(customWindow, &QObject::destroyed, this, [this]() {
        emit requestReturnToFullScreen();
    });

    CustomDrawArea *area = customWindow->getDrawArea();
    customWindow->showFullScreen();

    QTimer::singleShot(0, customWindow, [area, outline]() mutable {
        if (!area)
            return;
        QPainterPath scaled = ImportedImageGeometryHelper::fitCentered(outline, area->size());
        if (scaled.isEmpty())
            return;

        QList<QPainterPath> subs = PathGenerator::separateIntoSubpaths(scaled);
        for (const QPainterPath &sp : subs)
            area->addImportedLogoSubpath(sp);
    });
}

void NavigationController::openWifiTransfer(QWidget *from)
{
    if (from)
        from->hide();

    WifiTransferWidget *window = new WifiTransferWidget();
    window->setAttribute(Qt::WA_DeleteOnClose);
    connect(window, &QObject::destroyed, this, [this]() {
        emit requestReturnToFullScreen();
    });
    window->showFullScreen();
}

void NavigationController::openWifiSettings(QWidget *from)
{
    if (from)
        from->hide();

    auto *dialog = new WifiConfigDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QObject::destroyed, this, [this]() {
        emit requestReturnToFullScreen();
    });
    dialog->showFullScreen();
}

void NavigationController::openBluetoothReceiver(QWidget *from)
{
    if (from)
        from->hide();

    auto *dialog = new BluetoothReceiverDialog();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QObject::destroyed, this, [this]() {
        emit requestReturnToFullScreen();
    });
    dialog->showFullScreen();
}

void NavigationController::openTestGpio(QWidget *from)
{
    if (from)
        from->hide();

    auto *dialog = new TestGpio();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QObject::destroyed, this, [this]() {
        emit requestReturnToFullScreen();
    });
    dialog->showFullScreen();
}

void NavigationController::openFolder(QWidget *from, Language language)
{
    if (from)
        from->hide();

    auto *dialog = new FolderWidget(language);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QObject::destroyed, this, [this]() {
        emit requestReturnToFullScreen();
    });
    dialog->showFullScreen();
}

void NavigationController::openLayoutsDialog(QWidget *from,
                                             const QString &shapeName,
                                             const QList<LayoutData> &layouts,
                                             const QList<QPolygonF> &shapePolygons,
                                             Language language,
                                             bool isBaseShape,
                                             ShapeModel::Type baseType)
{
    if (from)
        from->hide();

    auto *dialog = new LayoutsDialog(shapeName, layouts, shapePolygons, language, isBaseShape, baseType);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &LayoutsDialog::layoutSelected, this, [this, isBaseShape, baseType](const LayoutData &layout) {
        if (isBaseShape) {
            emit baseShapeLayoutSelected(baseType, layout);
            return;
        }
        emit layoutSelected(layout);
    });
    connect(dialog, &LayoutsDialog::shapeOnlySelected, this, [this, isBaseShape, baseType]() {
        if (isBaseShape)
            emit baseShapeOnlySelected(baseType);
    });
    connect(dialog, &LayoutsDialog::requestOpenInventory, this, [this, dialog]() {
        emit requestOpenInventory();
        dialog->deleteLater();
    });
    connect(dialog, &LayoutsDialog::closed, this, [this, dialog]() {
        emit requestReturnToFullScreen();
        dialog->deleteLater();
    });
    dialog->showFullScreen();
}

QString NavigationController::promptForName(QWidget *parent,
                                            const QString &title,
                                            const QString &label,
                                            bool *ok) const
{
    bool accepted = false;
    const QString text = QInputDialog::getText(parent,
                                               title,
                                               label,
                                               QLineEdit::Normal,
                                               "",
                                               &accepted);
    if (ok)
        *ok = accepted;
    return text;
}

void NavigationController::handleSaveLayoutRequest(QWidget *parent,
                                                   ShapeVisualization *shapeVisualization,
                                                   ShapeModel::Type selectedShapeType)
{
    if (!shapeVisualization)
        return;

    auto promptAndSaveCurrentCustomShape = [this, parent, shapeVisualization]() -> bool {
        QList<QPolygonF> shapes = shapeVisualization->currentCustomShapes();
        if (shapes.isEmpty())
            return false;

        bool ok = false;
        QString shapeName;
        do {
            shapeName = promptForName(parent,
                                      tr("Nom de la forme"),
                                      tr("Entrez un nom pour votre forme :"),
                                      &ok);
            if (!ok)
                return false;
            if (shapeName.isEmpty())
                continue;
            if (Inventory::getInstance()->shapeNameExists(shapeName)) {
                QMessageBox msg(QMessageBox::Warning,
                                tr("Nom déjà utilisé"),
                                tr("Ce nom est déjà utilisé, veuillez en choisir un autre."),
                                QMessageBox::NoButton,
                                parent);
                QTimer::singleShot(2300, &msg, &QMessageBox::accept);
                msg.exec();
                ok = false;
            }
        } while (!ok);

        Inventory::getInstance()->addSavedCustomShape(shapes, shapeName);
        shapeVisualization->setCurrentCustomShapeName(shapeName);
        return true;
    };

    auto saveLayout = [this, parent, shapeVisualization, selectedShapeType]() {
        bool ok = false;
        QString name = promptForName(parent,
                                     tr("Nom de la disposition"),
                                     tr("Entrez un nom"),
                                     &ok);
        if (!ok || name.isEmpty())
            return;

        LayoutData layout = shapeVisualization->captureCurrentLayout(name);
        if (shapeVisualization->isCustomMode())
            Inventory::getInstance()->addLayoutToShape(shapeVisualization->currentCustomShapeName(), layout);
        else
            Inventory::getInstance()->addLayoutToBaseShape(selectedShapeType, layout);
    };

    if (shapeVisualization->isCustomMode() && shapeVisualization->currentCustomShapeName().isEmpty()) {
        QMessageBox::warning(parent,
                             tr("Disposition"),
                             tr("Forme non sauvegardée — veuillez la sauvegarder d'abord."));
        if (promptAndSaveCurrentCustomShape())
            saveLayout();
        return;
    }

    saveLayout();
}
