/*======================================================================================================================
 * json/JsonXPath.cpp is part of Brewken, and is copyright the following authors 2022:
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
 =====================================================================================================================*/
#include "json/JsonXPath.h"

#include <QDebug>
#include <QtGlobal> // For Q_ASSERT

// There's probably some optimal way to do this construction of "/" + parameter, but this will do for now at least
JsonXPath::JsonXPath(char const * const xPath) : valueAsJsonPointer{std::string{"/"}.append(xPath)} {
   return;
}

JsonXPath::~JsonXPath() = default;

std::string_view JsonXPath::asJsonPtr() const {
   return std::string_view{this->valueAsJsonPointer};
}

char const *  JsonXPath::asXPath_c_str() const {
//   qDebug() << Q_FUNC_INFO << "valueAsJsonPointer=" << this->valueAsJsonPointer.c_str();
   Q_ASSERT('/' == *this->valueAsJsonPointer.c_str());
   return this->valueAsJsonPointer.c_str() + 1;
}
