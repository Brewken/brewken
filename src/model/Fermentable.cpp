/*======================================================================================================================
 * model/Fermentable.cpp is part of Brewken, and is copyright the following authors 2009-2022:
 *   • Blair Bonnett <blair.bonnett@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#include "model/Fermentable.h"

#include <QDebug>
#include <QObject>
#include <QVariant>

#include "database/ObjectStoreWrapper.h"
#include "model/Inventory.h"
#include "model/NamedParameterBundle.h"
#include "model/Recipe.h"

std::array<Fermentable::Type, 8> const Fermentable::allTypes {
   Fermentable::Type::Dry_Extract,
   Fermentable::Type::Extract,
   Fermentable::Type::Grain,
   Fermentable::Type::Sugar,
   Fermentable::Type::Fruit,
   Fermentable::Type::Juice,
   Fermentable::Type::Honey,
   Fermentable::Type::Other_Adjunct
};

// Note that Hop::typeStringMapping and Hop::FormMapping are as defined by BeerJSON, but we also use them for the DB and
// for the UI.  We can't use them for BeerXML as it only supports subsets of these types.
EnumStringMapping const Fermentable::typeStringMapping {
   {"dry extract", Fermentable::Type::Dry_Extract},
   {"extract",     Fermentable::Type::Extract},
   {"grain",       Fermentable::Type::Grain},
   {"sugar",       Fermentable::Type::Sugar},
   {"fruit",       Fermentable::Type::Fruit},
   {"juice",       Fermentable::Type::Juice},
   {"honey",       Fermentable::Type::Honey},
   {"other",       Fermentable::Type::Other_Adjunct}
};

QMap<Fermentable::Type, QString> const Fermentable::typeDisplayNames {
   {Fermentable::Type::Dry_Extract,   tr("Dry Extract"  )},
   {Fermentable::Type::Extract,       tr("Extract"      )},
   {Fermentable::Type::Grain,         tr("Grain"        )},
   {Fermentable::Type::Sugar,         tr("Sugar"        )},
   {Fermentable::Type::Fruit,         tr("Fruit"        )},
   {Fermentable::Type::Juice,         tr("Juice"        )},
   {Fermentable::Type::Honey,         tr("Honey"        )},
   {Fermentable::Type::Other_Adjunct, tr("Other Adjunct")},
};

std::array<Fermentable::GrainGroup, 7> const Fermentable::allGrainGroups {
   Fermentable::GrainGroup::Base,
   Fermentable::GrainGroup::Caramel,
   Fermentable::GrainGroup::Flaked,
   Fermentable::GrainGroup::Roasted,
   Fermentable::GrainGroup::Specialty,
   Fermentable::GrainGroup::Smoked,
   Fermentable::GrainGroup::Adjunct
};

// This is based on the BeerJSON encoding
EnumStringMapping const Fermentable::grainGroupStringMapping {
   {"base",       Fermentable::GrainGroup::Base     },
   {"caramel",    Fermentable::GrainGroup::Caramel  },
   {"flaked",     Fermentable::GrainGroup::Flaked   },
   {"roasted",    Fermentable::GrainGroup::Roasted  },
   {"specialty",  Fermentable::GrainGroup::Specialty},
   {"smoked",     Fermentable::GrainGroup::Smoked   },
   {"adjunct",    Fermentable::GrainGroup::Adjunct  }
};

bool Fermentable::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Fermentable const & rhs = static_cast<Fermentable const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_type           == rhs.m_type           &&
      this->m_yieldPct       == rhs.m_yieldPct       &&
      this->m_colorSrm       == rhs.m_colorSrm       &&
      this->m_origin         == rhs.m_origin         &&
      this->m_supplier       == rhs.m_supplier       &&
      this->m_coarseFineDiff == rhs.m_coarseFineDiff &&
      this->m_moisturePct    == rhs.m_moisturePct    &&
      this->m_diastaticPower == rhs.m_diastaticPower &&
      this->m_proteinPct     == rhs.m_proteinPct     &&
      this->m_maxInBatchPct  == rhs.m_maxInBatchPct
   );
}

ObjectStore & Fermentable::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Fermentable>::getInstance();
}

Fermentable::Fermentable(QString name) :
   NamedEntityWithInventory{name, true},
   m_typeStr       {QString()         },
   m_type          {Fermentable::Type::Grain},
   m_amountKg      {0.0               },
   m_yieldPct      {0.0               },
   m_colorSrm      {0.0               },
   m_isAfterBoil   {false             },
   m_origin        {QString()         },
   m_supplier      {QString()         },
   m_notes         {QString()         },
   m_coarseFineDiff{0.0               },
   m_moisturePct   {0.0               },
   m_diastaticPower{0.0               },
   m_proteinPct    {0.0               },
   m_maxInBatchPct {100.0             },
   m_recommendMash {false             },
   m_ibuGalPerLb   {0.0               },
   m_isMashed      {false             },
   m_grainGroup    {std::nullopt      } {
   return;
}

Fermentable::Fermentable(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithInventory{namedParameterBundle},
   m_typeStr       {""},
   m_type          {namedParameterBundle.val<Fermentable::Type             >(PropertyNames::Fermentable::type)},
   m_amountKg      {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::amount_kg                        )},
   m_yieldPct      {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::yield_pct                        )},
   m_colorSrm      {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::color_srm                        )},
   m_isAfterBoil   {namedParameterBundle.val<bool                          >(PropertyNames::Fermentable::addAfterBoil                     )},
   m_origin        {namedParameterBundle.val<QString                       >(PropertyNames::Fermentable::origin                , QString())},
   m_supplier      {namedParameterBundle.val<QString                       >(PropertyNames::Fermentable::supplier              , QString())},
   m_notes         {namedParameterBundle.val<QString                       >(PropertyNames::Fermentable::notes                 , QString())},
   m_coarseFineDiff{namedParameterBundle.val<double                        >(PropertyNames::Fermentable::coarseFineDiff_pct               )},
   m_moisturePct   {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::moisture_pct                     )},
   m_diastaticPower{namedParameterBundle.val<double                        >(PropertyNames::Fermentable::diastaticPower_lintner           )},
   m_proteinPct    {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::protein_pct                      )},
   m_maxInBatchPct {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::maxInBatch_pct                   )},
   m_recommendMash {namedParameterBundle.val<bool                          >(PropertyNames::Fermentable::recommendMash                    )},
   m_ibuGalPerLb   {namedParameterBundle.val<double                        >(PropertyNames::Fermentable::ibuGalPerLb                      )},
   m_isMashed      {namedParameterBundle.val<bool                          >(PropertyNames::Fermentable::isMashed              , false    )},
   m_grainGroup    {namedParameterBundle.optEnumVal<Fermentable::GrainGroup>(PropertyNames::Fermentable::grainGroup                       )} {
   return;
}

Fermentable::Fermentable(Fermentable const & other) :
   NamedEntityWithInventory{other                 },
   m_typeStr       {other.m_typeStr       },
   m_type          {other.m_type          },
   m_amountKg      {other.m_amountKg      },
   m_yieldPct      {other.m_yieldPct      },
   m_colorSrm      {other.m_colorSrm      },
   m_isAfterBoil   {other.m_isAfterBoil   },
   m_origin        {other.m_origin        },
   m_supplier      {other.m_supplier      },
   m_notes         {other.m_notes         },
   m_coarseFineDiff{other.m_coarseFineDiff},
   m_moisturePct   {other.m_moisturePct   },
   m_diastaticPower{other.m_diastaticPower},
   m_proteinPct    {other.m_proteinPct    },
   m_maxInBatchPct {other.m_maxInBatchPct },
   m_recommendMash {other.m_recommendMash },
   m_ibuGalPerLb   {other.m_ibuGalPerLb   },
   m_isMashed      {other.m_isMashed      },
   m_grainGroup    {other.m_grainGroup    } {
   return;
}

Fermentable::~Fermentable() = default;

//============================================= "GETTER" MEMBER FUNCTIONS ==============================================
Fermentable::Type                      Fermentable::type()                   const { return this->m_type;           }
double                                 Fermentable::amount_kg()              const { return this->m_amountKg;       }
double                                 Fermentable::yield_pct()              const { return this->m_yieldPct;       }
double                                 Fermentable::color_srm()              const { return this->m_colorSrm;       }
bool                                   Fermentable::addAfterBoil()           const { return this->m_isAfterBoil;    }
QString                                Fermentable::origin()                 const { return this->m_origin;         }
QString                                Fermentable::supplier()               const { return this->m_supplier;       }
QString                                Fermentable::notes()                  const { return this->m_notes;          }
double                                 Fermentable::coarseFineDiff_pct()     const { return this->m_coarseFineDiff; }
double                                 Fermentable::moisture_pct()           const { return this->m_moisturePct;    }
double                                 Fermentable::diastaticPower_lintner() const { return this->m_diastaticPower; }
double                                 Fermentable::protein_pct()            const { return this->m_proteinPct;     }
double                                 Fermentable::maxInBatch_pct()         const { return this->m_maxInBatchPct;  }
bool                                   Fermentable::recommendMash()          const { return this->m_recommendMash;  }
double                                 Fermentable::ibuGalPerLb()            const { return this->m_ibuGalPerLb;    }
bool                                   Fermentable::isMashed()               const { return this->m_isMashed;       }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
std::optional<Fermentable::GrainGroup> Fermentable::grainGroup()             const { return this->m_grainGroup;     }


bool Fermentable::isExtract() const {
   return ((type() == Fermentable::Type::Extract) || (type() == Fermentable::Type::Dry_Extract));
}

bool Fermentable::isSugar() const {
   return (type() == Fermentable::Type::Sugar);
}

double Fermentable::equivSucrose_kg() const {
   double const ret = this->amount_kg() * this->yield_pct() * (1.0 - this->moisture_pct() / 100.0) / 100.0;

   // If this is a steeped grain...
   if (this->type() == Fermentable::Type::Grain && !this->isMashed()) {
      return 0.60 * ret; // Reduce the yield by 60%.
   }
   return ret;
}

//============================================= "SETTER" MEMBER FUNCTIONS ==============================================
void Fermentable::setType                  (Type                      const   val) { this->setAndNotify(PropertyNames::Fermentable::type,                   this->m_type,           val); }
void Fermentable::setAddAfterBoil          (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::addAfterBoil,           this->m_isAfterBoil,    val); }
void Fermentable::setOrigin                (QString                   const & val) { this->setAndNotify(PropertyNames::Fermentable::origin,                 this->m_origin,         val); }
void Fermentable::setSupplier              (QString                   const & val) { this->setAndNotify(PropertyNames::Fermentable::supplier,               this->m_supplier,       val); }
void Fermentable::setNotes                 (QString                   const & val) { this->setAndNotify(PropertyNames::Fermentable::notes,                  this->m_notes,          val); }
void Fermentable::setRecommendMash         (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::recommendMash,          this->m_recommendMash,  val); }
void Fermentable::setIsMashed              (bool                      const   val) { this->setAndNotify(PropertyNames::Fermentable::isMashed,               this->m_isMashed,       val); }
void Fermentable::setIbuGalPerLb           (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::ibuGalPerLb,            this->m_ibuGalPerLb,    val); }
void Fermentable::setAmount_kg             (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::amount_kg,              this->m_amountKg,       this->enforceMin      (val, "amount"));                     }
void Fermentable::setYield_pct             (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::yield_pct,              this->m_yieldPct,       this->enforceMinAndMax(val, "amount",         0.0, 100.0)); }
void Fermentable::setColor_srm             (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::color_srm,              this->m_colorSrm,       this->enforceMin      (val, "color"));                      }
void Fermentable::setCoarseFineDiff_pct    (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::coarseFineDiff_pct,     this->m_coarseFineDiff, this->enforceMinAndMax(val, "coarseFineDiff", 0.0, 100.0)); }
void Fermentable::setMoisture_pct          (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::moisture_pct,           this->m_moisturePct,    this->enforceMinAndMax(val, "moisture",       0.0, 100.0)); }
void Fermentable::setDiastaticPower_lintner(double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::diastaticPower_lintner, this->m_diastaticPower, this->enforceMin      (val, "diastatic power"));            }
void Fermentable::setProtein_pct           (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::protein_pct,            this->m_proteinPct,     this->enforceMinAndMax(val, "protein",        0.0, 100.0)); }
void Fermentable::setMaxInBatch_pct        (double                    const   val) { this->setAndNotify(PropertyNames::Fermentable::maxInBatch_pct,         this->m_maxInBatchPct,  this->enforceMinAndMax(val, "max in batch",   0.0, 100.0)); }
// ⮜⮜⮜ All below added for BeerJSON support ⮞⮞⮞
void Fermentable::setGrainGroup            (std::optional<GrainGroup> const   val) { this->setAndNotify(PropertyNames::Fermentable::grainGroup,             this->m_grainGroup,     val); }


void Fermentable::setInventoryAmount(double num) {
   InventoryUtils::setAmount(*this, num);
   return;
}

double Fermentable::inventory() const {
   return InventoryUtils::getAmount(*this);
}

Recipe * Fermentable::getOwningRecipe() {
   return ObjectStoreWrapper::findFirstMatching<Recipe>( [this](Recipe * rec) {return rec->uses(*this);} );
}

bool fermentablesLessThanByWeight(Fermentable const * const lhs, Fermentable const * const rhs) {
   // Sort by name if the two fermentables are of equal weight
   if (qFuzzyCompare(lhs->amount_kg(), rhs->amount_kg())) {
      return lhs->name() < rhs->name();
   }

   // Yes. I know. This seems silly, but I want the returned list in descending not ascending order.
   return lhs->amount_kg() > rhs->amount_kg();
}
