#pragma once

#include <QtGlobal>          // Q_OS_LINUX / Q_OS_WIN, …
#include <QThread>
#include <QPointF>
#include <QString>

/**
 *  TouchGestureReader
 *  ------------------
 *  • Linux : lit les événements EVDEV et génère des signaux gestuels.
 *  • Autres OS : se compile avec des stubs (pas de gestes, log seulement).
 */
class TouchGestureReader : public QThread
{
    Q_OBJECT
public:
    explicit TouchGestureReader(QObject *parent = nullptr);

    void setDevicePath(const QString &p) { m_devicePath = p; }
    void stop();                       // interrompt proprement le thread

protected:
    void run() override;               // boucle principale

signals:
    void pinchZoomDetected     (const QPointF &center, qreal scaleFactor);
    void twoFingerPanDetected  (const QPointF &delta);
    void twoFingersTouchChanged(bool active);

private:
    bool    m_running = false;

#ifdef Q_OS_LINUX                      // par défaut sur un noyau Linux
    QString m_devicePath{"/dev/input/event5"};
#else                                   // sur Windows / macOS : vide
    QString m_devicePath{};
#endif
};
