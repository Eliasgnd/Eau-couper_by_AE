#include "AspectRatioWrapper.h"
#include <QSizePolicy>
#include <cmath>
#include <QDebug>

AspectRatioWrapper::AspectRatioWrapper(QWidget* child, double aspect, QWidget* parent)
    : QWidget(parent), m_child(nullptr), m_aspect(aspect)
{
    setContentsMargins(0,0,0,0);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (child) setChild(child);
}

void AspectRatioWrapper::setAspect(double a) {
    m_aspect = a;
    updateGeometry();
    applyLetterbox();
}

void AspectRatioWrapper::setChild(QWidget* child)
{
    if (m_child == child) return;
    if (m_child) {
        m_child->setParent(nullptr);
        m_child->hide();
    }
    m_child = child;
    if (m_child) {
        m_child->setParent(this);
        m_child->setContentsMargins(0,0,0,0);
        m_child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_child->show();
        applyLetterbox();
    }
}

int AspectRatioWrapper::heightForWidth(int w) const {
    if (m_child && m_child->hasHeightForWidth()) return m_child->heightForWidth(w);
    if (m_aspect <= 0.0) return QWidget::heightForWidth(w);
    return int(std::round(w / m_aspect));
}

// --- MODIFICATIONS ICI ---
QSize AspectRatioWrapper::sizeHint() const {
    if (m_child) return m_child->sizeHint();
    return QSize(800, 600); // Ne plus calculer de ratio ici
}

QSize AspectRatioWrapper::minimumSizeHint() const {
    return QSize(100, 100); // Empêche l'agrandissement forcé de la fenêtre globale
}
// -------------------------

void AspectRatioWrapper::resizeEvent(QResizeEvent*) {
    applyLetterbox();
}

void AspectRatioWrapper::applyLetterbox()
{
    if (!m_child) return;

    QRect r = rect();
    if (m_child->hasHeightForWidth()) {
        const int fullWidthHeight = m_child->heightForWidth(r.width());
        if (fullWidthHeight <= r.height()) {
            const int y = (r.height() - fullWidthHeight) / 2;
            m_child->setGeometry(0, y, r.width(), fullWidthHeight);
            return;
        }

        int low = 1;
        int high = qMax(1, r.width());
        int best = low;
        while (low <= high) {
            const int mid = low + (high - low) / 2;
            const int wantedHeight = m_child->heightForWidth(mid);
            if (wantedHeight <= r.height()) {
                best = mid;
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        const int x = (r.width() - best) / 2;
        m_child->setGeometry(x, 0, best, r.height());
        return;
    }

    if (m_aspect <= 0.0) {
        m_child->setGeometry(r);
        return;
    }

    const int W = r.width();
    const int H = r.height();
    const double wantedH = W / m_aspect;

    if (wantedH <= H) {
        int h = int(std::round(wantedH));
        int y = (H - h) / 2;
        m_child->setGeometry(0, y, W, h);
    } else {
        int w = int(std::round(H * m_aspect));
        int x = (W - w) / 2;
        m_child->setGeometry(x, 0, w, H);
    }
}
