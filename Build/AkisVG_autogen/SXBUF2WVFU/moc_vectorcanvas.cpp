/****************************************************************************
** Meta object code from reading C++ file 'vectorcanvas.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/canvas/vectorcanvas.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'vectorcanvas.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
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
struct qt_meta_tag_ZN12VectorCanvasE_t {};
} // unnamed namespace

template <> constexpr inline auto VectorCanvas::qt_create_metaobjectdata<qt_meta_tag_ZN12VectorCanvasE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "VectorCanvas",
        "aboutToRefreshFrame",
        "",
        "referenceImageDropped",
        "path",
        "QPointF",
        "position",
        "audioDropped",
        "objectGroupCreated",
        "ObjectGroup*",
        "group",
        "contextMenuRequestedAt",
        "QPoint",
        "globalPos",
        "scenePos",
        "interpolationModeChanged",
        "active",
        "onFrameChanged",
        "frame",
        "setupLayerConnections",
        "enterInterpolationMode",
        "exitInterpolationMode",
        "showSelectionOverlays",
        "QList<VectorObject*>",
        "sourceObjects"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'aboutToRefreshFrame'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'referenceImageDropped'
        QtMocHelpers::SignalData<void(const QString &, const QPointF &)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 4 }, { 0x80000000 | 5, 6 },
        }}),
        // Signal 'audioDropped'
        QtMocHelpers::SignalData<void(const QString &)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 4 },
        }}),
        // Signal 'objectGroupCreated'
        QtMocHelpers::SignalData<void(ObjectGroup *)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Signal 'contextMenuRequestedAt'
        QtMocHelpers::SignalData<void(const QPoint &, const QPointF &)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 }, { 0x80000000 | 5, 14 },
        }}),
        // Signal 'interpolationModeChanged'
        QtMocHelpers::SignalData<void(bool)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 16 },
        }}),
        // Slot 'onFrameChanged'
        QtMocHelpers::SlotData<void(int)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 18 },
        }}),
        // Slot 'setupLayerConnections'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'enterInterpolationMode'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'exitInterpolationMode'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'showSelectionOverlays'
        QtMocHelpers::SlotData<void(const QList<VectorObject*> &)>(22, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 23, 24 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<VectorCanvas, qt_meta_tag_ZN12VectorCanvasE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject VectorCanvas::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsScene::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12VectorCanvasE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12VectorCanvasE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12VectorCanvasE_t>.metaTypes,
    nullptr
} };

void VectorCanvas::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<VectorCanvas *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->aboutToRefreshFrame(); break;
        case 1: _t->referenceImageDropped((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        case 2: _t->audioDropped((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->objectGroupCreated((*reinterpret_cast<std::add_pointer_t<ObjectGroup*>>(_a[1]))); break;
        case 4: _t->contextMenuRequestedAt((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        case 5: _t->interpolationModeChanged((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->onFrameChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->setupLayerConnections(); break;
        case 8: _t->enterInterpolationMode(); break;
        case 9: _t->exitInterpolationMode(); break;
        case 10: _t->showSelectionOverlays((*reinterpret_cast<std::add_pointer_t<QList<VectorObject*>>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (VectorCanvas::*)()>(_a, &VectorCanvas::aboutToRefreshFrame, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (VectorCanvas::*)(const QString & , const QPointF & )>(_a, &VectorCanvas::referenceImageDropped, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (VectorCanvas::*)(const QString & )>(_a, &VectorCanvas::audioDropped, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (VectorCanvas::*)(ObjectGroup * )>(_a, &VectorCanvas::objectGroupCreated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (VectorCanvas::*)(const QPoint & , const QPointF & )>(_a, &VectorCanvas::contextMenuRequestedAt, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (VectorCanvas::*)(bool )>(_a, &VectorCanvas::interpolationModeChanged, 5))
            return;
    }
}

const QMetaObject *VectorCanvas::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VectorCanvas::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12VectorCanvasE_t>.strings))
        return static_cast<void*>(this);
    return QGraphicsScene::qt_metacast(_clname);
}

int VectorCanvas::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsScene::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void VectorCanvas::aboutToRefreshFrame()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void VectorCanvas::referenceImageDropped(const QString & _t1, const QPointF & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void VectorCanvas::audioDropped(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void VectorCanvas::objectGroupCreated(ObjectGroup * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void VectorCanvas::contextMenuRequestedAt(const QPoint & _t1, const QPointF & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}

// SIGNAL 5
void VectorCanvas::interpolationModeChanged(bool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}
QT_WARNING_POP
