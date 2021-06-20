/**
 * model/Water.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
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
 */
#include "model/Water.h"

#include <QDomElement>
#include <QDomText>
#include <QObject>
#include <QVector>

#include "Brewken.h"
#include "database/ObjectStoreWrapper.h"

bool Water::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Water const & rhs = static_cast<Water const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_calcium_ppm      == rhs.m_calcium_ppm      &&
      this->m_bicarbonate_ppm  == rhs.m_bicarbonate_ppm  &&
      this->m_sulfate_ppm      == rhs.m_sulfate_ppm      &&
      this->m_chloride_ppm     == rhs.m_chloride_ppm     &&
      this->m_sodium_ppm       == rhs.m_sodium_ppm       &&
      this->m_magnesium_ppm    == rhs.m_magnesium_ppm    &&
      this->m_ph               == rhs.m_ph
   );
}

ObjectStore & Water::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Water>::getInstance();
}

QString Water::classNameStr()
{
   static const QString name("Water");
   return name;
}

Water::Water(QString name, bool cache) :
   NamedEntity(-1, cache, name, true),
   m_amount(0.0),
   m_calcium_ppm(0.0),
   m_bicarbonate_ppm(0.0),
   m_sulfate_ppm(0.0),
   m_chloride_ppm(0.0),
   m_sodium_ppm(0.0),
   m_magnesium_ppm(0.0),
   m_ph(0.0),
   m_alkalinity(0.0),
   m_notes(QString()),
   m_type(NONE),
   m_mash_ro(0.0),
   m_sparge_ro(0.0),
   m_alkalinity_as_hco3(true) {
   return;
}

Water::Water(Water const& other) :
   NamedEntity         {other                        },
   m_amount            {other.m_amount               },
   m_calcium_ppm       {other.m_calcium_ppm          },
   m_bicarbonate_ppm   {other.m_bicarbonate_ppm      },
   m_sulfate_ppm       {other.m_sulfate_ppm          },
   m_chloride_ppm      {other.m_chloride_ppm         },
   m_sodium_ppm        {other.m_sodium_ppm           },
   m_magnesium_ppm     {other.m_magnesium_ppm        },
   m_ph                {other.m_ph                   },
   m_alkalinity        {other.m_alkalinity           },
   m_notes             {other.m_notes                },
   m_type              {other.m_type                 },
   m_mash_ro           {other.m_mash_ro              },
   m_sparge_ro         {other.m_sparge_ro            },
   m_alkalinity_as_hco3{other.m_alkalinity_as_hco3   } {
   return;
}

Water::Water(NamedParameterBundle const & namedParameterBundle) :
   NamedEntity         {namedParameterBundle},
   m_amount            {namedParameterBundle(PropertyNames::Water::amount).toDouble()},
   m_calcium_ppm       {namedParameterBundle(PropertyNames::Water::calcium_ppm).toDouble()},
   m_bicarbonate_ppm   {namedParameterBundle(PropertyNames::Water::bicarbonate_ppm).toDouble()},
   m_sulfate_ppm       {namedParameterBundle(PropertyNames::Water::sulfate_ppm).toDouble()},
   m_chloride_ppm      {namedParameterBundle(PropertyNames::Water::chloride_ppm).toDouble()},
   m_sodium_ppm        {namedParameterBundle(PropertyNames::Water::sodium_ppm).toDouble()},
   m_magnesium_ppm     {namedParameterBundle(PropertyNames::Water::magnesium_ppm).toDouble()},
   m_ph                {namedParameterBundle(PropertyNames::Water::ph).toDouble()},
   m_alkalinity        {namedParameterBundle(PropertyNames::Water::alkalinity).toDouble()},
   m_notes             {namedParameterBundle(PropertyNames::Water::notes).toString()},
   m_type              {static_cast<Water::Types>(namedParameterBundle(PropertyNames::Water::type).toInt())},
   m_mash_ro           {namedParameterBundle(PropertyNames::Water::mashRO).toDouble()},
   m_sparge_ro         {namedParameterBundle(PropertyNames::Water::spargeRO).toDouble()},
   m_alkalinity_as_hco3{namedParameterBundle(PropertyNames::Water::alkalinityAsHCO3).toBool()} {
   return;
}

//================================"SET" METHODS=================================
void Water::setAmount( double var )
{
   m_amount = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::amount, var);
   }
}

void Water::setCalcium_ppm( double var )
{
   m_calcium_ppm = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::calcium_ppm, var);
   }
}

void Water::setBicarbonate_ppm( double var )
{
   m_bicarbonate_ppm = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::bicarbonate_ppm, var);
   }
}

void Water::setChloride_ppm( double var )
{
   m_chloride_ppm = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::chloride_ppm, var);
   }
}

void Water::setSodium_ppm( double var )
{
   m_sodium_ppm = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::sodium_ppm, var);
   }
}

void Water::setMagnesium_ppm( double var )
{
   m_magnesium_ppm = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::magnesium_ppm, var);
   }
}

void Water::setPh( double var )
{
   m_ph = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::ph, var);
   }
}

void Water::setAlkalinity(double var)
{
   m_alkalinity = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::alkalinity, var);
   }
}

void Water::setSulfate_ppm( double var )
{
   m_sulfate_ppm = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::sulfate_ppm, var);
   }
}

void Water::setNotes( const QString &var )
{
   m_notes = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::notes, var);
   }
}

void Water::setType(Types type)
{
   if ( type < NONE || type > TARGET ) {
      return;
   }

   m_type = type;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::type, type);
   }
}

void Water::setMashRO( double var )
{
   m_mash_ro = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::mashRO, var);
   }
}

void Water::setSpargeRO( double var )
{
   m_sparge_ro = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::spargeRO, var);
   }
}

void Water::setAlkalinityAsHCO3(bool var)
{
   m_alkalinity_as_hco3 = var;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Water::alkalinityAsHCO3, var);
   }
}

//=========================="GET" METHODS=======================================
QString Water::notes() const { return m_notes; }
double Water::sulfate_ppm() const { return m_sulfate_ppm; }
double Water::amount() const { return m_amount; }
double Water::calcium_ppm() const { return m_calcium_ppm; }
double Water::bicarbonate_ppm() const { return m_bicarbonate_ppm; }
double Water::chloride_ppm() const { return m_chloride_ppm; }
double Water::sodium_ppm() const { return m_sodium_ppm; }
double Water::magnesium_ppm() const { return m_magnesium_ppm; }
double Water::ph() const { return m_ph; }
double Water::alkalinity() const { return m_alkalinity; }
Water::Types Water::type() const { return m_type; }
double Water::mashRO() const { return m_mash_ro; }
double Water::spargeRO() const { return m_sparge_ro; }
bool Water::alkalinityAsHCO3() const { return m_alkalinity_as_hco3; }

double Water::ppm( Water::Ions ion )
{
   switch(ion) {
      case Water::Ca:   return m_calcium_ppm;
      case Water::Cl:   return m_chloride_ppm;
      case Water::HCO3: return m_bicarbonate_ppm;
      case Water::Mg:   return m_magnesium_ppm;
      case Water::Na:   return m_sodium_ppm;
      case Water::SO4:  return m_sulfate_ppm;
      default: return 0.0;
   }

   return 0.0;
}
