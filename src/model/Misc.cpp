/**
 * model/Misc.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
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
 */
#include "model/Misc.h"

#include <iostream>
#include <string>

#include <QDebug>
#include <QDomElement>
#include <QDomText>
#include <QObject>
#include <QVector>

#include "Brewken.h"
#include "database/ObjectStoreWrapper.h"
#include "model/Inventory.h"

QStringList Misc::uses = QStringList() << "Boil" << "Mash" << "Primary" << "Secondary" << "Bottling";
QStringList Misc::types = QStringList() << "Spice" << "Fining" << "Water Agent" << "Herb" << "Flavor" << "Other";
QStringList Misc::amountTypes = QStringList() << "Weight" << "Volume";

QString Misc::classNameStr()
{
   static const QString name("Misc");
   return name;
}

bool Misc::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Misc const & rhs = static_cast<Misc const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_type == rhs.m_type &&
      this->m_use  == rhs.m_use
   );
}

ObjectStore & Misc::getObjectStoreTypedInstance() const {
   return ObjectStoreTyped<Misc>::getInstance();
}

//============================CONSTRUCTORS======================================
Misc::Misc(Misc const & other) :
   NamedEntityWithInventory{other                 },
   m_typeString            {other.m_typeString    },
   m_type                  {other.m_type          },
   m_useString             {other.m_useString     },
   m_use                   {other.m_use           },
   m_time                  {other.m_time          },
   m_amount                {other.m_amount        },
   m_amountIsWeight        {other.m_amountIsWeight},
   m_useFor                {other.m_useFor        },
   m_notes                 {other.m_notes         } {
   return;
}

Misc::Misc(QString name, bool cache) :
   NamedEntityWithInventory{-1, cache, name, true},
   m_typeString            {""                   },
   m_type                  {Misc::Spice          },
   m_useString             {""                   },
   m_use                   {Misc::Boil           },
   m_time                  {0.0                  },
   m_amount                {0.0                  },
   m_amountIsWeight        {false                },
   m_useFor                {""                   },
   m_notes                 {""                   } {
   return;
}

Misc::Misc(NamedParameterBundle const & namedParameterBundle) :
   NamedEntityWithInventory{namedParameterBundle},
   m_type                  {static_cast<Misc::Type>(namedParameterBundle(PropertyNames::Misc::type).toInt())},
   m_use                   {static_cast<Misc::Use>(namedParameterBundle(PropertyNames::Misc::use).toInt())},
   m_time                  {namedParameterBundle(PropertyNames::Misc::time          ).toDouble()},
   m_amount                {namedParameterBundle(PropertyNames::Misc::amount        ).toDouble()},
   m_amountIsWeight        {namedParameterBundle(PropertyNames::Misc::amountIsWeight).toBool()},
   m_useFor                {namedParameterBundle(PropertyNames::Misc::useFor        ).toString()},
   m_notes                 {namedParameterBundle(PropertyNames::Misc::notes         ).toString()} {
   return;
}


//============================"GET" METHODS=====================================
Misc::Type Misc::type() const { return m_type; }

const QString Misc::typeString() const { return m_typeString; }

Misc::Use Misc::use() const { return m_use; }

const QString Misc::useString() const { return m_useString; }

double Misc::amount() const { return m_amount; }

double Misc::time() const { return m_time; }

bool Misc::amountIsWeight() const { return m_amountIsWeight; }

QString Misc::useFor() const { return m_useFor; }

QString Misc::notes() const { return m_notes; }

double Misc::inventory() const {
   return InventoryUtils::getAmount(*this);
}

Misc::AmountType Misc::amountType() const { return m_amountIsWeight ? AmountType_Weight : AmountType_Volume; }

const QString Misc::amountTypeString() const { return amountTypes.at(amountType()); }

const QString Misc::typeStringTr() const
{
   QStringList typesTr = QStringList() << tr("Spice") << tr("Fining") << tr("Water Agent") << tr("Herb") << tr("Flavor") << tr("Other");
   if ( m_type >=  Spice && m_type < typesTr.size()  ) {
      return typesTr.at(m_type);
   }
   else {
      return QString("Spice");
   }
}

const QString Misc::useStringTr() const
{
   QStringList usesTr = QStringList() << tr("Boil") << tr("Mash") << tr("Primary") << tr("Secondary") << tr("Bottling");
   if ( m_use >= Boil && m_use < usesTr.size() ) {
      return usesTr.at(use());
   }
   else {
      return QString("Boil");
   }
}

const QString Misc::amountTypeStringTr() const
{
   QStringList amountTypesTr = QStringList() << tr("Weight") << tr("Volume");
   if ( amountType() ) {
      return amountTypesTr.at(amountType());
   }
   else {
      return QString("Weight");
   }
}

//============================"SET" METHODS=====================================
void Misc::setType( Type t )
{
   m_type = t;
   m_typeString = types.at(t);
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Misc::type, m_typeString );
   }
}

void Misc::setUse( Use u )
{
   m_use = u;
   m_useString = uses.at(u);
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Misc::use, m_useString );
   }
}

void Misc::setUseFor( const QString& var )
{
   m_useFor = var;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Misc::useFor, var );
   }
}

void Misc::setNotes( const QString& var )
{
   m_notes = var;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Misc::notes, var );
   }
}

void Misc::setAmountType( AmountType t )
{
   m_amountIsWeight = t == AmountType_Weight;
   if ( ! m_cacheOnly ) {
      setAmountIsWeight(m_amountIsWeight);
   }
}

void Misc::setAmountIsWeight( bool var )
{
   m_amountIsWeight = var;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Misc::amountIsWeight, var );
   }
}

void Misc::setAmount( double var )
{
   if( var < 0.0 )
      qWarning() << QString("Misc: amount < 0: %1").arg(var);
   else {
      m_amount = var;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Misc::amount, var );
      }
   }
}

void Misc::setInventoryAmount(double var) {
   InventoryUtils::setAmount(*this, var);
   return;
}

void Misc::setTime( double var )
{
   if( var < 0.0 )
      qWarning() << QString("Misc: time < 0: %1").arg(var);
   else {
      m_time = var;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Misc::time, var );
      }
   }
}

//========================OTHER METHODS=========================================

bool Misc::isValidUse( const QString& var )
{
   static const QString uses[] = {"Boil", "Mash", "Primary", "Secondary", "Bottling"};
   static const unsigned int size = 5;
   unsigned int i;

   for( i = 0; i < size; ++i )
      if( var == uses[i] )
         return true;

   return false;
}

bool Misc::isValidType( const QString& var )
{
   static const QString types[] = {"Spice", "Fining", "Water Agent", "Herb", "Flavor", "Other"};
   static const unsigned int size = 6;
   unsigned int i;

   for( i = 0; i < size; ++i )
      if( var == types[i] )
         return true;

   return false;
}
