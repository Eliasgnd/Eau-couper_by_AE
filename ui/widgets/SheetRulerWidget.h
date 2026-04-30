#ifndef SHEETRULERWIDGET_H
#define SHEETRULERWIDGET_H

#include <QGraphicsView>
#include <QWidget>

class SheetRulerWidget : public QWidget
{
public:
    explicit SheetRulerWidget(Qt::Orientation orientation,
                              QGraphicsView *view,
                              QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal chooseMajorStep(qreal pixelsPerMillimeter) const;

    Qt::Orientation m_orientation;
    QGraphicsView *m_view = nullptr;
};

#endif // SHEETRULERWIDGET_H
