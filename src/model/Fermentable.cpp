/**
 * model/Fermentable.cpp is part of Brewken, and is copyright the following authors 2009-2020:
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
 */

#include <QDomElement>
#include <QDomText>
#include <QVariant>
#include <QObject>
#include <QDebug>
#include "model/Fermentable.h"
#include "Brewken.h"
#include "database/Database.h"

#include "database/TableSchemaConst.h"
#include "database/FermentableSchema.h"
#define SUPER NamedEntity

QStringList Fermentable::types = QStringList() << "Grain" << "Sugar" << "Extract" << "Dry Extract" << "Adjunct";

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

DbRecords & Fermentable::getDbNamedEntityRecordsInstance() const {
   return DbNamedEntityRecords<Fermentable>::getInstance();
}


QString Fermentable::classNameStr()
{
   static const QString name("Fermentable");
   return name;
}

Fermentable::Fermentable(QString name, bool cache)
   : NamedEntity(DatabaseConstants::FERMTABLE, -1, name, true),
     m_typeStr(QString()),
     m_type(static_cast<Fermentable::Type>(0)),
     m_amountKg(0.0),
     m_yieldPct(0.0),
     m_colorSrm(0.0),
     m_isAfterBoil(false),
     m_origin(QString()),
     m_supplier(QString()),
     m_notes(QString()),
     m_coarseFineDiff(0.0),
     m_moisturePct(0.0),
     m_diastaticPower(0.0),
     m_proteinPct(0.0),
     m_maxInBatchPct(100.0),
     m_recommendMash(false),
     m_ibuGalPerLb(0.0),
     m_inventory(-1.0),
     m_inventory_id(0),
     m_isMashed(false),
     m_cacheOnly(cache)
{
}

Fermentable::Fermentable(NamedParameterBundle & namedParameterBundle) :
   NamedEntity(namedParameterBundle, DatabaseConstants::FERMTABLE),
   m_type          {static_cast<Fermentable::Type>(namedParameterBundle(PropertyNames::Fermentable::type).toInt())},
   m_amountKg      {namedParameterBundle(PropertyNames::Fermentable::amount_kg             ).toDouble()},
   m_yieldPct      {namedParameterBundle(PropertyNames::Fermentable::yield_pct             ).toDouble()},
   m_colorSrm      {namedParameterBundle(PropertyNames::Fermentable::color_srm             ).toDouble()},
   m_isAfterBoil   {namedParameterBundle(PropertyNames::Fermentable::addAfterBoil          ).toBool()},
   m_origin        {namedParameterBundle(PropertyNames::Fermentable::origin                ).toString()},
   m_supplier      {namedParameterBundle(PropertyNames::Fermentable::supplier              ).toString()},
   m_notes         {namedParameterBundle(PropertyNames::Fermentable::notes                 ).toString()},
   m_coarseFineDiff{namedParameterBundle(PropertyNames::Fermentable::coarseFineDiff_pct    ).toDouble()},
   m_moisturePct   {namedParameterBundle(PropertyNames::Fermentable::moisture_pct          ).toDouble()},
   m_diastaticPower{namedParameterBundle(PropertyNames::Fermentable::diastaticPower_lintner).toDouble()},
   m_proteinPct    {namedParameterBundle(PropertyNames::Fermentable::protein_pct           ).toDouble()},
   m_maxInBatchPct {namedParameterBundle(PropertyNames::Fermentable::maxInBatch_pct        ).toDouble()},
   m_recommendMash {namedParameterBundle(PropertyNames::Fermentable::recommendMash         ).toBool()},
   m_ibuGalPerLb   {namedParameterBundle(PropertyNames::Fermentable::ibuGalPerLb           ).toDouble()},
   //m_inventory     {namedParameterBundle(PropertyNames::Fermentable::inventory             ).toDouble()},
   //m_inventory_id  {namedParameterBundle(PropertyNames::Fermentable::inventory_id          ).toInt()},
   m_isMashed      {namedParameterBundle(PropertyNames::Fermentable::isMashed              ).toBool()},
   m_cacheOnly     {false} {
   return;
}


