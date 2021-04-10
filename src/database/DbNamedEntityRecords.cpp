/**
 * database/DbNamedEntityRecords.cpp is part of Brewken, and is copyright the following authors 2021:
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
 */
#include "database/DbNamedEntityRecords.h"

#include "model/Water.h"

namespace {

   DbNamedEntityRecords<Water>::FieldDefinitions const WATER_FIELDS {
      {DbNamedEntityRecords<Water>::FieldType::Int,    "id",          PropertyNames::NamedEntity::key},
      {DbNamedEntityRecords<Water>::FieldType::String, "name",        PropertyNames::NamedEntity::name},
      {DbNamedEntityRecords<Water>::FieldType::Bool,   "display",     PropertyNames::NamedEntity::display},
      {DbNamedEntityRecords<Water>::FieldType::Bool,   "deleted",     PropertyNames::NamedEntity::deleted},
      {DbNamedEntityRecords<Water>::FieldType::String, "folder",      PropertyNames::NamedEntity::folder},
      {DbNamedEntityRecords<Water>::FieldType::String, "notes",       PropertyNames::Water::notes},
      {DbNamedEntityRecords<Water>::FieldType::Double, "amount",      PropertyNames::Water::amount},
      {DbNamedEntityRecords<Water>::FieldType::Double, "calcium",     PropertyNames::Water::calcium_ppm},
      {DbNamedEntityRecords<Water>::FieldType::Double, "bicarbonate", PropertyNames::Water::bicarbonate_ppm},
      {DbNamedEntityRecords<Water>::FieldType::Double, "sulfate",     PropertyNames::Water::sulfate_ppm},
      {DbNamedEntityRecords<Water>::FieldType::Double, "sodium",      PropertyNames::Water::sodium_ppm},
      {DbNamedEntityRecords<Water>::FieldType::Double, "chloride",    PropertyNames::Water::chloride_ppm},
      {DbNamedEntityRecords<Water>::FieldType::Double, "magnesium",   PropertyNames::Water::magnesium_ppm},
      {DbNamedEntityRecords<Water>::FieldType::Double, "ph",          PropertyNames::Water::ph},
      {DbNamedEntityRecords<Water>::FieldType::Double, "alkalinity",  PropertyNames::Water::alkalinity},
      {DbNamedEntityRecords<Water>::FieldType::Int,    "wtype",       PropertyNames::Water::type},             // TODO: Would be less fragile to store this as text than a number
      {DbNamedEntityRecords<Water>::FieldType::Double, "mash_ro",     PropertyNames::Water::mashRO},
      {DbNamedEntityRecords<Water>::FieldType::Double, "sparge_ro",   PropertyNames::Water::spargeRO},
      {DbNamedEntityRecords<Water>::FieldType::Bool,   "as_hco3",     PropertyNames::Water::alkalinityAsHCO3}
   };



}

template<> DbNamedEntityRecords<Water> & DbNamedEntityRecords<Water>::getInstance() {
   static DbNamedEntityRecords<Water> waterRecords{"water", WATER_FIELDS};
   return waterRecords;
}

//
// Functions to extract from Database class:
//
//   Water* newWater(Water* other = nullptr);
//   int    insertWater(Water* ins);
//   Water* water(int key); AKA getWater(int key)
//      Water * addToRecipe( Recipe* rec, Water* w, bool noCopy = false, bool transact = true);
//   Q_PROPERTY( QList<Water*> waters READ waters /*WRITE*/ NOTIFY changed STORED false )
//   QList<Water*> waters();
//   //! Return a list of all the waters in a recipe.
//   QList<Water*> waters( Recipe const* parent );
// signals:
//   void changed(QMetaProperty prop, QVariant value);
//   void newWaterSignal(Water*);
//   void deletedSignal(Water*);
//   QHash< int, Water* > allWaters;
//
//   void updateEntry( NamedEntity* object, QString propName, QVariant value, bool notify = true, bool transact = false );
//
//   template<class T> T* newNamedEntity(QHash<int,T*>* all) {...}
//   template<class T> T* newNamedEntity(QString name, QHash<int,T*>* all) {...}
//   int    insertElement(NamedEntity* ins);
//   //! \returns true if this ingredient is stored in the DB, false otherwise
//   bool isStored(NamedEntity const & ingredient);
//   void setInventory(NamedEntity* ins, QVariant value, int invKey = 0, bool notify=true );
//   /**
//   * \brief  This function is intended to be called by an ingredient that has not already cached its parent's key
//   * \return Key of parent ingredient if there is one, 0 otherwise
//   */
//   int getParentNamedEntityKey(NamedEntity const & ingredient);
//
//   /*! \brief Removes the specified ingredient from the recipe, then calls the changed()
//    *         signal corresponding to the appropriate QList
//    *         of ingredients in rec.
//    *  \param rec
//    *  \param ing
//    *  \returns the parent of the ingredient deleted (which is needed to be able to undo the removal)
//    */
//   NamedEntity * removeNamedEntityFromRecipe( Recipe* rec, NamedEntity* ing );
//
//   template <class T> void populateElements( QHash<int,T*>& hash, DatabaseConstants::DbTableId table );
//
//   template <class T> bool getElementsByName( QList<T*>& list, DatabaseConstants::DbTableId table, QString name, QHash<int,T*> allElements, QString id=QString("") )
//   void deleteRecord( NamedEntity* object );
//   template<class T> T* addNamedEntityToRecipe(
//      Recipe* rec,
//      NamedEntity* ing,
//      bool noCopy = false,
//      QHash<int,T*>* keyHash = 0,
//      bool doNotDisplay = true,
//      bool transact = true
//   );
//   /*!
//    * \brief Create a deep copy of the \b object.
//    * \em T must be a subclass of \em NamedEntity.
//    * \returns a pointer to the new copy. You must manually emit the changed()
//    * signal after a copy() call. Also, does not insert things magically into
//    * allHop or allInstructions etc. hashes. This just simply duplicates a
//    * row in a table, unless you provide \em keyHash.
//    * \param object is the thing you want to copy.
//    * \param displayed is true if you want the \em displayed column set to true.
//    * \param keyHash if nonzero, inserts the new (key,T*) pair into the hash.
//    */
//   template<class T> T* copy( NamedEntity const* object, QHash<int,T*>* keyHash, bool displayed = true );
//   // Do an sql update.
//   void sqlUpdate( DatabaseConstants::DbTableId table, QString const& setClause, QString const& whereClause );
//
//   // Do an sql delete.
//   void sqlDelete( DatabaseConstants::DbTableId table, QString const& whereClause );
//   QMap<QString, std::function<NamedEntity*(QString name)> > makeTableParams();
//

//


//
// Water table name "water"
// Int "id" "key"
/* CREATE TABLE water(
   id integer PRIMARY KEY autoincrement,
   -- BeerXML properties
   name varchar(256) not null DEFAULT '',
   amount real DEFAULT 0.0,
   calcium real DEFAULT 0.0,
   bicarbonate real DEFAULT 0.0,
   sulfate real DEFAULT 0.0,
   chloride real DEFAULT 0.0,
   sodium real DEFAULT 0.0,
   magnesium real DEFAULT 0.0,
   ph real DEFAULT 7.0,
   notes text DEFAULT '',
   -- metadata
   deleted boolean DEFAULT 0,
   display boolean DEFAULT 1,
   folder varchar(256) DEFAULT ''
, wtype int DEFAULT 0, alkalinity real DEFAULT 0, as_hco3 boolean DEFAULT true, sparge_ro real DEFAULT 0, mash_ro real DEFAULT 0)
*/
//
