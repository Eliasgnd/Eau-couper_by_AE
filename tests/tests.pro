QT += testlib widgets
CONFIG += console testcase
TARGET = placement_tests
SOURCES += placement_tests.cpp \
           inventory_safety_tests.cpp \
           shape_manager_tests.cpp \
           main.cpp \
           ../infrastructure/hardware/MotorControl.cpp \
           ../domain/geometry/GeometryUtils.cpp \
           ../domain/shapes/ShapeManager.cpp
INCLUDEPATH += .. \
               ../domain/geometry \
               ../domain/shapes \
               ../domain/interfaces \
               ../infrastructure/hardware \
               ../shared
HEADERS += ../infrastructure/hardware/MotorControl.h \
           placement_tests.h \
           inventory_safety_tests.h \
           shape_manager_tests.h \
           ../domain/shapes/ShapeManager.h