Fermentable::Fermentable(DatabaseConstants::DbTableId table, int key)
   : NamedEntity(table, key, QString(), true),
     m_typeStr(QString()),
     m_type(static_cast<Fermentable::Type>(0)),
     m_amountKg(0.0),
     m_yieldPct(0.0),
     m_colorSrm(0.0),
     m_isAfterBoil(false),
     m_origin(QString()),
     m_supplier(QString()),
     m_notes(QString()),
     m_coarseFineDiff(0.0),
     m_moisturePct(0.0),
     m_diastaticPower(0.0),
     m_proteinPct(0.0),
     m_maxInBatchPct(0.0),
     m_recommendMash(true),
     m_ibuGalPerLb(0.0),
     m_inventory(-1.0),
     m_inventory_id(0),
     m_isMashed(true),
     m_cacheOnly(false)
{
}

Fermentable::Fermentable(DatabaseConstants::DbTableId table, int key, QSqlRecord rec)
   : NamedEntity(table, key, rec.value(kcolName).toString(), rec.value(kcolDisplay).toBool(), rec.value(kcolFolder).toString()),
     m_typeStr(rec.value(kcolFermType).toString()),
     m_type(static_cast<Fermentable::Type>(types.indexOf(m_typeStr))),
     m_amountKg(rec.value(kcolFermAmount).toDouble()),
     m_yieldPct(rec.value(kcolFermYield).toDouble()),
     m_colorSrm(rec.value(kcolFermColor).toDouble()),
     m_isAfterBoil(rec.value(kcolFermAddAfterBoil).toBool()),
     m_origin(rec.value(kcolFermOrigin).toString()),
     m_supplier(rec.value(kcolFermSupplier).toString()),
     m_notes(rec.value(kcolNotes).toString()),
     m_coarseFineDiff(rec.value(kcolFermCoarseFineDiff).toDouble()),
     m_moisturePct(rec.value(kcolFermMoisture).toDouble()),
     m_diastaticPower(rec.value(kcolFermDiastaticPower).toDouble()),
     m_proteinPct(rec.value(kcolFermProtein).toDouble()),
     m_maxInBatchPct(rec.value(kcolFermMaxInBatch).toDouble()),
     m_recommendMash(rec.value(kcolFermRecommendMash).toBool()),
     m_ibuGalPerLb(rec.value(kcolFermIBUGalPerLb).toDouble()),
     m_inventory(-1.0),
     m_inventory_id(rec.value(kcolInventoryId).toInt()),
     m_isMashed(rec.value(kcolFermIsMashed).toBool()),
     m_cacheOnly(false)
{
}

Fermentable::Fermentable(Fermentable const & other) :
   NamedEntity( other ),
   m_typeStr       (other.m_typeStr),
   m_type            (other.m_type),
   m_amountKg      (other.m_amountKg),
   m_yieldPct      (other.m_yieldPct),
   m_colorSrm      (other.m_colorSrm),
   m_isAfterBoil   (other.m_isAfterBoil),
   m_origin        (other.m_origin),
   m_supplier      (other.m_supplier),
   m_notes         (other.m_notes),
   m_coarseFineDiff(other.m_coarseFineDiff),
   m_moisturePct   (other.m_moisturePct),
   m_diastaticPower(other.m_diastaticPower),
   m_proteinPct    (other.m_proteinPct),
   m_maxInBatchPct (other.m_maxInBatchPct),
   m_recommendMash (other.m_recommendMash),
   m_ibuGalPerLb   (other.m_ibuGalPerLb),
   m_inventory     (other.m_inventory),
   m_inventory_id  (other.m_inventory_id),
   m_isMashed      (other.m_isMashed),
   m_cacheOnly     (other.m_cacheOnly) {
   return;
}

// Gets

