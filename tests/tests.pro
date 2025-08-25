QT += testlib widgets
CONFIG += console testcase
TARGET = placement_tests
SOURCES += placement_tests.cpp \
           inventory_safety_tests.cpp \
           main.cpp \
           ../motorcontrol.cpp \
           ../GeometryUtils.cpp \
           ../SafeInventoryLoader.cpp
INCLUDEPATH += ..
HEADERS += ../motorcontrol.h \
           ../SafeInventoryLoader.h \
           placement_tests.h \
           inventory_safety_tests.h
