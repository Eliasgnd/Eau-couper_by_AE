#ifndef NAVIGATIONCONTROLLER_H
#define NAVIGATIONCONTROLLER_H

#include <QObject>

class QWidget;
class custom;
class WifiTransferWidget;
enum class Language;

class NavigationController : public QObject
{
    Q_OBJECT
public:
    explicit NavigationController(QObject *parent = nullptr);

    void showInventaire(QWidget *from, QWidget *inventaire);
    custom *openCustomEditor(QWidget *from, Language language);
    WifiTransferWidget *openWifiTransfer(QWidget *from);
};

#endif // NAVIGATIONCONTROLLER_H
