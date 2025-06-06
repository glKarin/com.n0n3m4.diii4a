/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWDIALOG.H -- LightWave Standard Dialogs
 *
 * LightWave makes some of its more common requests from the user using
 * standard dialogs.  These dialogs (or "requesters") are used for getting
 * files and paths for saving and loading, and for getting color choices.
 * By default, the standard system dialogs are used, but these can be
 * overridden by plug-ins of the right class.
 */
#ifndef LWSDK_DIALOG_H
#define LWSDK_DIALOG_H


/*
 * File dialogs can be configured by servers of this class.
 */
#define LWFILEREQ_CLASS         "FileRequester"
#define LWFILEREQ_VERSION       3

typedef struct st_LWFileReqLocal {
        int              reqType;
        int              result;
        const char      *title;
        const char      *fileType;
        char            *path;
        char            *baseName;
        char            *fullName;
        int              bufLen;
        int            (*pickName) (void);
} LWFileReqLocal;

#define FREQ_LOAD        1
#define FREQ_SAVE        2
#define FREQ_DIRECTORY   3
#define FREQ_MULTILOAD   4


/*
 * Color dialogs can be configured by servers of this class.
 */
#define LWCOLORPICK_CLASS       "ColorPicker"
#define LWCOLORPICK_VERSION     4

typedef void    LWHotColorFunc (void *data, float r, float g, float b);

typedef struct st_LWColorPickLocal {
        int              result;
        const char      *title;
        float            red, green, blue;
        void            *data;
        LWHotColorFunc  *hotFunc;
} LWColorPickLocal;

#endif

