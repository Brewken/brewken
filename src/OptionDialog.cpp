/*======================================================================================================================
 * OptionDialog.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Greg Meess <Daedalus12@gmail.com>
 *   • Idar Lund <idarlund@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Rob Taylor <robtaylor@floopily.org>
 *   • Théophane Martin <theophane.m@gmail.com>
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
 ======================================================================================================================*/
#include "OptionDialog.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QIcon>
#include <QMap>
#include <QMessageBox>
#include <QSizePolicy>
#include <QString>
#include <QVector>
#include <QWidget>

#include "Brewken.h"
#include "BtLineEdit.h"
#include "database/Database.h"
#include "IbuMethods.h"
#include "Logging.h"
#include "MainWindow.h"
#include "PersistentSettings.h"
#include "Unit.h"
#include "UnitSystem.h"

//
// Anonymous namespace for constants, global variables and functions used only in this file
//
namespace {

   enum DbConnectionTestStates {
      NO_CHANGE,
      NEEDS_TEST,
      TEST_FAILED,
      TEST_PASSED
   };

   struct LanguageInfo {
      QString            iso639_1Code;       // What we need to pass to Brewken::setLanguage()
      QIcon              countryFlag;        // Yes, we know some languages are spoken in more than one country...
      char const    *    nameInEnglish;
      QString            nameInCurrentLang;  // Don't strictly need to store this, but having the hard-coded tr() calls
      // in the initialisation flag up what language names need translating
   };
}

// This private implementation class holds all private non-virtual members of OptionDialog
class OptionDialog::impl {
public:

   /**
    * Constructor
    */
   impl(OptionDialog & optionDialog) :
      qFileDialog                {&optionDialog},
      label_pgHostname           {optionDialog.groupBox_dbConfig},
      input_pgHostname           {optionDialog.groupBox_dbConfig},
      label_pgPortNum            {optionDialog.groupBox_dbConfig},
      input_pgPortNum            {optionDialog.groupBox_dbConfig},
      label_pgSchema             {optionDialog.groupBox_dbConfig},
      input_pgSchema             {optionDialog.groupBox_dbConfig},
      label_pgDbName             {optionDialog.groupBox_dbConfig},
      input_pgDbName             {optionDialog.groupBox_dbConfig},
      label_pgUsername           {optionDialog.groupBox_dbConfig},
      input_pgUsername           {optionDialog.groupBox_dbConfig},
      label_pgPassword           {optionDialog.groupBox_dbConfig},
      input_pgPassword           {optionDialog.groupBox_dbConfig},
      checkBox_savePgPassword    {optionDialog.groupBox_dbConfig},
      label_userDataDir          {optionDialog.groupBox_dbConfig},
      input_userDataDir          {optionDialog.groupBox_dbConfig},
      pushButton_browseDataDir   {optionDialog.groupBox_dbConfig},
      label_backupDir            {optionDialog.groupBox_dbConfig},
      input_backupDir            {optionDialog.groupBox_dbConfig},
      pushButton_browseBackupDir {optionDialog.groupBox_dbConfig},
      label_numBackups           {optionDialog.groupBox_dbConfig},
      spinBox_numBackups         {optionDialog.groupBox_dbConfig},
      label_frequency            {optionDialog.groupBox_dbConfig},
      spinBox_frequency          {optionDialog.groupBox_dbConfig},
      languageInfo {
      {"ca", QIcon(":images/flagCatalonia.svg"),   "Catalan",          tr("Catalan")          },
      {"cs", QIcon(":images/flagCzech.svg"),       "Czech",            tr("Czech")            },
      {"da", QIcon(":images/flagDenmark.svg"),     "Danish",           tr("Danish")           },
      {"de", QIcon(":images/flagGermany.svg"),     "German",           tr("German")           },
      {"el", QIcon(":images/flagGreece.svg"),      "Greek",            tr("Greek")            },
      {"en", QIcon(":images/flagUK.svg"),          "English",          tr("English")          },
      {"es", QIcon(":images/flagSpain.svg"),       "Spanish",          tr("Spanish")          },
      {"et", QIcon(),                              "Estonian",         tr("Estonian")         },
      {"eu", QIcon(),                              "Basque",           tr("Basque")           },
      {"fr", QIcon(":images/flagFrance.svg"),      "French",           tr("French")           },
      {"gl", QIcon(),                              "Galician",         tr("Galician")         },
      {"hu", QIcon(),                              "Hungarian",        tr("Hungarian")        },
      {"it", QIcon(":images/flagItaly.svg"),       "Italian",          tr("Italian")          },
      {"lv", QIcon(),                              "Latvian",          tr("Latvian")          },
      {"nb", QIcon(":images/flagNorway.svg"),      "Norwegian Bokmål", tr("Norwegian Bokmål") },
      {"nl", QIcon(":images/flagNetherlands.svg"), "Dutch",            tr("Dutch")            },
      {"pl", QIcon(":images/flagPoland.svg"),      "Polish",           tr("Polish")           },
      {"pt", QIcon(":images/flagBrazil.svg"),      "Portuguese",       tr("Portuguese")       },
      {"ru", QIcon(":images/flagRussia.svg"),      "Russian",          tr("Russian")          },
      {"sr", QIcon(),                              "Serbian",          tr("Serbian")          },
      {"sv", QIcon(":images/flagSweden.svg"),      "Swedish",          tr("Swedish")          },
      {"tr", QIcon(),                              "Turkish",          tr("Turkish")          },
      {"zh", QIcon(":images/flagChina.svg"),       "Chinese",          tr("Chinese")          }
   } {
      //
      // Optimise the select file dialog to select directories
      //
      this->qFileDialog.setFileMode(QFileDialog::Directory);  // <- User can only select directories
      this->qFileDialog.setOptions(
         QFileDialog::ShowDirsOnly |        // <- Only show directories in the dialog
         QFileDialog::DontUseNativeDialog | // <- Use the Qt dialog for selecting directories as it's usually better at
         //    respecting all the other settings than the native dialog
         QFileDialog::HideNameFilterDetails // <- Don't have the file-types selector active, as it serves no purpose
      );                                    //    for selecting a directory
      this->qFileDialog.setFilter(QDir::AllDirs | QDir::Hidden); // <- We don't hide any directories from the user

      // PostgresSQL settings UI
      this->label_pgHostname.setObjectName(QStringLiteral("label_pgHostname"));
      this->input_pgHostname.setObjectName(QStringLiteral("input_pgHostname"));
      this->label_pgPortNum.setObjectName(QStringLiteral("label_pgPortNum"));
      this->input_pgPortNum.setObjectName(QStringLiteral("input_pgPortNum"));
      this->label_pgSchema.setObjectName(QStringLiteral("label_pgSchema"));
      this->input_pgSchema.setObjectName(QStringLiteral("input_pgSchema"));
      this->label_pgDbName.setObjectName(QStringLiteral("label_pgDbName"));
      this->input_pgDbName.setObjectName(QStringLiteral("input_pgDbName"));
      this->label_pgUsername.setObjectName(QStringLiteral("label_pgUsername"));
      this->input_pgUsername.setObjectName(QStringLiteral("input_pgUsername"));
      this->label_pgPassword.setObjectName(QStringLiteral("label_pgPassword"));
      this->input_pgPassword.setObjectName(QStringLiteral("input_pgPassword"));
      this->input_pgPassword.setEchoMode(QLineEdit::Password);
      this->checkBox_savePgPassword.setObjectName(QStringLiteral("checkBox_savePgPassword"));
      this->postgresVisible(false);

      // SQLite settings UI
      this->label_userDataDir.setObjectName(QStringLiteral("label_userDataDir"));
      this->input_userDataDir.setObjectName(QStringLiteral("input_userDataDir"));
      this->pushButton_browseDataDir.setObjectName(QStringLiteral("button_browseDataDir"));
      this->label_backupDir.setObjectName(QStringLiteral("label_backupDir"));
      this->input_backupDir.setObjectName(QStringLiteral("input_backupDir"));
      this->pushButton_browseBackupDir.setObjectName(QStringLiteral("button_browseBackupDir"));
      this->label_numBackups.setObjectName(QStringLiteral("label_numBackups"));
      this->spinBox_numBackups.setObjectName(QStringLiteral("spinBox_numBackups"));
      this->spinBox_numBackups.setMinimum(-1);
      this->spinBox_numBackups.setMaximum(9999);
      this->label_frequency.setObjectName(QStringLiteral("label_frequency"));
      this->spinBox_frequency.setObjectName(QStringLiteral("spinBox_frequency"));
      this->spinBox_frequency.setMinimum(1); // Couldn't make any semantic difference between 0 and 1. So start at 1
      this->spinBox_frequency.setMaximum(10);
      this->sqliteVisible(false);

      return;
   }

