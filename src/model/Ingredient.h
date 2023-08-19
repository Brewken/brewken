/*======================================================================================================================
 * model/Ingredient.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_INGREDIENT_H
#define MODEL_INGREDIENT_H
#pragma once

#include "model/NamedEntity.h"
#include "utils/EnumStringMapping.h"

class NamedParameterBundle;

/**
 * \brief Subclasses of this class are actual ingredients in a recipe (eg \c Hop, \c Fermentable).
 *
 *        Ingredients are the objects for which we keep inventory.
 */
class Ingredient : public NamedEntity {
   Q_OBJECT

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

///@{
   /**
    * \brief Flag values for the different ways-of-measurement that \c IngredientAmount can support
    *
    *        It would have been nice to use Qt's QFlags class and associated macros here for type safety, but they can't
    *        be used as template parameters, which is what we want to do in \c IngredientAmount.
    */
   typedef std::uint8_t Measures;
   enum class Measure {
      Mass_Kilograms = 1 << 0,   // 🡲 canonical units of Measurement::PhysicalQuantity::Mass   = Measurement::Units::kilograms
      Volume_Liters  = 1 << 1,   // 🡲 canonical units of Measurement::PhysicalQuantity::Volume = Measurement::Units::liters
      Count          = 1 << 2,   // 🡲 NonPhysicalQuantity::Count
   };
///@}

   //
   // This allows us to store the IngredientAmount::Measure enum class in a QVariant
   //
   Q_ENUM(Measure)

   /*!
    * \brief Mapping between \c Ingredient::Measure and string values suitable for serialisation in DB
    */
   static FlagEnumStringMapping const measureStringMapping;

   /*!
    * \brief Localised names of \c Ingredient::Form values suitable for displaying to the end user
    */
   static FlagEnumStringMapping const measureDisplayNames;

///   /**
///    * \return First set flag in the supplied parameter
///    */
///   static Measure extractFirstSet(Measures const measures) const;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   Ingredient(QString name = "");
   Ingredient(NamedParameterBundle const & namedParameterBundle);
   Ingredient(Ingredient const & other);

   virtual ~Ingredient();

};

constexpr Ingredient::Measures operator|(Ingredient::Measure const lhs,
                                         Ingredient::Measure const rhs) {
   return static_cast<Ingredient::Measures>(lhs) |
          static_cast<Ingredient::Measures>(rhs);
}

constexpr Ingredient::Measures operator&(Ingredient::Measure  const lhs,
                                         Ingredient::Measures const rhs) {
   return static_cast<Ingredient::Measures>(lhs) & rhs;
}

constexpr Ingredient::Measures operator&(Ingredient::Measures const lhs,
                                         Ingredient::Measure  const rhs) {
   return lhs & static_cast<Ingredient::Measures>(rhs);
}



#endif
