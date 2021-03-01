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

#include "database/TableSchemaConst.h"

// Needs the forward declaration

struct dbProp {
   QString m_propName;
   QString m_colName;
   QString m_xmlName;
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
                   Brewken::DBTypes dbType = Brewken::ALLDB,
                   QString colName = QString(),
                   QString xmlName = QString(),
                   QString colType = QString(),
                   QVariant defaultValue = QVariant(),
                   int colSize = 0,
                   QString constraint = QString() );

   void addForeignKey(QString propName,
                      Brewken::DBTypes dbType = Brewken::ALLDB,
                      QString colName = QString(),
                      DatabaseConstants::DbTableId fTable = DatabaseConstants::NOTABLE);

   // this may get revisited later, but if you either do not include the
   // dbType in the call or you send ALLDB, then you will get the default
   const QString propName(Brewken::DBTypes dbType = Brewken::ALLDB) const;
   const QString colName(Brewken::DBTypes dbType = Brewken::ALLDB) const;
   const QString xmlName(Brewken::DBTypes dbType = Brewken::ALLDB) const;
   const QString constraint(Brewken::DBTypes dbType = Brewken::ALLDB) const;
   const QString colType(Brewken::DBTypes dbType = Brewken::ALLDB) const;
   const QVariant defaultValue(Brewken::DBTypes dbType = Brewken::ALLDB) const;
   int colSize(Brewken::DBTypes dbType = Brewken::ALLDB) const;
   DatabaseConstants::DbTableId fTable(Brewken::DBTypes dbType = Brewken::ALLDB) const;

   // sets
   // NOTE: I am specifically not allowing the propName to be set. Do that
   // when you call addProperty or addForeignKey. I may
   void setColName(QString column, Brewken::DBTypes dbType = Brewken::ALLDB);
   void setXmlName(QString xmlName, Brewken::DBTypes dbType = Brewken::ALLDB);
   void setConstraint(QString constraint, Brewken::DBTypes dbType = Brewken::ALLDB);
   void setColType(QString colType, Brewken::DBTypes dbType = Brewken::ALLDB);
   void setDefaultValue(QVariant defVal, Brewken::DBTypes dbType = Brewken::ALLDB);
   void setColSize(int size, Brewken::DBTypes dbType = Brewken::ALLDB);
   void setFTable(DatabaseConstants::DbTableId fTable, Brewken::DBTypes dbType = Brewken::ALLDB);

private:
   PropertySchema();
   // if you use this constructor, it will default to ALLDB
   PropertySchema( QString propName,
                   QString colName,
                   QString xmlName,
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
