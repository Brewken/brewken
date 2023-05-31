/*======================================================================================================================
 * utils/MetaTypes.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "utils/MetaTypes.h"

void registerMetaTypes() {
   qRegisterMetaType<MassOrVolumeAmt                            >();
   qRegisterMetaType<std::optional<MassOrVolumeAmt>             >();
   qRegisterMetaType<MassOrVolumeConcentrationAmt               >();
   qRegisterMetaType<std::optional<MassOrVolumeConcentrationAmt>>();
   qRegisterMetaType<Measurement::Amount                        >();
   qRegisterMetaType<std::optional<Measurement::Amount>         >();

   return;
}