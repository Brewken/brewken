/**
 * database/DatabaseSchema.cpp is part of Brewken, and is copyright the following authors 2019-2020:
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
#include <QDebug>
#include <QString>
#include <QStringBuilder>

#include "database/TableSchema.h"
#include "database/TableSchemaConst.h"
#include "database/DatabaseSchema.h"
#include "database/InstructionSchema.h"


DatabaseSchema::DatabaseSchema()
{
   loadTables();
   m_type = Brewken::dbType();
}

void DatabaseSchema::loadTables()
{
   int it;

   for ( it = DatabaseConstants::NOTABLE; it <= DatabaseConstants::YEASTINVTABLE; it++ ) {
      TableSchema* tmp = new TableSchema(static_cast<DatabaseConstants::DbTableId>(it));
      m_tables.append(tmp);
   }
}

TableSchema *DatabaseSchema::table(DatabaseConstants::DbTableId table)
{
   if ( table > DatabaseConstants::NOTABLE && table < m_tables.size() ) {
      return m_tables.at(table);
   }

   return nullptr;
}

TableSchema *DatabaseSchema::table(QString tableName)
{
   if ( DatabaseConstants::dbTableToName.contains(tableName) ) {
      return m_tables.value( static_cast<DatabaseConstants::DbTableId>(DatabaseConstants::dbTableToName.indexOf(tableName)));
   }
   else {
      qDebug() << "Could not find table for " + tableName;
   }
   return nullptr;
}

QString DatabaseSchema::tableName(DatabaseConstants::DbTableId table)
{
   if ( table > DatabaseConstants::NOTABLE && table < m_tables.size() ) {
      return m_tables.at(table)->tableName();
   }

   return QString("");
}

// I believe one method replaces EVERY create_ method in DatabaseSchemaHelper.
// It is so beautiful, it must be evil.
const QString DatabaseSchema::generateCreateTable(DatabaseConstants::DbTableId table, QString name)
{
   if ( table <= DatabaseConstants::NOTABLE || table > m_tables.size() ) {
      return QString();
   }

   TableSchema* tSchema = m_tables.at(table);

   return tSchema->generateCreateTable(m_type,name);
}

// these two basically just pass the call to the proper table
const QString DatabaseSchema::generateInsertRow(DatabaseConstants::DbTableId table)
{
   TableSchema* tSchema = m_tables.value(table);
   return tSchema->generateInsertRow(m_type);
}

const QString DatabaseSchema::generateCopyTable(DatabaseConstants::DbTableId src, QString dest, Brewken::DBTypes type)
{
   TableSchema* tSchema = m_tables.value(src);
   return tSchema->generateCopyTable(dest,type);
}

const QString DatabaseSchema::generateUpdateRow(DatabaseConstants::DbTableId table, int key)
{
   TableSchema* tSchema = m_tables.value(table);
   return tSchema->generateUpdateRow(key, m_type);
}

DatabaseConstants::DbTableId DatabaseSchema::classNameToTable(QString className) const
{
   DatabaseConstants::DbTableId retval = DatabaseConstants::NOTABLE;

   foreach( TableSchema* here, m_tables ) {
      if ( here->className() == className ) {
          retval = here->dbTable();
          break;
      }
   }
   return retval;
}

const QString DatabaseSchema::classNameToTableName(QString className) const
{
   QString retval;

   foreach( TableSchema* here, m_tables ) {
      if ( here->className() == className ) {
          retval = here->tableName();
          break;
      }
   }
   return retval;
}

QVector<TableSchema*> DatabaseSchema::inventoryTables()
{
    QVector<TableSchema*> retVal;

    foreach( TableSchema* here, m_tables ) {
        if ( here->isInventoryTable() ) {
            retVal.append(here);
        }
    }

    return retVal;
}

QVector<TableSchema*> DatabaseSchema::childTables()
{
    QVector<TableSchema*> retVal;

    foreach( TableSchema* here, m_tables ) {
        if ( here->isChildTable() ) {
            retVal.append(here);
        }
    }

    return retVal;
}

QVector<TableSchema*> DatabaseSchema::inRecipeTables()
{
    QVector<TableSchema*> retVal;

    foreach( TableSchema* here, m_tables ) {
        if ( here->isInRecTable() ) {
            retVal.append(here);
        }
    }

    return retVal;
}

QVector<TableSchema*> DatabaseSchema::baseTables()
{
    QVector<TableSchema*> retVal;

    foreach( TableSchema* here, m_tables ) {
        if ( here->isBaseTable() ) {
            retVal.append(here);
        }
    }

    return retVal;
}

QVector<TableSchema*> DatabaseSchema::btTables()
{
    QVector<TableSchema*> retVal;

    foreach( TableSchema* here, m_tables ) {
        if ( here->isBtTable() ) {
            retVal.append(here);
        }
    }

    return retVal;
}

QVector<TableSchema*>  DatabaseSchema::allTables(bool createOrder)
{
    QVector<TableSchema*> retval;

    // when creating the database from scratch, we need to make sure the
    // inventory tables happen before the base tables. otherwise, we will
    // get constraint violations because the inventory_id has nothing to point
    // to
    for( int i = 1; i < DatabaseConstants::dbTableToName.size(); ++i ) {
       TableSchema* tmp = m_tables.value( static_cast<DatabaseConstants::DbTableId>(i));
       if ( createOrder && tmp->isInventoryTable() ) {
          retval.prepend(tmp);
       }
       else {
          retval.append( tmp );
       }
    }
    return retval;
}

TableSchema* DatabaseSchema::childTable(DatabaseConstants::DbTableId dbTable)
{
    TableSchema* tbl = table(dbTable);
    TableSchema* retVal = nullptr;

    if ( tbl != nullptr ) {
       DatabaseConstants::DbTableId idx = tbl->childTable();
       retVal = table( idx );
    }
    return retVal;
}

TableSchema* DatabaseSchema::inRecTable(DatabaseConstants::DbTableId dbTable)
{
    TableSchema* tbl = table(dbTable);
    TableSchema* retVal = nullptr;

    if ( tbl != nullptr ) {
       DatabaseConstants::DbTableId idx = tbl->inRecTable();
       retVal = table( idx );
    }
    return retVal;
}

TableSchema* DatabaseSchema::invTable(DatabaseConstants::DbTableId dbTable)
{
    TableSchema* tbl = table(dbTable);
    TableSchema* retVal = nullptr;

    if ( tbl != nullptr ) {
       DatabaseConstants::DbTableId idx = tbl->invTable();
       retVal = table( idx );
    }
    return retVal;
}

TableSchema* DatabaseSchema::btTable(DatabaseConstants::DbTableId dbTable)
{
    TableSchema* tbl = table(dbTable);
    TableSchema* retVal = nullptr;

    if ( tbl != nullptr ) {
       DatabaseConstants::DbTableId idx = tbl->btTable();
       retVal = table( idx );
    }
    return retVal;
}

const QString DatabaseSchema::childTableName(DatabaseConstants::DbTableId dbTable)
{
    TableSchema* chld = childTable(dbTable);

    return chld == nullptr ? QString() : chld->tableName();
}

const QString DatabaseSchema::inRecTableName(DatabaseConstants::DbTableId dbTable)
{
    TableSchema* chld = inRecTable(dbTable);

    return chld == nullptr ? QString() : chld->tableName();
}

const QString DatabaseSchema::invTableName(DatabaseConstants::DbTableId dbTable)
{
    TableSchema* chld = invTable(dbTable);

    return chld == nullptr ? QString() : chld->tableName();
}

const QString DatabaseSchema::btTableName(DatabaseConstants::DbTableId dbTable)
{
   TableSchema* chld = btTable(dbTable);

   return chld == nullptr ? QString() : chld->tableName();
}
