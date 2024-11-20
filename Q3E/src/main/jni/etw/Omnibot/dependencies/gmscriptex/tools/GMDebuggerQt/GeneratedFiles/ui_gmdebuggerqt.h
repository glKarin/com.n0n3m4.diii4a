/********************************************************************************
** Form generated from reading UI file 'gmdebuggerqt.ui'
**
** Created: Sun Feb 21 04:09:08 2010
**      by: Qt User Interface Compiler version 4.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GMDEBUGGERQT_H
#define UI_GMDEBUGGERQT_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDockWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QStatusBar>
#include <QtGui/QTableWidget>
#include <QtGui/QTextEdit>
#include <QtGui/QToolBar>
#include <QtGui/QTreeWidget>
#include <QtGui/QWidget>
#include "codeeditor.h"

QT_BEGIN_NAMESPACE

class Ui_GMDebuggerQtClass
{
public:
    QAction *actionConnect;
    QAction *actionExit;
    QAction *actionStep_In;
    QAction *actionStep_Out;
    QAction *actionStep_Over;
    QAction *actionRun_Thread;
    QAction *actionStop_Thread;
    QAction *actionStop_All_Threads;
    QAction *actionRun_All_Threads;
    QAction *actionSet_Break_Point;
    QAction *actionDisconnect;
    QAction *actionOpen_Script;
    QWidget *centralWidget;
    QHBoxLayout *horizontalLayout_4;
    CodeEditor *scriptEdit;
    QMenuBar *menuBar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
    QDockWidget *dockWidget;
    QWidget *dockWidgetContents_3;
    QHBoxLayout *horizontalLayout_3;
    QTableWidget *threadTable;
    QDockWidget *dockWidget_2;
    QWidget *dockWidgetContents_4;
    QHBoxLayout *horizontalLayout_2;
    QTableWidget *callStack;
    QDockWidget *dockWidget_3;
    QWidget *dockWidgetContents_5;
    QHBoxLayout *horizontalLayout;
    QTableWidget *context;
    QDockWidget *dockWidget_4;
    QWidget *dockWidgetContents_6;
    QHBoxLayout *horizontalLayout_6;
    QTreeWidget *globalsTable;
    QDockWidget *dockWidget_5;
    QWidget *dockWidgetContents;
    QHBoxLayout *horizontalLayout_5;
    QTextEdit *outputWindow;

    void setupUi(QMainWindow *GMDebuggerQtClass)
    {
        if (GMDebuggerQtClass->objectName().isEmpty())
            GMDebuggerQtClass->setObjectName(QString::fromUtf8("GMDebuggerQtClass"));
        GMDebuggerQtClass->resize(800, 762);
        actionConnect = new QAction(GMDebuggerQtClass);
        actionConnect->setObjectName(QString::fromUtf8("actionConnect"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/connect.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionConnect->setIcon(icon);
        actionExit = new QAction(GMDebuggerQtClass);
        actionExit->setObjectName(QString::fromUtf8("actionExit"));
        actionStep_In = new QAction(GMDebuggerQtClass);
        actionStep_In->setObjectName(QString::fromUtf8("actionStep_In"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/step_in.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStep_In->setIcon(icon1);
        actionStep_Out = new QAction(GMDebuggerQtClass);
        actionStep_Out->setObjectName(QString::fromUtf8("actionStep_Out"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/step_out.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStep_Out->setIcon(icon2);
        actionStep_Over = new QAction(GMDebuggerQtClass);
        actionStep_Over->setObjectName(QString::fromUtf8("actionStep_Over"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/step_over.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStep_Over->setIcon(icon3);
        actionRun_Thread = new QAction(GMDebuggerQtClass);
        actionRun_Thread->setObjectName(QString::fromUtf8("actionRun_Thread"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/thread_run.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionRun_Thread->setIcon(icon4);
        actionStop_Thread = new QAction(GMDebuggerQtClass);
        actionStop_Thread->setObjectName(QString::fromUtf8("actionStop_Thread"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/thread_stop.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStop_Thread->setIcon(icon5);
        actionStop_All_Threads = new QAction(GMDebuggerQtClass);
        actionStop_All_Threads->setObjectName(QString::fromUtf8("actionStop_All_Threads"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/thread_stopall.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStop_All_Threads->setIcon(icon6);
        actionRun_All_Threads = new QAction(GMDebuggerQtClass);
        actionRun_All_Threads->setObjectName(QString::fromUtf8("actionRun_All_Threads"));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/thread_runall.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionRun_All_Threads->setIcon(icon7);
        actionSet_Break_Point = new QAction(GMDebuggerQtClass);
        actionSet_Break_Point->setObjectName(QString::fromUtf8("actionSet_Break_Point"));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/breakpoint.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSet_Break_Point->setIcon(icon8);
        actionDisconnect = new QAction(GMDebuggerQtClass);
        actionDisconnect->setObjectName(QString::fromUtf8("actionDisconnect"));
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/disconnect.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionDisconnect->setIcon(icon9);
        actionOpen_Script = new QAction(GMDebuggerQtClass);
        actionOpen_Script->setObjectName(QString::fromUtf8("actionOpen_Script"));
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/GMDebuggerQt/Resources/script_open.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpen_Script->setIcon(icon10);
        centralWidget = new QWidget(GMDebuggerQtClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        centralWidget->setEnabled(true);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralWidget->sizePolicy().hasHeightForWidth());
        centralWidget->setSizePolicy(sizePolicy);
        horizontalLayout_4 = new QHBoxLayout(centralWidget);
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        scriptEdit = new CodeEditor(centralWidget);
        scriptEdit->setObjectName(QString::fromUtf8("scriptEdit"));
        sizePolicy.setHeightForWidth(scriptEdit->sizePolicy().hasHeightForWidth());
        scriptEdit->setSizePolicy(sizePolicy);

        horizontalLayout_4->addWidget(scriptEdit);

        GMDebuggerQtClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(GMDebuggerQtClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setEnabled(true);
        menuBar->setGeometry(QRect(0, 0, 800, 18));
        menuBar->setNativeMenuBar(true);
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuEdit = new QMenu(menuBar);
        menuEdit->setObjectName(QString::fromUtf8("menuEdit"));
        GMDebuggerQtClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(GMDebuggerQtClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        mainToolBar->setEnabled(true);
        mainToolBar->setMovable(false);
        mainToolBar->setIconSize(QSize(16, 16));
        mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mainToolBar->setFloatable(false);
        GMDebuggerQtClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(GMDebuggerQtClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        statusBar->setEnabled(true);
        statusBar->setSizeGripEnabled(true);
        GMDebuggerQtClass->setStatusBar(statusBar);
        dockWidget = new QDockWidget(GMDebuggerQtClass);
        dockWidget->setObjectName(QString::fromUtf8("dockWidget"));
        dockWidget->setFloating(false);
        dockWidget->setFeatures(QDockWidget::AllDockWidgetFeatures);
        dockWidgetContents_3 = new QWidget();
        dockWidgetContents_3->setObjectName(QString::fromUtf8("dockWidgetContents_3"));
        horizontalLayout_3 = new QHBoxLayout(dockWidgetContents_3);
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        threadTable = new QTableWidget(dockWidgetContents_3);
        if (threadTable->columnCount() < 4)
            threadTable->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        threadTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        threadTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        threadTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        threadTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        threadTable->setObjectName(QString::fromUtf8("threadTable"));
        threadTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        threadTable->setProperty("showDropIndicator", QVariant(false));
        threadTable->setDragDropMode(QAbstractItemView::NoDragDrop);
        threadTable->setSelectionMode(QAbstractItemView::SingleSelection);
        threadTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        threadTable->setSortingEnabled(true);
        threadTable->setWordWrap(false);
        threadTable->setColumnCount(4);
        threadTable->horizontalHeader()->setVisible(true);
        threadTable->horizontalHeader()->setCascadingSectionResizes(true);
        threadTable->horizontalHeader()->setDefaultSectionSize(75);
        threadTable->horizontalHeader()->setMinimumSectionSize(50);
        threadTable->horizontalHeader()->setStretchLastSection(true);
        threadTable->verticalHeader()->setDefaultSectionSize(20);
        threadTable->verticalHeader()->setProperty("showSortIndicator", QVariant(true));

        horizontalLayout_3->addWidget(threadTable);

        dockWidget->setWidget(dockWidgetContents_3);
        GMDebuggerQtClass->addDockWidget(static_cast<Qt::DockWidgetArea>(8), dockWidget);
        dockWidget_2 = new QDockWidget(GMDebuggerQtClass);
        dockWidget_2->setObjectName(QString::fromUtf8("dockWidget_2"));
        dockWidget_2->setFeatures(QDockWidget::AllDockWidgetFeatures);
        dockWidgetContents_4 = new QWidget();
        dockWidgetContents_4->setObjectName(QString::fromUtf8("dockWidgetContents_4"));
        horizontalLayout_2 = new QHBoxLayout(dockWidgetContents_4);
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        callStack = new QTableWidget(dockWidgetContents_4);
        if (callStack->columnCount() < 1)
            callStack->setColumnCount(1);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        callStack->setHorizontalHeaderItem(0, __qtablewidgetitem4);
        callStack->setObjectName(QString::fromUtf8("callStack"));
        callStack->setEditTriggers(QAbstractItemView::NoEditTriggers);
        callStack->setDragDropMode(QAbstractItemView::DropOnly);
        callStack->setSelectionMode(QAbstractItemView::SingleSelection);
        callStack->setSelectionBehavior(QAbstractItemView::SelectRows);
        callStack->setColumnCount(1);
        callStack->horizontalHeader()->setDefaultSectionSize(200);
        callStack->horizontalHeader()->setMinimumSectionSize(200);
        callStack->horizontalHeader()->setProperty("showSortIndicator", QVariant(true));
        callStack->horizontalHeader()->setStretchLastSection(true);
        callStack->verticalHeader()->setVisible(false);
        callStack->verticalHeader()->setDefaultSectionSize(20);

        horizontalLayout_2->addWidget(callStack);

        dockWidget_2->setWidget(dockWidgetContents_4);
        GMDebuggerQtClass->addDockWidget(static_cast<Qt::DockWidgetArea>(8), dockWidget_2);
        dockWidget_3 = new QDockWidget(GMDebuggerQtClass);
        dockWidget_3->setObjectName(QString::fromUtf8("dockWidget_3"));
        dockWidgetContents_5 = new QWidget();
        dockWidgetContents_5->setObjectName(QString::fromUtf8("dockWidgetContents_5"));
        horizontalLayout = new QHBoxLayout(dockWidgetContents_5);
        horizontalLayout->setSpacing(6);
        horizontalLayout->setContentsMargins(11, 11, 11, 11);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        context = new QTableWidget(dockWidgetContents_5);
        if (context->columnCount() < 3)
            context->setColumnCount(3);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        context->setHorizontalHeaderItem(0, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        context->setHorizontalHeaderItem(1, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        context->setHorizontalHeaderItem(2, __qtablewidgetitem7);
        context->setObjectName(QString::fromUtf8("context"));
        context->setEditTriggers(QAbstractItemView::NoEditTriggers);
        context->setDragDropMode(QAbstractItemView::DropOnly);
        context->setSelectionMode(QAbstractItemView::SingleSelection);
        context->setSelectionBehavior(QAbstractItemView::SelectRows);
        context->setColumnCount(3);
        context->horizontalHeader()->setDefaultSectionSize(50);
        context->horizontalHeader()->setMinimumSectionSize(50);
        context->horizontalHeader()->setProperty("showSortIndicator", QVariant(true));
        context->horizontalHeader()->setStretchLastSection(true);
        context->verticalHeader()->setVisible(false);
        context->verticalHeader()->setDefaultSectionSize(20);

        horizontalLayout->addWidget(context);

        dockWidget_3->setWidget(dockWidgetContents_5);
        GMDebuggerQtClass->addDockWidget(static_cast<Qt::DockWidgetArea>(8), dockWidget_3);
        dockWidget_4 = new QDockWidget(GMDebuggerQtClass);
        dockWidget_4->setObjectName(QString::fromUtf8("dockWidget_4"));
        dockWidget_4->setLayoutDirection(Qt::LeftToRight);
        dockWidgetContents_6 = new QWidget();
        dockWidgetContents_6->setObjectName(QString::fromUtf8("dockWidgetContents_6"));
        horizontalLayout_6 = new QHBoxLayout(dockWidgetContents_6);
        horizontalLayout_6->setSpacing(6);
        horizontalLayout_6->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        globalsTable = new QTreeWidget(dockWidgetContents_6);
        globalsTable->setObjectName(QString::fromUtf8("globalsTable"));
        globalsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        globalsTable->header()->setDefaultSectionSize(100);
        globalsTable->header()->setMinimumSectionSize(75);

        horizontalLayout_6->addWidget(globalsTable);

        dockWidget_4->setWidget(dockWidgetContents_6);
        GMDebuggerQtClass->addDockWidget(static_cast<Qt::DockWidgetArea>(2), dockWidget_4);
        dockWidget_5 = new QDockWidget(GMDebuggerQtClass);
        dockWidget_5->setObjectName(QString::fromUtf8("dockWidget_5"));
        dockWidget_5->setFloating(true);
        dockWidget_5->setFeatures(QDockWidget::AllDockWidgetFeatures);
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
        horizontalLayout_5 = new QHBoxLayout(dockWidgetContents);
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        outputWindow = new QTextEdit(dockWidgetContents);
        outputWindow->setObjectName(QString::fromUtf8("outputWindow"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(outputWindow->sizePolicy().hasHeightForWidth());
        outputWindow->setSizePolicy(sizePolicy1);
        outputWindow->setMinimumSize(QSize(0, 0));
        outputWindow->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        horizontalLayout_5->addWidget(outputWindow);

        dockWidget_5->setWidget(dockWidgetContents);
        GMDebuggerQtClass->addDockWidget(static_cast<Qt::DockWidgetArea>(8), dockWidget_5);

        menuBar->addAction(menuFile->menuAction());
        menuBar->addAction(menuEdit->menuAction());
        menuFile->addAction(actionConnect);
        menuFile->addAction(actionExit);
        mainToolBar->addAction(actionOpen_Script);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actionConnect);
        mainToolBar->addAction(actionDisconnect);
        mainToolBar->addSeparator();
        mainToolBar->addAction(actionStep_In);
        mainToolBar->addAction(actionStep_Out);
        mainToolBar->addAction(actionStep_Over);
        mainToolBar->addAction(actionRun_Thread);
        mainToolBar->addAction(actionStop_Thread);
        mainToolBar->addAction(actionStop_All_Threads);
        mainToolBar->addAction(actionRun_All_Threads);
        mainToolBar->addAction(actionSet_Break_Point);

        retranslateUi(GMDebuggerQtClass);

        QMetaObject::connectSlotsByName(GMDebuggerQtClass);
    } // setupUi

    void retranslateUi(QMainWindow *GMDebuggerQtClass)
    {
        GMDebuggerQtClass->setWindowTitle(QApplication::translate("GMDebuggerQtClass", "GMDebuggerQt", 0, QApplication::UnicodeUTF8));
        actionConnect->setText(QApplication::translate("GMDebuggerQtClass", "Connect", 0, QApplication::UnicodeUTF8));
        actionConnect->setIconText(QApplication::translate("GMDebuggerQtClass", "&Connect", 0, QApplication::UnicodeUTF8));
        actionExit->setText(QApplication::translate("GMDebuggerQtClass", "Exit", 0, QApplication::UnicodeUTF8));
        actionStep_In->setText(QApplication::translate("GMDebuggerQtClass", "Step In", 0, QApplication::UnicodeUTF8));
        actionStep_In->setShortcut(QApplication::translate("GMDebuggerQtClass", "F11", 0, QApplication::UnicodeUTF8));
        actionStep_Out->setText(QApplication::translate("GMDebuggerQtClass", "Step Out", 0, QApplication::UnicodeUTF8));
        actionStep_Out->setShortcut(QApplication::translate("GMDebuggerQtClass", "Shift+F11", 0, QApplication::UnicodeUTF8));
        actionStep_Over->setText(QApplication::translate("GMDebuggerQtClass", "Step Over", 0, QApplication::UnicodeUTF8));
        actionStep_Over->setShortcut(QApplication::translate("GMDebuggerQtClass", "F10", 0, QApplication::UnicodeUTF8));
        actionRun_Thread->setText(QApplication::translate("GMDebuggerQtClass", "Run Thread", 0, QApplication::UnicodeUTF8));
        actionRun_Thread->setShortcut(QApplication::translate("GMDebuggerQtClass", "F5", 0, QApplication::UnicodeUTF8));
        actionStop_Thread->setText(QApplication::translate("GMDebuggerQtClass", "Stop Thread", 0, QApplication::UnicodeUTF8));
        actionStop_All_Threads->setText(QApplication::translate("GMDebuggerQtClass", "Stop All Threads", 0, QApplication::UnicodeUTF8));
        actionStop_All_Threads->setShortcut(QApplication::translate("GMDebuggerQtClass", "Pause", 0, QApplication::UnicodeUTF8));
        actionRun_All_Threads->setText(QApplication::translate("GMDebuggerQtClass", "Run All Threads", 0, QApplication::UnicodeUTF8));
        actionSet_Break_Point->setText(QApplication::translate("GMDebuggerQtClass", "Set Break Point", 0, QApplication::UnicodeUTF8));
        actionSet_Break_Point->setShortcut(QApplication::translate("GMDebuggerQtClass", "F9", 0, QApplication::UnicodeUTF8));
        actionDisconnect->setText(QApplication::translate("GMDebuggerQtClass", "Disconnect", 0, QApplication::UnicodeUTF8));
        actionOpen_Script->setText(QApplication::translate("GMDebuggerQtClass", "Open Script", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("GMDebuggerQtClass", "File", 0, QApplication::UnicodeUTF8));
        menuEdit->setTitle(QApplication::translate("GMDebuggerQtClass", "Edit", 0, QApplication::UnicodeUTF8));
        dockWidget->setWindowTitle(QApplication::translate("GMDebuggerQtClass", "Threads", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem = threadTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QApplication::translate("GMDebuggerQtClass", "Thread", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem1 = threadTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QApplication::translate("GMDebuggerQtClass", "Status", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem2 = threadTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QApplication::translate("GMDebuggerQtClass", "Function", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem3 = threadTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QApplication::translate("GMDebuggerQtClass", "Script", 0, QApplication::UnicodeUTF8));
        dockWidget_2->setWindowTitle(QApplication::translate("GMDebuggerQtClass", "Callstack", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem4 = callStack->horizontalHeaderItem(0);
        ___qtablewidgetitem4->setText(QApplication::translate("GMDebuggerQtClass", "Function", 0, QApplication::UnicodeUTF8));
        dockWidget_3->setWindowTitle(QApplication::translate("GMDebuggerQtClass", "Locals", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem5 = context->horizontalHeaderItem(0);
        ___qtablewidgetitem5->setText(QApplication::translate("GMDebuggerQtClass", "Name", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem6 = context->horizontalHeaderItem(1);
        ___qtablewidgetitem6->setText(QApplication::translate("GMDebuggerQtClass", "Type", 0, QApplication::UnicodeUTF8));
        QTableWidgetItem *___qtablewidgetitem7 = context->horizontalHeaderItem(2);
        ___qtablewidgetitem7->setText(QApplication::translate("GMDebuggerQtClass", "Value", 0, QApplication::UnicodeUTF8));
        dockWidget_4->setWindowTitle(QApplication::translate("GMDebuggerQtClass", "Globals", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem = globalsTable->headerItem();
        ___qtreewidgetitem->setText(2, QApplication::translate("GMDebuggerQtClass", "Value", 0, QApplication::UnicodeUTF8));
        ___qtreewidgetitem->setText(1, QApplication::translate("GMDebuggerQtClass", "Type", 0, QApplication::UnicodeUTF8));
        ___qtreewidgetitem->setText(0, QApplication::translate("GMDebuggerQtClass", "Name", 0, QApplication::UnicodeUTF8));
        dockWidget_5->setWindowTitle(QApplication::translate("GMDebuggerQtClass", "Output", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class GMDebuggerQtClass: public Ui_GMDebuggerQtClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GMDEBUGGERQT_H
