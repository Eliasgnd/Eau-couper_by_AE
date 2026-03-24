#ifndef DISPOSITIONS_H
#define DISPOSITIONS_H

#include <QWidget>
#include <QList>
#include <QPolygonF>
#include <QFrame>
#include <QString>
#include "inventaire.h" // for LayoutData
#include "Language.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Dispositions; }
QT_END_NAMESPACE

class Dispositions : public QWidget
{
    Q_OBJECT
public:
    Dispositions(const QString &shapeName,
                 const QList<LayoutData> &layouts,
                 const QList<QPolygonF> &shapePolygons,
                 Language lang,
                 bool isBaseShape = false,
                 ShapeModel::Type baseType = ShapeModel::Type::Circle,
                 QWidget *parent = nullptr);
    ~Dispositions();

signals:
    void layoutSelected(const LayoutData &layout);
    void shapeOnlySelected();
    void requestOpenInventaire();
    void closed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onCloseButtonClicked();
    void onMenuButtonClicked();
    void onSearchTextChanged(const QString &text);
    void onClearSearchClicked();
    void onSortChanged(int index);

private:
    Ui::Dispositions *ui;
    QFrame* createLayoutFrame(int index);
    QFrame* createBaseShapeFrame();
    void displayLayouts(const QString &filter = QString());
    QList<LayoutData> m_layouts;
    QList<QPolygonF> m_polygons;
    Language m_lang {Language::French};
    QString m_shapeName;
    bool m_isBaseShape {false};
    ShapeModel::Type m_baseType {ShapeModel::Type::Circle};
};

#endif // DISPOSITIONS_H
