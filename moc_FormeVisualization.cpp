/****************************************************************************
** Meta object code from reading C++ file 'FormeVisualization.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "FormeVisualization.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FormeVisualization.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FormeVisualization_t {
    QByteArrayData data[30];
    char stringdata0[403];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FormeVisualization_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FormeVisualization_t qt_meta_stringdata_FormeVisualization = {
    {
QT_MOC_LITERAL(0, 0, 18), // "FormeVisualization"
QT_MOC_LITERAL(1, 19, 24), // "optimizationStateChanged"
QT_MOC_LITERAL(2, 44, 0), // ""
QT_MOC_LITERAL(3, 45, 9), // "optimized"
QT_MOC_LITERAL(4, 55, 17), // "shapesPlacedCount"
QT_MOC_LITERAL(5, 73, 14), // "spacingChanged"
QT_MOC_LITERAL(6, 88, 10), // "newSpacing"
QT_MOC_LITERAL(7, 99, 19), // "blackPixelsProgress"
QT_MOC_LITERAL(8, 119, 9), // "remaining"
QT_MOC_LITERAL(9, 129, 5), // "total"
QT_MOC_LITERAL(10, 135, 19), // "displayCustomShapes"
QT_MOC_LITERAL(11, 155, 16), // "QList<QPolygonF>"
QT_MOC_LITERAL(12, 172, 6), // "shapes"
QT_MOC_LITERAL(13, 179, 18), // "moveSelectedShapes"
QT_MOC_LITERAL(14, 198, 2), // "dx"
QT_MOC_LITERAL(15, 201, 2), // "dy"
QT_MOC_LITERAL(16, 204, 20), // "rotateSelectedShapes"
QT_MOC_LITERAL(17, 225, 10), // "angleDelta"
QT_MOC_LITERAL(18, 236, 14), // "getBlackPixels"
QT_MOC_LITERAL(19, 251, 13), // "QList<QPoint>"
QT_MOC_LITERAL(20, 265, 20), // "startDecoupeProgress"
QT_MOC_LITERAL(21, 286, 8), // "maxSteps"
QT_MOC_LITERAL(22, 295, 21), // "updateDecoupeProgress"
QT_MOC_LITERAL(23, 317, 11), // "currentStep"
QT_MOC_LITERAL(24, 329, 18), // "endDecoupeProgress"
QT_MOC_LITERAL(25, 348, 15), // "resetCutMarkers"
QT_MOC_LITERAL(26, 364, 12), // "bufferedPath"
QT_MOC_LITERAL(27, 377, 12), // "QPainterPath"
QT_MOC_LITERAL(28, 390, 4), // "path"
QT_MOC_LITERAL(29, 395, 7) // "spacing"

    },
    "FormeVisualization\0optimizationStateChanged\0"
    "\0optimized\0shapesPlacedCount\0"
    "spacingChanged\0newSpacing\0blackPixelsProgress\0"
    "remaining\0total\0displayCustomShapes\0"
    "QList<QPolygonF>\0shapes\0moveSelectedShapes\0"
    "dx\0dy\0rotateSelectedShapes\0angleDelta\0"
    "getBlackPixels\0QList<QPoint>\0"
    "startDecoupeProgress\0maxSteps\0"
    "updateDecoupeProgress\0currentStep\0"
    "endDecoupeProgress\0resetCutMarkers\0"
    "bufferedPath\0QPainterPath\0path\0spacing"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FormeVisualization[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   79,    2, 0x06 /* Public */,
       4,    1,   82,    2, 0x06 /* Public */,
       5,    1,   85,    2, 0x06 /* Public */,
       7,    2,   88,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    1,   93,    2, 0x0a /* Public */,
      13,    2,   96,    2, 0x0a /* Public */,
      16,    1,  101,    2, 0x0a /* Public */,
      18,    0,  104,    2, 0x0a /* Public */,
      20,    1,  105,    2, 0x0a /* Public */,
      22,    1,  108,    2, 0x0a /* Public */,
      24,    0,  111,    2, 0x0a /* Public */,
      25,    0,  112,    2, 0x0a /* Public */,
      26,    2,  113,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    8,    9,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal,   14,   15,
    QMetaType::Void, QMetaType::QReal,   17,
    0x80000000 | 19,
    QMetaType::Void, QMetaType::Int,   21,
    QMetaType::Void, QMetaType::Int,   23,
    QMetaType::Void,
    QMetaType::Void,
    0x80000000 | 27, 0x80000000 | 27, QMetaType::Int,   28,   29,

       0        // eod
};

void FormeVisualization::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FormeVisualization *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->optimizationStateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->shapesPlacedCount((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->spacingChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->blackPixelsProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->displayCustomShapes((*reinterpret_cast< const QList<QPolygonF>(*)>(_a[1]))); break;
        case 5: _t->moveSelectedShapes((*reinterpret_cast< qreal(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2]))); break;
        case 6: _t->rotateSelectedShapes((*reinterpret_cast< qreal(*)>(_a[1]))); break;
        case 7: { QList<QPoint> _r = _t->getBlackPixels();
            if (_a[0]) *reinterpret_cast< QList<QPoint>*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->startDecoupeProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->updateDecoupeProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->endDecoupeProgress(); break;
        case 11: _t->resetCutMarkers(); break;
        case 12: { QPainterPath _r = _t->bufferedPath((*reinterpret_cast< const QPainterPath(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QPainterPath*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<QPolygonF> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FormeVisualization::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FormeVisualization::optimizationStateChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FormeVisualization::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FormeVisualization::shapesPlacedCount)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (FormeVisualization::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FormeVisualization::spacingChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (FormeVisualization::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FormeVisualization::blackPixelsProgress)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject FormeVisualization::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_FormeVisualization.data,
    qt_meta_data_FormeVisualization,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *FormeVisualization::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FormeVisualization::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FormeVisualization.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int FormeVisualization::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void FormeVisualization::optimizationStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FormeVisualization::shapesPlacedCount(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void FormeVisualization::spacingChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void FormeVisualization::blackPixelsProgress(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
