QT += testlib widgets
CONFIG += console testcase
TARGET = placement_tests
SOURCES += placement_tests.cpp \
           inventory_safety_tests.cpp \
           shape_manager_tests.cpp \
           main.cpp \
           ../MotorControl.cpp \
           ../GeometryUtils.cpp \
           ../drawing/ShapeManager.cpp
INCLUDEPATH += ..
HEADERS += ../MotorControl.h \
           placement_tests.h \
           inventory_safety_tests.h \
           shape_manager_tests.h \
           ../drawing/ShapeManager.h
