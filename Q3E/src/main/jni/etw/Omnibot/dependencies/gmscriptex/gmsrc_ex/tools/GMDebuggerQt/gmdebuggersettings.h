#ifndef GMDEBUGGERSETTINGS_H
#define GMDEBUGGERSETTINGS_H

#include <QDialog>
#include "ui_gmdebuggersettings.h"

class GMDebuggerSettings : public QDialog
{
	Q_OBJECT
public:
	GMDebuggerSettings(QWidget *parent = 0);
	~GMDebuggerSettings();
	
private:
	Ui::GMDebuggerSettingsClass ui;

	QStyle * oldStyle;

private slots:
	void StyleChanged();
	void SelectFont();
};

#endif // GMDEBUGGERSETTINGS_H
