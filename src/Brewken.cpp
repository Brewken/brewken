/*======================================================================================================================
 * Brewken.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • A.J. Drobnich <aj.drobnich@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Chris Pavetto <chrispavetto@gmail.com>
 *   • Dan Cavanagh <dan@dancavanagh.com>
 *   • Daniel Moreno <danielm5@users.noreply.github.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Greg Meess <Daedalus12@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Medic Momcilo <medicmomcilo@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Mikhail Gorbunov <mikhail@sirena2000.ru>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Rob Taylor <robtaylor@floopily.org>
 *   • Scott Peshak <scott@peshak.net>
 *   • Ted Wright <tedwright@users.sourceforge.net>
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
 =====================================================================================================================*/
#include "Brewken.h"

#include <iostream>

#include <QDebug>
#include <QDesktopServices>
#include <QEventLoop>
#include <QFile>
#include <QIODevice>
#include <QLibraryInfo>
#include <QLocale>
#include <QMessageBox>
#include <QObject>
#include <QPixmap>
#include <QSettings>
#include <QSharedPointer>
#include <QSplashScreen>
#include <QString>
#include <QTextStream>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QUrl>

#include "Algorithms.h"
#include "BtSplashScreen.h"
#include "config.h"
#include "database/Database.h"
#include "database/ObjectStoreWrapper.h"
#include "Localization.h"
#include "MainWindow.h"
#include "measurement/ColorMethods.h"
#include "measurement/IbuMethods.h"
#include "measurement/Measurement.h"
#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Instruction.h"
#include "model/Mash.h"
#include "model/Salt.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"
#include "PersistentSettings.h"

// Needed for kill(2)
#if defined(Q_OS_UNIX)
#include <sys/types.h>
#include <signal.h>
#endif

namespace {
   /**
    * \brief Create a directory if it doesn't exist, popping a error dialog if creation fails
    */
   bool createDir(QDir dir) {
      if( ! dir.mkpath(dir.absolutePath()) ) {
         // Write a message to the log, the usablity check below will alert the user
         QString errText(QObject::tr("Error attempting to create directory \"%1\""));
         qCritical() << errText.arg(dir.path());
      }

      // It's possible that the path exists, but is useless to us
      if( ! dir.exists() || ! dir.isReadable() ) {

         QString errText{QObject::tr("\"%1\" cannot be read.")};
         qWarning() << errText.arg(dir.path());
         if (Brewken::isInteractive()) {
            QString errTitle(QObject::tr("Directory Problem"));
            QMessageBox::information(
               nullptr,
               errTitle,
               errText.arg(dir.path())
            );
         }
         return false;
      }

      return true;
   }

   /**
    * \brief Ensure our directories exist.
    */
   bool ensureDirectoriesExist() {
      // A missing resource directory is a serious issue, without it we're missing the default DB, sound files &
      // translations.  We could attempt to create it, like the other config/data directories, but an empty resource
      // dir is just as bad as a missing one.  So, instead, we'll display a little more dire warning, and not try to
      // create it.
      QDir resourceDir = Brewken::getResourceDir();
      bool resourceDirSuccess = resourceDir.exists();
      if (!resourceDirSuccess) {
         QString errMsg{
            QObject::tr("Resource directory \"%1\" is missing.  Some features will be unavailable.").arg(resourceDir.path())
         };
         qCritical() << Q_FUNC_INFO << errMsg;

         if (Brewken::isInteractive()) {
            QMessageBox::critical(
               nullptr,
               QObject::tr("Directory Problem"),
               errMsg
            );
         }
      }

      return resourceDirSuccess &&
             createDir(PersistentSettings::getConfigDir()) &&
             createDir(PersistentSettings::getUserDataDir());
   }

}

MainWindow* Brewken::m_mainWindow = nullptr;
QDomDocument* Brewken::optionsDoc;
bool Brewken::_isInteractive = true;

bool Brewken::checkVersion = true;

// .:TODO:. This needs to be updated
void Brewken::checkForNewVersion(MainWindow* mw)
{

   // Don't do anything if the checkVersion flag was set false
   if ( checkVersion == false )
      return;

   QNetworkAccessManager manager;
   QUrl url("https://github.com/Brewken/brewken/releases/latest");
   QNetworkReply* reply = manager.get( QNetworkRequest(url) );
   QObject::connect( reply, &QNetworkReply::finished, mw, &MainWindow::finishCheckingVersion );
}

