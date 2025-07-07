#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "FormeVisualization.h"
#include "ShapeModel.h"
#include "CustomDrawArea.h"
#include "trajetmotor.h"
#include "Language.h"
#include <QMenu>
#include <QAction>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Classe représentant la fenêtre principale de l'application
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void StartPixel();
    static MainWindow* getInstance();

public slots:
    void updateProgressBar(int remaining, int total);
private slots:
    void updateForme();
    void changeToCircle();
    void changeToRectangle();
    void changeToTriangle();
    void changeToStar();
    void changeToHeart();
    void showInventaire();
    void showCustom();
    void applyCustomShape(QList<QPolygonF>);
    void resetDrawing();

    void updateSpinBoxLargeur(int value);
    void updateSpinBoxLongueur(int value);
    void updateSliderLongueur(int value);
    void updateSliderLargeur(int value);
    void updateShapeCount(); // Met à jour le nombre de formes affichées

    // Nouveau slot pour recevoir le nombre de formes placées du widget
    void updateShapeCountLabel(int count);
    void updateSpacing(int value);
    //pour les formes qui arrive de l'inventaire
    void onCustomShapeSelected(const QList<QPolygonF> &polygons);

    void setLanguageFrench();
    void setLanguageEnglish();

private:
    void updateTranslations();

    Language currentLanguage = Language::French;
    QMenu *settingsMenu = nullptr;
    QMenu *languageMenu = nullptr;
    QAction *actionFrench = nullptr;
    QAction *actionEnglish = nullptr;


private:
    Ui::MainWindow *ui;
    FormeVisualization *formeVisualization = nullptr;
    CustomDrawArea *drawArea = nullptr;
    ShapeModel::Type selectedShapeType;
    QLabel *shapeCountLabel = nullptr; // Membre pour le label du compteur
    TrajetMotor* trajetMotor;

protected:
    void showEvent(QShowEvent *event) override;

};

#endif // MAINWINDOW_H
