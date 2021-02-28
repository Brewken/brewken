/**
 * TableSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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
#ifndef _TABLESCHEMA_H
#define _TABLESCHEMA_H

#include "PropertySchema.h"
#include "Brewken.h"
#include <QString>

class TableSchema : QObject
{

   Q_OBJECT

   friend class DatabaseSchemaHelper;
   friend class DatabaseSchema;
   friend class Database;
   friend class BeerXML;

public:

   enum TableType {
      BASE,
      INV,
      CHILD,
      INREC,
      BT,
      META
   };

   const QString tableName() const;
   const QString className() const;
   Brewken::DBTable dbTable() const;
   Brewken::DBTable childTable() const;
   Brewken::DBTable inRecTable() const;
   Brewken::DBTable invTable() const;
   Brewken::DBTable btTable() const;
   const QMap<QString, PropertySchema*> properties() const;
   const QMap<QString, PropertySchema*> foreignKeys() const;
   const PropertySchema* key() const;

   // Things to do for properties

   // Get the property object. Try not to use this?
   const PropertySchema* property(QString prop) const;
   // some properties may be named differently (like inventory v quanta)
   const QString propertyName(QString prop, Brewken::DBTypes type = Brewken::ALLDB) const;
   // get the database column name for this property
   const QString propertyToColumn(QString prop, Brewken::DBTypes type = Brewken::ALLDB) const;
   // get the database column type
   const QString propertyColumnType(QString prop, Brewken::DBTypes type = Brewken::ALLDB) const;
   // get the XML tag for this column
   const QString propertyToXml(QString prop, Brewken::DBTypes type = Brewken::ALLDB) const;
   // get the default value for this column
   const QVariant propertyColumnDefault(QString prop, Brewken::DBTypes type = Brewken::ALLDB) const;
   // get the column size of the property's column
   int propertyColumnSize(QString prop, Brewken::DBTypes type = Brewken::ALLDB) const;
   // given an XML tag, get the associated property name
   const QString xmlToProperty(QString xmlName, Brewken::DBTypes type = Brewken::ALLDB) const;
   // returns the property to be used for the increment/decrement triggers
   const QString triggerProperty() const;

   const QStringList allPropertyNames(Brewken::DBTypes type = Brewken::ALLDB) const;
   const QStringList allColumnNames(Brewken::DBTypes type = Brewken::ALLDB) const;

   // things to do on foreign keys
   // get a specific foreign key column name
   const QString foreignKeyToColumn(QString fkey, Brewken::DBTypes type = Brewken::ALLDB) const;
   // a lot of tables have one foreign key. This is a nice shortcut for that
   const QString foreignKeyToColumn(Brewken::DBTypes type = Brewken::ALLDB) const;

   // which table does this foreign key point to
   Brewken::DBTable foreignTable(QString fkey, Brewken::DBTypes type = Brewken::ALLDB) const;
   // a lot of tables have one foreign key. This is a nice shortcut for that
   Brewken::DBTable foreignTable(Brewken::DBTypes type = Brewken::ALLDB) const;

   const QStringList allForeignKeyNames(Brewken::DBTypes type = Brewken::ALLDB) const;
   const QStringList allForeignKeyColumnNames(Brewken::DBTypes type = Brewken::ALLDB) const;

   //!brief Use this to get the not recipe_id index from an inrec table
   const QString inRecIndexName(Brewken::DBTypes type = Brewken::ALLDB);
   //!brief Use this to get the child_id index from a children table
   const QString childIndexName(Brewken::DBTypes type = Brewken::ALLDB);
   //!brief Use this to get the recipe_id from a inrec table
   const QString recipeIndexName(Brewken::DBTypes type = Brewken::ALLDB);
   //!brief Use this to get the parent_id from a child table
   const QString parentIndexName(Brewken::DBTypes type = Brewken::ALLDB);

   // Not sure these belong here yet, but maybe
   const QString generateCreateTable(Brewken::DBTypes type = Brewken::ALLDB, QString tmpName = QString("") );
   const QString generateUpdateRow(int key, Brewken::DBTypes type = Brewken::ALLDB);
   const QString generateUpdateRow(Brewken::DBTypes type = Brewken::ALLDB);
   // this one includes the foreign keys and is really only suitable for copying databases
   const QString generateInsertRow(Brewken::DBTypes type = Brewken::ALLDB);
   // this one ignores the foreign keys and is more generally useful
   const QString generateInsertProperties(Brewken::DBTypes type = Brewken::ALLDB);
   // when dropping columns, we have to copy tables in sqlite. This does that.
   const QString generateCopyTable( QString dest, Brewken::DBTypes type = Brewken::ALLDB);

   const QString generateDecrementTrigger(Brewken::DBTypes type);
   const QString generateIncrementTrigger(Brewken::DBTypes type);

   bool isInventoryTable();
   bool isBaseTable();
   bool isChildTable();
   bool isInRecTable();
   bool isBtTable();
   bool isMetaTable();

   const QString keyName(Brewken::DBTypes type = Brewken::ALLDB) const;

private:

   // I only allow table schema to be made with a DBTable constant
   // It saves a lot of work, and I think the name to constant
   // mapping doesn't belong here -- it belongs in DatabaseSchema
   TableSchema(Brewken::DBTable dbTable);

   QString m_tableName;
   QString m_className;
   Brewken::DBTable m_dbTable;
   TableType m_type;

   // these are only set by the base tables
   Brewken::DBTable m_childTable;
   Brewken::DBTable m_inRecTable;
   Brewken::DBTable m_invTable;
   Brewken::DBTable m_btTable;

   QString m_trigger;

   PropertySchema* m_key;
   QMap<QString,PropertySchema*> m_properties;
   QMap<QString,PropertySchema*> m_foreignKeys;
   // It all depends on the call I want to make. I can require the type on
   // every call to a TableSchema object which is dull, repetitive and makes
   // some already difficult to read calls harder to read. Or I can cache the
   // default in the table and use that if ALLDB is sent, which breaks the
   // metaphor.
   Brewken::DBTypes m_defType;

   // getter only. But this is private because only my dearest,
   // closest friends can do this
   Brewken::DBTypes defType() const;

   void defineTable();
   void defineStyleTable();
   void defineEquipmentTable();
   void defineFermentableTable();
   void defineHopTable();
   void defineInstructionTable();
   void defineMashTable();
   void defineMashstepTable();
   void defineMiscTable();
   void defineRecipeTable();
   void defineYeastTable();
   void defineWaterTable();
   void defineSaltTable();
   void defineBrewnoteTable();
   void defineSettingsTable();

   // and we can get away with one method for the child tables
   void defineChildTable(Brewken::DBTable table);

   // and almost one method for all the in_recipe tables
   void defineInRecipeTable(QString childIdx, Brewken::DBTable table);
   // Instructions in recipe actually carry information. Sigh.
   void defineInstructionInRecipeTable( QString childIdx, Brewken::DBTable table);

   // one method for all the bt_tables
   void defineBtTable(QString childIdx, Brewken::DBTable table);

   // Inventory tables are strange and I didn't feel quite comfortable trying to make one
   // method for all of them;
   void defineFermInventoryTable();
   void defineHopInventoryTable();
   void defineMiscInventoryTable();
   void defineYeastInventoryTable();

};

#endif
