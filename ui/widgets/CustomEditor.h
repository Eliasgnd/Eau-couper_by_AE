#ifndef CUSTOMEDITOR_H
#define CUSTOMEDITOR_H

#include <QWidget>
#include "CustomDrawArea.h"
#include "Language.h"
#include <QGraphicsView>
#include <QGraphicsScene>

namespace Ui {
class CustomEditor;
}

class CustomEditorViewModel;

// Classe représentant la fenêtre personnalisée
class CustomEditor : public QWidget
{
    Q_OBJECT

public:
    explicit CustomEditor(CustomEditorViewModel *viewModel,
                          Language lang = Language::French,
                          QWidget *parent = nullptr);
    ~CustomEditor(); // Destructeur
    void updateShapeButtonIcon(CustomDrawArea::DrawMode mode);

protected:
    void changeEvent(QEvent *event) override;


private:
    Ui::CustomEditor *ui;
    CustomDrawArea *drawArea; // Instance de la zone de dessin
    QGraphicsView  *m_colorView{};
    QGraphicsView  *m_edgeView{};
    QGraphicsScene *m_colorScene{};
    QGraphicsScene *m_edgeScene{};
    CustomEditorViewModel *m_viewModel = nullptr;
    QStringList m_favoriteFonts;


private slots:
    void goToMainWindow(); // Retourner à la fenêtre principale
    void openKeyboardDialog(); // Ouvrir le clavier virtuel
    void closeEditor(); // Fermer la fenêtre custom
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

#endif // CUSTOMEDITOR_H
