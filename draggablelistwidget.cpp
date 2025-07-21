#include "draggablelistwidget.h"
#include <QDrag>
#include <QMimeData>

DraggableListWidget::DraggableListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
}

void DraggableListWidget::startDrag(Qt::DropActions supportedActions)
{
    QListWidgetItem *item = currentItem();
    if (!item) {
        QListWidget::startDrag(supportedActions);
        return;
    }

    QWidget *widget = itemWidget(item);
    QPixmap pixmap;
    if (widget)
        pixmap = widget->grab();

    QMimeData *mime = model()->mimeData(selectedIndexes());
    if (!mime)
        return;

    QDrag drag(this);
    drag.setMimeData(mime);
    if (!pixmap.isNull()) {
        drag.setPixmap(pixmap);
        drag.setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    }

    emit dragStarted();
    drag.exec(Qt::MoveAction);
    emit dragFinished();
}
