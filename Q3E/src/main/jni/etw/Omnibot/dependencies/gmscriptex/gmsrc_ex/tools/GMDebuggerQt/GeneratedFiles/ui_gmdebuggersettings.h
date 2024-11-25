/********************************************************************************
** Form generated from reading UI file 'gmdebuggersettings.ui'
**
** Created: Tue Mar 2 23:11:18 2010
**      by: Qt User Interface Compiler version 4.6.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GMDEBUGGERSETTINGS_H
#define UI_GMDEBUGGERSETTINGS_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_GMDebuggerSettingsClass
{
public:
    QComboBox *comboStyles;
    QLabel *Styles;
    QPushButton *buttonSetFont;

    void setupUi(QDialog *GMDebuggerSettingsClass)
    {
        if (GMDebuggerSettingsClass->objectName().isEmpty())
            GMDebuggerSettingsClass->setObjectName(QString::fromUtf8("GMDebuggerSettingsClass"));
        GMDebuggerSettingsClass->setWindowModality(Qt::ApplicationModal);
        GMDebuggerSettingsClass->resize(400, 300);
        GMDebuggerSettingsClass->setModal(true);
        comboStyles = new QComboBox(GMDebuggerSettingsClass);
        comboStyles->setObjectName(QString::fromUtf8("comboStyles"));
        comboStyles->setGeometry(QRect(100, 30, 201, 22));
        Styles = new QLabel(GMDebuggerSettingsClass);
        Styles->setObjectName(QString::fromUtf8("Styles"));
        Styles->setGeometry(QRect(50, 40, 46, 13));
        buttonSetFont = new QPushButton(GMDebuggerSettingsClass);
        buttonSetFont->setObjectName(QString::fromUtf8("buttonSetFont"));
        buttonSetFont->setGeometry(QRect(160, 60, 75, 23));

        retranslateUi(GMDebuggerSettingsClass);

        QMetaObject::connectSlotsByName(GMDebuggerSettingsClass);
    } // setupUi

    void retranslateUi(QDialog *GMDebuggerSettingsClass)
    {
        GMDebuggerSettingsClass->setWindowTitle(QApplication::translate("GMDebuggerSettingsClass", "GMDebuggerSettings", 0, QApplication::UnicodeUTF8));
        Styles->setText(QApplication::translate("GMDebuggerSettingsClass", "Styles", 0, QApplication::UnicodeUTF8));
        buttonSetFont->setText(QApplication::translate("GMDebuggerSettingsClass", "Select Font", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class GMDebuggerSettingsClass: public Ui_GMDebuggerSettingsClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GMDEBUGGERSETTINGS_H
