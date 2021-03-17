/**
 * database/WaterSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
 *   • Mik Firestone <mikfire@gmail.com>
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

#ifndef WATERSCHEMA_H
#define WATERSCHEMA_H

#include <QSqlTableModel>

#include <QString>
// Columns for the yeast table
// What isn't here (like name) is defined in TableSchemaConstants
static const QString kcolWaterCalcium("calcium");
static const QString kcolWaterBiCarbonate("bicarbonate");
static const QString kcolWaterSulfate("sulfate");
static const QString kcolWaterChloride("chloride");
static const QString kcolWaterSodium("sodium");
static const QString kcolWaterMagnesium("magnesium");
static const QString kcolWaterAlkalinity("alkalinity");
static const QString kcolWaterMashRO("mash_ro");
static const QString kcolWaterSpargeRO("sparge_ro");
static const QString kcolWaterAsHCO3("as_hco3");
static const QString kcolWaterType("wtype");

// properties for objects

// XML properties
static const QString kxmlPropCalcium("CALCIUM");
static const QString kxmlPropBiCarbonate("BICARBONATE");
static const QString kxmlPropSulfate("SULFATE");
static const QString kxmlPropChloride("CHLORIDE");
static const QString kxmlPropSodium("SODIUM");
static const QString kxmlPropMagnesium("MAGNESIUM");

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

template<class NE>
class DatabaseStore : public QSqlTableModel {
public:
   enum FieldType {
      Bool,
      Int,
      UInt,
      Double,
      String,
      Date,
      Enum,
      Record
   };

   struct FieldDefinition {
      char const * const  propertyName;
      QString             columnName;
      FieldType           fieldType;
   };

   typedef QVector<FieldDefinition> FieldDefinitions;

   DatabaseStore(char const * const tableName,
                 FieldDefinitions const & fieldDefinitions);

   NE * getObject();
   QHash<int, NE*> allObjects;

};

#endif
