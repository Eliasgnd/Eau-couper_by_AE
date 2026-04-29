#ifndef CUSTOMEDITOR_H
#define CUSTOMEDITOR_H

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QWidget>

#include "CustomDrawArea.h"
#include "Language.h"

class QFrame;
class QLabel;
class QPushButton;
class QDoubleSpinBox;
class QWidget;

namespace Ui {
class CustomEditor;
}

class CustomEditorViewModel;
class CanvasViewModel;

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
    CanvasViewModel *m_canvasViewModel = nullptr;
    QGraphicsView  *m_colorView{};
    QGraphicsView  *m_edgeView{};
    QGraphicsScene *m_colorScene{};
    QGraphicsScene *m_edgeScene{};
    CustomEditorViewModel *m_viewModel = nullptr;
    QStringList m_favoriteFonts;
    bool m_isDarkTheme = false;

    QFrame *m_assistanceBar = nullptr;
    QLabel *m_assistanceTitle = nullptr;
    QLabel *m_assistanceHint = nullptr;
    QLabel *m_assistanceDetail = nullptr;
    QPushButton *m_cancelModeButton = nullptr;
    QPushButton *m_precisionConstraintButton = nullptr;
    QPushButton *m_segmentStatusButton = nullptr;
    QPushButton *m_placementAssistButton = nullptr;
    QPushButton *m_placementMagnetButton = nullptr;

    QWidget *m_selectionActionsWidget = nullptr;
    QWidget *m_selectionInspectorWidget = nullptr;
    QLabel *m_selectionCountLabel = nullptr;
    QDoubleSpinBox *m_selectionXSpin = nullptr;
    QDoubleSpinBox *m_selectionYSpin = nullptr;
    QDoubleSpinBox *m_selectionWidthSpin = nullptr;
    QDoubleSpinBox *m_selectionHeightSpin = nullptr;
    QDoubleSpinBox *m_selectionRotationSpin = nullptr;
    bool m_updatingSelectionInspector = false;

    QPushButton *m_touchDuplicateButton = nullptr;
    QPushButton *m_alignMenuButton = nullptr;
    QPushButton *m_touchDeleteButton = nullptr;
    QPushButton *m_zoomSelectionButton = nullptr;
    QPushButton *m_fitDrawingButton = nullptr;
    QPushButton *m_finishPointPathButton = nullptr;
    QPushButton *m_undoPointButton = nullptr;
    QPushButton *m_undoSegmentButton = nullptr;
    QPushButton *m_touchNudgeLeftButton = nullptr;
    QPushButton *m_touchNudgeRightButton = nullptr;
    QPushButton *m_touchNudgeUpButton = nullptr;
    QPushButton *m_touchNudgeDownButton = nullptr;

    void applyStyleSheets();
    void updateThemeButton();
    void updateTouchSelectionPanel(bool hasSelection, const QString &summary);
    void refreshSelectionInspector();
    void updateCanvasStatus(const QString &modeLabel, const QString &hint, const QString &detail);
    void updateHistoryButtons(bool canUndo, const QString &undoText,
                              bool canRedo, const QString &redoText);
    void refreshModeButtons();

private slots:
    void goToMainWindow();
    void openKeyboardDialog();
    void closeEditor();
    void importerLogo();
    void importerImageCouleur();
    void saveCustomShape();
    void toggleTheme();

signals:
    void applyCustomShapeSignal(QList<QPolygonF> shapes);
    void resetDrawingSignal();

public:
    CustomDrawArea* getDrawArea() const { return drawArea; }
};

#endif // CUSTOMEDITOR_H
