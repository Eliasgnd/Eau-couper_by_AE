#ifndef SHAPE_MANAGER_TESTS_H
#define SHAPE_MANAGER_TESTS_H

#include <QObject>

class ShapeManagerTests : public QObject
{
    Q_OBJECT
private slots:
    void addAndRemoveShape();
    void selectionBounds();
    void undoRestoresPreviousState();
};

#endif // SHAPE_MANAGER_TESTS_H
