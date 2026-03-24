#ifndef CUSTOM_H
#define CUSTOM_H

#include <QWidget>
#include "CustomDrawArea.h"
#include "Language.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include "ImageEdgeImporter.h"

namespace Ui {
class Custom;
}

// Classe représentant la fenêtre personnalisée
class Custom : public QWidget
{
    Q_OBJECT

public:
    explicit Custom(Language lang = Language::French, QWidget *parent = nullptr); // Constructeur
    ~Custom(); // Destructeur
    void updateFormeButtonIcon(CustomDrawArea::DrawMode mode);

protected:
    void changeEvent(QEvent *event) override;


private:
    Ui::Custom *ui;
    CustomDrawArea *drawArea; // Instance de la zone de dessin
    QGraphicsView  *m_colorView{};
    QGraphicsView  *m_edgeView{};
    QGraphicsScene *m_colorScene{};
    QGraphicsScene *m_edgeScene{};
    ImageEdgeImporter m_imageImporter;
    QStringList m_favoriteFonts;


private slots:
    void goToMainWindow(); // Retourner à la fenêtre principale
    void ouvrirClavier(); // Ouvrir le clavier virtuel
    void closeCustom(); // Fermer la fenêtre custom
    void importerLogo(); // Importer un logo dans la zone de dessin
    void importerImageCouleur(); // Importer une image couleur et afficher les bords
    void saveCustomShape(); // Sauvegarder une forme personnalisée
    void onCopyPasteClicked(); // Gérer le bouton copier/coller

signals:
    void applyCustomShapeSignal(QList<QPolygonF> shapes); // Signal pour appliquer une forme
    void resetDrawingSignal(); // Signal pour réinitialiser le dessin

public:
    CustomDrawArea* getDrawArea() const { return drawArea; }
};

#endif // CUSTOM_H
