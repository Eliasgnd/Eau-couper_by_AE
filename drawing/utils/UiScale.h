// UiScale.h
#pragma once

#include <QWidget>
#include <QScreen>
#include <QGuiApplication>
#include <QFont>

namespace UiScale {
    inline void scaleUiToScreen(QWidget* parent)
    {
        // taille « design » de la fenêtre (celle dans QtDesigner)
        QSize origSize = parent->size();

        // taille réelle du 2ᵉ écran
        auto screens = QGuiApplication::screens();
        if (screens.size() < 2)
            return;
        QRect screenGeo = screens.at(1)->availableGeometry();

        // ratios de mise à l’échelle
        qreal fx = qreal(screenGeo.width())  / origSize.width();
        qreal fy = qreal(screenGeo.height()) / origSize.height();

        // on ajuste chaque widget enfant
        for (QWidget* w : parent->findChildren<QWidget*>()) {
            if (w == parent) continue;
            QRect g = w->geometry();
            g.setX(int(g.x() * fx));
            g.setY(int(g.y() * fy));
            g.setWidth (int(g.width()  * fx));
            g.setHeight(int(g.height() * fy));
            w->setGeometry(g);

            // scale de la police
            QFont f = w->font();
            f.setPointSizeF(f.pointSizeF() * ((fx + fy) / 2));
            w->setFont(f);
        }
    }
}
