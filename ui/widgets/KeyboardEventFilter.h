#ifndef KEYBOARDEVENTFILTER_H
#define KEYBOARDEVENTFILTER_H

#include <QObject>
#include <QEvent>        // <— nécessaire pour QEvent
class FormeVisualization;

class KeyboardEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit KeyboardEventFilter(QObject *parent = nullptr);
    void setFormeVisualization(FormeVisualization* visu);

protected:
    // La signature doit matcher celle de QObject::eventFilter
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool m_keyboardActive;
    FormeVisualization* m_visu = nullptr;
};

#endif // KEYBOARDEVENTFILTER_H