bool Brewken::copyDataFiles(const QDir newPath)
{
   QString dbFileName = "database.sqlite";
   return QFile::copy(PersistentSettings::getUserDataDir().filePath(dbFileName), newPath.filePath(dbFileName));
}

QDir Brewken::getResourceDir() {
   // Unlike some of the other directories, the resource dir needs to be something that can be determined at
   // compile-time
   QString dir = qApp->applicationDirPath();
#if defined(Q_OS_LINUX) // Linux OS.

   dir = QString(CONFIGDATADIR);

#elif defined(Q_OS_MAC) // MAC OS.

   // We should be inside an app bundle.
   dir += "/../Resources/";

#elif defined(Q_OS_WIN) // Windows OS.

   dir += "/../data/";

#else
# error "Unsupported OS"
#endif

   if (!dir.endsWith('/')) {
      dir += "/";
   }

   return dir;
}

bool Brewken::initialize()
{
   // Need these for changed(QMetaProperty,QVariant) to be emitted across threads.
   qRegisterMetaType<QMetaProperty>();
   qRegisterMetaType<Equipment*>();
   qRegisterMetaType<Mash*>();
   qRegisterMetaType<Style*>();
   qRegisterMetaType<Salt*>();
   qRegisterMetaType< QList<BrewNote*> >();
   qRegisterMetaType< QList<Hop*> >();
   qRegisterMetaType< QList<Instruction*> >();
   qRegisterMetaType< QList<Fermentable*> >();
   qRegisterMetaType< QList<Misc*> >();
   qRegisterMetaType< QList<Yeast*> >();
   qRegisterMetaType< QList<Water*> >();
   qRegisterMetaType< QList<Salt*> >();

   // Make sure all the necessary directories and files we need exist before starting.
   ensureDirectoriesExist();

   readSystemOptions();

   loadMap();

   Localization::loadTranslations(); // Do internationalization.

#if defined(Q_OS_MAC)
   qt_set_sequence_auto_mnemonic(true); // turns on Mac Keyboard shortcuts
#endif

   // Check if the database was successfully loaded before
   // loading the main window.
   qDebug() << "Loading Database...";
   return Database::instance().loadSuccessful();
}

void Brewken::cleanup() {
   qDebug() << "Brewken is cleaning up.";
   // Should I do qApp->removeTranslator() first?
   delete m_mainWindow;

   Database::instance().unload();
   return;
}

bool Brewken::isInteractive()
{
   return _isInteractive;
}

void Brewken::setInteractive(bool val)
{
   _isInteractive = val;
}

int Brewken::run() {
   int ret = 0;

   BtSplashScreen splashScreen;
   splashScreen.show();
   qApp->processEvents();
   if( !initialize() )
   {
      cleanup();
      return 1;
   }
   qInfo() << QString("Starting Brewken v%1 on %2.").arg(VERSIONSTRING).arg(QSysInfo::prettyProductName());
   Database::instance().checkForNewDefaultData();
   m_mainWindow = new MainWindow();
   m_mainWindow->init();
   m_mainWindow->setVisible(true);
   splashScreen.finish(m_mainWindow);

   checkForNewVersion(m_mainWindow);
   do {
      ret = qApp->exec();
   } while (ret == 1000);

   cleanup();

   qDebug() << Q_FUNC_INFO << "Cleaned up.  Returning " << ret;

   return ret;
}

void Brewken::updateConfig() {
   int cVersion = PersistentSettings::value(PersistentSettings::Names::config_version, QVariant(0)).toInt();
   while ( cVersion < CONFIG_VERSION ) {
      switch ( ++cVersion ) {
         case 1:
            // Update the dbtype, because I had to increase the NODB value from -1 to 0
            int newType = static_cast<Database::DbType>(PersistentSettings::value(PersistentSettings::Names::dbType, Database::NODB).toInt() + 1);
            // Write that back to the config file
            PersistentSettings::insert(PersistentSettings::Names::dbType, static_cast<int>(newType));
            // and make sure we don't do it again.
            PersistentSettings::insert(PersistentSettings::Names::config_version, QVariant(cVersion));
            break;
      }
   }
   return;
}

