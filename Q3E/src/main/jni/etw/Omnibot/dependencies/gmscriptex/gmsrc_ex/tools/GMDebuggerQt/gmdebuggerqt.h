#ifndef GMDEBUGGERQT_H
#define GMDEBUGGERQT_H

#include <QtGui/QMainWindow>
#include <QTcpSocket>

#include "codeeditor.h"
#include "ui_gmdebuggerqt.h"

#include "gmdebuggersettings.h"

#include "../../src/gm/gmDebugger.h"

class GMDebuggerQt : public QMainWindow, public gmDebuggerSession
{
	Q_OBJECT

public:
	GMDebuggerQt(QWidget *parent = 0, Qt::WFlags flags = 0);
	~GMDebuggerQt();

	void closeEvent(QCloseEvent *event);
	public slots:
		void OnActionScriptOpen(const QString &path = QString());
		void OnActionSettings();
		void OnActionConnect();
		void OnActionDisConnect();
		void OnActionRunAll();
		void OnActionBreakAll();
		void OnActionKillAll();
		void OnActionStepIn();
		void OnActionStepOut();
		void OnActionStepOver();
		void OnActionRunThread();
		void OnActionStopThread();
private:
	Ui::GMDebuggerQtClass ui;

	QTcpSocket *tcpSocket;
	QByteArray	socketData;
	QByteArray	pumpData;

	QTimer *removeThreadTimer;

	void writeSettings();
	void readSettings();
protected:
	virtual void DebuggerSendMessage(const void * a_command, int a_len);
	virtual const void * DebuggerPumpMessage(int &a_len);

	void gmDebuggerBreak(int a_threadId, int a_sourceId, int a_lineNumber);
	void gmDebuggerRun(int a_threadId, int a_lineNum, const char *a_func, const char *a_file);
	void gmDebuggerStop(int a_threadId);
	void gmDebuggerSource(int a_sourceId, const char * a_sourceName, const char * a_source);
	void gmDebuggerException(int a_threadId);

	void gmDebuggerBeginContext(int a_threadId, int a_callFrame);
	void gmDebuggerContextCallFrame(int a_callFrame, const char * a_functionName, int a_sourceId, int a_lineNumber, const char * a_thisSymbol, const char * a_thisValue, const char * a_thisType, int a_thisId);
	void gmDebuggerContextVariable(const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId);
	void gmDebuggerContextBreakpoint(int a_lineNum);
	void gmDebuggerEndContext();

	void gmDebuggerBeginThreadInfo();
	void gmDebuggerThreadInfo(int a_threadId, const char * a_threadState, int a_lineNum, const char * a_func, const char * a_file);
	void gmDebuggerEndThreadInfo();

	void gmDebuggerError(const char * a_error);
	void gmDebuggerMessage(const char * a_message);
	void gmDebuggerBreakPointSet(int a_sourceId, int a_lineNum, int a_enabled);
	void gmDebuggerBreakClear();
	void gmDebuggerQuit();

	void gmDebuggerBeginGlobals(int a_VarId);
	void gmDebuggerGlobal(const char * a_varSymbol, const char * a_varValue, const char * a_varType, int a_varId);
	void gmDebuggerEndGlobals();

	void gmDebuggerReturnValue(const char * a_retVal, const char * a_retType, int a_retVarId);

	enum GlobalTreeColumns {
		TreeColumn_Name,
		TreeColumn_Type,
		TreeColumn_Value,

		TreeColumn_Num
	};
	typedef QMap<int,QTreeWidgetItem*> VarItemMap;
	VarItemMap varIdTreeMap;
	QTreeWidgetItem * parentGlobals;
	QTreeWidgetItem * parentCurrent;

	enum ThreadColumns {
		Thread_Id,
		Thread_Status,
		Thread_Function,
		Thread_Script,

		ThreadColumn_Num
	};

	enum CallStackColumns {
		Callstack_FuncName,
	};

	enum ContextColumns {
		Context_Name,
		Context_Type,
		Context_Value,

		ContextColumn_Num,
	};

	int				globalColumnWidths[TreeColumn_Num];
	int				threadColumnWidths[ThreadColumn_Num];
	int				contextColumnWidths[ContextColumn_Num];

	struct SourceInfo {
		QString			sourceFile;
		QString			sourceData;
	};
	QMap<int,SourceInfo>	sourceMap;
	int					ackResponseId;

	int					currentThreadId;
	int					currentSourceId;
	int					currentCallFrame;
	int					currentLineOnSrcRecieved;
	private slots:

		void SocketConnected();
		void SocketDisconnected();
		void SocketReadyToRead();
		void SocketDisplayError(QAbstractSocket::SocketError socketError);

		int AddUniqueThread( int a_threadId, const char * a_status, int a_line, const char * a_func, const char * a_file );
		void RemoveThread( int a_threadId );
		void RemoveExpiredThreads();
		void ThreadSelectionChanged();

		void OnBreakPointChanged( int lineNum, bool enabled );
};

#endif // GMDEBUGGERQT_H
