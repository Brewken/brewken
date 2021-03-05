/**
 * PersistentSettings.cpp is part of Brewken, and is copyright the following authors 2021:
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
#include "PersistentSettings.h"

#include <QSettings>

namespace {
   QString generateName(QString attribute, const QString section, PersistentSettings::iUnitOps ops) {

      if ( section.isNull() ) {
         return attribute;
      }

      QString ret = QString("%1/%2").arg(section).arg(attribute);

      if ( ops != PersistentSettings::NOOP ) {
         ret += ops == PersistentSettings::UNIT ? "_unit" : "_scale";
      }
      return ret;
   }

}

bool PersistentSettings::hasOption(QString attribute, const QString section, PersistentSettings::iUnitOps ops) {
   QString name{generateName(attribute, section, ops)};
   return QSettings().contains(name);
}

void PersistentSettings::setOption(QString attribute, QVariant value, const QString section, PersistentSettings::iUnitOps ops) {
   QString name{generateName(attribute, section, ops)};
   QSettings().setValue(name,value);
   return;
}

QVariant PersistentSettings::option(QString attribute, QVariant default_value, QString section, PersistentSettings::iUnitOps ops) {
   QString name{generateName(attribute, section, ops)};
   return QSettings().value(name,default_value);
}

void PersistentSettings::removeOption(QString attribute, QString section) {
   QString name{generateName(attribute, section, PersistentSettings::NOOP)};

   if ( PersistentSettings::hasOption(name) ) {
      QSettings().remove(name);
   }
   return;
}