void Brewken::readSystemOptions()
{
   QString text;

   // update the config file before we do anything
   updateConfig();

   //================Version Checking========================
   checkVersion = PersistentSettings::value(PersistentSettings::Names::check_version, QVariant(false)).toBool();

   //=====================Last DB Merge Request======================
   if (PersistentSettings::contains(PersistentSettings::Names::last_db_merge_req)) {
      Database::lastDbMergeRequest = QDateTime::fromString(PersistentSettings::value(PersistentSettings::Names::last_db_merge_req,"").toString(), Qt::ISODate);
   }

   Measurement::loadDisplayScales();

   //======================Time======================
   // Set the one and only time system.
///   Measurement::thingToUnitSystem.insert(Measurement::Unit::Time,&Measurement::time_CoordinatedUniversalTime);

   //===================IBU===================
   IbuMethods::loadIbuFormula();

   //========================Color Formula======================
   ColorMethods::loadColorFormulaSettings();

   //=======================Language & Date format===================
   Localization::loadSettings();

   return;

}

void Brewken::saveSystemOptions() {
   QString text;

   PersistentSettings::insert(PersistentSettings::Names::check_version, checkVersion);
   PersistentSettings::insert(PersistentSettings::Names::last_db_merge_req, Database::lastDbMergeRequest.toString(Qt::ISODate));
   //setOption("user_data_dir", userDataDir);

   Localization::saveSettings();

   IbuMethods::saveIbuFormula();

   ColorMethods::saveColorFormulaSettings();

   Measurement::saveDisplayScales();

   return;
}

// the defaults come from readSystemOptions. This just fleshes out the hash
// for later use.
void Brewken::loadMap()
{
/*   // ==== mass ====
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Mass | Measurement::Unit::displaySI, &Measurement::mass_Metric );
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Mass | Measurement::Unit::displayUS, &Measurement::mass_ImperialAndUsCustomary );
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Mass | Measurement::Unit::displayImp,&Measurement::mass_ImperialAndUsCustomary );

   // ==== volume ====
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Volume | Measurement::Unit::displaySI, &Measurement::volume_Metric );
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Volume | Measurement::Unit::displayUS, &Measurement::volume_UsCustomary );
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Volume | Measurement::Unit::displayImp,&Measurement::volume_Imperial );

   // ==== time is empty ==== (this zen moment was free)

   // ==== temp ====
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Temperature | Measurement::Unit::displaySI,&Measurement::temperature_MetricIsCelsius );
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Temperature | Measurement::Unit::displayUS,&Measurement::temperature_UsCustomaryIsFahrenheit );

   // ==== color ====
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Color | Measurement::Unit::displaySrm,&Measurement::color_StandardReferenceMethod );
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Color | Measurement::Unit::displayEbc,&Measurement::color_EuropeanBreweryConvention );

   // ==== density ====
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Density | Measurement::Unit::displaySg,   &Measurement::density_SpecificGravity );
   Measurement::thingToUnitSystem.insert(Measurement::Unit::Density | Measurement::Unit::displayPlato,&Measurement::density_Plato );

   // ==== diastatic power ====
   Measurement::thingToUnitSystem.insert(Measurement::Unit::DiastaticPower | Measurement::Unit::displayLintner,&Measurement::diastaticPower_Lintner );
   Measurement::thingToUnitSystem.insert(Measurement::Unit::DiastaticPower | Measurement::Unit::displayWK,&Measurement::diastaticPower_WindischKolbach );
   */
}



/*
QString Brewken::colorUnitName(Measurement::Unit::unitDisplay display)
{
   if ( display == Measurement::Unit::noUnit )
      display = getColorUnit();

   if ( display == Measurement::Unit::displaySrm )
      return QString("SRM");
   else
      return QString("EBC");
}

QString Brewken::diastaticPowerUnitName(Measurement::Unit::unitDisplay display)
{
   if ( display == Measurement::Unit::noUnit )
      display = getDiastaticPowerUnit();

   if ( display == Measurement::Unit::displayLintner )
      return QString("Lintner");
   else
      return QString("WK");
}
*/

