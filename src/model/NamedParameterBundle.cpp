/**
 * model/NamedParameterBundle.cpp is part of Brewken, and is copyright the following authors 2021:
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
#include "model/NamedParameterBundle.h"

#include <string>
#include <stdexcept>

#include <QDebug>
#include <QString>

NamedParameterBundle::NamedParameterBundle() : QHash<char const * const, QVariant>() {
   return;
}

NamedParameterBundle::~NamedParameterBundle() = default;


QVariant NamedParameterBundle::operator()(char const * const parameterName) {
   if (!this->contains(parameterName)) {
      //
      // We want to throw an exception here because it's a lot less code than checking a return value on every call
      // and, usually, missing required parameter is a coding error.
      //
      // Qt doesn't have its own exceptions, so we use a C++ Standard Library one, which in turn means using
      // std::string.
      //
      QString errorMessage = QString("No value supplied for required parameter, %1").arg(parameterName);
      qCritical() << Q_FUNC_INFO << errorMessage;
      throw std::invalid_argument(errorMessage.toStdString());
   }
   QVariant returnValue = this->value(parameterName);
   if (!returnValue.isValid()) {
      QString errorMessage = QString("Invalid value (%1) supplied for required parameter, %2").arg(returnValue.toString()).arg(parameterName);
      qCritical() << Q_FUNC_INFO << errorMessage;
      throw std::invalid_argument(errorMessage.toStdString());
   }
   return returnValue;
}
