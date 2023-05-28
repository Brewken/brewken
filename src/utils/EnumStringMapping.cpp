/*======================================================================================================================
 * utils/EnumStringMapping.cpp is part of Brewken, and is copyright the following authors 2021-2023:
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
#include "utils/EnumStringMapping.h"

#include <algorithm>

EnumAndItsString::EnumAndItsString() :
   native{-1},
   string{""} {
   return;
}

EnumAndItsString::EnumAndItsString(int native, QString string) :
   native{native},
   string{string} {
   return;
}

EnumStringMapping::EnumStringMapping(std::initializer_list<EnumAndItsString> args) {
   this->reserve(args.size());
   for (auto arg : args) {
      // Uncomment this debug statement for debugging -- eg if you are hitting the assert at start-up!
//      qDebug().noquote() <<
//         Q_FUNC_INFO << "Inserting at" << arg.native << ". Size=" << this->size() <<
//         Logging::getStackTrace();
      // Essentially we are asserting here that args are passed in enum order and that our enum values always start from
      // 0 and never skip any numbers.  If we ever pass things in in the wrong order, we'll get an assert at start-up,
      // so it's pretty immediate feedback of the coding error.
      Q_ASSERT(arg.native == this->size());
      this->append(arg);
   }
   return;
}

std::optional<int> EnumStringMapping::stringToEnumAsInt(QString const & stringValue,
                                                        bool const caseInensitiveFallback) const {
   auto match = std::find_if(this->begin(),
                             this->end(),
                             [stringValue](EnumAndItsString const & ii){return stringValue == ii.string;});
   //
   // If we didn't find an exact match, we'll try a case-insensitive one if so-configured.  (We don't do this by
   // default as the assumption is that it's rare we'll need the case insensitivity.)
   //
   if (match == this->end() && caseInensitiveFallback) {
      match = std::find_if(
         this->begin(),
         this->end(),
         [stringValue](EnumAndItsString const & ii){return stringValue.toLower() == ii.string.toLower();}
      );
   }

   if (match == this->end()) {
      return std::nullopt;
   }

   return std::optional<int>{match->native};
}

std::optional<QString> EnumStringMapping::enumAsIntToString(int const enumValue) const {
   // So here's the advantage of forcing construction to be in enum order
   if (enumValue < 0 || enumValue >= this->size()) {
      return std::nullopt;
   }
   EnumAndItsString const & match = this->at(enumValue);
   Q_ASSERT(match.native == enumValue);

   return std::optional<QString>{match.string};
}
