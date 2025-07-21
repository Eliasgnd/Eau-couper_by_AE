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
    void startDrag(Qt::DropActions supportedActions) override;
};

#endif // DRAGGABLELISTWIDGET_H
