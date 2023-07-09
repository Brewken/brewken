/*======================================================================================================================
 * model/FermentationStep.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "model/FermentationStep.h"

#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"

QString const FermentationStep::LocalisedName = tr("Fermentation Step");

bool FermentationStep::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   FermentationStep const & rhs = static_cast<FermentationStep const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_vessel == rhs.m_vessel &&
      // Parent classes have to be equal too
      this->StepExtended::isEqualTo(other)
   );
}

TypeLookup const FermentationStep::typeLookup {
   "FermentationStep",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::FermentationStep::vessel, FermentationStep::m_vessel, NonPhysicalQuantity::String),
   },
   // Parent class lookup
   &StepExtended::typeLookup
};

//==================================================== CONSTRUCTORS ====================================================

FermentationStep::FermentationStep(QString name) :
   StepExtended{name},
   m_vessel    {""  } {
   return;
}

FermentationStep::FermentationStep(NamedParameterBundle const & namedParameterBundle) :
   StepExtended  (namedParameterBundle                                                                ),
   m_vessel(namedParameterBundle.val<QString>(PropertyNames::FermentationStep::vessel, QString())) {
   return;
}

FermentationStep::FermentationStep(FermentationStep const & other) :
   StepExtended{other         },
   m_vessel    {other.m_vessel} {
   return;
}

FermentationStep::~FermentationStep() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
QString FermentationStep::vessel() const { return this->m_vessel; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void FermentationStep::setVessel(QString const & val) { this->setAndNotify(PropertyNames::FermentationStep::vessel, this->m_vessel, val ); return; }

// Insert boiler-plate wrapper functions that call down to StepBase
STEP_COMMON_CODE(Fermentation)
