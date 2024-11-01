/****************************************************************************
** Meta object code from reading C++ file 'gmdebuggerqt.h'
**
** Created: Fri Mar 5 01:06:06 2010
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
      22,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      19,   14,   13,   13, 0x0a,
      47,   13,   13,   13, 0x2a,
      68,   13,   13,   13, 0x0a,
      87,   13,   13,   13, 0x0a,
     105,   13,   13,   13, 0x0a,
     126,   13,   13,   13, 0x0a,
     143,   13,   13,   13, 0x0a,
     162,   13,   13,   13, 0x0a,
     180,   13,   13,   13, 0x0a,
     197,   13,   13,   13, 0x0a,
     215,   13,   13,   13, 0x0a,
     234,   13,   13,   13, 0x0a,
     254,   13,   13,   13, 0x0a,
     275,   13,   13,   13, 0x08,
     293,   13,   13,   13, 0x08,
     314,   13,   13,   13, 0x08,
     346,  334,   13,   13, 0x08,
     440,  399,  395,   13, 0x08,
     512,  501,   13,   13, 0x08,
     530,   13,   13,   13, 0x08,
     553,   13,   13,   13, 0x08,
     594,  578,   13,   13, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_GMDebuggerQt[] = {
    "GMDebuggerQt\0\0path\0OnActionScriptOpen(QString)\0"
    "OnActionScriptOpen()\0OnActionSettings()\0"
    "OnActionConnect()\0OnActionDisConnect()\0"
    "OnActionRunAll()\0OnActionBreakAll()\0"
    "OnActionKillAll()\0OnActionStepIn()\0"
    "OnActionStepOut()\0OnActionStepOver()\0"
    "OnActionRunThread()\0OnActionStopThread()\0"
    "SocketConnected()\0SocketDisconnected()\0"
    "SocketReadyToRead()\0socketError\0"
    "SocketDisplayError(QAbstractSocket::SocketError)\0"
    "int\0a_threadId,a_status,a_line,a_func,a_file\0"
    "AddUniqueThread(int,const char*,int,const char*,const char*)\0"
    "a_threadId\0RemoveThread(int)\0"
    "RemoveExpiredThreads()\0ThreadSelectionChanged()\0"
    "lineNum,enabled\0OnBreakPointChanged(int,bool)\0"
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
        case 2: OnActionSettings(); break;
        case 3: OnActionConnect(); break;
        case 4: OnActionDisConnect(); break;
        case 5: OnActionRunAll(); break;
        case 6: OnActionBreakAll(); break;
        case 7: OnActionKillAll(); break;
        case 8: OnActionStepIn(); break;
        case 9: OnActionStepOut(); break;
        case 10: OnActionStepOver(); break;
        case 11: OnActionRunThread(); break;
        case 12: OnActionStopThread(); break;
        case 13: SocketConnected(); break;
        case 14: SocketDisconnected(); break;
        case 15: SocketReadyToRead(); break;
        case 16: SocketDisplayError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 17: { int _r = AddUniqueThread((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const char*(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< const char*(*)>(_a[4])),(*reinterpret_cast< const char*(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = _r; }  break;
        case 18: RemoveThread((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 19: RemoveExpiredThreads(); break;
        case 20: ThreadSelectionChanged(); break;
        case 21: OnBreakPointChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        default: ;
        }
        _id -= 22;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
