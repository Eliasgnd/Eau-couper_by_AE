/****************************************************************************
** Meta object code from reading C++ file 'CustomDrawArea.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "CustomDrawArea.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CustomDrawArea.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CustomDrawArea_t {
    QByteArrayData data[27];
    char stringdata0[320];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CustomDrawArea_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CustomDrawArea_t qt_meta_stringdata_CustomDrawArea = {
    {
QT_MOC_LITERAL(0, 0, 14), // "CustomDrawArea"
QT_MOC_LITERAL(1, 15, 11), // "zoomChanged"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 8), // "newScale"
QT_MOC_LITERAL(4, 37, 16), // "closeModeChanged"
QT_MOC_LITERAL(5, 54, 7), // "enabled"
QT_MOC_LITERAL(6, 62, 14), // "shapeSelection"
QT_MOC_LITERAL(7, 77, 16), // "smoothingChanged"
QT_MOC_LITERAL(8, 94, 14), // "undoLastAction"
QT_MOC_LITERAL(9, 109, 23), // "mergeShapesAndConnector"
QT_MOC_LITERAL(10, 133, 4), // "idx1"
QT_MOC_LITERAL(11, 138, 4), // "idx2"
QT_MOC_LITERAL(12, 143, 17), // "closeCurrentShape"
QT_MOC_LITERAL(13, 161, 14), // "startCloseMode"
QT_MOC_LITERAL(14, 176, 15), // "cancelCloseMode"
QT_MOC_LITERAL(15, 192, 11), // "onPinchZoom"
QT_MOC_LITERAL(16, 204, 6), // "center"
QT_MOC_LITERAL(17, 211, 11), // "scaleFactor"
QT_MOC_LITERAL(18, 223, 14), // "onTwoFingerPan"
QT_MOC_LITERAL(19, 238, 5), // "delta"
QT_MOC_LITERAL(20, 244, 15), // "handlePinchZoom"
QT_MOC_LITERAL(21, 260, 6), // "factor"
QT_MOC_LITERAL(22, 267, 15), // "setTwoFingersOn"
QT_MOC_LITERAL(23, 283, 6), // "active"
QT_MOC_LITERAL(24, 290, 14), // "setGridSpacing"
QT_MOC_LITERAL(25, 305, 2), // "px"
QT_MOC_LITERAL(26, 308, 11) // "gridSpacing"

    },
    "CustomDrawArea\0zoomChanged\0\0newScale\0"
    "closeModeChanged\0enabled\0shapeSelection\0"
    "smoothingChanged\0undoLastAction\0"
    "mergeShapesAndConnector\0idx1\0idx2\0"
    "closeCurrentShape\0startCloseMode\0"
    "cancelCloseMode\0onPinchZoom\0center\0"
    "scaleFactor\0onTwoFingerPan\0delta\0"
    "handlePinchZoom\0factor\0setTwoFingersOn\0"
    "active\0setGridSpacing\0px\0gridSpacing"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CustomDrawArea[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   89,    2, 0x06 /* Public */,
       4,    1,   92,    2, 0x06 /* Public */,
       6,    1,   95,    2, 0x06 /* Public */,
       7,    1,   98,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       8,    0,  101,    2, 0x0a /* Public */,
       9,    2,  102,    2, 0x0a /* Public */,
      12,    0,  107,    2, 0x0a /* Public */,
      13,    0,  108,    2, 0x0a /* Public */,
      14,    0,  109,    2, 0x0a /* Public */,
      15,    2,  110,    2, 0x08 /* Private */,
      18,    1,  115,    2, 0x08 /* Private */,
      20,    2,  118,    2, 0x0a /* Public */,
      22,    1,  123,    2, 0x0a /* Public */,
      24,    1,  126,    2, 0x0a /* Public */,
      26,    0,  129,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, QMetaType::Bool,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   10,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QReal,   16,   17,
    QMetaType::Void, QMetaType::QPointF,   19,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QReal,   16,   21,
    QMetaType::Void, QMetaType::Bool,   23,
    QMetaType::Void, QMetaType::Int,   25,
    QMetaType::Int,

       0        // eod
};

void CustomDrawArea::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CustomDrawArea *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->zoomChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 1: _t->closeModeChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->shapeSelection((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->smoothingChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->undoLastAction(); break;
        case 5: _t->mergeShapesAndConnector((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->closeCurrentShape(); break;
        case 7: _t->startCloseMode(); break;
        case 8: _t->cancelCloseMode(); break;
        case 9: _t->onPinchZoom((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2]))); break;
        case 10: _t->onTwoFingerPan((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 11: _t->handlePinchZoom((*reinterpret_cast< QPointF(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2]))); break;
        case 12: _t->setTwoFingersOn((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->setGridSpacing((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: { int _r = _t->gridSpacing();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CustomDrawArea::*)(double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomDrawArea::zoomChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CustomDrawArea::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomDrawArea::closeModeChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CustomDrawArea::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomDrawArea::shapeSelection)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CustomDrawArea::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CustomDrawArea::smoothingChanged)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CustomDrawArea::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CustomDrawArea.data,
    qt_meta_data_CustomDrawArea,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CustomDrawArea::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CustomDrawArea::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CustomDrawArea.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CustomDrawArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void CustomDrawArea::zoomChanged(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CustomDrawArea::closeModeChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CustomDrawArea::shapeSelection(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CustomDrawArea::smoothingChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
