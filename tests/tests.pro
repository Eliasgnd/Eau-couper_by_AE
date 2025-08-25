QT += testlib widgets
CONFIG += console testcase
TARGET = placement_tests
SOURCES += placement_tests.cpp \
           ../motorcontrol.cpp \
           ../GeometryUtils.cpp
INCLUDEPATH += ..
HEADERS += ../motorcontrol.h
