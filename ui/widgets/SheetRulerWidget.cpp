#include "SheetRulerWidget.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPaintEvent>
#include <QPolygon>
#include <QRectF>
#include <QVector>

#include <cmath>

namespace {
constexpr int kHorizontalRulerHeight = 28;
constexpr int kVerticalRulerWidth = 46;
constexpr int kTargetMajorTickPixels = 70;
}

SheetRulerWidget::SheetRulerWidget(Qt::Orientation orientation,
                                   QGraphicsView *view,
                                   QWidget *parent)
    : QWidget(parent)
    , m_orientation(orientation)
    , m_view(view)
{
    if (m_orientation == Qt::Horizontal) {
        setMinimumHeight(kHorizontalRulerHeight);
        setMaximumHeight(kHorizontalRulerHeight);
    } else {
        setMinimumWidth(kVerticalRulerWidth);
        setMaximumWidth(kVerticalRulerWidth);
    }
}

QSize SheetRulerWidget::minimumSizeHint() const
{
    return sizeHint();
}

QSize SheetRulerWidget::sizeHint() const
{
    return m_orientation == Qt::Horizontal
        ? QSize(120, kHorizontalRulerHeight)
        : QSize(kVerticalRulerWidth, 120);
}

void SheetRulerWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect(), QColor(248, 250, 252));

    if (!m_view || !m_view->scene()) {
        painter.setPen(QColor(203, 213, 225));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
        return;
    }

    const QRectF sceneRect = m_view->scene()->sceneRect();
    if (sceneRect.width() <= 0.0 || sceneRect.height() <= 0.0) {
        painter.setPen(QColor(203, 213, 225));
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
        return;
    }

    const QRect renderedRect = m_view->mapFromScene(sceneRect).boundingRect();
    const qreal pixelsPerMillimeter = m_orientation == Qt::Horizontal
        ? renderedRect.width() / sceneRect.width()
        : renderedRect.height() / sceneRect.height();

    const qreal majorStep = chooseMajorStep(pixelsPerMillimeter);
    const qreal minorStep = majorStep >= 50.0 ? majorStep / 5.0 : majorStep / 2.0;
    const int axisStart = m_orientation == Qt::Horizontal ? renderedRect.left() : renderedRect.top();
    const int axisEnd = m_orientation == Qt::Horizontal ? renderedRect.right() : renderedRect.bottom();
    const qreal logicalSpan = m_orientation == Qt::Horizontal ? sceneRect.width() : sceneRect.height();

    painter.setPen(Qt::NoPen);
    if (m_orientation == Qt::Horizontal) {
        painter.fillRect(QRect(axisStart, 0, qMax(0, axisEnd - axisStart), height()), QColor(241, 245, 249));
    } else {
        painter.fillRect(QRect(0, axisStart, width(), qMax(0, axisEnd - axisStart)), QColor(241, 245, 249));
    }

    painter.setPen(QColor(148, 163, 184));
    if (m_orientation == Qt::Horizontal) {
        painter.drawLine(axisStart, height() - 1, axisEnd, height() - 1);
    } else {
        painter.drawLine(width() - 1, axisStart, width() - 1, axisEnd);
    }

    painter.setPen(QColor(71, 85, 105));
    QFont labelFont = painter.font();
    labelFont.setPointSize(qMax(8, labelFont.pointSize() - 1));
    painter.setFont(labelFont);

    for (qreal value = 0.0; value <= logicalSpan + 0.001; value += minorStep) {
        const bool isMajor = std::fmod(value, majorStep) < 0.001
            || std::fabs(std::fmod(value, majorStep) - majorStep) < 0.001;
        const int tickLength = isMajor ? 10 : 5;

        if (m_orientation == Qt::Horizontal) {
            const int x = qRound(renderedRect.left() + value * pixelsPerMillimeter);
            painter.drawLine(x, height() - 1, x, height() - 1 - tickLength);
            if (isMajor) {
                const QString label = QString::number(qRound(value));
                painter.drawText(QRect(x + 3, 2, 48, height() - 6),
                                 Qt::AlignLeft | Qt::AlignTop,
                                 label);
            }
        } else {
            const int y = qRound(renderedRect.top() + value * pixelsPerMillimeter);
            painter.drawLine(width() - 1, y, width() - 1 - tickLength, y);
            if (isMajor) {
                const QString label = QString::number(qRound(value));
                painter.save();
                painter.translate(2, y - 2);
                painter.rotate(-90);
                painter.drawText(QRect(0, 0, 48, width() - 8),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 label);
                painter.restore();
            }
        }
    }

    painter.setPen(QColor(148, 163, 184));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

qreal SheetRulerWidget::chooseMajorStep(qreal pixelsPerMillimeter) const
{
    static const QVector<qreal> candidateSteps = {10.0, 20.0, 25.0, 50.0, 100.0, 200.0, 250.0, 500.0};

    for (qreal step : candidateSteps) {
        if (step * pixelsPerMillimeter >= kTargetMajorTickPixels)
            return step;
    }

    return candidateSteps.isEmpty() ? 100.0 : candidateSteps.constLast();
}
