/**
 * database/PropertySchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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
#ifndef PROPERTYSCHEMA_H
#define PROPERTYSCHEMA_H

#include "Brewken.h"
#include <QString>

#include "database/Database.h"
#include "database/TableSchemaConst.h"

// Needs the forward declaration

struct dbProp {
   QString m_propName;
   QString m_colName;
   QString m_constraint;
   QString m_colType;
   QVariant m_defaultValue;
   int m_colSize;
   DatabaseConstants::DbTableId m_ftable;
};

class PropertySchema : QObject
{

   friend class TableSchema;

   Q_OBJECT
public:
   virtual ~PropertySchema() {}

   // since I've removed all the things from the initializer, we need two
   // methods to actually add properties
   void addProperty(QString propName,
                   Database::DbType dbType = Database::ALLDB,
                   QString colName = QString(),
                   QString colType = QString(),
                   QVariant defaultValue = QVariant(),
                   int colSize = 0,
                   QString constraint = QString() );

   void addForeignKey(QString propName,
                      Database::DbType dbType = Database::ALLDB,
                      QString colName = QString(),
                      DatabaseConstants::DbTableId fTable = DatabaseConstants::NOTABLE);

   // this may get revisited later, but if you either do not include the
   // dbType in the call or you send ALLDB, then you will get the default
   const QString propName(Database::DbType dbType = Database::ALLDB) const;
   const QString colName(Database::DbType dbType = Database::ALLDB) const;
   const QString constraint(Database::DbType dbType = Database::ALLDB) const;
   const QString colType(Database::DbType dbType = Database::ALLDB) const;
   const QVariant defaultValue(Database::DbType dbType = Database::ALLDB) const;
   int colSize(Database::DbType dbType = Database::ALLDB) const;
   DatabaseConstants::DbTableId fTable(Database::DbType dbType = Database::ALLDB) const;

   // sets
   // NOTE: I am specifically not allowing the propName to be set. Do that
   // when you call addProperty or addForeignKey. I may
   void setColName(QString column, Database::DbType dbType = Database::ALLDB);
   void setConstraint(QString constraint, Database::DbType dbType = Database::ALLDB);
   void setColType(QString colType, Database::DbType dbType = Database::ALLDB);
   void setDefaultValue(QVariant defVal, Database::DbType dbType = Database::ALLDB);
   void setColSize(int size, Database::DbType dbType = Database::ALLDB);
   void setFTable(DatabaseConstants::DbTableId fTable, Database::DbType dbType = Database::ALLDB);

private:
   PropertySchema();
   // if you use this constructor, it will default to ALLDB
   PropertySchema( QString propName,
                   QString colName,
                   QString colType,
                   QVariant defVal,
                   QString constraint = QString(""),
                   int colSize = 0
   );

   PropertySchema( QString propName,
                   QString colName,
                   QString colType,
                   DatabaseConstants::DbTableId fTable
   );

   // any given property has at least one element in this list and possible as
   // many as we support. Using a vector allows me to prepopulate
   QVector<dbProp*> m_properties;
};

#endif // PROPERTYSCHEMA_H
