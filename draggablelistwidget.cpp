#include "draggablelistwidget.h"
#include <QApplication>
#include <QMimeData>
#include <QMouseEvent>

DraggableListWidget::DraggableListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setDragDropMode(QAbstractItemView::NoDragDrop);
    setDefaultDropAction(Qt::MoveAction);
}

void DraggableListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_startPos = event->pos();
        m_dragItem = itemAt(m_startPos);
    }
    QListWidget::mousePressEvent(event);
}

void DraggableListWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        QListWidget::mouseMoveEvent(event);
        return;
    }

    if (m_dragItem && !m_dragging &&
        (event->pos() - m_startPos).manhattanLength() >= QApplication::startDragDistance()) {
        startManualDrag();
    }

    if (m_dragging) {
        updateDrag(event->pos());
    } else {
        QListWidget::mouseMoveEvent(event);
    }
}

void DraggableListWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging)
        finishDrag();
    QListWidget::mouseReleaseEvent(event);
    m_dragItem = nullptr;
}

void DraggableListWidget::startManualDrag()
{
    if (!m_dragItem)
        return;

    emit dragStarted();

    m_dragging = true;
    m_dragWidget = itemWidget(m_dragItem);
    removeItemWidget(m_dragItem);
    int row = this->row(m_dragItem);
    takeItem(row);

    m_placeholderItem = new QListWidgetItem;
    m_placeholderItem->setSizeHint(m_dragWidget->size());
    insertItem(row, m_placeholderItem);

    m_dragOffset = m_startPos - visualItemRect(m_placeholderItem).topLeft();
    m_dragWidget->setParent(viewport());
    m_dragWidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_dragWidget->raise();
    m_dragWidget->move(m_startPos - m_dragOffset);
    m_dragWidget->show();
    viewport()->setCursor(Qt::ClosedHandCursor);
}

void DraggableListWidget::updateDrag(const QPoint &pos)
{
    if (!m_dragging)
        return;

    m_dragWidget->move(pos - m_dragOffset);

    QListWidgetItem *target = itemAt(pos);
    int targetRow = target ? row(target) : count();
    int placeholderRow = row(m_placeholderItem);
    if (targetRow != placeholderRow && targetRow != -1) {
        takeItem(placeholderRow);
        insertItem(targetRow, m_placeholderItem);
    }
}

void DraggableListWidget::finishDrag()
{
    if (!m_dragging)
        return;

    int row = this->row(m_placeholderItem);
    insertItem(row, m_dragItem);
    setItemWidget(m_dragItem, m_dragWidget);
    delete m_placeholderItem;
    m_placeholderItem = nullptr;
    m_dragWidget = nullptr;
    m_dragging = false;
    viewport()->unsetCursor();

    emit dragFinished();
}
