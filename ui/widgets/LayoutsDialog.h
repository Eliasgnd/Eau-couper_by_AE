#ifndef DISPOSITIONS_H
#define DISPOSITIONS_H

#include <QWidget>
#include <QList>
#include <QPolygonF>
#include <QFrame>
#include <QString>
#include "InventoryDomainTypes.h"
#include "Language.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LayoutsDialog; }
QT_END_NAMESPACE

class InventoryViewModel;

class LayoutsDialog : public QWidget
{
    Q_OBJECT
public:
    LayoutsDialog(const QString &shapeName,
                 const QList<LayoutData> &layouts,
                 const QList<QPolygonF> &shapePolygons,
                 Language lang,
                 InventoryViewModel *vm,
                 bool isBaseShape = false,
                 int baseType = 0,
                 QWidget *parent = nullptr);
    ~LayoutsDialog();

signals:
    void layoutSelected(const LayoutData &layout);
    void shapeOnlySelected();
    void requestOpenInventory();
    void closed();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public:
    void applyTheme(bool isDark);

private slots:
    void onCloseButtonClicked();
    void onMenuButtonClicked();
    void onSearchTextChanged(const QString &text);
    void onClearSearchClicked();
    void onSortChanged(int index);
    void toggleTheme();

private:
    bool m_isDarkTheme = false;
    void applyStyleSheets();
    void updateThemeButton();

    Ui::LayoutsDialog *ui;
    QFrame* createLayoutFrame(int index);
    QFrame* createBaseShapeFrame();
    void displayLayouts(const QString &filter = QString());
    QList<LayoutData> m_layouts;
    QList<QPolygonF> m_polygons;
    Language m_lang {Language::French};
    QString m_shapeName;
    bool m_isBaseShape {false};
    int m_baseType {0};
    InventoryViewModel *m_vm {nullptr};
};

#endif // DISPOSITIONS_H
