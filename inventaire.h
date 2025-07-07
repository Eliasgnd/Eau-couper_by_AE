#ifndef INVENTAIRE_H
#define INVENTAIRE_H

#include <QWidget>
#include <QPolygonF>
#include <QList>
#include "ShapeModel.h"
#include <QFrame>
#include "Language.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Inventaire; }
QT_END_NAMESPACE

// Structure pour stocker une forme custom (avec plusieurs tracés) et son nom
struct CustomShapeData {
    QList<QPolygonF> polygons;
    QString name;
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

signals:
    // Signal émis lorsqu'une forme prédéfinie est sélectionnée (Cercle, Rectangle, etc.)
    void shapeSelected(ShapeModel::Type type, int width, int height);

    // Signal émis lorsqu'une forme custom est sélectionnée
    void customShapeSelected(const QList<QPolygonF> &polygons);
protected:
    // Pour intercepter les clics sur les cadres (vignettes)
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // Retour vers le MainWindow
    void goToMainWindow();

private:
    Ui::Inventaire *ui;
    static Inventaire *instance;

    // Affiche l'ensemble des formes (prédéfinies + custom) dans le scrollArea
    void displayShapes();

    // Construit et renvoie le QFrame d'une forme custom à l'index donné
    QFrame* addCustomShapeToGrid(int index);

    // Liste des formes custom sauvegardées
    QList<CustomShapeData> m_customShapes;
    Language currentLanguage = Language::French;
};

#endif // INVENTAIRE_H
