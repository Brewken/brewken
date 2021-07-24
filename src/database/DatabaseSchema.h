/**
 * database/DatabaseSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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

#ifndef DATABASESCHEMA_H
#define DATABASESCHEMA_H

#include "Brewken.h"
#include "database/TableSchema.h"

#include <QString>

/* A fundamental problem in all OO is when to stop abstracting.
 *
 * These three classes are intended to represent the database schema in a way
 * that can be manipulated by the objects that need to, instead of the
 * mess'o'hash and lists we have now.
 *
 * My goals are to:
 *   1. Remove as many of the table hashes as I can.
 *   2. Make it easier to add a column to the database
 *   3. Make DatabaseSchemaHelper .. better.
 *
 * Removing the hashes
 *   When the Database() object is created, it should get a static
 *   DatabaseSchema object that defines *every* table (ugh, this means the
 *   parent of tables, the inventory tables, etc.)
 *
 *   Anything that needs to know can query for a specific table/property and
 *   get back what it needs. As a side effect, I think we can clean all the
 *   set() methods up, which means we can delete a bunch of constants from the
 *   file.
 */
class DatabaseSchema
{

   friend class Database;

public:

   virtual ~DatabaseSchema() {}

   // For those that just want the TableSchema
   TableSchema* table(QString tableName);
   TableSchema* table(DatabaseConstants::DbTableId table);
   QString tableName(DatabaseConstants::DbTableId table);

   // These generate SQL strings
   const QString generateCreateTable(DatabaseConstants::DbTableId table, QString name = QString());
   const QString generateInsertRow(DatabaseConstants::DbTableId table);
   const QString generateUpdateRow(DatabaseConstants::DbTableId table, int key);
//   const QString generateCopyTable(DatabaseConstants::DbTableId src, QString dest, Database::DbType type);

   // these translate from a class to a table or its name
   DatabaseConstants::DbTableId classNameToTable(QString className) const;
   const QString classNameToTableName(QString className) const;

   // ways to get different types of tables
   QVector<TableSchema*> inventoryTables();
   QVector<TableSchema*> childTables();
   QVector<TableSchema*> inRecipeTables();
   QVector<TableSchema*> baseTables();
   QVector<TableSchema*> btTables();
   QVector<TableSchema*> allTables(bool createOrder = false);

   // the idea here is to be able to give a base table and get back either its
   // inRec, child, inventory or bt table. I am thinking I will calculate these for
   // now, but I am wondering if I shouldn't store that info in the table? If
   // I change my mind later, these can all be pretty simple lookups
   TableSchema* inRecTable(DatabaseConstants::DbTableId dbTable);
   TableSchema* childTable(DatabaseConstants::DbTableId dbTable);
   TableSchema* invTable(DatabaseConstants::DbTableId dbTable);
   TableSchema* btTable(DatabaseConstants::DbTableId dbTable);

   // I seem to be needing these frequently, so lets take code from 100 places and put it here
   // It can't go into the TableSchema because it doesn't know the db
   const QString childTableName(DatabaseConstants::DbTableId dbTable);
   const QString inRecTableName(DatabaseConstants::DbTableId dbTable);
   const QString invTableName(DatabaseConstants::DbTableId dbTable);
   const QString btTableName(DatabaseConstants::DbTableId dbTable);

private:
   QVector<TableSchema*> m_tables;
//   Database::DbType m_type;
   QString m_id;
   QString m_name;
public:
   DatabaseSchema();
private:
   void loadTables();

};

Q_DECLARE_METATYPE( DatabaseConstants::DbTableId )

#endif
