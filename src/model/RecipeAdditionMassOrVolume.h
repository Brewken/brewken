/*======================================================================================================================
 * model/RecipeAdditionMassOrVolume.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef MODEL_RECIPEADDITIONMASSORVOLUME_H
#define MODEL_RECIPEADDITIONMASSORVOLUME_H
#pragma once

#include "measurement/Amount.h"
#include "measurement/ConstrainedAmount.h"
#include "model/RecipeAddition.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::RecipeAdditionMassOrVolume { BtStringConst const property{#property}; }
AddPropertyName(amount                   )
AddPropertyName(amountIsWeight           )
AddPropertyName(amountWithUnits          )
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/**
 * \brief Extends \c RecipeAddition for ammounts that can be either mass or volume (\c RecipeAdditionHop and
 *        \c RecipeAdditionFermentable)
 *
 * DEPRECATED - USE IngredientAmount instead
 */
class RecipeAdditionMassOrVolume : public RecipeAddition {
   Q_OBJECT

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   RecipeAdditionMassOrVolume(QString name = "", int const recipeId = -1, int const ingredientId = -1);
   RecipeAdditionMassOrVolume(RecipeAdditionMassOrVolume const & other);
   RecipeAdditionMassOrVolume(NamedParameterBundle const & namedParameterBundle);

   virtual ~RecipeAdditionMassOrVolume();

   //=================================================== PROPERTIES ====================================================
   /**
    * \brief Mass (in kg) or Volume (in liters)
    */
   Q_PROPERTY(double          amount           READ amount           WRITE setAmount         )
   // Normally you aren't going to need to change this if it can be obtained from the Hop/Fermentable/etc you're adding
   Q_PROPERTY(bool            amountIsWeight   READ amountIsWeight   WRITE setAmountIsWeight )
   Q_PROPERTY(MassOrVolumeAmt amountWithUnits  READ amountWithUnits  WRITE setAmountWithUnits)

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   double          amount         () const;
   bool            amountIsWeight () const;
   MassOrVolumeAmt amountWithUnits() const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setAmount         (double          const val);
   void setAmountIsWeight (bool            const val);
   void setAmountWithUnits(MassOrVolumeAmt const val);

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;

protected:
   double m_amount        ;
   bool   m_amountIsWeight;
};

#endif
