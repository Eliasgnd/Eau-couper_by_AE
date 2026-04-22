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
    ~CustomEditor();
    void updateShapeButtonIcon(CustomDrawArea::DrawMode mode);
    void applyTheme(bool isDark);

protected:
    void changeEvent(QEvent *event) override;

private:
    Ui::CustomEditor *ui;
    CustomDrawArea *drawArea;
    QGraphicsView  *m_colorView{};
    QGraphicsView  *m_edgeView{};
    QGraphicsScene *m_colorScene{};
    QGraphicsScene *m_edgeScene{};
    CustomEditorViewModel *m_viewModel = nullptr;
    QStringList m_favoriteFonts;
    bool m_isDarkTheme = false;

    void applyStyleSheets();
    void updateThemeButton();


private slots:
    void goToMainWindow();
    void openKeyboardDialog();
    void closeEditor();
    void importerLogo();
    void importerImageCouleur();
    void saveCustomShape();
    void onCopyPasteClicked();
    void toggleTheme();

signals:
    void applyCustomShapeSignal(QList<QPolygonF> shapes); // Signal pour appliquer une forme
    void resetDrawingSignal(); // Signal pour réinitialiser le dessin

public:
    CustomDrawArea* getDrawArea() const { return drawArea; }
};

#endif // CUSTOMEDITOR_H
