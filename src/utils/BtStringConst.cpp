/*======================================================================================================================
 * utils/BtStringConst.cpp is part of Brewken, and is copyright the following authors 2021:
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
#include "utils/BtStringConst.h"

#include <cstring>

BtStringConst::BtStringConst(char const * const cString) : cString(cString) {
   return;
}

BtStringConst::BtStringConst(BtStringConst const &) = default;

BtStringConst::~BtStringConst() = default;

bool BtStringConst::operator==(BtStringConst const & rhs) const {
   if (this->cString == nullptr && rhs.cString == nullptr) { return true; }
   if (this->cString == nullptr || rhs.cString == nullptr) { return false; }
   return 0 == std::strcmp(this->cString, rhs.cString);
}

bool BtStringConst::operator!=(BtStringConst const & rhs) const {
   return !(*this == rhs);
}

bool BtStringConst::isNull() const {
   return (nullptr == this->cString);
}

char const * const BtStringConst::operator*() const {
   return this->cString;
}
