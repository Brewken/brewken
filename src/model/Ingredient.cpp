/*======================================================================================================================
 * model/Ingredient.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "model/Ingredient.h"

#include "model/NamedParameterBundle.h"

QString const Ingredient::LocalisedName = tr("Ingredient");

// For measureStringMapping, we want the "in kilograms", "in liters" bit, because it makes data in the database
// unambiguous for a human being browsing it.  (Most of the time people aren't manually browsing the database, but we
// still want users of the program to be able, if they want, to see how their data is stored.
FlagEnumStringMapping const Ingredient::measureStringMapping {
   {Ingredient::Measure::Mass_Kilograms, "mass_in_kilograms"},
   {Ingredient::Measure::Volume_Liters , "volume_in_liters" },
   {Ingredient::Measure::Count         , "count"            },
};

// In contrast, for measureDisplayNames, we intentionally omit the "in kilograms", "in liters" bit because this is for
// the UI and, in that context, the user will have choice of units.  (Even though everything gets converted to canonical
// units for internal use, display and entry can be in any other units (for the same physical quantity) that the user
// prefers.)
FlagEnumStringMapping const Ingredient::measureDisplayNames {
   {Ingredient::Measure::Mass_Kilograms, tr("Mass"  )},
   {Ingredient::Measure::Volume_Liters , tr("Volume")},
   {Ingredient::Measure::Count         , tr("Count" )},
};

///Ingredient::Measure Ingredient::extractFirstSet(Ingredient::Measures const measures) const {
///   static const Measure measureList[] {
///      Measure::Mass_Kilograms,
///      Measure::Volume_Liters ,
///      Measure::Count         ,
///   };
///   for (auto measure : measureList) {
///      if (static_cast<Ingredient::Measures>(measure) & measures) {
///         return measure;
///      }
///   }
///
///   // It's a coding error if none of the Measure flags was set in measures
///   Q_ASSERT(false);
///
///   // But we have to return something here or the compiler will complain
///   return measureList[0];
///}

TypeLookup const Ingredient::typeLookup {
   "Ingredient",
   {
      // Empty list - for now at least
   },
   // Parent class lookup
   {&NamedEntity::typeLookup}
};

Ingredient::Ingredient(QString name) :
   NamedEntity{name, true} {
   return;
}

Ingredient::Ingredient(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity{namedParameterBundle} {
   return;
}

Ingredient::Ingredient(Ingredient const & other) :
   NamedEntity{other} {
   return;
}

Ingredient::~Ingredient() = default;