   void initLangs(OptionDialog & optionDialog) {

      for (auto langInfo : this->languageInfo) {
         optionDialog.comboBox_lang->addItem(langInfo.countryFlag, langInfo.nameInCurrentLang, langInfo.iso639_1Code);
      }

      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   void postgresVisible(bool canSee) {
      this->label_pgHostname.setVisible(canSee);
      this->input_pgHostname.setVisible(canSee);
      this->label_pgPortNum.setVisible(canSee);
      this->input_pgPortNum.setVisible(canSee);
      this->label_pgSchema.setVisible(canSee);
      this->input_pgSchema.setVisible(canSee);
      this->label_pgDbName.setVisible(canSee);
      this->input_pgDbName.setVisible(canSee);
      this->label_pgUsername.setVisible(canSee);
      this->input_pgUsername.setVisible(canSee);
      this->label_pgPassword.setVisible(canSee);
      this->input_pgPassword.setVisible(canSee);
      this->checkBox_savePgPassword.setVisible(canSee);
      this->label_pgPassword.setVisible(canSee);
      return;
   }

   void sqliteVisible(bool canSee) {
      this->label_userDataDir.setVisible(canSee);
      this->input_userDataDir.setVisible(canSee);
      this->pushButton_browseDataDir.setVisible(canSee);
      this->label_backupDir.setVisible(canSee);
      this->input_backupDir.setVisible(canSee);
      this->pushButton_browseBackupDir.setVisible(canSee);
      this->label_numBackups.setVisible(canSee);
      this->spinBox_numBackups.setVisible(canSee);
      this->label_frequency.setVisible(canSee);
      this->spinBox_frequency.setVisible(canSee);
      return;
   }


   /**
    *
    */
   void clearLayout(OptionDialog & optionDialog) {
      QLayoutItem * child;
      while ((child = optionDialog.gridLayout->takeAt(0)) != nullptr) {
         optionDialog.gridLayout->removeItem(child);
      }
      return;
   }

   /**
    * Determine which set of DB config params to show, based on whether PostgresSQL or SQLite is selected
    */
   void setDbDialog(OptionDialog & optionDialog, Database::DbType db) {
      qDebug() << Q_FUNC_INFO << "Set " << (db == Database::PGSQL ? "PostgresSQL" : "SQLite") << " config params visible";
      optionDialog.groupBox_dbConfig->setVisible(false);

      this->clearLayout(optionDialog);
      if (db == Database::PGSQL) {
         this->postgresVisible(true);
         this->sqliteVisible(false);

         optionDialog.gridLayout->addWidget(&this->label_pgHostname, 0, 0);
         optionDialog.gridLayout->addWidget(&this->input_pgHostname, 0, 1, 1, 2);

         optionDialog.gridLayout->addWidget(&this->label_pgPortNum, 0, 3);
         optionDialog.gridLayout->addWidget(&this->input_pgPortNum, 0, 4);

         optionDialog.gridLayout->addWidget(&this->label_pgSchema, 1, 0);
         optionDialog.gridLayout->addWidget(&this->input_pgSchema, 1, 1);

         optionDialog.gridLayout->addWidget(&this->label_pgDbName, 2, 0);
         optionDialog.gridLayout->addWidget(&this->input_pgDbName, 2, 1);

         optionDialog.gridLayout->addWidget(&this->label_pgUsername, 3, 0);
         optionDialog.gridLayout->addWidget(&this->input_pgUsername, 3, 1);

         optionDialog.gridLayout->addWidget(&this->label_pgPassword, 4, 0);
         optionDialog.gridLayout->addWidget(&this->input_pgPassword, 4, 1);

         optionDialog.gridLayout->addWidget(&this->checkBox_savePgPassword, 4, 4);

      } else {
         this->postgresVisible(false);
         this->sqliteVisible(true);

         optionDialog.gridLayout->addWidget(&this->label_userDataDir, 0, 0);
         optionDialog.gridLayout->addWidget(&this->input_userDataDir, 0, 1, 1, 2);
         optionDialog.gridLayout->addWidget(&this->pushButton_browseDataDir, 0, 3);

         optionDialog.gridLayout->addWidget(&this->label_backupDir, 1, 0);
         optionDialog.gridLayout->addWidget(&this->input_backupDir, 1, 1, 1, 2);
         optionDialog.gridLayout->addWidget(&this->pushButton_browseBackupDir, 1, 3);

         optionDialog.gridLayout->addWidget(&this->label_numBackups, 3, 0);
         optionDialog.gridLayout->addWidget(&this->spinBox_numBackups, 3, 1);

         optionDialog.gridLayout->addWidget(&this->label_frequency, 4, 0);
         optionDialog.gridLayout->addWidget(&this->spinBox_frequency, 4, 1);
      }
      optionDialog.groupBox_dbConfig->setVisible(true);
      return;
   }


   void retranslateDbDialog() {
      //PostgreSQL stuff
      this->label_pgHostname.setText(QApplication::translate("optionsDialog", "Hostname", nullptr));
      this->label_pgPortNum.setText(QApplication::translate("optionsDialog", "Port", nullptr));
      this->label_pgSchema.setText(QApplication::translate("optionsDialog", "Schema", nullptr));
      this->label_pgDbName.setText(QApplication::translate("optionsDialog", "Database", nullptr));
      this->label_pgUsername.setText(QApplication::translate("optionsDialog", "Username", nullptr));
      this->label_pgPassword.setText(QApplication::translate("optionsDialog", "Password", nullptr));
      this->checkBox_savePgPassword.setText(QApplication::translate("optionsDialog", "Save password", nullptr));

      // SQLite things
      this->label_userDataDir.setText(QApplication::translate("optionsDialog", "Data Directory", nullptr));
      this->pushButton_browseDataDir.setText(QApplication::translate("optionsDialog", "Browse", nullptr));
      this->label_backupDir.setText(QApplication::translate("optionsDialog", "Backup Directory", nullptr));
      this->pushButton_browseBackupDir.setText(QApplication::translate("optionsDialog", "Browse", nullptr));
      this->label_numBackups.setText(QApplication::translate("optionsDialog", "Number of Backups", nullptr));
      this->label_frequency.setText(QApplication::translate("optionsDialog", "Frequency of Backups", nullptr));

      // set up the tooltips if we are using them
#ifndef QT_NO_TOOLTIP
      this->input_pgHostname.setToolTip(QApplication::translate("optionsDialog", "PostgresSQL's host name or IP address",
                                                                nullptr));
      this->input_pgPortNum.setToolTip(QApplication::translate("optionsDialog", "Port the PostgreSQL is listening on",
                                                               nullptr));
      this->input_pgSchema.setToolTip(QApplication::translate("optionsDialog", "The schema containing the database",
                                                              nullptr));
      this->input_pgUsername.setToolTip(QApplication::translate("optionsDialog", "User with create/delete table access",
                                                                nullptr));
      this->input_pgPassword.setToolTip(QApplication::translate("optionsDialog", "Password for the user", nullptr));
      this->input_pgDbName.setToolTip(QApplication::translate("optionsDialog", "The name of the database", nullptr));
      this->label_userDataDir.setToolTip(QApplication::translate("optionsDialog", "Where your database file is", nullptr));
      this->label_backupDir.setToolTip(QApplication::translate("optionsDialog", "Where to save your backups", nullptr));
      this->label_numBackups.setToolTip(QApplication::translate("optionsDialog",
                                                                "Number of backups to keep: -1 means never remove, 0 means never backup", nullptr));
      // Actually the backups happen after every X times the program is closed, but the tooltip is already long enough!
      this->label_frequency.setToolTip(QApplication::translate("optionsDialog",
                                                               "How many times Brewken needs to be run to trigger another backup: 1 means always backup", nullptr));
#endif
      return;
   }

   /**
    * \brief Update UI strings according to current language.
    */
   void retranslate(OptionDialog & optionDialog) {
      // Let the Ui take care of its business
      optionDialog.retranslateUi(&optionDialog);
      this->retranslateDbDialog();

      // Retranslate the language combobox.
      for (int ii = 0; ii < this->languageInfo.size(); ++ii) {
         this->languageInfo[ii].nameInCurrentLang = tr(this->languageInfo[ii].nameInEnglish);
         optionDialog.comboBox_lang->setItemText(ii, this->languageInfo[ii].nameInCurrentLang);
      }
      return;
   }

   void changeColors(OptionDialog & optionDialog) {
      // Yellow when the test is needed
      // Red when the test failed
      // Green when the test passed
      // Black otherwise.

      switch (dbConnectionTestState) {
         case NEEDS_TEST:
            optionDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            optionDialog.pushButton_testConnection->setEnabled(true);
            optionDialog.pushButton_testConnection->setStyleSheet("color:rgb(240,225,25)");
            break;
         case TEST_FAILED:
            optionDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            optionDialog.pushButton_testConnection->setStyleSheet("color:red");
            break;
         case TEST_PASSED:
            optionDialog.pushButton_testConnection->setStyleSheet("color:green");
            optionDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            optionDialog.pushButton_testConnection->setEnabled(false);
            break;
         case NO_CHANGE:
            optionDialog.pushButton_testConnection->setStyleSheet("color:grey");
            optionDialog.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
            optionDialog.pushButton_testConnection->setEnabled(false);
            break;
      }
      return;
   }

   // Update dialog with current options.
   void showChanges(OptionDialog & optionDialog) {
      // Set the right language
      int index = optionDialog.comboBox_lang->findData(Brewken::getCurrentLanguage());
      if (index >= 0) {
         optionDialog.comboBox_lang->setCurrentIndex(index);
      }

      optionDialog.weightComboBox->setCurrentIndex(optionDialog.weightComboBox->findData(Brewken::weightUnitSystem));
      optionDialog.temperatureComboBox->setCurrentIndex(optionDialog.temperatureComboBox->findData(Brewken::tempScale));
      optionDialog.volumeComboBox->setCurrentIndex(optionDialog.volumeComboBox->findData(Brewken::volumeUnitSystem));
      optionDialog.gravityComboBox->setCurrentIndex(optionDialog.gravityComboBox->findData(Brewken::densityUnit));
      optionDialog.dateComboBox->setCurrentIndex(optionDialog.dateComboBox->findData(Brewken::dateFormat));
      optionDialog.colorComboBox->setCurrentIndex(optionDialog.colorComboBox->findData(Brewken::colorUnit));
      optionDialog.diastaticPowerComboBox->setCurrentIndex(optionDialog.diastaticPowerComboBox->findData(
                                                              Brewken::diastaticPowerUnit));

      optionDialog.colorFormulaComboBox->setCurrentIndex(optionDialog.colorFormulaComboBox->findData(Brewken::colorFormula));
      optionDialog.ibuFormulaComboBox->setCurrentIndex(optionDialog.ibuFormulaComboBox->findData(IbuMethods::ibuFormula));

      // User data directory
      this->input_userDataDir.setText(PersistentSettings::getUserDataDir().canonicalPath());

      // Backup stuff
      // By default backups go in the same directory as the DB
      this->input_backupDir.setText(PersistentSettings::value(PersistentSettings::Names::directory,
                                                              PersistentSettings::getUserDataDir().canonicalPath(), PersistentSettings::Sections::backups).toString());
      this->spinBox_numBackups.setValue(PersistentSettings::value(PersistentSettings::Names::maximum, 10, PersistentSettings::Sections::backups).toInt());
      this->spinBox_frequency.setValue(PersistentSettings::value(PersistentSettings::Names::frequency, 4, PersistentSettings::Sections::backups).toInt());

      // The IBU modifications. These will all be calculated from a 60 min boil. This is gonna get confusing.
      double amt = Brewken::toDouble(PersistentSettings::value(PersistentSettings::Names::mashHopAdjustment, 0).toString(),
                                     "OptionDialog::showChanges()");
      optionDialog.ibuAdjustmentMashHopDoubleSpinBox->setValue(amt * 100);

      amt = Brewken::toDouble(PersistentSettings::value(PersistentSettings::Names::firstWortHopAdjustment, 1.1).toString(),
                              "OptionDialog::showChanges()");
      optionDialog.ibuAdjustmentFirstWortDoubleSpinBox->setValue(amt * 100);

      // Database stuff -- this looks weird, but trust me. We want SQLITE to be
      // the default for this field
      int tmp = PersistentSettings::value(PersistentSettings::Names::dbType, Database::SQLITE).toInt() - 1;
      optionDialog.comboBox_engine->setCurrentIndex(tmp);

      this->input_pgHostname.setText(PersistentSettings::value(PersistentSettings::Names::dbHostname, "localhost").toString());
      this->input_pgPortNum.setText(PersistentSettings::value(PersistentSettings::Names::dbPortnum, "5432").toString());
      this->input_pgSchema.setText(PersistentSettings::value(PersistentSettings::Names::dbSchema, "public").toString());
      this->input_pgDbName.setText(PersistentSettings::value(PersistentSettings::Names::dbName, "brewken").toString());
      this->input_pgUsername.setText(PersistentSettings::value(PersistentSettings::Names::dbUsername, "brewken").toString());
      this->input_pgPassword.setText(PersistentSettings::value(PersistentSettings::Names::dbPassword, "").toString());
      this->checkBox_savePgPassword.setChecked(PersistentSettings::contains(PersistentSettings::Names::dbPassword));

      this->dbConnectionTestState = NO_CHANGE;
      this->changeColors(optionDialog);

      if (RecipeHelper::getAutomaticVersioningEnabled()) {
         optionDialog.checkBox_versioning->setCheckState(Qt::Checked);
         optionDialog.groupBox_deleteBehavior->setEnabled(true);
         switch (PersistentSettings::value(PersistentSettings::Names::deletewhat, Recipe::DESCENDANT).toInt()) {
            case Recipe::ANCESTOR:
               optionDialog.radioButton_deleteAncestor->setChecked(true);
               break;
            default:
               optionDialog.radioButton_deleteDescendant->setChecked(true);
               break;
         }
      } else {
         optionDialog.checkBox_versioning->setCheckState(Qt::Unchecked);
         optionDialog.groupBox_deleteBehavior->setEnabled(false);
      }

      if (PersistentSettings::value(PersistentSettings::Names::showsnapshots, false).toBool()) {
         optionDialog.checkBox_alwaysShowSnaps->setCheckState(Qt::Checked);
      } else {
         optionDialog.checkBox_alwaysShowSnaps->setCheckState(Qt::Unchecked);
      }

      return;
   }

   // Used for selecting directories
   QFileDialog qFileDialog;

   // UI stuff to make this work as I want
   // Postgres things
   QLabel    label_pgHostname;
   QLineEdit input_pgHostname;
   QLabel    label_pgPortNum;
   QLineEdit input_pgPortNum;
   QLabel    label_pgSchema;
   QLineEdit input_pgSchema;
   QLabel    label_pgDbName;
   QLineEdit input_pgDbName;
   QLabel    label_pgUsername;
   QLineEdit input_pgUsername;
   QLabel    label_pgPassword;
   QLineEdit input_pgPassword;
   QCheckBox checkBox_savePgPassword;
   // SQLite things
   QLabel      label_userDataDir;
   QLineEdit   input_userDataDir;
   QPushButton pushButton_browseDataDir;
   QLabel      label_backupDir;
   QLineEdit   input_backupDir;
   QPushButton pushButton_browseBackupDir;
   QLabel      label_numBackups;
   QSpinBox    spinBox_numBackups;
   QLabel      label_frequency;
   QSpinBox    spinBox_frequency;

   DbConnectionTestStates dbConnectionTestState;

   QVector<LanguageInfo> languageInfo;

};

OptionDialog::OptionDialog(QWidget * parent) : QDialog{},
   Ui::optionsDialog{},
   pimpl{std::make_unique<impl>(*this)} {

   // I need a lot of control over what is displayed on the DbConfig dialog.
   // Maybe designer can do it? No idea. So I did this hybrid model, and I
   // think it will end up biting my ...
   // anyway. It isn't pretty
   this->setupUi(this);
   this->pimpl->initLangs(*this);

   if (parent != nullptr) {
      setWindowIcon(parent->windowIcon());
   }

   // populate the combo boxes on the units tab
   configure_unitCombos();

   // populate the combo boxes on the formulas tab
   configure_formulaCombos();

   // populate the combo boxes on the logging tab
   configure_logging();

   // database panel stuff
   comboBox_engine->addItem(tr("SQLite (default)"), QVariant(Database::SQLITE));
   comboBox_engine->addItem(tr("PostgreSQL"), QVariant(Database::PGSQL));

   // figure out which database we have
   int idx = comboBox_engine->findData(PersistentSettings::value(PersistentSettings::Names::dbType, Database::SQLITE).toInt());
   this->pimpl->setDbDialog(*this, static_cast<Database::DbType>(idx));

   // connect all the signals
   connect_signals();

   pushButton_testConnection->setEnabled(false);
}

void OptionDialog::configure_unitCombos() {
   // Populate combo boxes on the "Units" tab
   weightComboBox->addItem(tr("SI units"), QVariant(SI));
   weightComboBox->addItem(tr("US traditional units"), QVariant(USCustomary));
   weightComboBox->addItem(tr("British imperial units"), QVariant(Imperial));

   temperatureComboBox->addItem(tr("Celsius"), QVariant(Celsius));
   temperatureComboBox->addItem(tr("Fahrenheit"), QVariant(Fahrenheit));

   volumeComboBox->addItem(tr("SI units"), QVariant(SI));
   volumeComboBox->addItem(tr("US traditional units"), QVariant(USCustomary));
   volumeComboBox->addItem(tr("British imperial units"), QVariant(Imperial));

   gravityComboBox->addItem(tr("20C/20C Specific Gravity"), QVariant(Brewken::SG));
   gravityComboBox->addItem(tr("Plato/Brix/Balling"), QVariant(Brewken::PLATO));

   dateComboBox->addItem(tr("mm-dd-YYYY"), QVariant(Unit::displayUS));
   dateComboBox->addItem(tr("dd-mm-YYYY"), QVariant(Unit::displayImp));
   dateComboBox->addItem(tr("YYYY-mm-dd"), QVariant(Unit::displaySI));

   colorComboBox->addItem(tr("SRM"), QVariant(Brewken::SRM));
   colorComboBox->addItem(tr("EBC"), QVariant(Brewken::EBC));
}

void OptionDialog::configure_formulaCombos() {
   diastaticPowerComboBox->addItem(tr("Lintner"), QVariant(Brewken::LINTNER));
   diastaticPowerComboBox->addItem(tr("WK"), QVariant(Brewken::WK));

   // Populate combo boxes on the "Formulas" tab
   ibuFormulaComboBox->addItem(tr("Tinseth's approximation"), QVariant(IbuMethods::TINSETH));
   ibuFormulaComboBox->addItem(tr("Rager's approximation"), QVariant(IbuMethods::RAGER));
   ibuFormulaComboBox->addItem(tr("Noonan's approximation"), QVariant(IbuMethods::NOONAN));

   colorFormulaComboBox->addItem(tr("Mosher's approximation"), QVariant(Brewken::MOSHER));
   colorFormulaComboBox->addItem(tr("Daniel's approximation"), QVariant(Brewken::DANIEL));
   colorFormulaComboBox->addItem(tr("Morey's approximation"), QVariant(Brewken::MOREY));
}

void OptionDialog::configure_logging() {
   //Populate options on the "Logging" tab
   for (auto ii : Logging::levelDetails) {
      loggingLevelComboBox->addItem(ii.description, QVariant(ii.level));
   }
   loggingLevelComboBox->setCurrentIndex(Logging::getLogLevel());
   checkBox_LogFileLocationUseDefault->setChecked(Logging::getLogInConfigDir());
   lineEdit_LogFileLocation->setText(Logging::getDirectory().absolutePath());
   this->setFileLocationState(Logging::getLogInConfigDir());
   return;
}

void OptionDialog::connect_signals() {
   connect(buttonBox, &QDialogButtonBox::accepted, this, &OptionDialog::saveAndClose);
   connect(buttonBox, &QDialogButtonBox::rejected, this, &OptionDialog::cancel);

   // QOverload is needed on next line because the signal currentIndexChanged is overloaded in QComboBox - see
   // https://doc.qt.io/qt-5/qcombobox.html#currentIndexChanged
   connect(comboBox_engine, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &OptionDialog::setEngine);
   connect(pushButton_testConnection, &QAbstractButton::clicked, this, &OptionDialog::testConnection);

   // figure out which database we have
   int idx = comboBox_engine->findData(PersistentSettings::value(PersistentSettings::Names::dbType, Database::SQLITE).toInt());
   this->pimpl->setDbDialog(*this, static_cast<Database::DbType>(idx));

   // Set the signals
   connect(&this->pimpl->checkBox_savePgPassword, &QAbstractButton::clicked, this, &OptionDialog::savePassword);
   connect(this->checkBox_LogFileLocationUseDefault, &QAbstractButton::clicked, this, &OptionDialog::setFileLocationState);

   connect(&this->pimpl->input_pgHostname, &QLineEdit::editingFinished, this, &OptionDialog::testRequired);
   connect(&this->pimpl->input_pgPortNum,  &QLineEdit::editingFinished, this, &OptionDialog::testRequired);
   connect(&this->pimpl->input_pgSchema,   &QLineEdit::editingFinished, this, &OptionDialog::testRequired);
   connect(&this->pimpl->input_pgDbName,   &QLineEdit::editingFinished, this, &OptionDialog::testRequired);
   connect(&this->pimpl->input_pgUsername, &QLineEdit::editingFinished, this, &OptionDialog::testRequired);
   connect(&this->pimpl->input_pgPassword, &QLineEdit::editingFinished, this, &OptionDialog::testRequired);

   connect(&this->pimpl->pushButton_browseDataDir, &QAbstractButton::clicked, this, &OptionDialog::setDataDir);
   connect(&this->pimpl->pushButton_browseBackupDir, &QAbstractButton::clicked, this, &OptionDialog::setBackupDir);
   connect(pushButton_resetToDefault, &QAbstractButton::clicked, this, &OptionDialog::resetToDefault);
   connect(pushButton_LogFileLocationBrowse, &QAbstractButton::clicked, this, &OptionDialog::setLogDir);
   pushButton_testConnection->setEnabled(false);

   connect(checkBox_versioning, &QAbstractButton::clicked, this, &OptionDialog::versioningChanged);
   connect(checkBox_alwaysShowSnaps, &QAbstractButton::clicked, this, &OptionDialog::signalAncestors);

   // Call this here to set up translatable strings.
   this->pimpl->retranslate(*this);
   return;
}

void OptionDialog::signalAncestors() {
   emit showAllAncestors(checkBox_alwaysShowSnaps->checkState() == Qt::Checked);
}

// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the
// header file)
OptionDialog::~OptionDialog() = default;


void OptionDialog::show() {
   this->pimpl->showChanges(*this);
   this->setVisible(true);
   return;
}

void OptionDialog::setDataDir() {
   this->pimpl->qFileDialog.setDirectory(this->pimpl->input_userDataDir.text());
   this->pimpl->qFileDialog.setWindowTitle(tr("Choose User Data Directory"));
   if (this->pimpl->qFileDialog.exec() == QDialog::Accepted && this->pimpl->qFileDialog.selectedFiles().size() > 0) {
      this->pimpl->input_userDataDir.setText(this->pimpl->qFileDialog.selectedFiles().value(0));
   }
   return;
}

void OptionDialog::setBackupDir() {
   this->pimpl->qFileDialog.setDirectory(this->pimpl->input_backupDir.text());
   this->pimpl->qFileDialog.setWindowTitle(tr("Choose Backups Directory"));
   if (this->pimpl->qFileDialog.exec() == QDialog::Accepted && this->pimpl->qFileDialog.selectedFiles().size() > 0) {
      this->pimpl->input_backupDir.setText(this->pimpl->qFileDialog.selectedFiles().value(0));
   }
   return;
}

void OptionDialog::setLogDir() {
   this->pimpl->qFileDialog.setDirectory(lineEdit_LogFileLocation->text());
   this->pimpl->qFileDialog.setWindowTitle(tr("Choose Logging Directory"));
   if (this->pimpl->qFileDialog.exec() == QDialog::Accepted && this->pimpl->qFileDialog.selectedFiles().size() > 0) {
      lineEdit_LogFileLocation->setText(this->pimpl->qFileDialog.selectedFiles().value(0));
   }
   return;
}

void OptionDialog::resetToDefault() {
   Database::DbType engine = static_cast<Database::DbType>(comboBox_engine->currentData().toInt());
   if (engine == Database::PGSQL) {
      this->pimpl->input_pgHostname.setText(QString("localhost"));
      this->pimpl->input_pgPortNum.setText(QString("5432"));
      this->pimpl->input_pgSchema.setText(QString("public"));
      this->pimpl->input_pgDbName.setText(QString("brewken"));
      this->pimpl->input_pgUsername.setText(QString("brewken"));
      this->pimpl->input_pgPassword.setText(QString(""));
      this->pimpl->checkBox_savePgPassword.setChecked(false);
   } else {
      this->pimpl->input_userDataDir.setText(PersistentSettings::getConfigDir().canonicalPath());
      this->pimpl->input_backupDir.setText(PersistentSettings::getConfigDir().canonicalPath());
      this->pimpl->spinBox_frequency.setValue(4);
      this->pimpl->spinBox_numBackups.setValue(10);
   }
}


void OptionDialog::cancel() {
   this->setVisible(false);
   return;
}


void OptionDialog::changeEvent(QEvent * e) {
   switch (e->type()) {
      case QEvent::LanguageChange:
         this->pimpl->retranslate(*this);
         e->accept();
         break;
      default:
         QDialog::changeEvent(e);
         break;
   }
   return;
}

void OptionDialog::setEngine(int selected) {

   QVariant data = comboBox_engine->currentData();
   Database::DbType newEngine = static_cast<Database::DbType>(data.toInt());

   this->pimpl->setDbDialog(*this, newEngine);
   testRequired();
   return;
}

void OptionDialog::testConnection() {
   bool success;
   QString hostname, schema, database, username, password;
   int port;

   Database::DbType newType = static_cast<Database::DbType>(comboBox_engine->currentData().toInt());
   // Do nothing if nothing is required.
   if (this->pimpl->dbConnectionTestState == NO_CHANGE || this->pimpl->dbConnectionTestState == TEST_PASSED) {
      return;
   }

   switch (newType) {
      case Database::PGSQL:
         hostname = this->pimpl->input_pgHostname.text();
         schema   = this->pimpl->input_pgSchema.text();
         database = this->pimpl->input_pgDbName.text();
         username = this->pimpl->input_pgUsername.text();
         password = this->pimpl->input_pgPassword.text();
         port     = this->pimpl->input_pgPortNum.text().toInt();

         success = Database::verifyDbConnection(newType, hostname, port, schema, database, username, password);
         break;
      default:
         hostname = QString("%1/%2").arg(this->pimpl->input_userDataDir.text()).arg("database.sqlite");
         success = Database::verifyDbConnection(newType, hostname);
   }

   if (success) {
      QMessageBox::information(nullptr,
                               QObject::tr("Connection Test"),
                               QString(QObject::tr("Connection to database was successful"))
                              );
      this->pimpl->dbConnectionTestState = TEST_PASSED;
   } else {
      // Database::testConnection already popped the dialog
      this->pimpl->dbConnectionTestState = TEST_FAILED;
   }
   this->pimpl->changeColors(*this);
   return;
}

void OptionDialog::testRequired() {
   this->pimpl->dbConnectionTestState = NEEDS_TEST;
   this->pimpl->changeColors(*this);
   return;
}


void OptionDialog::savePassword(bool state) {
   if (state) {
      QMessageBox::warning(nullptr, QObject::tr("Plaintext"),
                           QObject::tr("Passwords are saved in plaintext. We make no effort to hide, obscure or otherwise protect the password. By enabling this option, you take full responsibility for any potential problems."));
   }
   return;
}

void OptionDialog::setFileLocationState(bool state) {
   this->lineEdit_LogFileLocation->setEnabled(! state);
   this->pushButton_LogFileLocationBrowse->setEnabled(! state);
   return;
}

void OptionDialog::versioningChanged(bool state) {
   groupBox_deleteBehavior->setEnabled(state);
}

void OptionDialog::saveAndClose() {
   saveDatabaseConfig();
   saveDefaultUnits();
   saveFormulae();
   saveLoggingSettings();
   saveVersioningSettings();

   // Set the right language.
   Brewken::setLanguage(this->comboBox_lang->currentData().toString());

   setVisible(false);
}

bool OptionDialog::saveDefaultUnits() {
   bool okay = true;

   okay &= saveWeightUnits();
   okay &= saveTemperatureUnits();
   okay &= saveVolumeUnits();
   okay &= saveGravityUnits();
   okay &= saveDateFormat();
   okay &= saveColorUnits();
   okay &= saveDiastaticUnits();

   return okay;
}

void OptionDialog::saveFormulae() {
   bool okay = false;

   int ndx = ibuFormulaComboBox->itemData(ibuFormulaComboBox->currentIndex()).toInt(&okay);
   IbuMethods::ibuFormula = static_cast<IbuMethods::IbuType>(ndx);
   ndx = colorFormulaComboBox->itemData(colorFormulaComboBox->currentIndex()).toInt(&okay);
   Brewken::colorFormula = static_cast<Brewken::ColorType>(ndx);

   PersistentSettings::insert(PersistentSettings::Names::mashHopAdjustment, ibuAdjustmentMashHopDoubleSpinBox->value() / 100);
   PersistentSettings::insert(PersistentSettings::Names::firstWortHopAdjustment, ibuAdjustmentFirstWortDoubleSpinBox->value() / 100);
}

void OptionDialog::saveLoggingSettings() {
   // Saving Logging Options to the Log object
   Logging::setLogLevel(static_cast<Logging::Level>(loggingLevelComboBox->currentData().toInt()));
   Logging::setDirectory(
      checkBox_LogFileLocationUseDefault->isChecked() ?
      std::optional<QDir>(std::nullopt) : std::optional<QDir>(lineEdit_LogFileLocation->text())
   );
   // Make sure the main window updates.
   if (Brewken::mainWindow()) {
      Brewken::mainWindow()->showChanges();
   }
}

void OptionDialog::saveVersioningSettings() {
   // Save versioning options
   if (checkBox_versioning->checkState() == Qt::Checked) {
      RecipeHelper::setAutomaticVersioningEnabled(true);
      if (radioButton_deleteAncestor->isChecked()) {
         PersistentSettings::insert(PersistentSettings::Names::deletewhat, Recipe::ANCESTOR);
      } else {
         PersistentSettings::insert(PersistentSettings::Names::deletewhat, Recipe::DESCENDANT);
      }
   } else {
      // the default when versioning is off is to only delete descendant
      RecipeHelper::setAutomaticVersioningEnabled(false);
      PersistentSettings::insert(PersistentSettings::Names::deletewhat, Recipe::DESCENDANT);
   }

   PersistentSettings::insert(PersistentSettings::Names::showsnapshots, checkBox_alwaysShowSnaps->checkState() == Qt::Checked);

}

bool OptionDialog::saveDatabaseConfig() {

   bool saveDbConfig = true;

   // TODO:: FIX THIS UI. I am really not sure what the best approach is here.
   if (this->pimpl->dbConnectionTestState == NEEDS_TEST || this->pimpl->dbConnectionTestState == TEST_FAILED) {
      QMessageBox::critical(nullptr,
                            tr("Test connection or cancel"),
                            tr("Saving the options without testing the connection can cause Brewken to not restart. Your changes have been discarded, which is likely really, really crappy UX. Please open a bug explaining exactly how you got to this message.")
                           );
      return false;
   }

   // ask the user if they want to transfer data
   if (this->pimpl->dbConnectionTestState == TEST_PASSED) {
      saveDbConfig = transferDatabase();
   }

   if (saveDbConfig && this->pimpl->checkBox_savePgPassword.checkState() == Qt::Checked) {
      PersistentSettings::insert(PersistentSettings::Names::dbPassword, this->pimpl->input_pgPassword.text());
   } else {
      PersistentSettings::remove(PersistentSettings::Names::dbPassword);
   }

   Database::DbType dbEngine = static_cast<Database::DbType>(comboBox_engine->currentData().toInt());
   if (dbEngine == Database::SQLITE) {
      saveSqliteConfig();
   }

   return saveDbConfig;
}

bool OptionDialog::transferDatabase() {
   bool success = true;
   // This got unpleasant. There are multiple possible transfer paths.
   // SQLite->Pgsql, Pgsql->Pgsql and Pgsql->SQLite. This will ensure we
   // preserve the information required.
   try {
      QString theQuestion =
         tr("Would you like Brewken to transfer your data to the new database? NOTE: If you've already loaded the data, say No");
      if (QMessageBox::Yes == QMessageBox::question(this, tr("Transfer database"), theQuestion)) {
         Database::instance().convertDatabase(this->pimpl->input_pgHostname.text(),
                                              this->pimpl->input_pgDbName.text(),
                                              this->pimpl->input_pgUsername.text(),
                                              this->pimpl->input_pgPassword.text(),
                                              this->pimpl->input_pgPortNum.text().toInt(),
                                              static_cast<Database::DbType>(this->comboBox_engine->currentData().toInt()));
      }
      // Database engine stuff
      int engine = comboBox_engine->currentData().toInt();
      PersistentSettings::insert(PersistentSettings::Names::dbType, engine);
      // only write these changes when switching TO pgsql
      if (engine == Database::PGSQL) {
         PersistentSettings::insert(PersistentSettings::Names::dbHostname, this->pimpl->input_pgHostname.text());
         PersistentSettings::insert(PersistentSettings::Names::dbPortnum,  this->pimpl->input_pgPortNum.text());
         PersistentSettings::insert(PersistentSettings::Names::dbSchema,   this->pimpl->input_pgSchema.text());
         PersistentSettings::insert(PersistentSettings::Names::dbName,     this->pimpl->input_pgDbName.text());
         PersistentSettings::insert(PersistentSettings::Names::dbUsername, this->pimpl->input_pgUsername.text());
      }
      QMessageBox::information(this, tr("Restart"), tr("Please restart Brewken to connect to the new database"));
   } catch (QString e) {
      qCritical() << Q_FUNC_INFO << e;
      success = false;
   }
   return success;
}

void OptionDialog::saveSqliteConfig() {
   // Check the new userDataDir.
   QString newUserDataDir = this->pimpl->input_userDataDir.text();
   QDir userDirectory(newUserDataDir);

   // I think this is redundant and could be handled as just a simple db
   // transfer using the TEST_PASSED loop above.
   if (userDirectory != PersistentSettings::getUserDataDir()) {
      // If there are no data files present...
      if (! QFileInfo(userDirectory, "database.sqlite").exists()) {
         // ...tell user we will copy old data files to new location.
         QMessageBox::information(this,
                                  tr("Copy Data"),
                                  tr("There do not seem to be any data files in this directory, so we will copy your old data here.")
                                 );
         Brewken::copyDataFiles(newUserDataDir);
      }

      PersistentSettings::setUserDataDir(newUserDataDir);
      QMessageBox::information(
         this,
         tr("Restart"),
         tr("Please restart Brewken.")
      );
   }

   PersistentSettings::insert(PersistentSettings::Names::maximum,   this->pimpl->spinBox_numBackups.value(), PersistentSettings::Sections::backups);
   PersistentSettings::insert(PersistentSettings::Names::frequency, this->pimpl->spinBox_frequency.value(),  PersistentSettings::Sections::backups);
   PersistentSettings::insert(PersistentSettings::Names::directory, this->pimpl->input_backupDir.text(),     PersistentSettings::Sections::backups);

   return;
}

bool OptionDialog::saveWeightUnits() {
   bool okay = false;
   switch (weightComboBox->itemData(weightComboBox->currentIndex()).toInt(&okay)) {
      case SI:
      default:
         Brewken::weightUnitSystem = SI;
         Brewken::thingToUnitSystem.insert(Unit::Mass, &UnitSystems::siWeightUnitSystem);
         break;
      case USCustomary:
         Brewken::weightUnitSystem  = USCustomary;
         Brewken::thingToUnitSystem.insert(Unit::Mass, &UnitSystems::usWeightUnitSystem);
         break;
      case Imperial:
         Brewken::weightUnitSystem  = Imperial;
         Brewken::thingToUnitSystem.insert(Unit::Mass, &UnitSystems::usWeightUnitSystem);
         break;
   }
   return okay;
}

bool OptionDialog::saveTemperatureUnits() {
   bool okay = false;
   switch (temperatureComboBox->itemData(temperatureComboBox->currentIndex()).toInt(&okay)) {
      case Celsius:
      default:
         Brewken::tempScale = Celsius;
         Brewken::thingToUnitSystem.insert(Unit::Temp, &UnitSystems::celsiusTempUnitSystem);
         break;
      case Fahrenheit:
         Brewken::tempScale = Fahrenheit;
         Brewken::thingToUnitSystem.insert(Unit::Temp, &UnitSystems::fahrenheitTempUnitSystem);
         break;
   }
   return okay;
}

bool OptionDialog::saveVolumeUnits() {
   bool okay = false;
   switch (volumeComboBox->itemData(volumeComboBox->currentIndex()).toInt(&okay)) {
      case SI:
      default:
         Brewken::volumeUnitSystem = SI;
         Brewken::thingToUnitSystem.insert(Unit::Volume, &UnitSystems::siVolumeUnitSystem);
         break;
      case USCustomary:
         Brewken::volumeUnitSystem = USCustomary;
         Brewken::thingToUnitSystem.insert(Unit::Volume, &UnitSystems::usVolumeUnitSystem);
         break;
      case Imperial:
         Brewken::volumeUnitSystem = Imperial;
         Brewken::thingToUnitSystem.insert(Unit::Volume, &UnitSystems::imperialVolumeUnitSystem);
         break;
   }
   return okay;
}

bool OptionDialog::saveGravityUnits() {
   bool okay = false;
   switch (gravityComboBox->itemData(gravityComboBox->currentIndex()).toInt(&okay)) {
      case Brewken::SG:
      default:
         Brewken::densityUnit = Brewken::SG;
         Brewken::thingToUnitSystem.insert(Unit::Density, &UnitSystems::sgDensityUnitSystem);
         break;
      case Brewken::PLATO:
         Brewken::densityUnit = Brewken::PLATO;
         Brewken::thingToUnitSystem.insert(Unit::Density, &UnitSystems::platoDensityUnitSystem);
         break;
   }
   return okay;
}

bool OptionDialog::saveDateFormat() {
   bool okay = false;
   switch (dateComboBox->itemData(dateComboBox->currentIndex()).toInt(&okay)) {
      case Unit::displayUS:
      default:
         Brewken::dateFormat = Unit::displayUS;
         break;
      case Unit::displayImp:
         Brewken::dateFormat = Unit::displayImp;
         break;
      case Unit::displaySI:
         Brewken::dateFormat = Unit::displaySI;
         break;
   }
   return okay;
}

bool OptionDialog::saveColorUnits() {
   bool okay = false;
   switch (colorComboBox->itemData(colorComboBox->currentIndex()).toInt(&okay)) {
      case Brewken::SRM:
      default:
         Brewken::thingToUnitSystem.insert(Unit::Color, &UnitSystems::srmColorUnitSystem);
         Brewken::colorUnit = Brewken::SRM;
         break;
      case Brewken::EBC:
         Brewken::thingToUnitSystem.insert(Unit::Color, &UnitSystems::ebcColorUnitSystem);
         Brewken::colorUnit = Brewken::EBC;
         break;
   }
   return okay;
}

bool OptionDialog::saveDiastaticUnits() {
   bool okay = false;
   switch (diastaticPowerComboBox->itemData(diastaticPowerComboBox->currentIndex()).toInt(&okay)) {
      case Brewken::LINTNER:
      default:
         Brewken::thingToUnitSystem.insert(Unit::DiastaticPower, &UnitSystems::lintnerDiastaticPowerUnitSystem);
         Brewken::diastaticPowerUnit = Brewken::LINTNER;
         break;
      case Brewken::WK:
         Brewken::thingToUnitSystem.insert(Unit::DiastaticPower, &UnitSystems::wkDiastaticPowerUnitSystem);
         Brewken::diastaticPowerUnit = Brewken::WK;
         break;
   }

   return okay;
}
