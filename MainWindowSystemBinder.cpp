#include "MainWindowSystemBinder.h"

#include "ui_mainwindow.h"
#include "ShapeController.h"
#include "ShapeVisualization.h"

#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

namespace MainWindowSystemBinder {

void bind(Ui::MainWindow *ui,
          ShapeController *shapeController,
          ShapeVisualization *shapeVisualization,
          QObject *context)
{
    QObject::connect(ui->Longueur, &QSpinBox::valueChanged, ui->Slider_longueur, &QSlider::setValue);
    QObject::connect(ui->Slider_longueur, &QSlider::valueChanged, ui->Longueur, &QSpinBox::setValue);
    QObject::connect(ui->Largeur, &QSpinBox::valueChanged, ui->Slider_largeur, &QSlider::setValue);
    QObject::connect(ui->Slider_largeur, &QSlider::valueChanged, ui->Largeur, &QSpinBox::setValue);

    QObject::connect(ui->shapeCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), context,
                     [ui, shapeController](int count) {
                         shapeController->updateShapeCount(count,
                                                           ui->Largeur->value(),
                                                           ui->Longueur->value());
                     });

    QObject::connect(ui->optimizePlacementButton, &QPushButton::clicked, context,
                     [ui, shapeController](bool checked) {
                         ui->optimizePlacementButton2->setChecked(false);
                         shapeController->onOptimizePlacementClicked(checked,
                                                                     ui->Largeur->value(),
                                                                     ui->Longueur->value());
                     });

    QObject::connect(ui->optimizePlacementButton2, &QPushButton::clicked, context,
                     [ui, shapeController](bool checked) {
                         ui->optimizePlacementButton->setChecked(false);
                         shapeController->onOptimizePlacement2Clicked(checked,
                                                                      ui->Largeur->value(),
                                                                      ui->Longueur->value());
                     });

    QObject::connect(ui->spaceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     shapeController, &ShapeController::updateSpacing);

    QObject::connect(shapeVisualization, &ShapeVisualization::spacingChanged, context,
                     [ui](int newSpacing) {
                         ui->spaceSpinBox->setValue(newSpacing);
                     });

    QObject::connect(ui->ButtonUp, &QPushButton::clicked, shapeController, &ShapeController::onMoveUpClicked);
    QObject::connect(ui->ButtonDown, &QPushButton::clicked, shapeController, &ShapeController::onMoveDownClicked);
    QObject::connect(ui->ButtonLeft, &QPushButton::clicked, shapeController, &ShapeController::onMoveLeftClicked);
    QObject::connect(ui->ButtonRight, &QPushButton::clicked, shapeController, &ShapeController::onMoveRightClicked);

    QObject::connect(ui->ButtonRotationLeft, &QPushButton::clicked, shapeController, &ShapeController::rotateLeft);
    QObject::connect(ui->ButtonRotationRight, &QPushButton::clicked, shapeController, &ShapeController::rotateRight);
    QObject::connect(ui->ButtonAddShape, &QPushButton::clicked, shapeController, &ShapeController::addShape);
    QObject::connect(ui->ButtonDeleteShape, &QPushButton::clicked, shapeController, &ShapeController::deleteShape);
}

} // namespace MainWindowSystemBinder
