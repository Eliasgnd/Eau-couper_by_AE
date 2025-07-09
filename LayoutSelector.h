#ifndef LAYOUTSELECTOR_H
#define LAYOUTSELECTOR_H

#include <QDialog>
#include <QList>
#include <QPolygonF>
#include <QFrame>
#include "inventaire.h" // for LayoutData
#include "Language.h"

class LayoutSelector : public QDialog {
    Q_OBJECT
public:
    LayoutSelector(const QList<LayoutData>& layouts,
                   const QList<QPolygonF>& shapePolygons,
                   Language lang,
                   QWidget *parent = nullptr);

    bool hasSelection() const { return m_hasSelection; }
    LayoutData selectedLayout() const { return m_selectedLayout; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QFrame* createLayoutFrame(int index);
    QList<LayoutData> m_layouts;
    QList<QPolygonF> m_polygons;
    LayoutData m_selectedLayout;
    bool m_hasSelection = false;
    Language m_lang {Language::French};
};

#endif // LAYOUTSELECTOR_H