/* TODO THESE NEED TO BE REINSTATED, BUT NOT NECESSARILY HERE
// These are used in at least two places. I hate cut'n'paste coding so I am
// putting them here.
// I use a QActionGroup to make sure only one button is ever selected at once.
// It allows me to cache the menus later and speeds the response time up.
QMenu* Brewken::setupColorMenu(QWidget* parent, Measurement::Unit::unitDisplay unit)
{
   QMenu* menu = new QMenu(parent);
   QActionGroup* qgrp = new QActionGroup(parent);

   generateAction(menu, tr("Default"), Measurement::Unit::noUnit, unit, qgrp);
   generateAction(menu, tr("EBC"), Measurement::Unit::displayEbc, unit, qgrp);
   generateAction(menu, tr("SRM"), Measurement::Unit::displaySrm, unit, qgrp);

   return menu;
}

QMenu* Brewken::setupDateMenu(QWidget* parent, Measurement::Unit::unitDisplay unit)
{
   QMenu* menu = new QMenu(parent);
   QActionGroup* qgrp = new QActionGroup(parent);

   generateAction(menu, tr("Default"),    Measurement::Unit::noUnit,     unit, qgrp);
   generateAction(menu, tr("YYYY-mm-dd"), Measurement::Unit::displaySI,  unit, qgrp);
   generateAction(menu, tr("dd-mm-YYYY"), Measurement::Unit::displayImp, unit, qgrp);
   generateAction(menu, tr("mm-dd-YYYY"), Measurement::Unit::displayUS,  unit, qgrp);

   return menu;
}

QMenu* Brewken::setupDensityMenu(QWidget* parent, Measurement::Unit::unitDisplay unit)
{
   QMenu* menu = new QMenu(parent);
   QActionGroup* qgrp = new QActionGroup(parent);

   generateAction(menu, tr("Default"), Measurement::Unit::noUnit, unit, qgrp);
   generateAction(menu, tr("Plato"), Measurement::Unit::displayPlato, unit, qgrp);
   generateAction(menu, tr("Specific Gravity"), Measurement::Unit::displaySg, unit, qgrp);

   return menu;
}

QMenu* Brewken::setupMassMenu(QWidget* parent, Measurement::Unit::unitDisplay unit, Measurement::UnitSystem::RelativeScale scale, bool generateScale)
{
   QMenu* menu = new QMenu(parent);
   QMenu* sMenu;
   QActionGroup* qgrp = new QActionGroup(parent);

   generateAction(menu, tr("Default"), Measurement::Unit::noUnit, unit, qgrp);
   generateAction(menu, tr("SI"), Measurement::Unit::displaySI, unit, qgrp);
   generateAction(menu, tr("US Customary"), Measurement::Unit::displayUS, unit, qgrp);

   // Some places can't do scale -- like yeast tables and misc tables because
   // they can be mixed. It doesn't stop the unit selection from working, but
   // the scale menus don't make sense
   if ( generateScale == false )
      return menu;

   if ( unit == Measurement::Unit::noUnit )
   {
      if ( Measurement::thingToUnitSystem.value(Measurement::Unit::Mass) == &Measurement::mass_ImperialAndUsCustomary )
         unit = Measurement::Unit::displayUS;
      else
         unit = Measurement::Unit::displaySI;
   }

   sMenu = new QMenu(menu);
   QActionGroup* qsgrp = new QActionGroup(menu);
   switch(unit)
   {
      case Measurement::Unit::displaySI:
         generateAction(sMenu, tr("Default"), Measurement::UnitSystem::noScale, scale,qsgrp);
         generateAction(sMenu, tr("Milligrams"), Measurement::Unit::scaleExtraSmall, scale,qsgrp);
         generateAction(sMenu, tr("Grams"), Measurement::Unit::scaleSmall, scale,qsgrp);
         generateAction(sMenu, tr("Kilograms"), Measurement::Unit::scaleMedium, scale,qsgrp);
         break;
      default:
         generateAction(sMenu, tr("Default"), Measurement::UnitSystem::noScale, scale,qsgrp);
         generateAction(sMenu, tr("Ounces"), Measurement::Unit::scaleExtraSmall, scale,qsgrp);
         generateAction(sMenu, tr("Pounds"), Measurement::Unit::scaleSmall, scale,qsgrp);
         break;
   }
   sMenu->setTitle(tr("Scale"));
   menu->addMenu(sMenu);

   return menu;
}

QMenu* Brewken::setupTemperatureMenu(QWidget* parent, Measurement::Unit::unitDisplay unit)
{
   QMenu* menu = new QMenu(parent);
   QActionGroup* qgrp = new QActionGroup(parent);

   generateAction(menu, tr("Default"), Measurement::Unit::noUnit, unit, qgrp);
   generateAction(menu, tr("Celsius"), Measurement::Unit::displaySI, unit, qgrp);
   generateAction(menu, tr("Fahrenheit"), Measurement::Unit::displayUS, unit, qgrp);

   return menu;
}

// Time menus only have scale
QMenu* Brewken::setupTimeMenu(QWidget* parent, Measurement::UnitSystem::RelativeScale scale)
{
   QMenu* menu = new QMenu(parent);
   QMenu* sMenu = new QMenu(menu);
   QActionGroup* qgrp = new QActionGroup(parent);

   generateAction(sMenu, tr("Default"), Measurement::UnitSystem::noScale, scale, qgrp);
   generateAction(sMenu, tr("Seconds"), Measurement::Unit::scaleExtraSmall, scale, qgrp);
   generateAction(sMenu, tr("Minutes"), Measurement::Unit::scaleSmall, scale, qgrp);
   generateAction(sMenu, tr("Hours"),   Measurement::Unit::scaleMedium, scale, qgrp);
   generateAction(sMenu, tr("Days"),    Measurement::Unit::scaleLarge, scale, qgrp);

   sMenu->setTitle(tr("Scale"));
   menu->addMenu(sMenu);

   return menu;
}

QMenu* Brewken::setupVolumeMenu(QWidget* parent, Measurement::Unit::unitDisplay unit, Measurement::UnitSystem::RelativeScale scale, bool generateScale)
{
   QMenu* menu = new QMenu(parent);
   QActionGroup* qgrp = new QActionGroup(parent);
   QMenu* sMenu;

   generateAction(menu, tr("Default"), Measurement::Unit::noUnit, unit, qgrp);
   generateAction(menu, tr("SI"), Measurement::Unit::displaySI, unit, qgrp);
   generateAction(menu, tr("US Customary"), Measurement::Unit::displayUS, unit, qgrp);
   generateAction(menu, tr("British Imperial"), Measurement::Unit::displayImp, unit, qgrp);

   if ( generateScale == false )
      return menu;

   if ( unit == Measurement::Unit::noUnit )
   {
      if ( Measurement::thingToUnitSystem.value(Measurement::Unit::Volume) == &Measurement::volume_UsCustomary )
         unit = Measurement::Unit::displayUS;
      else if ( Measurement::thingToUnitSystem.value(Measurement::Unit::Volume) == &Measurement::volume_Imperial )
         unit = Measurement::Unit::displayImp;
      else
         unit = Measurement::Unit::displaySI;
   }


   sMenu = new QMenu(menu);
   QActionGroup* qsgrp = new QActionGroup(menu);
   switch(unit)
   {
      case Measurement::Unit::displaySI:
         generateAction(sMenu, tr("Default"), Measurement::UnitSystem::noScale, scale,qsgrp);
         generateAction(sMenu, tr("MilliLiters"), Measurement::Unit::scaleExtraSmall, scale,qsgrp);
         generateAction(sMenu, tr("Liters"), Measurement::Unit::scaleSmall, scale,qsgrp);
         break;
        // I can cheat because Imperial and US use the same names
      default:
         generateAction(sMenu, tr("Default"), Measurement::UnitSystem::noScale, scale,qsgrp);
         generateAction(sMenu, tr("Teaspoons"), Measurement::Unit::scaleExtraSmall, scale,qsgrp);
         generateAction(sMenu, tr("Tablespoons"), Measurement::Unit::scaleSmall, scale,qsgrp);
         generateAction(sMenu, tr("Cups"), Measurement::Unit::scaleMedium, scale,qsgrp);
         generateAction(sMenu, tr("Quarts"), Measurement::Unit::scaleLarge, scale,qsgrp);
         generateAction(sMenu, tr("Gallons"), Measurement::Unit::scaleExtraLarge, scale,qsgrp);
         generateAction(sMenu, tr("Barrels"), Measurement::Unit::scaleHuge, scale,qsgrp);
         break;
   }
   sMenu->setTitle(tr("Scale"));
   menu->addMenu(sMenu);

   return menu;
}

QMenu* Brewken::setupDiastaticPowerMenu(QWidget* parent, Measurement::Unit::unitDisplay unit)
{
   QMenu* menu = new QMenu(parent);
   QActionGroup* qgrp = new QActionGroup(parent);

   generateAction(menu, tr("Default"), Measurement::Unit::noUnit, unit, qgrp);
   generateAction(menu, tr("WK"), Measurement::Unit::displayWK, unit, qgrp);
   generateAction(menu, tr("Lintner"), Measurement::Unit::displayLintner, unit, qgrp);

   return menu;
}

void Brewken::generateAction(QMenu* menu, QString text, QVariant data, QVariant currentVal, QActionGroup* qgrp)
{
   QAction* action = new QAction(menu);

   action->setText(text);
   action->setData(data);
   action->setCheckable(true);
   action->setChecked(currentVal == data);;
   if ( qgrp )
      qgrp->addAction(action);

   menu->addAction(action);
   return;
}
*/
MainWindow* Brewken::mainWindow()
{
   return m_mainWindow;
}
