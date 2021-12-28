/*======================================================================================================================
 * Brewken.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Dan Cavanagh <dan@dancavanagh.com>
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Greg Meess <Daedalus12@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Rob Taylor <robtaylor@floopily.org>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Scott Peshak <scott@peshak.net>
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
#ifndef BREWKEN_H
#define BREWKEN_H
#pragma once

#define CONFIG_VERSION 1

/*
#define BTICON ":/images/brewken.svg"
//#define ICON96 ":/images/BrewkenIcon_96.png"
#define GLASS ":/images/glass2.png"
#define SMALLBARLEY ":/images/smallBarley.svg"
#define SMALLHOP ":/images/smallHop.svg"
#define SMALLWATER ":/images/smallWater.svg"
#define SMALLYEAST ":/images/smallYeast.svg"
#define SMALLKETTLE ":/images/smallKettle.svg"
#define SMALLQUESTION ":/images/smallQuestion.svg"
#define SMALLSTYLE ":/images/smallStyle.svg"
#define SMALLPLUS ":/images/smallPlus.svg"
#define SMALLMINUS ":/images/smallMinus.svg"
#define SMALLARROW ":/images/smallArrow.svg"
#define SMALLINFO ":/images/smallInfo.svg"
#define SMALLOUTARROW ":/images/smallOutArrow.svg"
#define SHRED ":/images/editshred.svg"
#define EXITPNG ":/images/exit.svg"
#define SAVEPNG ":/images/filesave.svg"
#define SAVEDIRTYPNG ":/images/filesavedirty.svg"
#define CLOCKPNG ":/images/clock.svg"
#define SOUND ":/images/sound.png"
#define STOP ":/images/alarm_stop.png"
*/

// need to use this to turn on Mac keyboard shortcuts (see https://doc.qt.io/qt-5/qkeysequence.html#qt_set_sequence_auto_mnemonic)
extern void qt_set_sequence_auto_mnemonic(bool b);

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QList>
#include <QMenu>
#include <QMetaProperty>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTextStream>
#include <QTranslator>

#include "Logging.h"
#include "measurement/UnitSystem.h"
#include "utils/BtStringConst.h"

class NamedEntity;
class MainWindow;

// Need these for changed(QMetaProperty,QVariant) to be emitted across threads.
Q_DECLARE_METATYPE( QMetaProperty )

/*!
 * \class Brewken
 *
 * \brief The main class. Figures out stuff from the system, formats things appropriately, handles translation, etc.
 *
 * TODO: Lots of things in this class belong elsewhere...
 *
 * .:TODO:. Find #includes of this file that are no longer needed
 */
class Brewken : public QObject
{
   Q_OBJECT
   Q_ENUMS(DbType)
   Q_ENUMS(delOptions)

   friend class OptionDialog;
   friend class RecipeFormatter;
   friend class Unit;

   friend class MainWindow;
   friend class Testing;

public:
   Brewken();

   /**
    * \return the resource directory where some files that ship with Brewken live (default DB, sounds, translations)
    *
    *         Most resources are compiled into the app with the Qt Resource System (see
    *         https://doc.qt.io/qt-5/resources.html) but, for some files, we want the user also to be able to access
    *         the file directly.  Such files are stored in this directory.
    */
   static QDir getResourceDir();

   /*!
    * \brief Blocking call that executes the application.
    * \param userDirectory If !isEmpty, overwrites the current settings.
    * \return Exit code from the application.
    */
   static int run();

   //! \brief Every so often, we need to update the config file itself. This does that.
   static void updateConfig();
   //! \brief Read options from options. This replaces readPersistentOptions()
   static void readSystemOptions();
   //! \brief Writes the persisten options back to the options store
   static void saveSystemOptions();

   // Grr. Shortcuts never, ever pay  off
/*   TODO REINSTATE THESE!
   static QMenu* setupColorMenu(QWidget* parent, Measurement::Unit::unitDisplay unit);
   static QMenu* setupDateMenu(QWidget* parent, Measurement::Unit::unitDisplay unit);
   static QMenu* setupDensityMenu(QWidget* parent, Measurement::Unit::unitDisplay unit);
   static QMenu* setupMassMenu(QWidget* parent, Measurement::Unit::unitDisplay unit, Measurement::UnitSystem::RelativeScale scale = Measurement::UnitSystem::noScale, bool generateScale = true);
   static QMenu* setupTemperatureMenu(QWidget* parent, Measurement::Unit::unitDisplay unit);
   static QMenu* setupVolumeMenu(QWidget* parent, Measurement::Unit::unitDisplay unit, Measurement::UnitSystem::RelativeScale scale = Measurement::UnitSystem::noScale, bool generateScale = true);
   static QMenu* setupDiastaticPowerMenu(QWidget* parent, Measurement::Unit::unitDisplay unit);
   static QMenu* setupTimeMenu(QWidget* parent, Measurement::UnitSystem::RelativeScale scale);
   static void generateAction(QMenu* menu, QString text, QVariant data, QVariant currentVal, QActionGroup* qgrp = nullptr);
*/
   //! \return the main window.
   // .:TODO:. Move this to MainWindow class!
   static MainWindow* mainWindow();

private:
   static MainWindow* m_mainWindow;
   static QDomDocument* optionsDoc;
   static QSettings btSettings;
   static QFile pidFile;
   static bool _isInteractive;

   //! \brief If this option is false, do not bother the user about new versions.
   static bool checkVersion;

   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   /*!
    * \brief Run before showing MainWindow, does all system setup.
    *
    * Creates a PID file, reads system options,
    * ensures the data directories and files exist, loads translations,
    * and loads database.
    *
    * \returns false if anything goes awry, true if it's ok to start MainWindow
    */
   static bool initialize();

   /*!
    * \brief Run after QApplication exits to clean up shit, close database, etc.
    */
   static void cleanup();

public:
   /*!
    * \brief If false, run Brewken in a way that requires no user interaction
    *
    * For example, if running a test case, ensure that no dialogs pop up that
    * prevent Brewken from starting
    */
   static bool isInteractive();
   //! \brief Set the mode to an interactive or non-interactive state
   static void setInteractive(bool val);

private:
   /*!
    *  \brief Copies the SQLite database file to another directory.
    *  \returns false iff the copy is unsuccessful.
    */
   static bool copyDataFiles(const QDir newPath);


   //! \brief Checks for a newer version and prompts user to download.
   static void checkForNewVersion(MainWindow* mw);

   static void loadMap();
};


/*!
 * \mainpage Brewken Source Code Documentation
 *
 * \section secIntro Introduction
 *
 * Brewken is a cross-platform open source beer recipe software suite.
 */

#endif
