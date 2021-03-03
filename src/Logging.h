/**
 * Logging.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
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
#ifndef LOGGING_H
#define LOGGING_H
#pragma once

#include <QDir>
#include <QFileInfoList>
#include <QString>
#include <QVector>

/*!
 * \brief Provides a proxy to an OS agnostic log file.
 */
namespace Logging {
   /**
    * \brief Defines the importance of an individual message and used to controls what type of messages to log
    *
    *        This is similar to QtMsgType except we need the numeric order of these levels to match the "logical
    *        order", which is not the case with QtMsgType (because QtInfoMsg was a later addition).  What we mean
    *        by the "logical order" is that the higher the level number, the more urgent/important the message is.
    *
    *        Thus, if logging level is set to LogLevel_WARNING then only messages of LogLevel_WARNING and
    *        LogLevel_ERROR will be logged.
    */
   enum Level {
      //! Message about the inner workings of the application.  Mainly used during development or debugging.  End users
      //  shouldn't normally need to see these messages.
      LogLevel_DEBUG,
      //! An FYI message that an end user can safely ignore but that might be useful to understand what the app has
      //  done or to diagnose a bug.  This is the default logging level.
      LogLevel_INFO,
      //! This is something that might be a problem and is almost certainly good to know when diagnosing problems.
      LogLevel_WARNING,
      //! Something that is definitely an error and that we always want to log
      LogLevel_ERROR
   };

   /**
    * \brief User-friendly info about logging levels.  Although we use an enum internally to identify a logging level,
    *        we also need:
    *           - A string name to record the level in the log messages themselves and to use in the config file
    *           - A description to show the user on the Options dialog
    */
   struct LevelDetail {
      Level level;
      char const * name;
      QString description;
   };
   extern QVector<LevelDetail> const levelDetails;

   /**
    * \brief Convert logging level to a string representation
    */
   extern QString getStringFromLogLevel(const Level type);

   /**
    * \brief Convert a string representation of a logging level to a logging level
    */
   extern Level getLogLevelFromString(QString const level = QString("INFO"));

   /**
    * Current logging level
    */
   extern Level logLevel;
   extern bool logUseConfigDir;
   extern int const logFileSize;
   extern int const logFileCount;

   /**
    * \brief Sets the directory in which log files are stored
    * \param newDirectory
    * \return true if succeeds, false otherwise
    */
   extern bool setDirectory(QDir newDirectory);

   /**
    * \brief Gets the directory in which log files are stored
    * \return The directory
    */
   extern QDir getDirectory();

   /**
    * \brief  Initialize logging to utilize the built in logging functionality in QT5
    *         This has to be called before any logging is done.  Should be self contained and not depend on anything
    *         being loaded.  Although user settings may alter location of files, this module will always start logging
    *         at the default application data path i.e. on linux: ~/.config/brewken or on Windows %APPDATA% path.
    * \return
    */
   extern bool initializeLogging();

   /**
    * \brief  Get the list of Logfiles present in the directory currently logging in
    */
   extern QFileInfoList getLogFileList();

   /**
    * \brief Terminate logging
    */
   extern void terminateLogging();
}

#endif
