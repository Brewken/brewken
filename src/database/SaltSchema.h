/**
 * database/SaltSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
 *   • Mik Firestone <mikfire@gmail.com>
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

#ifndef SALTSCHEMA_H
#define SALTSCHEMA_H

#include <QString>
// Columns for the salt table
// What isn't here (like name) is defined in TableSchemaConstants
static const QString kcolSaltType("stype");
static const QString kcolSaltAddTo("addTo");
static const QString kcolSaltAmtIsWgt("amount_is_weight");
static const QString kcolSaltPctAcid("percent_acid");
static const QString kcolSaltIsAcid("is_acid");

// properties for objects


// XML properties

#endif // SALTSCHEMA_H
