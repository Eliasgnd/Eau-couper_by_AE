#ifndef LAYOUTSELECTOR_H
#define LAYOUTSELECTOR_H

#include <QDialog>
#include <QList>
#include <QPolygonF>
#include <QFrame>
#include "inventaire.h" // for LayoutData
#include "Language.h"

namespace Ui {
class Dispositions;
}

class LayoutSelector : public QDialog {
    Q_OBJECT
public:
    LayoutSelector(const QList<LayoutData>& layouts,
                   const QList<QPolygonF>& shapePolygons,
                   Language lang,
                   QWidget *parent = nullptr);
    ~LayoutSelector();

    bool hasSelection() const { return m_hasSelection; }
    LayoutData selectedLayout() const { return m_selectedLayout; }
    bool shouldOpenInventaire() const { return m_openInventaire; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    Ui::Dispositions *ui;
    QFrame* createLayoutFrame(int index);
    QFrame* createBaseShapeFrame();
    QList<LayoutData> m_layouts;
    QList<QPolygonF> m_polygons;
    LayoutData m_selectedLayout;
    bool m_hasSelection = false;
    Language m_lang {Language::French};
    bool m_openInventaire = false;
};

#endif // LAYOUTSELECTOR_H
