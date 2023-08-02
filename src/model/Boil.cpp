/*======================================================================================================================
 * model/Boil.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "model/Boil.h"

#include "model/NamedParameterBundle.h"

QString const Boil::LocalisedName = tr("Boil");

bool Boil::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Boil const & rhs = static_cast<Boil const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_description   == rhs.m_description   &&
      this->m_notes         == rhs.m_notes         &&
      this->m_preBoilSize_l == rhs.m_preBoilSize_l &&
      this->m_boilTime_mins == rhs.m_boilTime_mins
      // .:TBD:. Should we check BoilSteps too?
   );
}

ObjectStore & Boil::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Boil>::getInstance();
}

TypeLookup const Boil::typeLookup {
   "Boil",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Boil::description  , Boil::m_description  ,           NonPhysicalQuantity::String),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Boil::notes        , Boil::m_notes        ,           NonPhysicalQuantity::String),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Boil::preBoilSize_l, Boil::m_preBoilSize_l, Measurement::PhysicalQuantity::Volume),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::Boil::boilTime_mins, Boil::m_boilTime_mins, Measurement::PhysicalQuantity::Time  ),

      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Boil::boilSteps        , Boil::boilSteps        ),
      PROPERTY_TYPE_LOOKUP_ENTRY_NO_MV(PropertyNames::Boil::boilStepsDowncast, Boil::boilStepsDowncast),
   },
   // Parent class lookup
   &NamedEntity::typeLookup
};

//==================================================== CONSTRUCTORS ====================================================

Boil::Boil(QString name) :
   NamedEntity{name, true},
   StepOwnerBase<Boil, BoilStep>{},
   m_description  {""          },
   m_notes        {""          },
   m_preBoilSize_l{std::nullopt},
   m_boilTime_mins{0.0         } {
   return;
}

Boil::Boil(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity                {namedParameterBundle},
   StepOwnerBase<Boil, BoilStep>{},
   SET_REGULAR_FROM_NPB (m_description  , namedParameterBundle, PropertyNames::Boil::description  ),
   SET_REGULAR_FROM_NPB (m_notes        , namedParameterBundle, PropertyNames::Boil::notes        ),
   SET_REGULAR_FROM_NPB (m_preBoilSize_l, namedParameterBundle, PropertyNames::Boil::preBoilSize_l),
   SET_REGULAR_FROM_NPB (m_boilTime_mins, namedParameterBundle, PropertyNames::Boil::boilTime_mins) {
   return;
}

Boil::Boil(Boil const & other) :
   NamedEntity{other},
   StepOwnerBase<Boil, BoilStep>{other},
   m_description  {other.m_description  },
   m_notes        {other.m_notes        },
   m_preBoilSize_l{other.m_preBoilSize_l},
   m_boilTime_mins{other.m_boilTime_mins} {
   return;
}

Boil::~Boil() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
QString               Boil::description  () const { return this->m_description  ; }
QString               Boil::notes        () const { return this->m_notes        ; }
std::optional<double> Boil::preBoilSize_l() const { return this->m_preBoilSize_l; }
double                Boil::boilTime_mins() const { return this->m_boilTime_mins; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void Boil::setDescription  (QString               const & val) { this->setAndNotify(PropertyNames::Boil::description  , this->m_description  , val); return; }
void Boil::setNotes        (QString               const & val) { this->setAndNotify(PropertyNames::Boil::notes        , this->m_notes        , val); return; }
void Boil::setPreBoilSize_l(std::optional<double> const   val) { this->setAndNotify(PropertyNames::Boil::preBoilSize_l, this->m_preBoilSize_l, val); return; }
void Boil::setBoilTime_mins(double                const   val) { this->setAndNotify(PropertyNames::Boil::boilTime_mins, this->m_boilTime_mins, val); return; }

void Boil::acceptStepChange([[maybe_unused]] QMetaProperty prop,
                            [[maybe_unused]] QVariant      val) {
   return;
}

// Insert boiler-plate wrapper functions that call down to StepOwnerBase
STEP_OWNER_COMMON_CODE(Boil, boil)
