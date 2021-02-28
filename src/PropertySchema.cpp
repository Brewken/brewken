/**
 * PropertySchema.cpp is part of Brewken, and is copyright the following authors 2019-2020:
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

#include <QString>
#include <QDebug>
#include "Brewken.h"
#include "PropertySchema.h"

// this initializer is simple. It populates the m_properties QVector with
// all the nullptr we need. Use this if initializing ALLDB doesn't present a
// win
PropertySchema::PropertySchema()
    : QObject(nullptr),
    m_properties(QVector<dbProp*>(1 + static_cast<int>(Brewken::ALLDB),nullptr))
{
}

// Normal property initializer, will set every DB to this definition
PropertySchema::PropertySchema( QString propName, QString colName,
                                QString xmlName,  QString colType,
                                QVariant defaultValue, QString constraint, int colSize )
    : QObject(nullptr),
    m_properties(QVector<dbProp*>(1 + static_cast<int>(Brewken::ALLDB),nullptr))
{
   dbProp* tmp = new dbProp;

   tmp->m_propName = propName;
   tmp->m_colName = colName;
   tmp->m_xmlName = xmlName;
   tmp->m_constraint = constraint;
   tmp->m_colType = colType;
   tmp->m_defaultValue = defaultValue;
   tmp->m_colSize = colSize;
   tmp->m_ftable = Brewken::NOTABLE;

   int i = static_cast<int>(Brewken::SQLITE);
   while ( i <= static_cast<int>(Brewken::ALLDB) ) {
      m_properties[i] = tmp;
      i++;
   }
}

// Foreign key initializer, will set all the DB to this definition
PropertySchema::PropertySchema(QString propName, QString colName, QString colType, Brewken::DBTable fTable)
    : QObject(nullptr),
    m_properties(QVector<dbProp*>(1 + static_cast<int>(Brewken::ALLDB),nullptr))
{
   dbProp* tmp = new dbProp;

   tmp->m_propName = propName;
   tmp->m_colName = colName;
   tmp->m_xmlName = QString();
   tmp->m_constraint = QString();
   tmp->m_colType = colType;
   tmp->m_defaultValue = QVariant();
   tmp->m_colSize = 0;
   tmp->m_ftable = fTable;

   int i = static_cast<int>(Brewken::SQLITE);
   while ( i <= static_cast<int>(Brewken::ALLDB) ) {
      m_properties[i] = tmp;
      i++;
   }
}

// The other property initializer does ALLDB. use this to add alternate
// definitions
// If you use ALLDB with this, it will initialize anything not already set to
// the value
void PropertySchema::addProperty( QString propName, Brewken::DBTypes dbType,
                  QString colName, QString xmlName, QString colType,
                  QVariant defaultValue, int colSize, QString constraint )
{
   dbProp* tmp = new dbProp;

   tmp->m_propName = propName;
   tmp->m_colName = colName;
   tmp->m_xmlName = xmlName;
   tmp->m_constraint = constraint;
   tmp->m_colType = colType;
   tmp->m_defaultValue = defaultValue;
   tmp->m_colSize = colSize;
   tmp->m_ftable = Brewken::NOTABLE;

   if ( dbType == Brewken::ALLDB ) {
      int i = static_cast<int>(Brewken::SQLITE);
      while ( i < static_cast<int>(Brewken::ALLDB) ) {
         if ( m_properties[i] == nullptr ) {
            m_properties[i] = tmp;
         }
         i++;
      }
   }
   else {
      m_properties[dbType] = tmp;
   }
}

// The other foreign key initializer does just ALLDB. use this to special case that
void PropertySchema::addForeignKey(QString propName, Brewken::DBTypes dbType,
                              QString colName, Brewken::DBTable fTable)
{
   dbProp* tmp = new dbProp;

   tmp->m_propName = propName;
   tmp->m_colName = colName;
   tmp->m_xmlName = QString();
   tmp->m_constraint = QString();
   tmp->m_colType = QString();
   tmp->m_defaultValue = QVariant();
   tmp->m_colSize = 0;
   tmp->m_ftable = fTable;

   if ( dbType == Brewken::ALLDB ) {
      int i = static_cast<int>(Brewken::SQLITE);
      while ( i < static_cast<int>(Brewken::ALLDB) ) {
         if ( m_properties[i] == nullptr ) {
            m_properties[i] = tmp;
         }
         i++;
      }
   }
   else {
      m_properties[dbType] = tmp;
   }
}

const QString PropertySchema::colName(Brewken::DBTypes dbType) const
{
   return m_properties[dbType]->m_colName;
}

const QString PropertySchema::propName(Brewken::DBTypes dbType) const
{
   return m_properties[dbType]->m_propName;
}

const QString PropertySchema::colType(Brewken::DBTypes dbType) const
{
   return m_properties[dbType]->m_colType;
}

const QString PropertySchema::xmlName(Brewken::DBTypes dbType) const
{
   return m_properties[dbType]->m_xmlName;
}

const QString PropertySchema::constraint(Brewken::DBTypes dbType) const
{
   return m_properties[dbType]->m_constraint;
}

const QVariant PropertySchema::defaultValue(Brewken::DBTypes dbType) const
{
   return m_properties[dbType]->m_defaultValue;
}

int PropertySchema::colSize(Brewken::DBTypes dbType) const
{
   return m_properties[dbType]->m_colSize;
}

Brewken::DBTable PropertySchema::fTable(Brewken::DBTypes dbType) const
{
   return m_properties[dbType]->m_ftable;
}

void PropertySchema::setColName(QString colName, Brewken::DBTypes dbType)
{
   m_properties[dbType]->m_colName = colName;
}

void PropertySchema::setXmlName(QString xmlName, Brewken::DBTypes dbType)
{
   m_properties[dbType]->m_xmlName = xmlName;
}

void PropertySchema::setConstraint(QString constraint, Brewken::DBTypes dbType)
{
   m_properties[dbType]->m_constraint = constraint;
}

void PropertySchema::setColType(QString colType, Brewken::DBTypes dbType)
{
   m_properties[dbType]->m_colType = colType;
}

void PropertySchema::setDefaultValue(QVariant defVal, Brewken::DBTypes dbType)
{
   m_properties[dbType]->m_defaultValue = defVal;
}

void PropertySchema::setColSize(int size, Brewken::DBTypes dbType)
{
   m_properties[dbType]->m_colSize = size;
}

void PropertySchema::setFTable(Brewken::DBTable ftable, Brewken::DBTypes dbType)
{
   m_properties[dbType]->m_ftable = ftable;
}
