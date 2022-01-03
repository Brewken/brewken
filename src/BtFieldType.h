/*======================================================================================================================
 * BtFieldType.h is part of Brewken, and is copyright the following authors 2022:
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
#ifndef BTFIELDTYPE_H
#define BTFIELDTYPE_H
#pragma once

#include <variant>

#include "measurement/PhysicalQuantity.h"

/**
 * \brief The types of value other than \c Measurement::PhysicalQuantity that can be shown in a UI field
 *
 *        Note that there is intentionally \b no value here for \c none or similar.
 */
enum class NonPhysicalQuantity {
   Date,
   String,
   Count
};

/**
 * \brief All types of value that can be shown in a UI field
 */
typedef std::variant<Measurement::PhysicalQuantity, NonPhysicalQuantity> BtFieldType;

#endif
