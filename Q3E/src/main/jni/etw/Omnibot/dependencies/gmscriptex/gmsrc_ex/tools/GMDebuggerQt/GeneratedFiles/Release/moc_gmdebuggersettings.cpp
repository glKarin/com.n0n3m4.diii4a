/****************************************************************************
** Meta object code from reading C++ file 'gmdebuggersettings.h'
**
** Created: Tue Feb 23 00:26:12 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.6.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../gmdebuggersettings.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gmdebuggersettings.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.6.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GMDebuggerSettings[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      20,   19,   19,   19, 0x08,
      35,   19,   19,   19, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_GMDebuggerSettings[] = {
    "GMDebuggerSettings\0\0StyleChanged()\0"
    "SelectFont()\0"
};

const QMetaObject GMDebuggerSettings::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_GMDebuggerSettings,
      qt_meta_data_GMDebuggerSettings, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GMDebuggerSettings::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GMDebuggerSettings::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GMDebuggerSettings::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GMDebuggerSettings))
        return static_cast<void*>(const_cast< GMDebuggerSettings*>(this));
    return QDialog::qt_metacast(_clname);
}

int GMDebuggerSettings::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: StyleChanged(); break;
        case 1: SelectFont(); break;
        default: ;
        }
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
