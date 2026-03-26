// ScreenUtils.h
#pragma once

#include <QWidget>
#include <QGuiApplication>
#include <QScreen>

namespace ScreenUtils {
    inline void placeOnSecondaryScreen(QWidget* w) {
        const auto screens = QGuiApplication::screens();
        if (screens.size() > 1) {
            QScreen* second = screens.at(0);
            QRect geo = second->availableGeometry();
            w->resize(geo.size());
            w->move(geo.topLeft());
        }
    }
}
