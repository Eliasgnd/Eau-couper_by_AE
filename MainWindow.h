#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ShapeModel.h"
#include "Language.h"
#include <QTranslator>
#include <QList>
#include <QPolygonF>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Classe représentant la fenêtre principale de l'application
class QAction;
class QMenu;
class QLabel;
class FormeVisualization;
class CustomDrawArea;
class TrajetMotor;
class OpenAIService;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static MainWindow* getInstance();
    FormeVisualization* getFormeVisualization() const;
    void openImageInCustom(const QString &filePath,
                           bool internalContours = false,
                           bool colorEdges = false);

public slots:
    // Reçoit un pourcentage + un texte déjà formaté par TrajetMotor.
    void updateProgressBar(int percentage, const QString &remainingTimeText);
    void setSpinboxSliderEnabled(bool enabled);

private slots:
    void updateForme();
    void changeToCircle();
    void changeToRectangle();
    void changeToTriangle();
    void changeToStar();
    void changeToHeart();
    void showInventaire();
    void showCustom();
    void openTestGpio();
    void on_receptionFichierButton_clicked();
    void openWifiTransfer();
    void openWifiConfig();
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
    void onCustomShapeSelected(const QList<QPolygonF> &polygons, const QString &name);

    void openAIImagePromptDialog();
    void showDossier();
    void onAiGenerationFinished(bool success, const QString &result);

    void setLanguageFrench();
    void setLanguageEnglish();

protected:
    void changeEvent(QEvent *event) override;

private:
    void setupUI();
    void setupConnections();
    void setupModels();
    bool loadLanguage(Language lang);


    Language currentLanguage = Language::French;
    QTranslator translator;
    QMenu *settingsMenu = nullptr;
    QMenu *languageMenu = nullptr;
    QAction *actionFrench = nullptr;
    QAction *actionEnglish = nullptr;
    QAction *actionWifiConfig = nullptr;

    void onShapeSelectedFromInventaire(ShapeModel::Type type);

private:
    Ui::MainWindow *ui;
    FormeVisualization *formeVisualization = nullptr;
    CustomDrawArea *drawArea = nullptr;
    ShapeModel::Type selectedShapeType = ShapeModel::Type::Circle;
    QLabel *shapeCountLabel = nullptr; // Membre pour le label du compteur
    TrajetMotor* trajetMotor = nullptr;
    void setFormEditingEnabled(bool enabled);
    void StartPixel();
    void retranslateDynamicUi();
    bool promptAndSaveCurrentCustomShape();
    OpenAIService *m_aiService = nullptr;

protected:
    void showEvent(QShowEvent *event) override;

};

#endif // MAINWINDOW_H
