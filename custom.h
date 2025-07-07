#ifndef CUSTOM_H
#define CUSTOM_H

#include <QWidget>
#include "CustomDrawArea.h"
#include "Language.h"

namespace Ui {
class custom;
}

// Classe représentant la fenêtre personnalisée
class custom : public QWidget
{
    Q_OBJECT

public:
    explicit custom(Language lang = Language::French, QWidget *parent = nullptr); // Constructeur
    ~custom(); // Destructeur

protected:
    void changeEvent(QEvent *event) override;


private:
    Ui::custom *ui;
    CustomDrawArea *drawArea; // Instance de la zone de dessin
    QStringList m_favoriteFonts;


private slots:
    void goToMainWindow(); // Retourner à la fenêtre principale
    void ouvrirClavier(); // Ouvrir le clavier virtuel
    void closeCustom(); // Fermer la fenêtre custom
    void importerLogo(); // Importer un logo dans la zone de dessin
    void saveCustomShape(); // Sauvegarder une forme personnalisée
    void onCopyPasteClicked(); // Gérer le bouton copier/coller

signals:
    void applyCustomShapeSignal(QList<QPolygonF> shapes); // Signal pour appliquer une forme
    void resetDrawingSignal(); // Signal pour réinitialiser le dessin
};

#endif // CUSTOM_H
