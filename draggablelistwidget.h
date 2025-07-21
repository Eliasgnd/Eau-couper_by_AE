#ifndef DRAGGABLELISTWIDGET_H
#define DRAGGABLELISTWIDGET_H

#include <QListWidget>

class DraggableListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit DraggableListWidget(QWidget *parent = nullptr);

signals:
    void dragStarted();
    void dragFinished();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void startManualDrag();
    void updateDrag(const QPoint &pos);
    void finishDrag();

    QPoint m_startPos;
    QPoint m_dragOffset;
    QListWidgetItem *m_dragItem {nullptr};
    QListWidgetItem *m_placeholderItem {nullptr};
    QWidget *m_dragWidget {nullptr};
    bool m_dragging {false};
};

#endif // DRAGGABLELISTWIDGET_H
