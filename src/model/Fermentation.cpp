/*======================================================================================================================
 * model/Fermentation.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "model/Fermentation.h"

#include "model/NamedParameterBundle.h"

QString const Fermentation::LocalisedName = tr("Fermentation");

bool Fermentation::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Fermentation const & rhs = static_cast<Fermentation const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_description == rhs.m_description &&
      this->m_notes       == rhs.m_notes
      // .:TBD:. Should we check FermentationSteps too?
   );
}

ObjectStore & Fermentation::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Fermentation>::getInstance();
}

TypeLookup const Fermentation::typeLookup {
   "Fermentation",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentation::description, Fermentation::m_description, NonPhysicalQuantity::String),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Fermentation::notes      , Fermentation::m_notes      , NonPhysicalQuantity::String),

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Fermentation::fermentationSteps        , Fermentation::fermentationSteps        ),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Fermentation::fermentationStepsDowncast, Fermentation::fermentationStepsDowncast),
   },
   // Parent classes lookup
   {&NamedEntity::typeLookup,
    std::addressof(FolderBase<Fermentation>::typeLookup)}
};
static_assert(std::is_base_of<FolderBase<Fermentation>, Fermentation>::value);

//==================================================== CONSTRUCTORS ====================================================

Fermentation::Fermentation(QString name) :
   NamedEntity{name, true},
   FolderBase<Fermentation>{},
   StepOwnerBase<Fermentation, FermentationStep>{},
   m_description  {""          },
   m_notes        {""          } {
   return;
}

Fermentation::Fermentation(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity                {namedParameterBundle},
   FolderBase<Fermentation>{namedParameterBundle},
   StepOwnerBase<Fermentation, FermentationStep>{},
   SET_REGULAR_FROM_NPB (m_description  , namedParameterBundle, PropertyNames::Fermentation::description  ),
   SET_REGULAR_FROM_NPB (m_notes        , namedParameterBundle, PropertyNames::Fermentation::notes        ) {
   return;
}

Fermentation::Fermentation(Fermentation const & other) :
   NamedEntity{other},
   FolderBase<Fermentation>{other},
   StepOwnerBase<Fermentation, FermentationStep>{other},
   m_description  {other.m_description  },
   m_notes        {other.m_notes        } {
   return;
}

Fermentation::~Fermentation() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
QString Fermentation::description() const { return this->m_description; }
QString Fermentation::notes      () const { return this->m_notes      ; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void Fermentation::setDescription (QString const & val) { SET_AND_NOTIFY(PropertyNames::Fermentation::description, this->m_description, val); return; }
void Fermentation::setNotes       (QString const & val) { SET_AND_NOTIFY(PropertyNames::Fermentation::notes      , this->m_notes      , val); return; }

void Fermentation::acceptStepChange([[maybe_unused]] QMetaProperty prop,
                                    [[maybe_unused]] QVariant      val) {
   return;
}

// Boilerplate code for FolderBase
FOLDER_BASE_COMMON_CODE(Fermentation)

// Insert fermentationer-plate wrapper functions that call down to StepOwnerBase
STEP_OWNER_COMMON_CODE(Fermentation, fermentation)
