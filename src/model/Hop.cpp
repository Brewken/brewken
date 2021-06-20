/**
 * model/Hop.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
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
#include "model/Hop.h"

#include <QDebug>
#include <QDomElement>
#include <QDomText>
#include <QObject>

#include "Brewken.h"
#include "database/ObjectStoreTyped.h"
#include "model/Inventory.h"

QStringList Hop::types = QStringList() << "Bittering" << "Aroma" << "Both";
QStringList Hop::forms = QStringList() << "Leaf" << "Pellet" << "Plug";
QStringList Hop::uses = QStringList() << "Mash" << "First Wort" << "Boil" << "Aroma" << "Dry Hop";

bool Hop::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Hop const & rhs = static_cast<Hop const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_use               == rhs.m_use               &&
      this->m_type              == rhs.m_type              &&
      this->m_form              == rhs.m_form              &&
      this->m_alpha_pct         == rhs.m_alpha_pct         &&
      this->m_beta_pct          == rhs.m_beta_pct          &&
      this->m_hsi_pct           == rhs.m_hsi_pct           &&
      this->m_origin            == rhs.m_origin            &&
      this->m_humulene_pct      == rhs.m_humulene_pct      &&
      this->m_caryophyllene_pct == rhs.m_caryophyllene_pct &&
      this->m_cohumulone_pct    == rhs.m_cohumulone_pct    &&
      this->m_myrcene_pct       == rhs.m_myrcene_pct
   );
}

ObjectStore & Hop::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Hop>::getInstance();
}

bool Hop::isValidUse(const QString& str)
{
   return (uses.indexOf(str) >= 0);
}

bool Hop::isValidType(const QString& str)
{
   return (types.indexOf(str) >= 0);
}

bool Hop::isValidForm(const QString& str)
{
   return (forms.indexOf(str) >= 0);
}

QString Hop::classNameStr()
{
   static const QString name("Hop");
   return name;
}

Hop::Hop(QString name, bool cache) :
   NamedEntityWithInventory{-1, cache, name, true},
   m_useStr           {"" },
   m_use              {Hop::Mash},
   m_typeStr          {"" },
   m_type             {Hop::Bittering},
   m_formStr          {"" },
   m_form             {Hop::Leaf},
   m_alpha_pct        {0.0},
   m_amount_kg        {0.0},
   m_time_min         {0.0},
   m_notes            {"" },
   m_beta_pct         {0.0},
   m_hsi_pct          {0.0},
   m_origin           {"" },
   m_substitutes      {"" },
   m_humulene_pct     {0.0},
   m_caryophyllene_pct{0.0},
   m_cohumulone_pct   {0.0},
   m_myrcene_pct      {0.0} {
   return;
}

Hop::Hop(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithInventory{namedParameterBundle},
   m_use                   {static_cast<Hop::Use>(namedParameterBundle(PropertyNames::Hop::use).toInt())},
   m_type                  {static_cast<Hop::Type>(namedParameterBundle(PropertyNames::Hop::type).toInt())},
   m_form                  {static_cast<Hop::Form>(namedParameterBundle(PropertyNames::Hop::form).toInt())},
   m_alpha_pct             {namedParameterBundle(PropertyNames::Hop::alpha_pct        ).toDouble()},
   m_amount_kg             {namedParameterBundle(PropertyNames::Hop::amount_kg        ).toDouble()},
   m_time_min              {namedParameterBundle(PropertyNames::Hop::time_min         ).toDouble()},
   m_notes                 {namedParameterBundle(PropertyNames::Hop::notes            ).toString()},
   m_beta_pct              {namedParameterBundle(PropertyNames::Hop::beta_pct         ).toDouble()},
   m_hsi_pct               {namedParameterBundle(PropertyNames::Hop::hsi_pct          ).toDouble()},
   m_origin                {namedParameterBundle(PropertyNames::Hop::origin           ).toString()},
   m_substitutes           {namedParameterBundle(PropertyNames::Hop::substitutes      ).toString()},
   m_humulene_pct          {namedParameterBundle(PropertyNames::Hop::humulene_pct     ).toDouble()},
   m_caryophyllene_pct     {namedParameterBundle(PropertyNames::Hop::caryophyllene_pct).toDouble()},
   m_cohumulone_pct        {namedParameterBundle(PropertyNames::Hop::cohumulone_pct   ).toDouble()},
   m_myrcene_pct           {namedParameterBundle(PropertyNames::Hop::myrcene_pct      ).toDouble()} {
   return;
}

Hop::Hop(Hop const & other) :
   NamedEntityWithInventory{other                    },
   m_useStr                {other.m_useStr           },
   m_use                   {other.m_use              },
   m_typeStr               {other.m_typeStr          },
   m_type                  {other.m_type             },
   m_formStr               {other.m_formStr          },
   m_form                  {other.m_form             },
   m_alpha_pct             {other.m_alpha_pct        },
   m_amount_kg             {other.m_amount_kg        },
   m_time_min              {other.m_time_min         },
   m_notes                 {other.m_notes            },
   m_beta_pct              {other.m_beta_pct         },
   m_hsi_pct               {other.m_hsi_pct          },
   m_origin                {other.m_origin           },
   m_substitutes           {other.m_substitutes      },
   m_humulene_pct          {other.m_humulene_pct     },
   m_caryophyllene_pct     {other.m_caryophyllene_pct},
   m_cohumulone_pct        {other.m_cohumulone_pct   },
   m_myrcene_pct           {other.m_myrcene_pct      } {
   return;
}

//============================="SET" METHODS====================================
void Hop::setAlpha_pct( double num )
{
   if( num < 0.0 || num > 100.0 )
   {
      qWarning() << QString("Hop: 0 < alpha < 100: %1").arg(num);
      return;
   }
   else
   {
      m_alpha_pct = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::alpha_pct, num);
      }
   }
}

void Hop::setAmount_kg( double num )
{
   if( num < 0.0 )
   {
      qWarning() << QString("Hop: amount < 0: %1").arg(num);
      return;
   }
   else
   {
      m_amount_kg = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::amount_kg,num);
      }
   }
}

void Hop::setInventoryAmount(double num) {
   InventoryUtils::setAmount(*this, num);
   return;
}

void Hop::setUse(Use u)
{
   if ( u < uses.size()) {
      m_use = u;
      m_useStr = uses.at(u);
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::use, uses.at(u));
      }
   }
}

void Hop::setTime_min( double num )
{
   if( num < 0.0 )
   {
      qWarning() << QString("Hop: time < 0: %1").arg(num);
      return;
   }
   else
   {
      m_time_min = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::time_min, num);
      }
   }
}

void Hop::setNotes( const QString& str )
{
   m_notes = str;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Hop::notes, str);
   }
}

void Hop::setType(Type t)
{
  if ( t < types.size() ) {
     m_type = t;
     m_typeStr = types.at(t);
     if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Hop::type, m_typeStr);
     }
  }
}

void Hop::setForm( Form f )
{
   if ( f < forms.size() ) {
      m_form = f;
      m_formStr = forms.at(f);
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::form, m_formStr);
      }
   }
}

void Hop::setBeta_pct( double num )
{
   if( num < 0.0 || num > 100.0 )
   {
      qWarning() << QString("Hop: 0 < beta < 100: %1").arg(num);
      return;
   }
   else
   {
      m_beta_pct = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::beta_pct, num);
      }
   }
}

void Hop::setHsi_pct( double num )
{
   if( num < 0.0 || num > 100.0 )
   {
      qWarning() << QString("Hop: 0 < hsi < 100: %1").arg(num);
      return;
   }
   else
   {
      m_hsi_pct = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::hsi_pct, num);
      }
   }
}

void Hop::setOrigin( const QString& str )
{
   m_origin = str;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Hop::origin, str);
   }
}

void Hop::setSubstitutes( const QString& str )
{
   m_substitutes = str;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Hop::substitutes, str);
   }
}

void Hop::setHumulene_pct( double num )
{
   if( num < 0.0 || num > 100.0 )
   {
      qWarning() << QString("Hop: 0 < humulene < 100: %1").arg(num);
      return;
   }
   else
   {
      m_humulene_pct = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::humulene_pct,num);
      }
   }
}

void Hop::setCaryophyllene_pct( double num )
{
   if( num < 0.0 || num > 100.0 )
   {
      qWarning() << QString("Hop: 0 < cary < 100: %1").arg(num);
      return;
   }
   else
   {
      m_caryophyllene_pct = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::caryophyllene_pct, num);
      }
   }
}

void Hop::setCohumulone_pct( double num )
{
   if( num < 0.0 || num > 100.0 )
   {
      qWarning() << QString("Hop: 0 < cohumulone < 100: %1").arg(num);
      return;
   }
   else
   {
      m_cohumulone_pct = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::cohumulone_pct, num);
      }
   }
}

void Hop::setMyrcene_pct( double num )
{
   if( num < 0.0 || num > 100.0 )
   {
      qWarning() << QString("Hop: 0 < myrcene < 100: %1").arg(num);
      return;
   }
   else
   {
      m_myrcene_pct = num;
      if ( ! m_cacheOnly ) {
         setEasy(PropertyNames::Hop::myrcene_pct, num);
      }
   }
}

//============================="GET" METHODS====================================

Hop::Use Hop::use() const { return m_use; }
const QString Hop::useString() const { return m_useStr; }
const QString Hop::notes() const { return m_notes; }
Hop::Type Hop::type() const { return m_type; }
const QString Hop::typeString() const { return m_typeStr; }
Hop::Form Hop::form() const { return m_form; }
const QString Hop::formString() const { return m_formStr; }
const QString Hop::origin() const { return m_origin; }
const QString Hop::substitutes() const { return m_substitutes; }
double Hop::alpha_pct() const { return m_alpha_pct; }
double Hop::amount_kg() const { return m_amount_kg; }
double Hop::time_min() const { return m_time_min; }
double Hop::beta_pct() const { return m_beta_pct; }
double Hop::hsi_pct() const { return m_hsi_pct; }
double Hop::humulene_pct() const { return m_humulene_pct; }
double Hop::caryophyllene_pct() const { return m_caryophyllene_pct; }
double Hop::cohumulone_pct() const { return m_cohumulone_pct; }
double Hop::myrcene_pct() const { return m_myrcene_pct; }

double Hop::inventory() const {
   return InventoryUtils::getAmount(*this);
}

const QString Hop::useStringTr() const
{
   static QStringList usesTr = QStringList() << tr("Mash") << tr("First Wort") << tr("Boil") << tr("Aroma") << tr("Dry Hop") ;
   if ( m_use < usesTr.size() && m_use >= 0 ) {
      return usesTr.at(m_use);
   }
   else {
      return "";
   }
}

const QString Hop::typeStringTr() const
{
   static QStringList typesTr = QStringList() << tr("Bittering") << tr("Aroma") << tr("Both");
   if ( m_type < typesTr.size()  && m_type >= 0 ) {
      return typesTr.at(m_type);
   }
   else {
      return "";
   }
}

const QString Hop::formStringTr() const
{
   static QStringList formsTr = QStringList() << tr("Leaf") << tr("Pellet") << tr("Plug");
   if ( m_form < formsTr.size() && m_form >= 0 ) {
      return formsTr.at(m_form);
   }
   else {
      return "";
   }
}
