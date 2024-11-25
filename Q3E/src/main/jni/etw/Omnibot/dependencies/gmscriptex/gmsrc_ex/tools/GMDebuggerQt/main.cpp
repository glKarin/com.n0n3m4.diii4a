#include "gmdebuggerqt.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	GMDebuggerQt w;
	w.show();
	return a.exec();
}
