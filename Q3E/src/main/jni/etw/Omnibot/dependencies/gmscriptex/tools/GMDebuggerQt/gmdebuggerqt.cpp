
#include <QtGui>
#include <QtNetwork>
#include "gmdebuggerqt.h"

GMDebuggerQt::GMDebuggerQt(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags)
{
	ackResponseId = 0;
	currentSourceId = 0;
	currentThreadId = 0;
	currentLineOnSrcRecieved = -1;
	
	ui.setupUi(this);

	ui.actionConnect->setEnabled(true);
	ui.actionDisconnect->setEnabled(false);

	// connect action slots
	connect(ui.actionConnect, SIGNAL(triggered(bool)), this, SLOT(OnActionConnect()));
	connect(ui.actionDisconnect, SIGNAL(triggered(bool)), this, SLOT(OnActionDisConnect()));
	connect(ui.actionOpen_Script, SIGNAL(triggered(bool)), this, SLOT(OnActionScriptOpen()));
	connect(ui.actionRun_All_Threads, SIGNAL(triggered(bool)), this, SLOT(OnActionRunAll()));
	connect(ui.actionStop_All_Threads, SIGNAL(triggered(bool)), this, SLOT(OnActionBreakAll()));
	// todo: OnActionKillAll
	connect(ui.actionStep_In, SIGNAL(triggered(bool)), this, SLOT(OnActionStepIn()));
	connect(ui.actionStep_Out, SIGNAL(triggered(bool)), this, SLOT(OnActionStepOut()));
	connect(ui.actionStep_Over, SIGNAL(triggered(bool)), this, SLOT(OnActionStepOver()));
	connect(ui.actionRun_Thread, SIGNAL(triggered(bool)), this, SLOT(OnActionRunThread()));
	connect(ui.actionStop_Thread, SIGNAL(triggered(bool)), this, SLOT(OnActionStopThread()));
	connect(ui.actionStop_Thread, SIGNAL(triggered(bool)), this, SLOT(OnActionSetBreakpoint()));
	
	// connect ui slots
	connect(ui.threadTable, SIGNAL(itemSelectionChanged()), this, SLOT(ThreadSelectionChanged()));
	

	// create network socket and slots
	tcpSocket = new QTcpSocket(this);
	connect(tcpSocket, SIGNAL(connected()), this, SLOT(SocketConnected()));
	connect(tcpSocket, SIGNAL(disconnected()), this, SLOT(SocketDisconnected()));
	connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(SocketReadyToRead()));
	connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
		this, SLOT(SocketDisplayError(QAbstractSocket::SocketError)));

	// create timers for various stuff
	removeThreadTimer = new QTimer(this);
	connect(removeThreadTimer, SIGNAL(timeout()), this, SLOT(RemoveExpiredThreads()));
	removeThreadTimer->start(1000);

	parentGlobals = new QTreeWidgetItem( ui.globalsTable );
	parentGlobals->setText(TreeColumn_Name, "globals");
	parentCurrent = 0;

	// TEMP
	//OnActionScriptOpen("D:/CVS/Omnibot/head/Installer/Files/et/scripts/goals/goal_checkstuck.gm");
}

GMDebuggerQt::~GMDebuggerQt()
{
}

struct PacketHeader {
	static const quint32 MAGIC_NUM = 0xDEADB33F;

	const quint32 magicNum;
	const quint32 dataSize;
	PacketHeader( quint32 sz ) : magicNum(MAGIC_NUM), dataSize( sz ) {}
};

void GMDebuggerQt::DebuggerSendMessage(const void * a_command, int a_len)
{
	if ( tcpSocket && tcpSocket->isValid() ) {
		PacketHeader hdr(a_len);
		tcpSocket->write((const char*)&hdr,sizeof(hdr));
		tcpSocket->write((const char*)a_command,a_len);
	}
}

const void * GMDebuggerQt::DebuggerPumpMessage(int &a_len)
{
	pumpData.resize(0);

	a_len = 0;
	if ( !socketData.isEmpty() ) {
		const char * dataPtr = socketData.constData();
		const PacketHeader * hdr = (const PacketHeader *)dataPtr;
		dataPtr += sizeof( PacketHeader );
		if ( hdr->magicNum != PacketHeader::MAGIC_NUM ) {
			ui.outputWindow->append( "MALFORMED PACKET!\n" );
			return NULL;
		}

		if ( socketData.size() >= hdr->dataSize+sizeof(PacketHeader) ) {
			a_len = hdr->dataSize;
			pumpData.append( dataPtr, hdr->dataSize );
			socketData.remove( 0, hdr->dataSize + sizeof(PacketHeader) );
			return pumpData.constData();
		}
	}	
	return NULL;
}

void GMDebuggerQt::SocketConnected()
{
	ui.actionConnect->setEnabled(false);
	ui.actionDisconnect->setEnabled(true);

	gmMachineGetThreadInfo();
	gmMachineGetGlobalsInfo(0);
	
	statusBar()->showMessage("Connected...", 2000);
}

void GMDebuggerQt::SocketDisconnected()
{
	ui.actionConnect->setEnabled(true);
	ui.actionDisconnect->setEnabled(false);
}

void GMDebuggerQt::SocketReadyToRead()
{
	QDataStream in(tcpSocket);
	const int sizeBefore = socketData.size();
	socketData.append(tcpSocket->readAll());
	UpdateDebugSession();
}

void GMDebuggerQt::SocketDisplayError(QAbstractSocket::SocketError socketError)
{
	switch (socketError) {
	case QAbstractSocket::RemoteHostClosedError:
		break;
	case QAbstractSocket::HostNotFoundError:
		QMessageBox::information(this, tr("GM Debugger"),
			tr("The host was not found. Please check the "
			"host name and port settings."));
		break;
	case QAbstractSocket::ConnectionRefusedError:
		QMessageBox::information(this, tr("GM Debugger"),
			tr("The connection was refused by the peer. "
			"Make sure the GM application is running, "
			"and check that the host name and port "
			"settings are correct."));
		break;
	default:
		QMessageBox::information(this, tr("GM Debugger"),
			tr("The following error occurred: %1.")
			.arg(tcpSocket->errorString()));
	}
}

void GMDebuggerQt::OnActionScriptOpen(const QString &path)
{
	QString fileName = path;

	if (fileName.isNull())
		fileName = QFileDialog::getOpenFileName(this,
		tr("Open File"), "", "Game Monkey Files (*.gm)");

	if (!fileName.isEmpty()) {
		QFile file(fileName);
		if (file.open(QFile::ReadOnly | QFile::Text))
		{
			ui.scriptEdit->setPlainText(file.readAll());
			ui.scriptEdit->setDocumentTitle(path);
		}
	}
}

void GMDebuggerQt::OnActionConnect()
{
	QString ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
	QString port = QString::number(49001);

	tcpSocket->abort();
	tcpSocket->connectToHost(ipAddress,port.toInt());
}

void GMDebuggerQt::OnActionDisConnect()
{
	tcpSocket->abort();
}

void GMDebuggerQt::OnActionRunAll()
{

}

void GMDebuggerQt::OnActionBreakAll()
{
	gmMachineBreakAll();
}

void GMDebuggerQt::OnActionKillAll()
{
	gmMachineKillAll();
}

void GMDebuggerQt::OnActionStepIn()
{

}

void GMDebuggerQt::OnActionStepOut()
{

}

void GMDebuggerQt::OnActionStepOver()
{

}

void GMDebuggerQt::OnActionRunThread()
{

}

void GMDebuggerQt::OnActionStopThread()
{

}

void GMDebuggerQt::OnActionSetBreakpoint()
{

}