Fermentable::Type Fermentable::type() const { return m_type; }
double Fermentable::amount_kg() const { return m_amountKg; }
double Fermentable::yield_pct() const { return m_yieldPct; }
double Fermentable::color_srm() const { return m_colorSrm; }
bool Fermentable::addAfterBoil() const { return m_isAfterBoil; }
const QString Fermentable::origin() const { return m_origin; }
const QString Fermentable::supplier() const { return m_supplier; }
const QString Fermentable::notes() const { return m_notes; }
double Fermentable::coarseFineDiff_pct() const { return m_coarseFineDiff; }
double Fermentable::moisture_pct() const { return m_moisturePct; }
double Fermentable::diastaticPower_lintner() const { return m_diastaticPower; }
double Fermentable::protein_pct() const { return m_proteinPct; }
double Fermentable::maxInBatch_pct() const { return m_maxInBatchPct; }
bool Fermentable::recommendMash() const { return m_recommendMash; }
double Fermentable::ibuGalPerLb() const { return m_ibuGalPerLb; }
bool Fermentable::isMashed() const { return m_isMashed; }
bool Fermentable::cacheOnly() const { return m_cacheOnly; }

Fermentable::AdditionMethod Fermentable::additionMethod() const
{
   Fermentable::AdditionMethod additionMethod;
   if(isMashed())
      additionMethod = Fermentable::Mashed;
   else
   {
      if(type() == Fermentable::Grain)
         additionMethod = Fermentable::Steeped;
      else
         additionMethod = Fermentable::Not_Mashed;
   }
   return additionMethod;
}

Fermentable::AdditionTime Fermentable::additionTime() const
{
   Fermentable::AdditionTime additionTime;
   if(addAfterBoil())
      additionTime = Fermentable::Late;
   else
      additionTime = Fermentable::Normal;

   return additionTime;
}

const QString Fermentable::typeString() const
{
   if ( m_type > types.length()) {
      return "";
   }
   return types.at(type());
}

const QString Fermentable::typeStringTr() const
{
   static QStringList typesTr = QStringList () << QObject::tr("Grain") << QObject::tr("Sugar") << QObject::tr("Extract") << QObject::tr("Dry Extract") << QObject::tr("Adjunct");
   if ( m_type > typesTr.length() || m_type < 0 ) {
      return "";
   }

   return typesTr.at(m_type);
}

const QString Fermentable::additionMethodStringTr() const
{
    QString retString;

    if(isMashed())
       retString = tr("Mashed");
    else
    {
       if(type() == Fermentable::Grain)
          retString = tr("Steeped");
       else
          retString = tr("Not mashed");
    }
    return retString;
}

const QString Fermentable::additionTimeStringTr() const
{
    QString retString;

    if(addAfterBoil())
       retString = tr("Late");
    else
       retString = tr("Normal");

    return retString;
}

bool Fermentable::isExtract() const
{
   return ((type() == Extract) || (type() == Dry_Extract));
}

bool Fermentable::isSugar() const
{
   return (type() == Sugar);
}

bool Fermentable::isValidType( const QString& str )
{
   return (types.indexOf(str) >= 0);
}


// Sets
void Fermentable::setType( Type t )
{
   m_type = t;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Fermentable::type, types.at(t));
   }
}

void Fermentable::setAdditionMethod( Fermentable::AdditionMethod m )
{
   setIsMashed(m == Fermentable::Mashed);
}

void Fermentable::setAdditionTime( Fermentable::AdditionTime t )
{
   setAddAfterBoil(t == Fermentable::Late);
}

void Fermentable::setAddAfterBoil( bool b )
{
   m_isAfterBoil = b;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Fermentable::addAfterBoil, b);
   }
}

void Fermentable::setOrigin( const QString& str )
{
   m_origin = str;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Fermentable::origin, str);
   }
}

void Fermentable::setSupplier( const QString& str)
{
   m_supplier = str;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Fermentable::supplier, str);
   }
}

void Fermentable::setNotes( const QString& str )
{
   m_notes = str;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Fermentable::notes, str);
   }
}

void Fermentable::setRecommendMash( bool b )
{
   m_recommendMash = b;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Fermentable::recommendMash, b);
   }
}

void Fermentable::setIsMashed(bool var)
{
   m_isMashed = var;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Fermentable::isMashed, var);
   }
}

void Fermentable::setIbuGalPerLb( double num )
{
   m_ibuGalPerLb = num;
   if ( ! m_cacheOnly ) {
      setEasy( PropertyNames::Fermentable::ibuGalPerLb, num);
   }
}

double Fermentable::equivSucrose_kg() const
{
   double ret = amount_kg() * yield_pct() * (1.0-moisture_pct()/100.0) / 100.0;

   // If this is a steeped grain...
   if( type() == Grain && !isMashed() )
      return 0.60 * ret; // Reduce the yield by 60%.
   else
      return ret;
}

