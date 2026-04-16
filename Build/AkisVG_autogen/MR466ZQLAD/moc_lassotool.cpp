/****************************************************************************
** Meta object code from reading C++ file 'lassotool.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/tools/lassotool.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lassotool.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9LassoToolE_t {};
} // unnamed namespace

template <> constexpr inline auto LassoTool::qt_create_metaobjectdata<qt_meta_tag_ZN9LassoToolE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "LassoTool",
        "selectionChanged",
        "",
        "actionFill",
        "QPolygonF",
        "polygon",
        "QColor",
        "color",
        "actionCut",
        "actionCopy",
        "actionPull",
        "QPointF",
        "dragStart"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'selectionChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'actionFill'
        QtMocHelpers::SignalData<void(const QPolygonF &, const QColor &)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 }, { 0x80000000 | 6, 7 },
        }}),
        // Signal 'actionCut'
        QtMocHelpers::SignalData<void(const QPolygonF &)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
        // Signal 'actionCopy'
        QtMocHelpers::SignalData<void(const QPolygonF &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 },
        }}),
        // Signal 'actionPull'
        QtMocHelpers::SignalData<void(const QPolygonF &, QPointF)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 4, 5 }, { 0x80000000 | 11, 12 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<LassoTool, qt_meta_tag_ZN9LassoToolE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject LassoTool::staticMetaObject = { {
    QMetaObject::SuperData::link<Tool::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9LassoToolE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9LassoToolE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9LassoToolE_t>.metaTypes,
    nullptr
} };

void LassoTool::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<LassoTool *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->selectionChanged(); break;
        case 1: _t->actionFill((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QColor>>(_a[2]))); break;
        case 2: _t->actionCut((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1]))); break;
        case 3: _t->actionCopy((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1]))); break;
        case 4: _t->actionPull((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (LassoTool::*)()>(_a, &LassoTool::selectionChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (LassoTool::*)(const QPolygonF & , const QColor & )>(_a, &LassoTool::actionFill, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (LassoTool::*)(const QPolygonF & )>(_a, &LassoTool::actionCut, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (LassoTool::*)(const QPolygonF & )>(_a, &LassoTool::actionCopy, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (LassoTool::*)(const QPolygonF & , QPointF )>(_a, &LassoTool::actionPull, 4))
            return;
    }
}

const QMetaObject *LassoTool::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LassoTool::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9LassoToolE_t>.strings))
        return static_cast<void*>(this);
    return Tool::qt_metacast(_clname);
}

int LassoTool::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Tool::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void LassoTool::selectionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void LassoTool::actionFill(const QPolygonF & _t1, const QColor & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void LassoTool::actionCut(const QPolygonF & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void LassoTool::actionCopy(const QPolygonF & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void LassoTool::actionPull(const QPolygonF & _t1, QPointF _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2);
}
QT_WARNING_POP
