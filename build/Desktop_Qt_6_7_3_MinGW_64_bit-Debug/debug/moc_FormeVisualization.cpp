/****************************************************************************
** Meta object code from reading C++ file 'FormeVisualization.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../FormeVisualization.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FormeVisualization.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSFormeVisualizationENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSFormeVisualizationENDCLASS = QtMocHelpers::stringData(
    "FormeVisualization",
    "optimizationStateChanged",
    "",
    "optimized",
    "shapesPlacedCount",
    "spacingChanged",
    "newSpacing",
    "blackPixelsProgress",
    "remaining",
    "total",
    "displayCustomShapes",
    "QList<QPolygonF>",
    "shapes",
    "moveSelectedShapes",
    "dx",
    "dy",
    "rotateSelectedShapes",
    "angleDelta",
    "getBlackPixels",
    "QList<QPoint>",
    "startDecoupeProgress",
    "maxSteps",
    "updateDecoupeProgress",
    "currentStep",
    "endDecoupeProgress",
    "bufferedPath",
    "QPainterPath",
    "path",
    "spacing"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSFormeVisualizationENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   86,    2, 0x06,    1 /* Public */,
       4,    1,   89,    2, 0x06,    3 /* Public */,
       5,    1,   92,    2, 0x06,    5 /* Public */,
       7,    2,   95,    2, 0x06,    7 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      10,    1,  100,    2, 0x0a,   10 /* Public */,
      13,    2,  103,    2, 0x0a,   12 /* Public */,
      16,    1,  108,    2, 0x0a,   15 /* Public */,
      18,    0,  111,    2, 0x0a,   17 /* Public */,
      20,    1,  112,    2, 0x0a,   18 /* Public */,
      22,    1,  115,    2, 0x0a,   20 /* Public */,
      24,    0,  118,    2, 0x0a,   22 /* Public */,
      25,    2,  119,    2, 0x08,   23 /* Private */,

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
    0x80000000 | 26, 0x80000000 | 26, QMetaType::Int,   27,   28,

       0        // eod
};

Q_CONSTINIT const QMetaObject FormeVisualization::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSFormeVisualizationENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSFormeVisualizationENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSFormeVisualizationENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<FormeVisualization, std::true_type>,
        // method 'optimizationStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'shapesPlacedCount'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'spacingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'blackPixelsProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'displayCustomShapes'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<QPolygonF> &, std::false_type>,
        // method 'moveSelectedShapes'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        // method 'rotateSelectedShapes'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qreal, std::false_type>,
        // method 'getBlackPixels'
        QtPrivate::TypeAndForceComplete<QList<QPoint>, std::false_type>,
        // method 'startDecoupeProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'updateDecoupeProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'endDecoupeProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'bufferedPath'
        QtPrivate::TypeAndForceComplete<QPainterPath, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPainterPath &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void FormeVisualization::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FormeVisualization *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->optimizationStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->shapesPlacedCount((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->spacingChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->blackPixelsProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 4: _t->displayCustomShapes((*reinterpret_cast< std::add_pointer_t<QList<QPolygonF>>>(_a[1]))); break;
        case 5: _t->moveSelectedShapes((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qreal>>(_a[2]))); break;
        case 6: _t->rotateSelectedShapes((*reinterpret_cast< std::add_pointer_t<qreal>>(_a[1]))); break;
        case 7: { QList<QPoint> _r = _t->getBlackPixels();
            if (_a[0]) *reinterpret_cast< QList<QPoint>*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->startDecoupeProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->updateDecoupeProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 10: _t->endDecoupeProgress(); break;
        case 11: { QPainterPath _r = _t->bufferedPath((*reinterpret_cast< std::add_pointer_t<QPainterPath>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QPainterPath*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QPolygonF> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FormeVisualization::*)(bool );
            if (_t _q_method = &FormeVisualization::optimizationStateChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FormeVisualization::*)(int );
            if (_t _q_method = &FormeVisualization::shapesPlacedCount; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (FormeVisualization::*)(int );
            if (_t _q_method = &FormeVisualization::spacingChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (FormeVisualization::*)(int , int );
            if (_t _q_method = &FormeVisualization::blackPixelsProgress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *FormeVisualization::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FormeVisualization::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSFormeVisualizationENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int FormeVisualization::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
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
