/*======================================================================================================================
 * model/FermentationStep.h is part of Brewken, and is copyright the following authors 2023-2024:
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
#ifndef MODEL_FERMENTATIONSTEP_H
#define MODEL_FERMENTATIONSTEP_H
#pragma once

#include <optional>

#include "model/Fermentation.h"
#include "model/StepBase.h"
#include "model/StepExtended.h"
#include "utils/EnumStringMapping.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::FermentationStep { BtStringConst const property{#property}; }
AddPropertyName(vessel)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================

/**
 * \class FermentationStep is a step in a a fermentation process.
 *
 * \brief As a \c MashStep is to a \c Mash, and a \c BoilStep is to a \c Boil, so a \c FermentationStep is to a
 *        \c Fermentation.
 */
class FermentationStep : public StepExtended, public StepBase<FermentationStep, Fermentation> {
   Q_OBJECT

   STEP_COMMON_DECL(Fermentation)

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

   FermentationStep(QString name = "");
   FermentationStep(NamedParameterBundle const & namedParameterBundle);
   FermentationStep(FermentationStep const & other);

   virtual ~FermentationStep();

   //=================================================== PROPERTIES ====================================================
   // ⮜⮜⮜ All below added for BeerJSON support(!) ⮞⮞⮞
   /**
    * \brief This is purely descriptive, and there is currently no direct link with \c Equipment
    */
   Q_PROPERTY(QString vessel  READ vessel  WRITE setVessel)

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   QString vessel() const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setVessel(QString const & val);

signals:

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;

private:
   QString m_vessel;
};

Q_DECLARE_METATYPE(QList<std::shared_ptr<FermentationStep>>)

#endif
