/****************************************************************************
** Meta object code from reading C++ file 'codeeditor.h'
**
** Created: Fri Mar 5 01:06:07 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.6.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../codeeditor.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'codeeditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.6.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_Highlighter[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_Highlighter[] = {
    "Highlighter\0"
};

const QMetaObject Highlighter::staticMetaObject = {
    { &QSyntaxHighlighter::staticMetaObject, qt_meta_stringdata_Highlighter,
      qt_meta_data_Highlighter, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Highlighter::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Highlighter::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Highlighter::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Highlighter))
        return static_cast<void*>(const_cast< Highlighter*>(this));
    return QSyntaxHighlighter::qt_metacast(_clname);
}

int Highlighter::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QSyntaxHighlighter::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_CodeEditor[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      28,   12,   11,   11, 0x05,

 // slots: signature, parameters, type, tag, flags
      64,   56,   11,   11, 0x0a,
     101,   87,   11,   11, 0x08,
     132,   11,   11,   11, 0x08,
     157,  155,   11,   11, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_CodeEditor[] = {
    "CodeEditor\0\0lineNum,enabled\0"
    "BreakPointChanged(int,bool)\0lineNum\0"
    "LineMarginClicked(int)\0newBlockCount\0"
    "updateLineNumberAreaWidth(int)\0"
    "highlightCurrentLine()\0,\0"
    "updateLineNumberArea(QRect,int)\0"
};

const QMetaObject CodeEditor::staticMetaObject = {
    { &QTextEdit::staticMetaObject, qt_meta_stringdata_CodeEditor,
      qt_meta_data_CodeEditor, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &CodeEditor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *CodeEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *CodeEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_CodeEditor))
        return static_cast<void*>(const_cast< CodeEditor*>(this));
    return QTextEdit::qt_metacast(_clname);
}

int CodeEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTextEdit::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: BreakPointChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 1: LineMarginClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: updateLineNumberAreaWidth((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: highlightCurrentLine(); break;
        case 4: updateLineNumberArea((*reinterpret_cast< const QRect(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void CodeEditor::BreakPointChanged(int _t1, bool _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
static const uint qt_meta_data_LineNumberArea[] = {

 // content:
       4,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   16,   15,   15, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_LineNumberArea[] = {
    "LineNumberArea\0\0lineNum\0lineClicked(int)\0"
};

const QMetaObject LineNumberArea::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_LineNumberArea,
      qt_meta_data_LineNumberArea, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &LineNumberArea::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *LineNumberArea::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *LineNumberArea::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_LineNumberArea))
        return static_cast<void*>(const_cast< LineNumberArea*>(this));
    return QWidget::qt_metacast(_clname);
}

int LineNumberArea::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: lineClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void LineNumberArea::lineClicked(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
