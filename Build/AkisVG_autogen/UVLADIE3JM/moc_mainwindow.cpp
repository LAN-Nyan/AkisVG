/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/mainwindow.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "newProject",
        "",
        "openProject",
        "saveProject",
        "saveProjectAs",
        "exportFrame",
        "about",
        "updateWindowTitle",
        "onSplineCommitted",
        "QList<QPointF>",
        "nodes",
        "onInstanceGroupRequested",
        "ObjectGroup*",
        "group",
        "importAudio",
        "importImage",
        "exportToMp4",
        "exportGifKeyframes",
        "exportGifAllFrames",
        "onLassoFill",
        "QPolygonF",
        "poly",
        "QColor",
        "color",
        "onLassoCut",
        "onLassoCopy",
        "onLassoPull",
        "QPointF",
        "dragStart",
        "provideWandSnapshot",
        "MagicWandTool*",
        "wand",
        "addToRecentFiles",
        "path",
        "updateRecentFilesMenu"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'newProject'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'openProject'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'saveProject'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'saveProjectAs'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'exportFrame'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'about'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateWindowTitle'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSplineCommitted'
        QtMocHelpers::SlotData<void(const QList<QPointF> &)>(9, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Slot 'onInstanceGroupRequested'
        QtMocHelpers::SlotData<void(ObjectGroup *)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'importAudio'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'importImage'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'exportToMp4'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'exportGifKeyframes'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'exportGifAllFrames'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onLassoFill'
        QtMocHelpers::SlotData<void(const QPolygonF &, const QColor &)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 }, { 0x80000000 | 23, 24 },
        }}),
        // Slot 'onLassoCut'
        QtMocHelpers::SlotData<void(const QPolygonF &)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Slot 'onLassoCopy'
        QtMocHelpers::SlotData<void(const QPolygonF &)>(26, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Slot 'onLassoPull'
        QtMocHelpers::SlotData<void(const QPolygonF &, QPointF)>(27, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 }, { 0x80000000 | 28, 29 },
        }}),
        // Slot 'provideWandSnapshot'
        QtMocHelpers::SlotData<void(MagicWandTool *)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 31, 32 },
        }}),
        // Slot 'addToRecentFiles'
        QtMocHelpers::SlotData<void(const QString &)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 34 },
        }}),
        // Slot 'updateRecentFilesMenu'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->newProject(); break;
        case 1: _t->openProject(); break;
        case 2: _t->saveProject(); break;
        case 3: _t->saveProjectAs(); break;
        case 4: _t->exportFrame(); break;
        case 5: _t->about(); break;
        case 6: _t->updateWindowTitle(); break;
        case 7: _t->onSplineCommitted((*reinterpret_cast<std::add_pointer_t<QList<QPointF>>>(_a[1]))); break;
        case 8: _t->onInstanceGroupRequested((*reinterpret_cast<std::add_pointer_t<ObjectGroup*>>(_a[1]))); break;
        case 9: _t->importAudio(); break;
        case 10: _t->importImage(); break;
        case 11: _t->exportToMp4(); break;
        case 12: _t->exportGifKeyframes(); break;
        case 13: _t->exportGifAllFrames(); break;
        case 14: _t->onLassoFill((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QColor>>(_a[2]))); break;
        case 15: _t->onLassoCut((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1]))); break;
        case 16: _t->onLassoCopy((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1]))); break;
        case 17: _t->onLassoPull((*reinterpret_cast<std::add_pointer_t<QPolygonF>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPointF>>(_a[2]))); break;
        case 18: _t->provideWandSnapshot((*reinterpret_cast<std::add_pointer_t<MagicWandTool*>>(_a[1]))); break;
        case 19: _t->addToRecentFiles((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->updateRecentFilesMenu(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 21)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 21;
    }
    return _id;
}
QT_WARNING_POP
