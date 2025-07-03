/****************************************************************************
** Meta object code from reading C++ file 'custom.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "custom.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'custom.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_custom_t {
    QByteArrayData data[11];
    char stringdata0[144];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_custom_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_custom_t qt_meta_stringdata_custom = {
    {
QT_MOC_LITERAL(0, 0, 6), // "custom"
QT_MOC_LITERAL(1, 7, 22), // "applyCustomShapeSignal"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 16), // "QList<QPolygonF>"
QT_MOC_LITERAL(4, 48, 6), // "shapes"
QT_MOC_LITERAL(5, 55, 18), // "resetDrawingSignal"
QT_MOC_LITERAL(6, 74, 14), // "goToMainWindow"
QT_MOC_LITERAL(7, 89, 13), // "ouvrirClavier"
QT_MOC_LITERAL(8, 103, 11), // "closeCustom"
QT_MOC_LITERAL(9, 115, 12), // "importerLogo"
QT_MOC_LITERAL(10, 128, 15) // "saveCustomShape"

    },
    "custom\0applyCustomShapeSignal\0\0"
    "QList<QPolygonF>\0shapes\0resetDrawingSignal\0"
    "goToMainWindow\0ouvrirClavier\0closeCustom\0"
    "importerLogo\0saveCustomShape"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_custom[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       5,    0,   52,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   53,    2, 0x08 /* Private */,
       7,    0,   54,    2, 0x08 /* Private */,
       8,    0,   55,    2, 0x08 /* Private */,
       9,    0,   56,    2, 0x08 /* Private */,
      10,    0,   57,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void custom::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<custom *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->applyCustomShapeSignal((*reinterpret_cast< QList<QPolygonF>(*)>(_a[1]))); break;
        case 1: _t->resetDrawingSignal(); break;
        case 2: _t->goToMainWindow(); break;
        case 3: _t->ouvrirClavier(); break;
        case 4: _t->closeCustom(); break;
        case 5: _t->importerLogo(); break;
        case 6: _t->saveCustomShape(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
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
            using _t = void (custom::*)(QList<QPolygonF> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&custom::applyCustomShapeSignal)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (custom::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&custom::resetDrawingSignal)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject custom::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_custom.data,
    qt_meta_data_custom,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *custom::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *custom::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_custom.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int custom::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void custom::applyCustomShapeSignal(QList<QPolygonF> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void custom::resetDrawingSignal()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
