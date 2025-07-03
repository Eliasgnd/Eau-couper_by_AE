/****************************************************************************
** Meta object code from reading C++ file 'touchgesturereader.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "touchgesturereader.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'touchgesturereader.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TouchGestureReader_t {
    QByteArrayData data[9];
    char stringdata0[114];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TouchGestureReader_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TouchGestureReader_t qt_meta_stringdata_TouchGestureReader = {
    {
QT_MOC_LITERAL(0, 0, 18), // "TouchGestureReader"
QT_MOC_LITERAL(1, 19, 17), // "pinchZoomDetected"
QT_MOC_LITERAL(2, 37, 0), // ""
QT_MOC_LITERAL(3, 38, 6), // "center"
QT_MOC_LITERAL(4, 45, 11), // "scaleFactor"
QT_MOC_LITERAL(5, 57, 20), // "twoFingerPanDetected"
QT_MOC_LITERAL(6, 78, 5), // "delta"
QT_MOC_LITERAL(7, 84, 22), // "twoFingersTouchChanged"
QT_MOC_LITERAL(8, 107, 6) // "active"

    },
    "TouchGestureReader\0pinchZoomDetected\0"
    "\0center\0scaleFactor\0twoFingerPanDetected\0"
    "delta\0twoFingersTouchChanged\0active"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TouchGestureReader[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   29,    2, 0x06 /* Public */,
       5,    1,   34,    2, 0x06 /* Public */,
       7,    1,   37,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QPointF, QMetaType::QReal,    3,    4,
    QMetaType::Void, QMetaType::QPointF,    6,
    QMetaType::Void, QMetaType::Bool,    8,

       0        // eod
};

void TouchGestureReader::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TouchGestureReader *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->pinchZoomDetected((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2]))); break;
        case 1: _t->twoFingerPanDetected((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 2: _t->twoFingersTouchChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TouchGestureReader::*)(const QPointF & , qreal );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TouchGestureReader::pinchZoomDetected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TouchGestureReader::*)(const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TouchGestureReader::twoFingerPanDetected)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TouchGestureReader::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TouchGestureReader::twoFingersTouchChanged)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject TouchGestureReader::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_TouchGestureReader.data,
    qt_meta_data_TouchGestureReader,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TouchGestureReader::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TouchGestureReader::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TouchGestureReader.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int TouchGestureReader::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void TouchGestureReader::pinchZoomDetected(const QPointF & _t1, qreal _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TouchGestureReader::twoFingerPanDetected(const QPointF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void TouchGestureReader::twoFingersTouchChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
