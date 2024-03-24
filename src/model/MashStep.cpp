/*======================================================================================================================
 * model/MashStep.cpp is part of Brewken, and is copyright the following authors 2009-2024:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#include "model/MashStep.h"

#include <QDebug>
#include <QVector>

#include "database/ObjectStoreWrapper.h"
#include "model/NamedParameterBundle.h"
#include "PhysicalConstants.h"

QString const MashStep::LocalisedName = tr("Mash Step");

EnumStringMapping const MashStep::typeStringMapping {
   {MashStep::Type::Infusion   , "infusion"      },
   {MashStep::Type::Temperature, "temperature"   },
   {MashStep::Type::Decoction  , "decoction"     },
   {MashStep::Type::FlySparge  , "sparge"        },
   {MashStep::Type::BatchSparge, "drain mash tun"},
   {MashStep::Type::SouringMash, "souring mash"  },
   {MashStep::Type::SouringWort, "souring wort"  },
};

EnumStringMapping const MashStep::typeDisplayNames {
   {MashStep::Type::Infusion   , tr("Infusion"                     )},
   {MashStep::Type::Temperature, tr("Temperature"                  )},
   {MashStep::Type::Decoction  , tr("Decoction"                    )},
   {MashStep::Type::FlySparge  , tr("Normal (Fly) Sparge"          )},
   {MashStep::Type::BatchSparge, tr("Batch Sparge (Drain Mash Tun)")},
   {MashStep::Type::SouringMash, tr("Souring Mash"                 )},
   {MashStep::Type::SouringWort, tr("Souring Wort"                 )},
};


bool MashStep::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   MashStep const & rhs = static_cast<MashStep const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_type         == rhs.m_type         &&
      this->m_amount_l     == rhs.m_amount_l     &&
///      this->m_stepTemp_c   == rhs.m_stepTemp_c   &&
      this->m_infuseTemp_c == rhs.m_infuseTemp_c &&
      // Parent classes have to be equal too
      this->Step::isEqualTo(other)
   );
}

///ObjectStore & MashStep::getObjectStoreTypedInstance() const {
///   return ObjectStoreTyped<MashStep>::getInstance();
///}

TypeLookup const MashStep::typeLookup {
   "MashStep",
   {
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::MashStep::type                  , MashStep::m_type                  ,           NonPhysicalQuantity::Enum          ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::MashStep::amount_l              , MashStep::m_amount_l              , Measurement::PhysicalQuantity::Volume        ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::MashStep::infuseAmount_l        , MashStep::m_amount_l              , Measurement::PhysicalQuantity::Volume        ), // Type Lookup retained for BeerXML
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::MashStep::decoctionAmount_l     , MashStep::m_amount_l              , Measurement::PhysicalQuantity::Volume        ), // Type Lookup retained for BeerXML
///      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::MashStep::stepTemp_c            , MashStep::m_stepTemp_c            , Measurement::PhysicalQuantity::Temperature   ),
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::MashStep::infuseTemp_c          , MashStep::m_infuseTemp_c          , Measurement::PhysicalQuantity::Temperature   ),
      // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
      PROPERTY_TYPE_LOOKUP_ENTRY(PropertyNames::MashStep::liquorToGristRatio_lKg, MashStep::m_liquorToGristRatio_lKg, Measurement::PhysicalQuantity::SpecificVolume),
   },
   // Parent class lookup.  NB: Step not NamedEntity!
   {&Step::typeLookup}
};
static_assert(std::is_base_of<Step, MashStep>::value);

//==================================================== CONSTRUCTORS ====================================================

MashStep::MashStep(QString name) :
   Step                    {name},
   m_type                  {MashStep::Type::Infusion},
   m_amount_l              {0.0                     },
///   m_stepTemp_c            {0.0                     },
   m_infuseTemp_c          {std::nullopt            },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_liquorToGristRatio_lKg{std::nullopt            } {
   return;
}

MashStep::MashStep(NamedParameterBundle const & namedParameterBundle) :
   Step                    (namedParameterBundle                                                                           ),
   SET_REGULAR_FROM_NPB (m_type                  , namedParameterBundle, PropertyNames::MashStep::type                  ),
   SET_REGULAR_FROM_NPB (m_amount_l              , namedParameterBundle, PropertyNames::MashStep::amount_l              , 0.0),
///   SET_REGULAR_FROM_NPB (m_stepTemp_c            , namedParameterBundle, PropertyNames::MashStep::stepTemp_c            ),
   SET_REGULAR_FROM_NPB (m_infuseTemp_c          , namedParameterBundle, PropertyNames::MashStep::infuseTemp_c          ),
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   SET_REGULAR_FROM_NPB (m_liquorToGristRatio_lKg, namedParameterBundle, PropertyNames::MashStep::liquorToGristRatio_lKg) {
   // See comment in Step constructor.  We're saying that, if rampTime_mins is present in the bundle (which it won't
   // always be because it's optional) then it is supported by this class.  In other words, either it's not there, or
   // (if it is then) it's supported.
   Q_ASSERT(!namedParameterBundle.contains(PropertyNames::Step::rampTime_mins) || this->rampTimeIsSupported());
   //
   // If we were constructed from BeerXML, it will have set decoctionAmount_l or infuseAmount_l instead of amount_l
   //
   if (0.0 == m_amount_l) {
      if (m_type == MashStep::Type::Decoction) {
         m_amount_l = namedParameterBundle.val<double>(PropertyNames::MashStep::decoctionAmount_l, 0.0);
      } else {
         m_amount_l = namedParameterBundle.val<double>(PropertyNames::MashStep::infuseAmount_l   , 0.0);
      }
   }
   return;
}

MashStep::MashStep(MashStep const & other) :
   Step                    {other},
   m_type                  {other.m_type                  },
   m_amount_l              {other.m_amount_l              },
///   m_stepTemp_c            {other.m_stepTemp_c            },
   m_infuseTemp_c          {other.m_infuseTemp_c          },
   // ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
   m_liquorToGristRatio_lKg{other.m_liquorToGristRatio_lKg} {
   return;
}

MashStep::~MashStep() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
MashStep::Type        MashStep::type                  () const { return this->m_type                  ; }
double                MashStep::amount_l              () const { return this->m_amount_l              ; }
///double                MashStep::stepTemp_c            () const { return this->m_stepTemp_c            ; }
std::optional<double> MashStep::infuseTemp_c          () const { return this->m_infuseTemp_c          ; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
std::optional<double> MashStep::liquorToGristRatio_lKg() const { return this->m_liquorToGristRatio_lKg; }

// TBD: Do we need to add type-checking logic in these legacy functions?
[[deprecated]] double MashStep::infuseAmount_l   () const { return this->m_amount_l; }
[[deprecated]] double MashStep::decoctionAmount_l() const { return this->m_amount_l; }

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void MashStep::setType                  (MashStep::Type        const   val) { SET_AND_NOTIFY(PropertyNames::MashStep::type                  , this->m_type                  , val                                                                ); return; }
void MashStep::setAmount_l              (double                const   val) { SET_AND_NOTIFY(PropertyNames::MashStep::amount_l              , this->m_amount_l              , val                                                                ); return; }
///void MashStep::setStepTemp_c            (double                const   val) { SET_AND_NOTIFY(PropertyNames::MashStep::stepTemp_c            , this->m_stepTemp_c            , this->enforceMin(val, "step temp", PhysicalConstants::absoluteZero)); return; }
void MashStep::setInfuseTemp_c          (std::optional<double> const   val) { SET_AND_NOTIFY(PropertyNames::MashStep::infuseTemp_c          , this->m_infuseTemp_c          , val                                                                ); return; }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void MashStep::setLiquorToGristRatio_lKg(std::optional<double> const   val) { SET_AND_NOTIFY(PropertyNames::MashStep::liquorToGristRatio_lKg, this->m_liquorToGristRatio_lKg, val                                                                ); return; }

//=============================================== OTHER MEMBER FUNCTIONS ===============================================
bool MashStep::isInfusion() const {
   return (m_type == MashStep::Type::Infusion    ||
           m_type == MashStep::Type::BatchSparge ||
           m_type == MashStep::Type::FlySparge);
}

bool MashStep::isSparge() const {
   // NOTE: We have a bit of a hack here checking the name.  It's because BeerXML doesn't natively support sparge as a
   //       mash step type.  So this is a heuristic to detect what was "really meant" in a recipe that originated in
   //       BeerXML.  (Once we have persuaded the entire brewing world to switch to BeerJSON, and to update all existing
   //       recipes, this will no longer be necessary.  So it's only temporary. :D)
   return (m_type == MashStep::Type::BatchSparge ||
           m_type == MashStep::Type::FlySparge   ||
           name() == "Final Batch Sparge");
}

bool MashStep::isTemperature() const {
   return (m_type == MashStep::Type::Temperature);
}

bool MashStep::isDecoction() const {
   return (m_type == MashStep::Type::Decoction);
}

///Recipe * MashStep::getOwningRecipe() const {
///   return Step::doGetOwningRecipe<Mash>(this);
///}

[[nodiscard]] bool MashStep:: stepTimeIsRequired() const { return true; }
[[nodiscard]] bool MashStep::startTempIsRequired() const { return true; }


// Insert boiler-plate wrapper functions that call down to StepBase
STEP_COMMON_CODE(Mash)
