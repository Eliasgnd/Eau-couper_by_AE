#ifndef KEYBOARDEVENTFILTER_H
#define KEYBOARDEVENTFILTER_H

#include <QObject>
#include <QEvent>        // <— nécessaire pour QEvent

class KeyboardEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit KeyboardEventFilter(QObject *parent = nullptr);

protected:
    // La signature doit matcher celle de QObject::eventFilter
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool m_keyboardActive;
};

#endif // KEYBOARDEVENTFILTER_H
