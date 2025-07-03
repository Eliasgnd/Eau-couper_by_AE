#pragma once
#include <QThread>
#include <QPointF>
#include <QString>

class TouchGestureReader : public QThread
{
    Q_OBJECT
public:
    explicit TouchGestureReader(QObject *parent = nullptr);

    void setDevicePath(const QString &path) { m_devicePath = path; }
    void stop();

protected:
    void run() override;      // thread-loop

signals:
    void pinchZoomDetected  (const QPointF &center, qreal scaleFactor);
    void twoFingerPanDetected(const QPointF &delta);
    void twoFingersTouchChanged(bool active);

private:
    bool     m_running   = false;
    QString  m_devicePath{"/dev/input/event5"};   // à adapter
};
