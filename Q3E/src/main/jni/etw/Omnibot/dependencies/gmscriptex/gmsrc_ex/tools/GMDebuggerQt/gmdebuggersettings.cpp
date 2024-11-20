#include "gmdebuggersettings.h"
#include <QFontDialog>

GMDebuggerSettings::GMDebuggerSettings(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	ui.comboStyles->addItem( "Windows", "windows" );
	ui.comboStyles->addItem( "WindowsXP", "windowsxp" );
	ui.comboStyles->addItem( "WindowsVista", "windowsvista" );
	ui.comboStyles->addItem( "Motif", "motif" );
	ui.comboStyles->addItem( "CDE", "cde" );
	ui.comboStyles->addItem( "Plastique", "plastique" );
	ui.comboStyles->addItem( "CleanLooks", "cleanlooks" );

	oldStyle = QApplication::style();
	QString oldStyleName = QApplication::style()->objectName();
	const int i = ui.comboStyles->findData( oldStyleName );
	ui.comboStyles->setCurrentIndex( i );

	// slots
	connect(ui.comboStyles, SIGNAL(currentIndexChanged(int)), this, SLOT(StyleChanged()));
	connect(ui.buttonSetFont, SIGNAL(clicked()), this, SLOT(SelectFont()));
}

GMDebuggerSettings::~GMDebuggerSettings()
{
}

void GMDebuggerSettings::StyleChanged()
{
	const int i = ui.comboStyles->currentIndex();
	QString styleName = ui.comboStyles->itemData( i ).toString();
	QApplication::setStyle( styleName );
}

void GMDebuggerSettings::SelectFont()
{
	bool ok = false;
	QFont newFont = QFontDialog::getFont(&ok, QApplication::font(), this);
	if ( ok ) {
		QApplication::setFont( newFont );
	}
}
