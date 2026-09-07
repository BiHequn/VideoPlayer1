/****************************************************************************
** Meta object code from reading C++ file 'videodialog.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.11)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../videodialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'videodialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.11. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_VideoDialog_t {
    QByteArrayData data[27];
    char stringdata0[391];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_VideoDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_VideoDialog_t qt_meta_stringdata_VideoDialog = {
    {
QT_MOC_LITERAL(0, 0, 11), // "VideoDialog"
QT_MOC_LITERAL(1, 12, 16), // "SIG_backToOnline"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 19), // "on_pb_start_clicked"
QT_MOC_LITERAL(4, 50, 13), // "slot_setImage"
QT_MOC_LITERAL(5, 64, 3), // "img"
QT_MOC_LITERAL(6, 68, 20), // "on_pb_resume_clicked"
QT_MOC_LITERAL(7, 89, 19), // "on_pb_pause_clicked"
QT_MOC_LITERAL(8, 109, 18), // "on_pb_stop_clicked"
QT_MOC_LITERAL(9, 128, 23), // "slot_PlayerStateChanged"
QT_MOC_LITERAL(10, 152, 5), // "state"
QT_MOC_LITERAL(11, 158, 17), // "slot_getTotalTime"
QT_MOC_LITERAL(12, 176, 4), // "uSec"
QT_MOC_LITERAL(13, 181, 15), // "slot_OpenFailed"
QT_MOC_LITERAL(14, 197, 6), // "reason"
QT_MOC_LITERAL(15, 204, 17), // "slot_TimerTimeOut"
QT_MOC_LITERAL(16, 222, 11), // "eventFilter"
QT_MOC_LITERAL(17, 234, 3), // "obj"
QT_MOC_LITERAL(18, 238, 7), // "QEvent*"
QT_MOC_LITERAL(19, 246, 5), // "event"
QT_MOC_LITERAL(20, 252, 20), // "on_pb_online_clicked"
QT_MOC_LITERAL(21, 273, 18), // "on_pb_back_clicked"
QT_MOC_LITERAL(22, 292, 31), // "on_cb_speed_currentIndexChanged"
QT_MOC_LITERAL(23, 324, 5), // "index"
QT_MOC_LITERAL(24, 330, 29), // "on_slider_volume_valueChanged"
QT_MOC_LITERAL(25, 360, 5), // "value"
QT_MOC_LITERAL(26, 366, 24) // "on_pb_fullscreen_clicked"

    },
    "VideoDialog\0SIG_backToOnline\0\0"
    "on_pb_start_clicked\0slot_setImage\0img\0"
    "on_pb_resume_clicked\0on_pb_pause_clicked\0"
    "on_pb_stop_clicked\0slot_PlayerStateChanged\0"
    "state\0slot_getTotalTime\0uSec\0"
    "slot_OpenFailed\0reason\0slot_TimerTimeOut\0"
    "eventFilter\0obj\0QEvent*\0event\0"
    "on_pb_online_clicked\0on_pb_back_clicked\0"
    "on_cb_speed_currentIndexChanged\0index\0"
    "on_slider_volume_valueChanged\0value\0"
    "on_pb_fullscreen_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_VideoDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       3,    0,   95,    2, 0x08 /* Private */,
       4,    1,   96,    2, 0x08 /* Private */,
       6,    0,   99,    2, 0x08 /* Private */,
       7,    0,  100,    2, 0x08 /* Private */,
       8,    0,  101,    2, 0x08 /* Private */,
       9,    1,  102,    2, 0x08 /* Private */,
      11,    1,  105,    2, 0x08 /* Private */,
      13,    1,  108,    2, 0x08 /* Private */,
      15,    0,  111,    2, 0x08 /* Private */,
      16,    2,  112,    2, 0x08 /* Private */,
      20,    0,  117,    2, 0x08 /* Private */,
      21,    0,  118,    2, 0x08 /* Private */,
      22,    1,  119,    2, 0x08 /* Private */,
      24,    1,  122,    2, 0x08 /* Private */,
      26,    0,  125,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QImage,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::LongLong,   12,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QObjectStar, 0x80000000 | 18,   17,   19,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   23,
    QMetaType::Void, QMetaType::Int,   25,
    QMetaType::Void,

       0        // eod
};

void VideoDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<VideoDialog *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->SIG_backToOnline(); break;
        case 1: _t->on_pb_start_clicked(); break;
        case 2: _t->slot_setImage((*reinterpret_cast< QImage(*)>(_a[1]))); break;
        case 3: _t->on_pb_resume_clicked(); break;
        case 4: _t->on_pb_pause_clicked(); break;
        case 5: _t->on_pb_stop_clicked(); break;
        case 6: _t->slot_PlayerStateChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->slot_getTotalTime((*reinterpret_cast< qint64(*)>(_a[1]))); break;
        case 8: _t->slot_OpenFailed((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 9: _t->slot_TimerTimeOut(); break;
        case 10: { bool _r = _t->eventFilter((*reinterpret_cast< QObject*(*)>(_a[1])),(*reinterpret_cast< QEvent*(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 11: _t->on_pb_online_clicked(); break;
        case 12: _t->on_pb_back_clicked(); break;
        case 13: _t->on_cb_speed_currentIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->on_slider_volume_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 15: _t->on_pb_fullscreen_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (VideoDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&VideoDialog::SIG_backToOnline)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject VideoDialog::staticMetaObject = { {
    &QDialog::staticMetaObject,
    qt_meta_stringdata_VideoDialog.data,
    qt_meta_data_VideoDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *VideoDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *VideoDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_VideoDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int VideoDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void VideoDialog::SIG_backToOnline()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
