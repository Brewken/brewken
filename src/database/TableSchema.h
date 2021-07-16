/**
 * database/TableSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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
#ifndef TABLESCHEMA_H
#define TABLESCHEMA_H

#include <QString>

#include "database/Database.h"
#include "database/PropertySchema.h"
#include "Brewken.h"

class TableSchema : QObject
{

   Q_OBJECT

   friend class DatabaseSchema;
   friend class Database;


public:

   enum TableType {
      BASE,
      INV,
      CHILD,
      INREC,
      BT,
      META
   };

   //!brief get this TableSchema's name
   const QString tableName() const;
   //!brief get the class associated with this table, if any
   const QString className() const;
   //!brief get the table ID for this TableSchema
   DatabaseConstants::DbTableId dbTable() const;
   //!brief get the table ID for this TableSchema's child table, if any
   DatabaseConstants::DbTableId childTable() const;
   //!brief get the table ID for this TableSchema's _in_recipe table, if any
   DatabaseConstants::DbTableId inRecTable() const;
   //!brief get the table ID for this TableSchema's _inventory table, if any
   DatabaseConstants::DbTableId invTable() const;
   //!brief get the table ID for this TableSchema's bt_ table, if any
   DatabaseConstants::DbTableId btTable() const;
   //!brief get all the properties in this schema as a map <name,PropertySchema>
   const QMap<QString, PropertySchema*> properties() const;
   //!brief get all the foreign keys in this schema as a map <name,PropertySchema>
   const QMap<QString, PropertySchema*> foreignKeys() const;
   //!brief get the PropertySchema for the unique id
   const PropertySchema* key() const;

   // Things to do for properties

   // Get the property object. Try not to use this?
   const PropertySchema* property(QString prop) const;
   // some properties may be named differently (like inventory v quanta)
   const QString propertyName(QString prop, Database::DbType type = Database::ALLDB) const;
   // get the database column name for this property
   const QString propertyToColumn(QString prop, Database::DbType type = Database::ALLDB) const;
   // get the database column type
   const QString propertyColumnType(QString prop, Database::DbType type = Database::ALLDB) const;
   // get the default value for this column
   const QVariant propertyColumnDefault(QString prop, Database::DbType type = Database::ALLDB) const;
   // get the column size of the property's column
   int propertyColumnSize(QString prop, Database::DbType type = Database::ALLDB) const;
   // returns the property to be used for the increment/decrement triggers
   const QString triggerProperty() const;

   //!brief get all the property names
   const QStringList allPropertyNames(Database::DbType type = Database::ALLDB) const;
   //!brief get all the database column names
   const QStringList allColumnNames(Database::DbType type = Database::ALLDB) const;
   //!brief get keys for the properties
   const QStringList allProperties() const;

   // things to do on foreign keys
   // get a specific foreign key column name
   const QString foreignKeyToColumn(QString fkey, Database::DbType type = Database::ALLDB) const;
   // a lot of tables have one foreign key. This is a nice shortcut for that
   const QString foreignKeyToColumn(Database::DbType type = Database::ALLDB) const;

   // which table does this foreign key point to
   DatabaseConstants::DbTableId foreignTable(QString fkey, Database::DbType type = Database::ALLDB) const;
   // a lot of tables have one foreign key. This is a nice shortcut for that
   DatabaseConstants::DbTableId foreignTable(Database::DbType type = Database::ALLDB) const;

   //!brief get all the foreign key property names
   const QStringList allForeignKeyNames(Database::DbType type = Database::ALLDB) const;
   //!brief get all the foreign key column names
   const QStringList allForeignKeyColumnNames(Database::DbType type = Database::ALLDB) const;
   //!brief get keys for the foreign keys
   const QStringList allForeignKeys() const;

   //!brief Use this to get the not recipe_id index from an inrec table
   const QString inRecIndexName(Database::DbType type = Database::ALLDB);
   //!brief Use this to get the child_id index from a children table
   const QString childIndexName(Database::DbType type = Database::ALLDB);
   //!brief Use this to get the recipe_id from a inrec table
   const QString recipeIndexName(Database::DbType type = Database::ALLDB);
   //!brief Use this to get the parent_id from a child table
   const QString parentIndexName(Database::DbType type = Database::ALLDB);

   // Not sure these belong here yet, but maybe
   const QString generateCreateTable(Database::DbType type = Database::ALLDB, QString tmpName = QString("") );
   const QString generateUpdateRow(int key, Database::DbType type = Database::ALLDB);
   const QString generateUpdateRow(Database::DbType type = Database::ALLDB);
   // this one includes the foreign keys and is really only suitable for copying databases
   const QString generateInsertRow(Database::DbType type = Database::ALLDB);
   // this one ignores the foreign keys and is more generally useful
   const QString generateInsertProperties(Database::DbType type = Database::ALLDB);
   // when dropping columns, we have to copy tables in sqlite. This does that.
   const QString generateCopyTable( QString dest, Database::DbType type = Database::ALLDB);

   const QString generateDecrementTrigger(Database::DbType type);
   const QString generateIncrementTrigger(Database::DbType type);

   bool isInventoryTable();
   bool isBaseTable();
   bool isChildTable();
   bool isInRecTable();
   bool isBtTable();
   bool isMetaTable();

   const QString keyName(Database::DbType type = Database::ALLDB) const;

private:
public:
   // I only allow table schema to be made with a DBTable constant
   // It saves a lot of work, and I think the name to constant
   // mapping doesn't belong here -- it belongs in DatabaseSchema
   TableSchema(DatabaseConstants::DbTableId dbTable);
private:
   QString m_tableName;
   QString m_className;
   DatabaseConstants::DbTableId m_dbTable;
   TableType m_type;

   // these are only set by the base tables
   DatabaseConstants::DbTableId m_childTable;
   DatabaseConstants::DbTableId m_inRecTable;
   DatabaseConstants::DbTableId m_invTable;
   DatabaseConstants::DbTableId m_btTable;

   QString m_trigger;

   PropertySchema* m_key;
   QMap<QString,PropertySchema*> m_properties;
   QMap<QString,PropertySchema*> m_foreignKeys;
   // It all depends on the call I want to make. I can require the type on
   // every call to a TableSchema object which is dull, repetitive and makes
   // some already difficult to read calls harder to read. Or I can cache the
   // default in the table and use that if ALLDB is sent, which breaks the
   // metaphor.
   Database::DbType m_defType;

   // getter only. But this is private because only my dearest,
   // closest friends can do this
   Database::DbType defType() const;

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
   void defineChildTable(DatabaseConstants::DbTableId table);

   // and almost one method for all the in_recipe tables
   void defineInRecipeTable(QString childIdx, DatabaseConstants::DbTableId table);
   // Instructions in recipe actually carry information. Sigh.
   void defineInstructionInRecipeTable( QString childIdx, DatabaseConstants::DbTableId table);

   // one method for all the bt_tables
   void defineBtTable(QString childIdx, DatabaseConstants::DbTableId table);

   // Inventory tables are strange and I didn't feel quite comfortable trying to make one
   // method for all of them;
   void defineFermInventoryTable();
   void defineHopInventoryTable();
   void defineMiscInventoryTable();
   void defineYeastInventoryTable();

};

#endif