void Fermentable::setAmount_kg( double num )
{
   if( num < 0.0 )
   {
      qWarning() << QString("Fermentable: negative amount: %1").arg(num);
      return;
   }
   else
   {
      m_amountKg = num;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Fermentable::amount_kg, num);
      }
   }
}

void Fermentable::setInventoryAmount( double num )
{
   if( num < 0.0 ) {
      qWarning() << QString("Fermentable: negative inventory: %1").arg(num);
      return;
   }
   else
   {
      m_inventory = num;
      if ( ! m_cacheOnly )
         setInventory(num,m_inventory_id);
   }
}

void Fermentable::setInventoryId( int key )
{
   if( key < 1 ) {
      qWarning() << QString("Fermentable: bad inventory id: %1").arg(key);
      return;
   }
   else
   {
      m_inventory_id = key;
      if ( ! m_cacheOnly )
         setEasy(kpropInventoryId,key);
   }
}

double Fermentable::inventory()
{
   if ( m_inventory < 0 ) {
      m_inventory = getInventory(PropertyNames::Fermentable::inventory).toDouble();
   }
   return m_inventory;
}

int Fermentable::inventoryId()
{
   return m_inventory_id;
}

void Fermentable::setYield_pct( double num )
{
   if( num >= 0.0 && num <= 100.0 )
   {
      m_yieldPct = num;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Fermentable::yield_pct, num);
      }
   }
   else
   {
      qWarning() << QString("Fermentable: 0 < yield < 100: %1").arg(num);
   }
}

void Fermentable::setColor_srm( double num )
{
   if( num < 0.0 )
   {
      qWarning() << QString("Fermentable: negative color: %1").arg(num);
      return;
   }
   else
   {
      m_colorSrm = num;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Fermentable::color_srm, num);
      }
   }
}

void Fermentable::setCoarseFineDiff_pct( double num )
{
   if( num >= 0.0 && num <= 100.0 )
   {
      m_coarseFineDiff = num;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Fermentable::coarseFineDiff_pct, num);
      }
   }
   else
   {
      qWarning() << QString("Fermentable: 0 < coarsefinediff < 100: %1").arg(num);
   }
}

void Fermentable::setMoisture_pct( double num )
{
   if( num >= 0.0 && num <= 100.0 )
   {
      m_moisturePct = num;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Fermentable::moisture_pct, num);
      }
   }
   else
   {
      qWarning() << QString("Fermentable: 0 < moisture < 100: %1").arg(num);
   }
}

void Fermentable::setDiastaticPower_lintner( double num )
{
   if( num < 0.0 )
   {
      qWarning() << QString("Fermentable: negative DP: %1").arg(num);
      return;
   }
   else
   {
      m_diastaticPower = num;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Fermentable::diastaticPower_lintner, num);
      }
   }
}

void Fermentable::setProtein_pct( double num )
{
   if( num >= 0.0 && num <= 100.0 )
   {
      m_proteinPct = num;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Fermentable::protein_pct, num);
      }
   }
   else
   {
      qWarning() << QString("Fermentable: 0 < protein < 100: %1").arg(num);
   }
}

void Fermentable::setMaxInBatch_pct( double num )
{
   if( num >= 0.0 && num <= 100.0 )
   {
      m_maxInBatchPct = num;
      if ( ! m_cacheOnly ) {
         setEasy( PropertyNames::Fermentable::maxInBatch_pct, num);
      }
   }
   else
   {
      qWarning() << QString("Fermentable: 0 < maxinbatch < 100: %1").arg(num);
   }
}

void Fermentable::setCacheOnly( bool cache ) { m_cacheOnly = cache; }

NamedEntity * Fermentable::getParent() {
   Fermentable * myParent = nullptr;

   // If we don't already know our parent, look it up
   if (!this->parentKey) {
      this->parentKey = Database::instance().getParentNamedEntityKey(*this);
   }

   // If we (now) know our parent, get a pointer to it
   if (this->parentKey) {
      myParent = ObjectStoreWrapper::getByIdRaw<Fermentable>(this->parentKey);
   }

   // Return whatever we got
   return myParent;
}
