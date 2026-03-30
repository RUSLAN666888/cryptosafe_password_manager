/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.13)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/gui/MainWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.13. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[23];
    char stringdata0[290];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 19), // "onInactivityTimeout"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 6), // "onLock"
QT_MOC_LITERAL(4, 39, 13), // "onNewDatabase"
QT_MOC_LITERAL(5, 53, 14), // "onOpenDatabase"
QT_MOC_LITERAL(6, 68, 8), // "onBackup"
QT_MOC_LITERAL(7, 77, 6), // "onExit"
QT_MOC_LITERAL(8, 84, 10), // "onAddEntry"
QT_MOC_LITERAL(9, 95, 11), // "onEditEntry"
QT_MOC_LITERAL(10, 107, 13), // "onDeleteEntry"
QT_MOC_LITERAL(11, 121, 10), // "onViewLogs"
QT_MOC_LITERAL(12, 132, 10), // "onSettings"
QT_MOC_LITERAL(13, 143, 7), // "onAbout"
QT_MOC_LITERAL(14, 151, 16), // "onFirstRunWizard"
QT_MOC_LITERAL(15, 168, 16), // "onChangePassword"
QT_MOC_LITERAL(16, 185, 15), // "showContextMenu"
QT_MOC_LITERAL(17, 201, 3), // "pos"
QT_MOC_LITERAL(18, 205, 14), // "onCopyUsername"
QT_MOC_LITERAL(19, 220, 14), // "onCopyPassword"
QT_MOC_LITERAL(20, 235, 17), // "runEncryptionTest"
QT_MOC_LITERAL(21, 253, 11), // "runCrudTest"
QT_MOC_LITERAL(22, 265, 24) // "runPasswordGeneratorTest"

    },
    "MainWindow\0onInactivityTimeout\0\0onLock\0"
    "onNewDatabase\0onOpenDatabase\0onBackup\0"
    "onExit\0onAddEntry\0onEditEntry\0"
    "onDeleteEntry\0onViewLogs\0onSettings\0"
    "onAbout\0onFirstRunWizard\0onChangePassword\0"
    "showContextMenu\0pos\0onCopyUsername\0"
    "onCopyPassword\0runEncryptionTest\0"
    "runCrudTest\0runPasswordGeneratorTest"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      20,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  114,    2, 0x08 /* Private */,
       3,    0,  115,    2, 0x08 /* Private */,
       4,    0,  116,    2, 0x08 /* Private */,
       5,    0,  117,    2, 0x08 /* Private */,
       6,    0,  118,    2, 0x08 /* Private */,
       7,    0,  119,    2, 0x08 /* Private */,
       8,    0,  120,    2, 0x08 /* Private */,
       9,    0,  121,    2, 0x08 /* Private */,
      10,    0,  122,    2, 0x08 /* Private */,
      11,    0,  123,    2, 0x08 /* Private */,
      12,    0,  124,    2, 0x08 /* Private */,
      13,    0,  125,    2, 0x08 /* Private */,
      14,    0,  126,    2, 0x08 /* Private */,
      15,    0,  127,    2, 0x08 /* Private */,
      16,    1,  128,    2, 0x08 /* Private */,
      18,    0,  131,    2, 0x08 /* Private */,
      19,    0,  132,    2, 0x08 /* Private */,
      20,    0,  133,    2, 0x08 /* Private */,
      21,    0,  134,    2, 0x08 /* Private */,
      22,    0,  135,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPoint,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onInactivityTimeout(); break;
        case 1: _t->onLock(); break;
        case 2: _t->onNewDatabase(); break;
        case 3: _t->onOpenDatabase(); break;
        case 4: _t->onBackup(); break;
        case 5: _t->onExit(); break;
        case 6: _t->onAddEntry(); break;
        case 7: _t->onEditEntry(); break;
        case 8: _t->onDeleteEntry(); break;
        case 9: _t->onViewLogs(); break;
        case 10: _t->onSettings(); break;
        case 11: _t->onAbout(); break;
        case 12: _t->onFirstRunWizard(); break;
        case 13: _t->onChangePassword(); break;
        case 14: _t->showContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 15: _t->onCopyUsername(); break;
        case 16: _t->onCopyPassword(); break;
        case 17: _t->runEncryptionTest(); break;
        case 18: _t->runCrudTest(); break;
        case 19: _t->runPasswordGeneratorTest(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 20)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 20;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 20)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 20;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
