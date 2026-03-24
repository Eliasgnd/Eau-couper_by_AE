#include "TouchGestureReader.h"

#ifdef Q_OS_LINUX
#   include <linux/input.h>
#   include <fcntl.h>
#   include <unistd.h>
#endif

#include <QDebug>
#include <QThread>
#include <cmath>

/* ------------------------------------------------------------------ */
/*                    Construction / arrêt du thread                   */
/* ------------------------------------------------------------------ */
TouchGestureReader::TouchGestureReader(QObject *parent)
    : QThread(parent)
{}

void TouchGestureReader::stop()
{
    m_running = false;
    wait();                         // bloque jusqu’à la fin du run()
}

/* ------------------------------------------------------------------ */
/*                         Boucle principale                           */
/* ------------------------------------------------------------------ */
void TouchGestureReader::run()
{
#ifdef Q_OS_LINUX
    qDebug() << "[GESTURE THREAD] EVDEV loop on" << m_devicePath;

    int fd = ::open(m_devicePath.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        qWarning() << "❌ Impossible d’ouvrir" << m_devicePath;
        return;
    }

    /* ---------- État courant des deux premiers contacts ------------- */
    QPointF tp[2]   { {-1,-1}, {-1,-1} };
    bool    down[2] { false  , false   };
    int     slot    = 0;

    /* ---------- Variables de geste ---------------------------------- */
    bool    gestureActive = false;
    qreal   prevDist      = -1.0;
    QPointF prevCenter    (-1,-1);
    bool    skipFirstPan  = false;
    bool    skipFirstZoom = false;

    struct input_event ev {};
    m_running = true;

    while (m_running)
    {
        if (::read(fd, &ev, sizeof ev) != sizeof ev) {
            QThread::msleep(1);
            continue;
        }

        /* ---------- Décodage EV_ABS multi-touch ---------------------- */
        if (ev.type == EV_ABS) {
            switch (ev.code) {
            case ABS_MT_SLOT:          slot = ev.value;                     break;

            case ABS_MT_TRACKING_ID:                                       // pose / levé
                if (slot < 2) {
                    down[slot] = (ev.value >= 0);
                    if (!down[slot]) tp[slot] = QPointF(-1,-1);
                }
                break;

            case ABS_MT_POSITION_X:   if (slot < 2) tp[slot].setX(ev.value); break;
            case ABS_MT_POSITION_Y:   if (slot < 2) tp[slot].setY(ev.value); break;
            }
        }

        /* ---------- Fin de frame EV_SYN ------------------------------ */
        if (ev.type == EV_SYN && ev.code == SYN_REPORT)
        {
            const bool twoDown =
                down[0] && down[1] &&
                tp[0] != QPointF(-1,-1) &&
                tp[1] != QPointF(-1,-1);

            if (twoDown)
            {
                const qreal   dx     = tp[0].x() - tp[1].x();
                const qreal   dy     = tp[0].y() - tp[1].y();
                const qreal   dist   = std::hypot(dx, dy);
                const QPointF center = (tp[0] + tp[1]) / 2.0;

                if (!gestureActive) {            // nouveau geste
                    gestureActive  = true;
                    prevDist       = dist;
                    prevCenter     = center;
                    skipFirstZoom  = true;
                    skipFirstPan   = true;
                    emit twoFingersTouchChanged(true);
                    continue;
                }

                /* -------- Pinch-zoom -------------------------------- */
                if (!skipFirstZoom && prevDist > 0.0) {
                    const qreal factor = dist / prevDist;
                    if (std::fabs(factor - 1.0) > 0.005)
                        emit pinchZoomDetected(center, factor);
                }
                prevDist      = dist;
                skipFirstZoom = false;

                /* -------- Pan --------------------------------------- */
                QPointF delta = center - prevCenter;
                if (!skipFirstPan && delta.manhattanLength() > 1.0)
                    emit twoFingerPanDetected(delta);
                prevCenter   = center;
                skipFirstPan = false;
            }
            else if (gestureActive)             // un doigt vient d’être levé
            {
                gestureActive = false;
                prevDist      = -1.0;
                prevCenter    = QPointF(-1,-1);
                emit twoFingersTouchChanged(false);
            }
        }
    }

    ::close(fd);

#else   /* ------------------------------------------------------------ */
    // Plateforme non prise en charge : boucle inerte (évite les #ifdef partout)
    qInfo() << "[GESTURE THREAD] Touch-gesture reader non disponible sur cet OS.";
    m_running = true;
    while (m_running)
        QThread::msleep(100);
#endif
}
