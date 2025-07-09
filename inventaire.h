#ifndef INVENTAIRE_H
#define INVENTAIRE_H

#include <QWidget>
#include <QPolygonF>
#include <QList>
#include <QMap>
#include "ShapeModel.h"
#include <QFrame>
#include "Language.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Inventaire; }
QT_END_NAMESPACE

// Représente un élément d'une disposition sauvegardée
struct LayoutItem {
    double x {0};
    double y {0};
    double rotation {0};
};

// Disposition complète d'une forme custom
struct LayoutData {
    QString name;
    int largeur {0};
    int longueur {0};
    int spacing {0};
    QList<LayoutItem> items;
};

// Structure pour stocker une forme custom (avec plusieurs tracés) et son nom
// ainsi que ses dispositions enregistrées
struct CustomShapeData {
    QList<QPolygonF> polygons;
    QString name;
    QList<LayoutData> layouts;
};


class Inventaire : public QWidget
{
    Q_OBJECT

public:

    bool gros_cavu;
    explicit Inventaire(QWidget *parent = nullptr);
    ~Inventaire();

    // Singleton pour accéder à l'inventaire globalement
    static Inventaire* getInstance();

    // Méthode pour ajouter une forme custom sauvegardée (liste de QPolygonF) dans l'inventaire
    void addSavedCustomShape(const QList<QPolygonF> &polygons, const QString &name);

    void updateTranslations(Language lang);

    // Enregistre une disposition pour une forme existante
    void addLayoutToShape(const QString &shapeName, const LayoutData &layout);

    // Enregistre une disposition pour une forme de base
    void addLayoutToBaseShape(ShapeModel::Type type, const LayoutData &layout);

    // Renomme une disposition
    void renameLayout(const QString &shapeName, int index, const QString &newName);

    void renameBaseLayout(ShapeModel::Type type, int index, const QString &newName);

    // Supprime une disposition
    void deleteLayout(const QString &shapeName, int index);

    void deleteBaseLayout(ShapeModel::Type type, int index);

    // Retourne les dispositions d'une forme
    QList<LayoutData> getLayoutsForShape(const QString &shapeName) const;
    QList<LayoutData> getLayoutsForBaseShape(ShapeModel::Type type) const;
    bool shapeNameExists(const QString &name) const;
    static QString baseShapeName(ShapeModel::Type type, Language lang);

protected:
    void changeEvent(QEvent *event) override;


signals:
    // Signal émis lorsqu'une forme prédéfinie est sélectionnée (Cercle, Rectangle, etc.)
    void shapeSelected(ShapeModel::Type type, int width, int height);

    // Signal émis lorsqu'une forme custom est sélectionnée
    void customShapeSelected(const QList<QPolygonF> &polygons, const QString &name);
protected:
    // Pour intercepter les clics sur les cadres (vignettes)
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // Retour vers le MainWindow
    void goToMainWindow();

private:
    Ui::Inventaire *ui;
    static Inventaire *instance;

    QString customShapesFilePath() const;
    void loadCustomShapes();
    void saveCustomShapes() const;

    // Affiche l'ensemble des formes (prédéfinies + custom) dans le scrollArea
    void displayShapes();

    // Construit et renvoie le QFrame d'une forme custom à l'index donné
    QFrame* addCustomShapeToGrid(int index);

    // Liste des formes custom sauvegardées
    QList<CustomShapeData> m_customShapes;
    QMap<ShapeModel::Type, QList<LayoutData>> m_baseShapeLayouts;
    Language currentLanguage = Language::French;
};

#endif // INVENTAIRE_H
