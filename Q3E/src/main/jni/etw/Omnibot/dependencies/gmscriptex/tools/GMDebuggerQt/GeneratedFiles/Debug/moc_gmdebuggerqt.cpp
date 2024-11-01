/****************************************************************************
** Meta object code from reading C++ file 'gmdebuggerqt.h'
**
** Created: Sun Feb 21 14:44:12 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.6.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../gmdebuggerqt.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gmdebuggerqt.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.6.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GMDebuggerQt[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      19,   14,   13,   13, 0x0a,
      47,   13,   13,   13, 0x2a,
      68,   13,   13,   13, 0x0a,
      86,   13,   13,   13, 0x0a,
     107,   13,   13,   13, 0x0a,
     124,   13,   13,   13, 0x0a,
     143,   13,   13,   13, 0x0a,
     161,   13,   13,   13, 0x0a,
     178,   13,   13,   13, 0x0a,
     196,   13,   13,   13, 0x0a,
     215,   13,   13,   13, 0x0a,
     235,   13,   13,   13, 0x0a,
     256,   13,   13,   13, 0x0a,
     280,   13,   13,   13, 0x08,
     298,   13,   13,   13, 0x08,
     319,   13,   13,   13, 0x08,
     351,  339,   13,   13, 0x08,
     438,  404,  400,   13, 0x08,
     506,  495,   13,   13, 0x08,
     524,   13,   13,   13, 0x08,
     547,   13,   13,   13, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_GMDebuggerQt[] = {
    "GMDebuggerQt\0\0path\0OnActionScriptOpen(QString)\0"
    "OnActionScriptOpen()\0OnActionConnect()\0"
    "OnActionDisConnect()\0OnActionRunAll()\0"
    "OnActionBreakAll()\0OnActionKillAll()\0"
    "OnActionStepIn()\0OnActionStepOut()\0"
    "OnActionStepOver()\0OnActionRunThread()\0"
    "OnActionStopThread()\0OnActionSetBreakpoint()\0"
    "SocketConnected()\0SocketDisconnected()\0"
    "SocketReadyToRead()\0socketError\0"
    "SocketDisplayError(QAbstractSocket::SocketError)\0"
    "int\0a_threadId,a_status,a_func,a_file\0"
    "AddUniqueThread(int,const char*,const char*,const char*)\0"
    "a_threadId\0RemoveThread(int)\0"
    "RemoveExpiredThreads()\0ThreadSelectionChanged()\0"
};

const QMetaObject GMDebuggerQt::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_GMDebuggerQt,
      qt_meta_data_GMDebuggerQt, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GMDebuggerQt::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GMDebuggerQt::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GMDebuggerQt::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GMDebuggerQt))
        return static_cast<void*>(const_cast< GMDebuggerQt*>(this));
    if (!strcmp(_clname, "gmDebuggerSession"))
        return static_cast< gmDebuggerSession*>(const_cast< GMDebuggerQt*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int GMDebuggerQt::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: OnActionScriptOpen((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: OnActionScriptOpen(); break;
        case 2: OnActionConnect(); break;
        case 3: OnActionDisConnect(); break;
        case 4: OnActionRunAll(); break;
        case 5: OnActionBreakAll(); break;
        case 6: OnActionKillAll(); break;
        case 7: OnActionStepIn(); break;
        case 8: OnActionStepOut(); break;
        case 9: OnActionStepOver(); break;
        case 10: OnActionRunThread(); break;
        case 11: OnActionStopThread(); break;
        case 12: OnActionSetBreakpoint(); break;
        case 13: SocketConnected(); break;
        case 14: SocketDisconnected(); break;
        case 15: SocketReadyToRead(); break;
        case 16: SocketDisplayError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 17: { int _r = AddUniqueThread((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2])),(*reinterpret_cast< const char*(*)>(_a[3])),(*reinterpret_cast< const char*(*)>(_a[4])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 18: RemoveThread((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 19: RemoveExpiredThreads(); break;
        case 20: ThreadSelectionChanged(); break;
        default: ;
        }
        _id -= 21;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
