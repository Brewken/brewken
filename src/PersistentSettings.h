/**
 * PersistentSettings.h is part of Brewken, and is copyright the following authors 2021:
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef PERSISTENTSETTINGS_H
#define PERSISTENTSETTINGS_H
#pragma once

#include <QString>
#include <QVariant>


/**
 * \brief Functions that manage remembering settings across sessions.
 *        Most of the heavy lifting is done by Qt's QSettings class.  We just add some minor extensions.
 */
namespace PersistentSettings {

   //! \brief Controls how units and scales are stored in the options file
   enum iUnitOps {
      NOOP = -1 ,
      SCALE,
      UNIT
   };

   bool hasOption(QString attribute, const QString section = QString(), iUnitOps ops = NOOP);
   void setOption(QString attribute, QVariant value, const QString section = QString(), iUnitOps ops = NOOP);
   QVariant option(QString attribute, QVariant default_value = QVariant(), QString section = QString(), iUnitOps = NOOP);
   void removeOption(QString attribute, QString section=QString());

}
#endif
