#pragma once
#include <QString>
#include <QCoreApplication>
#include <QDir>

namespace ImagePaths {
    inline QString rootDir() {
        QString dir = qApp->applicationDirPath() + "/images_generees";
        QDir().mkpath(dir);
        return dir;
    }

    inline QString iaDir() {
        QString d = rootDir() + "/ia";
        QDir().mkpath(d);
        return d;
    }

    inline QString wifiDir() {
        QString d = rootDir() + "/wifi";
        QDir().mkpath(d);
        return d;
    }

    inline QString bluetoothDir() {
        QString d = rootDir() + "/bluetooth";
        QDir().mkpath(d);
        return d;
    }
}
