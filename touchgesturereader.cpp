#include "touchgesturereader.h"
#include <linux/input.h>

#include <fcntl.h>
#include <unistd.h>
#include <cmath>
#include <QDebug>
#include <QThread>

TouchGestureReader::TouchGestureReader(QObject *parent)
    : QThread(parent)
{}

void TouchGestureReader::stop()
{
    m_running = false;
    wait();
}

/* ------------------------------------------------------------------ */
/*                         Thread principal                            */
/* ------------------------------------------------------------------ */
void TouchGestureReader::run()
{
    qDebug() << "[GESTURE THREAD] Loop active, device:" << m_devicePath;

    int fd = ::open(m_devicePath.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        qWarning() << "❌ Impossible d’ouvrir" << m_devicePath;
        return;
    }

    /* ---------- État courant des deux premiers slots tactiles -------- */
    QPointF tp[2]      { {-1,-1}, {-1,-1} };
    bool    down[2]    { false  , false   };
    int     slot       = 0;

    /* ---------- Variables pour les gestes ---------------------------- */
    bool    gestureActive   = false;
    qreal   prevDist        = -1.0;
    QPointF prevCenter      (-1,-1);
    bool    skipFirstPan    = false;
    bool    skipFirstZoom   = false;

    m_running = true;
    struct input_event ev {};

    while (m_running)
    {
        if (::read(fd, &ev, sizeof ev) != sizeof ev) {
            QThread::msleep(1);
            continue;
        }

        /* ---------- Décodage EV_ABS multi-touch ---------------------- */
        if (ev.type == EV_ABS) {
            switch (ev.code) {
            case ABS_MT_SLOT:          slot = ev.value;                   break;

            case ABS_MT_TRACKING_ID:                                   // doigt posé / levé
                if (slot < 2) {
                    down[slot] = (ev.value >= 0);
                    if (!down[slot]) tp[slot] = QPointF(-1,-1);
                }
                break;

            case ABS_MT_POSITION_X:
                if (slot < 2) tp[slot].setX(ev.value);
                break;

            case ABS_MT_POSITION_Y:
                if (slot < 2) tp[slot].setY(ev.value);
                break;
            }
        }

        /* ---------- Fin de frame ------------------------------------ */
        if (ev.type == EV_SYN && ev.code == SYN_REPORT)
        {
            const bool twoDown =
                    down[0] && down[1] &&
                    tp[0]   != QPointF(-1,-1) &&
                    tp[1]   != QPointF(-1,-1);

            if (twoDown)
            {
                /* centre + distance courants --------------------------------*/
                const qreal   dx      = tp[0].x() - tp[1].x();
                const qreal   dy      = tp[0].y() - tp[1].y();
                const qreal   dist    = std::hypot(dx, dy);
                const QPointF center  = (tp[0] + tp[1]) / 2.0;

                if (!gestureActive) {
                    /* -------- nouveau geste détecté ---------------------- */
                    gestureActive  = true;
                    prevDist       = dist;
                    prevCenter     = center;
                    skipFirstZoom  = true;
                    skipFirstPan   = true;
                    emit twoFingersTouchChanged(true);
                    continue;      // on attend la prochaine frame
                }

                /* -------- Pinch-zoom ------------------------------------ */
                if (!skipFirstZoom && prevDist > 0.0) {
                    const qreal factor = dist / prevDist;
                    if (std::fabs(factor - 1.0) > 0.005)   // seuil anti-bruit
                        emit pinchZoomDetected(center, factor);
                }
                prevDist     = dist;
                skipFirstZoom = false;

                /* -------- Pan ------------------------------------------- */
                QPointF delta = center - prevCenter;
                if (!skipFirstPan && delta.manhattanLength() > 1.0)
                    /* FIX  : plus d’inversion d’axes ici */
                    emit twoFingerPanDetected(delta);
                prevCenter   = center;
                skipFirstPan = false;
            }
            else if (gestureActive)
            {
                /* -------- on vient de lever un doigt -------------------- */
                gestureActive = false;
                prevDist      = -1.0;
                prevCenter    = QPointF(-1,-1);
                emit twoFingersTouchChanged(false);
            }
        }
    }

    ::close(fd);
}
