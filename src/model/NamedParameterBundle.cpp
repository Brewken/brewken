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
#include <sstream>

#include <boost/stacktrace.hpp>

#include <QDebug>
#include <QString>
#include <QTextStream>

namespace {
   template <class T> T valueFromQVariant(QVariant const & qv);
   template <> QString valueFromQVariant(QVariant const & qv) {return qv.toString();}
   template <> bool    valueFromQVariant(QVariant const & qv) {return qv.toBool();}
   template <> int     valueFromQVariant(QVariant const & qv) {return qv.toInt();}
   template <> double  valueFromQVariant(QVariant const & qv) {return qv.toDouble();}
}

NamedParameterBundle::NamedParameterBundle(NamedParameterBundle::OperationMode mode) :
   QHash<char const * const, QVariant>(), mode{mode} {
   return;
}

NamedParameterBundle::~NamedParameterBundle() = default;


QVariant NamedParameterBundle::operator()(char const * const parameterName) const {
   if (!this->contains(parameterName)) {
      QString errorMessage = QString("No value supplied for required parameter, %1").arg(parameterName);
      if (this->mode == NamedParameterBundle::Strict) {
         //
         // We want to throw an exception here because it's a lot less code than checking a return value on every call
         // and, usually, missing required parameter is a coding error.
         //
         // Qt doesn't have its own exceptions, so we use a C++ Standard Library one, which in turn means using
         // std::string.
         //
         // C++ exceptions don't give you a stack trace by default, so we use Boost to include one, as that's going to
         // be pretty helpful in debugging.
         //
         std::ostringstream stacktrace;
         stacktrace << boost::stacktrace::stacktrace();
         QTextStream errorMessageAsStream(&errorMessage);
         errorMessageAsStream << "\nStacktrace:\n" << QString::fromStdString(stacktrace.str());

         qCritical().noquote() << Q_FUNC_INFO << errorMessage;
         throw std::invalid_argument(errorMessage.toStdString());
      }
      // In non-strict mode we'll just construct an empty QVariant and return that in the hope that its default value
      // (eg 0 for a numeric type, empty string for a QString) is OK.
      qInfo() << Q_FUNC_INFO << errorMessage << ", so using generic default";
      return QVariant{};
   }
   QVariant returnValue = this->value(parameterName);
   if (!returnValue.isValid()) {
      QString errorMessage = QString("Invalid value (%1) supplied for required parameter, %2").arg(returnValue.toString()).arg(parameterName);
      qCritical() << Q_FUNC_INFO << errorMessage;
      throw std::invalid_argument(errorMessage.toStdString());
   }
   return returnValue;
}

template <class T> T NamedParameterBundle::operator()(char const * const parameterName, T const & defaultValue) const {
   return this->contains(parameterName) ? valueFromQVariant<T>(this->value(parameterName)) : defaultValue;
}

//
// Instantiate the above template function for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header.)
//
template QString    NamedParameterBundle::operator()(char const * const parameterName, QString const & defaultValue) const;
template bool       NamedParameterBundle::operator()(char const * const parameterName, bool    const & defaultValue) const;
template int        NamedParameterBundle::operator()(char const * const parameterName, int     const & defaultValue) const;
template double     NamedParameterBundle::operator()(char const * const parameterName, double  const & defaultValue) const;
