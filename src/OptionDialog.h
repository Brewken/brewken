/**
 * OptionDialog.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Greg Meess <Daedalus12@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Rob Taylor <robtaylor@floopily.org>
 *
 * Brewken is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * Brewken is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialog>
#include <QIcon>
#include <QMap>
#include <QString>
#include <QVector>
#include <QWidget>

#include "BtLineEdit.h"
#include "ui_optionsDialog.h"
#include "unit.h"
#include "Logging.h"

/*!
 * \class OptionDialog
 *
 * \brief View/controller dialog to manage options.
 */
class OptionDialog : public QDialog, public Ui::optionsDialog {

public:
   //! \brief Default constructor.
   OptionDialog(QWidget *parent=0);

   // UI stuff to make this work as I want
   // Postgres things
   QLabel *label_hostname;
   BtStringEdit *btStringEdit_hostname;
   QLabel *label_portnum;
   BtStringEdit *btStringEdit_portnum;
   QLabel *label_schema;
   BtStringEdit *btStringEdit_schema;
   QLabel *label_dbName;
   BtStringEdit *btStringEdit_dbname;
   QLabel *label_username;
   BtStringEdit *btStringEdit_username;
   QLabel *label_password;
   BtStringEdit *btStringEdit_password;
   QHBoxLayout *horizontalLayout_7;
   QSpacerItem *horizontalSpacer_3;
   QCheckBox *checkBox_savePassword;
   // SQLite things
   QLabel *label_dataDir;
   BtStringEdit *btStringEdit_dataDir;
   QPushButton *pushButton_browseDataDir;
   QLabel *label_backupDir;
   BtStringEdit *btStringEdit_backupDir;
   QPushButton *pushButton_browseBackupDir;
   QLabel *label_numBackups;
   QSpinBox *spinBox_numBackups;
   QLabel *label_frequency;
   QSpinBox *spinBox_frequency;

   void createPostgresElements();
   void createSQLiteElements();
   void clearLayout();
   void setDbDialog(Brewken::DBTypes db);
   void retranslateDbDialog(QDialog *optionsDialog);
   void sqliteVisible(bool canSee);
   void postgresVisible(bool canSee);

public slots:
   //! \brief Show the dialog.
   void show();
   //! \brief Save the options and close the dialog.
   void saveAndClose();
   //! \brief Close dialog without saving options.
   void cancel();
   //! \brief Pop up a dialog to choose the data directory.
   void setDataDir();
   void setBackupDir();
   //! \brief Pop up a dialog to choose the Log file directory.
   void setLogDir();
   //! \brief Reset data directory to default.
   void resetToDefault();

   //! \brief Enable or disable the configuration panel based on the engine choice
   void setEngine(int selected);
   //! \brief test connection to remote databases. This could get ugly
   void testConnection();
   //! \brief mark a change to the database config
   void testRequired();
   //! \brief handle the dialogs for saving passwords
   void savePassword(bool state);

   void setFileLocationState(bool state);

protected:

   //! \brief Reimplemented from QWidget.
   virtual void changeEvent(QEvent* e);

private:
   enum testStates {
      NOCHANGE,
      NEEDSTEST,
      TESTFAILED,
      TESTPASSED
   };

   testStates status;
   // Update UI strings according to current language.
   void retranslate();
   // Update dialog with current options.
   void showChanges();
   //
   void changeColors();
   QButtonGroup *colorGroup, *ibuGroup;
   QStringList ndxToLangCode;
   QVector<QIcon> langIcons;
};

#endif
