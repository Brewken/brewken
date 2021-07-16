/**
 * database/TableSchema.cpp is part of Brewken, and is copyright the following authors 2019-2021:
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
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
#include "database/TableSchema.h"

#include <QString>

#include "Brewken.h"
#include "database/BrewNoteSchema.h"
#include "database/Database.h"
#include "database/EquipmentSchema.h"
#include "database/FermentableSchema.h"
#include "database/HopSchema.h"
#include "database/InstructionSchema.h"
#include "database/MashSchema.h"
#include "database/MashStepSchema.h"
#include "database/MiscSchema.h"
#include "database/PropertySchema.h"
#include "database/RecipeSchema.h"
#include "database/SaltSchema.h"
#include "database/SettingsSchema.h"
#include "database/StyleSchema.h"
#include "database/TableSchemaConst.h"
#include "database/WaterSchema.h"
#include "database/YeastSchema.h"
#include "model/BrewNote.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Instruction.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/Misc.h"
#include "model/Recipe.h"
#include "model/Salt.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"
#include "PersistentSettings.h"

static const QString kDefault("DEFAULT");

TableSchema::TableSchema(DatabaseConstants::DbTableId table)
    : QObject(nullptr),
      m_tableName( DatabaseConstants::dbTableToName[ static_cast<int>(table) ] ),
      m_dbTable(table),
      m_childTable(DatabaseConstants::NOTABLE),
      m_inRecTable(DatabaseConstants::NOTABLE),
      m_invTable(DatabaseConstants::NOTABLE),
      m_btTable(DatabaseConstants::NOTABLE),
      m_trigger(QString()),
      m_defType(static_cast<Database::DbType>(PersistentSettings::value("dbType", Database::SQLITE).toInt()))
{
    // for this bit of ugly, I gain a lot of utility.
    defineTable();
}

// almost everything is a get. The initialization is expected all the parameters

const QString TableSchema::tableName() const { return m_tableName; }
const QString TableSchema::className() const { return m_className; }
DatabaseConstants::DbTableId TableSchema::dbTable() const { return m_dbTable; }
DatabaseConstants::DbTableId TableSchema::childTable() const  { return m_childTable; }
DatabaseConstants::DbTableId TableSchema::inRecTable() const { return m_inRecTable; }
DatabaseConstants::DbTableId TableSchema::invTable() const { return m_invTable; }
DatabaseConstants::DbTableId TableSchema::btTable() const { return m_btTable; }
const QString TableSchema::triggerProperty() const { return m_trigger; }

const QMap<QString,PropertySchema*> TableSchema::properties() const { return m_properties; }
const QMap<QString,PropertySchema*> TableSchema::foreignKeys() const { return m_foreignKeys; }
const PropertySchema* TableSchema::key() const { return m_key; }
Database::DbType TableSchema::defType() const { return m_defType; }

const QString TableSchema::keyName( Database::DbType type ) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;

   return m_key->colName(selected);
}

const QStringList TableSchema::allPropertyNames(Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;

   QMapIterator<QString,PropertySchema*> i(m_properties);
   QStringList retval;
   while ( i.hasNext() ) {
      i.next();
      retval.append( i.value()->propName(selected));
   }
   return retval;
}

const QStringList TableSchema::allProperties() const
{
   return m_properties.keys();
}
const QStringList TableSchema::allForeignKeyNames(Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QMapIterator<QString,PropertySchema*> i(m_foreignKeys);
   QStringList retval;
   while ( i.hasNext() ) {
      i.next();
      retval.append( i.value()->colName(selected));
   }
   return retval;
}

const QStringList TableSchema::allForeignKeys() const
{
   return m_foreignKeys.keys();
}

const QStringList TableSchema::allColumnNames(Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QStringList tmp;
   QMapIterator<QString,PropertySchema*> i(m_properties);

   while ( i.hasNext() ) {
      i.next();
      tmp.append(i.value()->colName(selected));
   }
   return tmp;
}

const QStringList TableSchema::allForeignKeyColumnNames(Database::DbType type) const
{
   QStringList tmp;
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;

   QMapIterator<QString,PropertySchema*> i(m_foreignKeys);

   while ( i.hasNext() ) {
      i.next();
      tmp.append(i.value()->colName(selected));
   } return tmp;
}

const PropertySchema* TableSchema::property(QString prop) const
{
   PropertySchema* retval = nullptr;
   if ( m_properties.contains(prop) ) {
      retval = m_properties.value(prop);
   }
   return retval;
}

const QString TableSchema::propertyName(QString prop, Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString retval;
   if ( m_properties.contains(prop) ) {
      retval =  m_properties.value(prop)->propName(selected);
   }
   return retval;

}
const QString TableSchema::propertyToColumn(QString prop, Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString retval;
   if ( m_properties.contains(prop) ) {
      retval =  m_properties.value(prop)->colName(selected);
   }
   return retval;
}

const QString TableSchema::foreignKeyToColumn(QString fkey, Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString retval;
   if ( m_foreignKeys.contains(fkey) ) {
      retval =  m_foreignKeys.value(fkey)->colName(selected);
   }
   return retval;
}

const QString TableSchema::foreignKeyToColumn(Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString retval;

   if ( m_foreignKeys.size() == 1 ) {
      retval = m_foreignKeys.first()->colName(selected);
   }
   return retval;
}

const QString TableSchema::propertyColumnType(QString prop, Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   if ( m_properties.contains(prop) ) {
      return m_properties.value(prop)->colType(selected);
   }
   else {
      return QString();
   }
}

const QVariant TableSchema::propertyColumnDefault(QString prop, Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QVariant retval = QString();
   if ( m_properties.contains(prop) ) {
      retval = m_properties.value(prop)->defaultValue(selected);
   }
   return retval;
}

int TableSchema::propertyColumnSize(QString prop, Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   if ( m_properties.contains(prop) ) {
      return m_properties.value(prop)->colSize(selected);
   }
   else {
      return 0;
   }
}

DatabaseConstants::DbTableId TableSchema::foreignTable(QString fkey, Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   DatabaseConstants::DbTableId retval = DatabaseConstants::NOTABLE;

   if ( m_foreignKeys.contains(fkey) ) {
      retval =  m_foreignKeys.value(fkey)->fTable(selected);
   }
   return retval;

}

DatabaseConstants::DbTableId TableSchema::foreignTable(Database::DbType type) const
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   DatabaseConstants::DbTableId retval = DatabaseConstants::NOTABLE;

   if ( m_foreignKeys.size() == 1 ) {
      retval =  m_foreignKeys.first()->fTable(selected);
   }
   return retval;
}

bool TableSchema::isInventoryTable() { return m_type == INV; }
bool TableSchema::isBaseTable()      { return m_type == BASE; }
bool TableSchema::isChildTable()     { return m_type == CHILD; }
bool TableSchema::isInRecTable()     { return m_type == INREC; }
bool TableSchema::isBtTable()        { return m_type == BT; }
bool TableSchema::isMetaTable()      { return m_type == META; }

const QString TableSchema::childIndexName(Database::DbType type)
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString cname;

   if ( m_type == CHILD || m_type == BT ) {
      QMapIterator<QString,PropertySchema*> i(m_foreignKeys);

      while ( i.hasNext() ) {
         i.next();
         if ( i.value()->colName(selected) != kpropRecipeId ) {
            cname = i.value()->colName(selected);
            break;
         }
      }
   }
   return cname;
}

const QString TableSchema::inRecIndexName(Database::DbType type)
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString cname;

   if ( m_type == INREC ) {
      QMapIterator<QString,PropertySchema*> i(m_foreignKeys);

      while ( i.hasNext() ) {
         i.next();
         if ( i.value()->colName(selected) != kpropRecipeId ) {
            cname = i.value()->colName(selected);
            break;
         }
      }
   }
   return cname;
}

const QString TableSchema::recipeIndexName(Database::DbType type)
{
   QString cname;
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;

   if ( m_foreignKeys.contains(kpropRecipeId) ) {
      cname = m_foreignKeys.value(kpropRecipeId)->colName(selected);
   }

   return cname;
}

const QString TableSchema::parentIndexName(Database::DbType type)
{
   QString cname;
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;

   if ( m_foreignKeys.contains(kpropParentId) ) {
      cname = m_foreignKeys.value(kpropParentId)->colName(selected);
   }

   return cname;
}

const QString TableSchema::generateCreateTable(Database::DbType type, QString tmpName)
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString tname = tmpName.isEmpty() ? m_tableName : tmpName;
   QString retVal = QString("CREATE TABLE %1 (\n%2 %3\n")
                     .arg( tname )
                     .arg( m_key->colName(selected) )
                     .arg( m_key->constraint(selected)
   );

   QString retKeys;
   QMapIterator<QString, PropertySchema*> i(m_properties);
   while ( i.hasNext() ) {
      i.next();
      PropertySchema* prop = i.value();

      // based on the different way a boolean is handled between sqlite and
      // pgsql, I need to single them out.
      QVariant defVal = prop->defaultValue(selected);
      if ( defVal.isValid() ) {
         QString tmp = defVal.toString();
         if ( prop->colType() == "boolean" ) {
            tmp = Database::dbBoolean(defVal.toBool(),type);
         }

         // this isn't quite perfect, as you will get two spaces between the type
         // and DEFAULT if there are no constraints. On the other hand, nobody
         // will know that but me and the person reading this comment.
         retVal.append( QString(",\n%1 %2 %3 %4 %5")
                           .arg( prop->colName() ).arg( prop->colType() )
                           .arg( prop->constraint() ).arg( kDefault ).arg( tmp )
         );
      }
      else {
         retVal.append( QString("%1 %2 %3,\n")
               .arg( prop->colName() ).arg( prop->colType() ).arg( prop->constraint() ));
      }
   }

   // SQLITE wants the foreign key declarations go at the end, and they cannot
   // be intermixed with other column defs. This is an ugly hack to make it
   // work
   QMapIterator<QString, PropertySchema*> j(m_foreignKeys);
   while ( j.hasNext() ) {
      j.next();
      PropertySchema* key = j.value();

      retVal.append( QString(",\n%1 %2").arg( key->colName(selected) ).arg( key->colType(selected) ));

      retKeys.append( QString(",\nFOREIGN KEY(%1) REFERENCES %2(id)")
                       .arg( key->colName(selected) )
                       .arg( DatabaseConstants::dbTableToName[ key->fTable() ] )
      );
   }

   if ( ! retKeys.isEmpty() ) {
      retVal.append( retKeys );
   }
   retVal.append(");");

   return retVal;
}

const QString TableSchema::generateInsertRow(Database::DbType type)
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString columns = keyName(selected);
   QString binding = QString(":%1").arg(keyName(selected));

   QMapIterator<QString, PropertySchema*> i(m_properties);
   while ( i.hasNext() ) {
      i.next();
      PropertySchema* prop = i.value();

      columns += QString(",%1").arg( prop->colName(selected));
      binding += QString(",:%1").arg( i.key());
   }

   QMapIterator<QString, PropertySchema*> j(m_foreignKeys);
   while ( j.hasNext() ) {
      j.next();
      PropertySchema* key = j.value();

      columns += QString(",%1").arg(key->colName(selected));
      binding += QString(",:%1").arg( j.key());
   }
   return QString("INSERT INTO %1 (%2) VALUES(%3)").arg(m_tableName).arg(columns).arg(binding);
}

// NOTE: This does NOT deal with foreign keys nor the primary key for the table. It assumes
// any calling method will handle those relationships. In my rough design ideas, a table knows
// of itself and foreign key *values* are part of the database.
// To make other parts of the code easier, I am making certain that the bound values use the property name
// and not the column name. It saves a call later.
const QString TableSchema::generateInsertProperties(Database::DbType type)
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString columns;
   QString binding;

   QMapIterator<QString, PropertySchema*> i(m_properties);
   while ( i.hasNext() ) {
      i.next();
      PropertySchema* prop = i.value();

      if ( prop->colName(selected) == keyName(selected) ) {
         continue;
      }

      if ( columns.isEmpty() ) {
         columns = QString("%1").arg(prop->colName(selected));
         binding = QString(":%1").arg(i.key());
      }
      else {
         columns += QString(",%1").arg(prop->colName(selected));
         binding += QString(",:%1").arg(i.key());
      }
   }

   return QString("INSERT INTO %1 (%2) VALUES(%3)").arg(m_tableName).arg(columns).arg(binding);

}

// note: this does not do anything with foreign keys. It is up to the calling code to handle those problems
const QString TableSchema::generateUpdateRow(int key, Database::DbType type)
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString columns;

   QMapIterator<QString, PropertySchema*> i(m_properties);
   while ( i.hasNext() ) {
      i.next();
      PropertySchema* prop = i.value();
      if ( ! columns.isEmpty() ) {
         columns += QString(",%1=:%2")
                        .arg( prop->colName(selected))
                        .arg( i.key());
      }
      else {
         columns = QString("%1=:%2")
                       .arg( prop->colName(selected))
                       .arg( i.key());
      }
   }

   return QString("UPDATE %1 SET %2 where %3=%4")
           .arg(m_tableName)
           .arg(columns)
           .arg(keyName(selected))
           .arg(key);
}

// note: this does not do anything with foreign keys. It is up to the calling code to handle those problems
// unlike the previous method, this one uses a bind named ":id" for the key value.
const QString TableSchema::generateUpdateRow(Database::DbType type)
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString columns;

   QMapIterator<QString, PropertySchema*> i(m_properties);
   while ( i.hasNext() ) {
      i.next();
      PropertySchema* prop = i.value();
      if ( ! columns.isEmpty() ) {
         columns += QString(",%1=:%2")
                        .arg( prop->colName(selected))
                        .arg( i.key());
      }
      else {
         columns = QString("%1=:%2")
                       .arg( prop->colName(selected))
                       .arg( i.key());
      }
   }

   return QString("UPDATE %1 SET %2 where %3=:id")
           .arg(m_tableName)
           .arg(columns)
           .arg(keyName(selected));
}

const QString TableSchema::generateCopyTable( QString dest, Database::DbType type )
{
   Database::DbType selected = type == Database::ALLDB ? m_defType : type;
   QString columns = keyName(selected);

   QMapIterator<QString, PropertySchema*> i(m_properties);
   while ( i.hasNext() ) {
      i.next();
      PropertySchema* prop = i.value();

      columns += QString(",%1").arg( prop->colName(selected));
   }

   QMapIterator<QString, PropertySchema*> j(m_foreignKeys);
   while ( j.hasNext() ) {
      j.next();
      PropertySchema* key = j.value();

      columns += QString(",%1").arg(key->colName(selected));
   }

   return QString("INSERT INTO %1 (%2) SELECT %2 FROM %3").arg(dest).arg(columns).arg(m_tableName);

}

// right now, only instruction_number has an increment (or decrement) trigger.
// if we invent others, the m_trigger property will need to be set for that table.
// this only handles one trigger per table. It could be made to handle a list, maybe.
const QString TableSchema::generateIncrementTrigger(Database::DbType type)
{
   QString retval;

   if ( m_trigger.isEmpty() )
      return retval;

   if ( type == Database::PGSQL ) {
      // create or replace function increment_instruction_num() returns trigger as $BODY$
      //   begin update instruction_in_recipe set instruction_number = (SELECT max(instruction_number) from instruction_in_recipe where recipe_id = new.recipe_id) + 1
      //         where id = NEW.id;
      //         return NULL;
      //   end;
      //   $BODY$ LANGUAGE plpgsql;
      retval = QString("CREATE OR REPLACE FUNCTION increment_instruction_num() RETURNS TRIGGER AS $BODY$ "
                       "BEGIN UPDATE %1 SET %2 = (SELECT max(%2) from %1 where %3 = NEW.%3) + 1 WHERE %4 = NEW.%4; "
                       "return NULL;"
                       "END;"
                       "$BODY$ LANGUAGE plpgsql;")
            .arg(m_tableName)
            .arg(propertyToColumn(m_trigger))
            .arg(recipeIndexName())
            .arg(keyName());
      // I do not like this, in that I am stringing these together in bad ways
      retval += QString("CREATE TRIGGER inc_ins_num AFTER INSERT ON %1 "
                        "FOR EACH ROW EXECUTE PROCEDURE increment_instruction_num();")
            .arg(m_tableName);
   }
   else {
     retval = QString("CREATE TRIGGER inc_ins_num AFTER INSERT ON %1 "
                      "BEGIN "
                         "UPDATE %1 SET %2 = (SELECT max(%2) from %1 where %3 = new.%3) + 1 "
                         "WHERE rowid = new.rowid;"
                      "END")
           .arg(m_tableName)
           .arg(propertyToColumn(m_trigger))
           .arg(recipeIndexName());
   }
   return retval;
}

const QString TableSchema::generateDecrementTrigger(Database::DbType type)
{
   QString retval;

   if ( m_trigger.isEmpty() )
      return retval;

   if ( type == Database::PGSQL ) {
      // create or replace function decrement_instruction_num() returns trigger as $BODY$
      //   begin update instruction_in_recipe set instruction_number = instruction_number - 1
      //         where recipe_id = OLD.recipe_id AND instruction_number > OLD.instruction_number;
      //         return NULL;
      //   end;
      //   $BODY$ LANGUAGE plpgsql;
      retval = QString("CREATE OR REPLACE FUNCTION decrement_instruction_num() RETURNS TRIGGER AS $BODY$ "
                       "BEGIN UPDATE %1 SET %2 = %2 - 1 "
                         "WHERE %3 = OLD.%3 AND %2 > OLD.%2;"
                         "return NULL;"
                       "END;"
                       "$BODY$ LANGUAGE plpgsql;")
            .arg(tableName())
            .arg(propertyToColumn(m_trigger))
            .arg(recipeIndexName());
      retval += QString("CREATE TRIGGER dec_ins_num AFTER DELETE ON %1 "
                        "FOR EACH ROW EXECUTE PROCEDURE decrement_instruction_num();")
            .arg(tableName());
   }
   else {
      // CREATE TRIGGER dec_ins_num after DELETE ON instruction_in_recipe
      // BEGIN
      //   UPDATE instuction_in_recipe SET instruction_number = instruction_number - 1
      //   WHERE recipe_id = OLD.recipe_id AND  instruction_number > OLD.instruction_number
      // END
     retval = QString("CREATE TRIGGER dec_ins_num AFTER DELETE ON %1 "
                      "BEGIN "
                        "UPDATE %1 SET %2 = %2 - 1 "
                        "WHERE %3 = OLD.%3 AND %2 > OLD.%2; "
                      "END")
           .arg( tableName() )
           .arg(propertyToColumn(m_trigger))
           .arg(recipeIndexName());
   }
   return retval;
}
// This got long. Not sure if there's a better way to do it.
void TableSchema::defineTable()
{
   switch( m_dbTable ) {
      case DatabaseConstants::SETTINGTABLE:
         defineSettingsTable();
         break;
      case DatabaseConstants::BREWNOTETABLE:
         defineBrewnoteTable();
         break;
      case DatabaseConstants::STYLETABLE:
         defineStyleTable();
         break;
      case DatabaseConstants::EQUIPTABLE:
         defineEquipmentTable();
         break;
      case DatabaseConstants::FERMTABLE:
         defineFermentableTable();
         break;
      case DatabaseConstants::HOPTABLE:
         defineHopTable();
         break;
      case DatabaseConstants::INSTRUCTIONTABLE:
         defineInstructionTable();
         break;
      case DatabaseConstants::MASHTABLE:
         defineMashTable();
         break;
      case DatabaseConstants::MASHSTEPTABLE:
         defineMashstepTable();
         break;
      case DatabaseConstants::MISCTABLE:
         defineMiscTable();
         break;
      case DatabaseConstants::RECTABLE:
         defineRecipeTable();
         break;
      case DatabaseConstants::YEASTTABLE:
         defineYeastTable();
         break;
      case DatabaseConstants::WATERTABLE:
         defineWaterTable();
         break;
      case DatabaseConstants::SALTTABLE:
         defineSaltTable();
         break;
      case DatabaseConstants::BT_EQUIPTABLE:
         defineBtTable(kcolEquipmentId, DatabaseConstants::EQUIPTABLE);
         break;
      case DatabaseConstants::BT_FERMTABLE:
         defineBtTable(kcolFermentableId, DatabaseConstants::FERMTABLE);
         break;
      case DatabaseConstants::BT_HOPTABLE:
         defineBtTable(kcolHopId, DatabaseConstants::HOPTABLE);
         break;
      case DatabaseConstants::BT_MISCTABLE:
         defineBtTable(kcolMiscId, DatabaseConstants::MISCTABLE);
         break;
      case DatabaseConstants::BT_STYLETABLE:
         defineBtTable(kcolStyleId, DatabaseConstants::STYLETABLE);
         break;
      case DatabaseConstants::BT_WATERTABLE:
         defineBtTable(kcolWaterId, DatabaseConstants::WATERTABLE);
         break;
      case DatabaseConstants::BT_YEASTTABLE:
         defineBtTable(kcolYeastId, DatabaseConstants::YEASTTABLE);
         break;
      case DatabaseConstants::EQUIPCHILDTABLE:
         defineChildTable(DatabaseConstants::EQUIPTABLE);
         break;
      case DatabaseConstants::FERMCHILDTABLE:
         defineChildTable(DatabaseConstants::FERMTABLE);
         break;
      case DatabaseConstants::HOPCHILDTABLE:
         defineChildTable(DatabaseConstants::HOPTABLE);
         break;
      case DatabaseConstants::MISCCHILDTABLE:
         defineChildTable(DatabaseConstants::MISCTABLE);
         break;
      case DatabaseConstants::RECIPECHILDTABLE:
         defineChildTable(DatabaseConstants::RECTABLE);
         break;
      case DatabaseConstants::STYLECHILDTABLE:
         defineChildTable(DatabaseConstants::STYLETABLE);
         break;
      case DatabaseConstants::WATERCHILDTABLE:
         defineChildTable(DatabaseConstants::WATERTABLE);
         break;
      case DatabaseConstants::YEASTCHILDTABLE:
         defineChildTable(DatabaseConstants::YEASTTABLE);
         break;
      case DatabaseConstants::FERMINRECTABLE:
         defineInRecipeTable(kcolFermentableId, DatabaseConstants::FERMTABLE);
         break;
      case DatabaseConstants::HOPINRECTABLE:
         defineInRecipeTable(kcolHopId, DatabaseConstants::HOPTABLE);
         break;
      case DatabaseConstants::INSTINRECTABLE:
         defineInstructionInRecipeTable( kcolInstructionId, DatabaseConstants::INSTRUCTIONTABLE);
         break;
      case DatabaseConstants::MISCINRECTABLE:
         defineInRecipeTable(kcolMiscId, DatabaseConstants::MISCTABLE);
         break;
      case DatabaseConstants::WATERINRECTABLE:
         defineInRecipeTable(kcolWaterId, DatabaseConstants::WATERTABLE);
         break;
      case DatabaseConstants::SALTINRECTABLE:
         defineInRecipeTable(kcolSaltId, DatabaseConstants::SALTTABLE);
         break;
      case DatabaseConstants::YEASTINRECTABLE:
         defineInRecipeTable(kcolYeastId, DatabaseConstants::YEASTTABLE);
         break;
      case DatabaseConstants::FERMINVTABLE:
         defineFermInventoryTable();
         break;
      case DatabaseConstants::HOPINVTABLE:
         defineHopInventoryTable();
         break;
      case DatabaseConstants::MISCINVTABLE:
         defineMiscInventoryTable();
         break;
      case DatabaseConstants::YEASTINVTABLE:
         defineYeastInventoryTable();
         break;
      default:
         break;
   }
}

// Finally, the methods to define the properties and foreign keys
static const QString kPgSQLConstraint("SERIAL PRIMARY KEY");
static const QString kSQLiteConstraint("INTEGER PRIMARY KEY autoincrement");

void TableSchema::defineStyleTable()
{
   m_type = BASE;
   m_className = QString("Style");
   m_childTable = DatabaseConstants::STYLECHILDTABLE;
   m_btTable = DatabaseConstants::BT_STYLETABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntity::name]      = new PropertySchema( PropertyNames::NamedEntity::name,       PropertyNames::NamedEntity::name, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Style::type]      = new PropertySchema( PropertyNames::Style::typeString, kcolStyleType, QString("text"), QString("'Ale'"));
   m_properties[PropertyNames::Style::category]       = new PropertySchema( PropertyNames::Style::category,        kcolStyleCat, QString("text"), QString("''"));
   m_properties[PropertyNames::Style::categoryNumber]    = new PropertySchema( PropertyNames::Style::categoryNumber,     kcolStyleCatNum, QString("text"), QString("''"));
   m_properties[PropertyNames::Style::styleLetter]    = new PropertySchema( PropertyNames::Style::styleLetter,     kcolStyleLetter, QString("text"), QString("''"));
   m_properties[PropertyNames::Style::styleGuide]     = new PropertySchema( PropertyNames::Style::styleGuide,      kcolStyleGuide, QString("text"), QString("''"));
   m_properties[PropertyNames::Style::ogMin]     = new PropertySchema( PropertyNames::Style::ogMin,      kcolStyleOGMin, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::ogMax]     = new PropertySchema( PropertyNames::Style::ogMax,      kcolStyleOGMax, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::fgMin]     = new PropertySchema( PropertyNames::Style::fgMin,      kcolStyleFGMin, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::fgMax]     = new PropertySchema( PropertyNames::Style::fgMax,      kcolStyleFGMax, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::ibuMin]    = new PropertySchema( PropertyNames::Style::ibuMin,     kcolStyleIBUMin, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::ibuMax]    = new PropertySchema( PropertyNames::Style::ibuMax,     kcolStyleIBUMax, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::colorMin_srm]  = new PropertySchema( PropertyNames::Style::colorMin_srm,   kcolStyleColorMin, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::colorMax_srm]  = new PropertySchema( PropertyNames::Style::colorMax_srm,   kcolStyleColorMax, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::abvMin_pct]    = new PropertySchema( PropertyNames::Style::abvMin_pct,     kcolStyleABVMin, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::abvMax_pct]    = new PropertySchema( PropertyNames::Style::abvMax_pct,     kcolStyleABVMax, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::carbMin_vol]   = new PropertySchema( PropertyNames::Style::carbMin_vol,    kcolStyleCarbMin, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::carbMax_vol]   = new PropertySchema( PropertyNames::Style::carbMax_vol,    kcolStyleCarbMax, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Style::notes]     = new PropertySchema( PropertyNames::Style::notes,      kcolNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Style::profile]   = new PropertySchema( PropertyNames::Style::profile,    kcolStyleProfile, QString("text"), QString("''"));
   m_properties[PropertyNames::Style::ingredients]   = new PropertySchema( PropertyNames::Style::ingredients,    kcolStyleIngreds, QString("text"), QString("''"));
   m_properties[PropertyNames::Style::examples]  = new PropertySchema( PropertyNames::Style::examples,   kcolStyleExamples, QString("text"), QString("''"));

   // not sure about these, but I think I'm gonna need them anyway
   m_properties[PropertyNames::NamedEntity::display]   = new PropertySchema(PropertyNames::NamedEntity::display,   kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]   = new PropertySchema(PropertyNames::NamedEntity::deleted,   kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]    = new PropertySchema(PropertyNames::NamedEntity::folder,    kcolFolder,  QString("text"),    QString("''"));
}

void TableSchema::defineEquipmentTable()
{
   m_type = BASE;
   m_className = QString("Equipment");
   m_childTable = DatabaseConstants::EQUIPCHILDTABLE;
   m_btTable = DatabaseConstants::BT_EQUIPTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntity::name]          = new PropertySchema( PropertyNames::NamedEntity::name,          PropertyNames::NamedEntity::name, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Equipment::boilSize_l]      = new PropertySchema( PropertyNames::Equipment::boilSize_l,      kcolEquipBoilSize, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::batchSize_l]     = new PropertySchema( PropertyNames::Equipment::batchSize_l,     kcolEquipBatchSize, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::tunVolume_l]     = new PropertySchema( PropertyNames::Equipment::tunVolume_l,     kcolEquipTunVolume, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::tunWeight_kg]     = new PropertySchema( PropertyNames::Equipment::tunWeight_kg,     kcolEquipTunWeight, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::tunSpecificHeat_calGC]   = new PropertySchema( PropertyNames::Equipment::tunSpecificHeat_calGC,   kcolEquipTunSpecHeat, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::topUpWater_l]    = new PropertySchema( PropertyNames::Equipment::topUpWater_l,    kcolEquipTopUpWater, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::trubChillerLoss_l] = new PropertySchema( PropertyNames::Equipment::trubChillerLoss_l, kcolEquipTrubChillLoss, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::evapRate_pctHr]      = new PropertySchema( PropertyNames::Equipment::evapRate_pctHr,      kcolEquipEvapRate, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::boilTime_min]      = new PropertySchema( PropertyNames::Equipment::boilTime_min,      kcolEquipBoilTime, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::calcBoilVolume]   = new PropertySchema( PropertyNames::Equipment::calcBoilVolume,   kcolEquipCalcBoilVol, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::Equipment::lauterDeadspace_l]   = new PropertySchema( PropertyNames::Equipment::lauterDeadspace_l,   kcolEquipLauterSpace, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::topUpKettle_l]   = new PropertySchema( PropertyNames::Equipment::topUpKettle_l,   kcolEquipTopUpKettle, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::hopUtilization_pct]       = new PropertySchema( PropertyNames::Equipment::hopUtilization_pct,       kcolEquipHopUtil, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::notes]         = new PropertySchema( PropertyNames::Equipment::notes,         kcolNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Equipment::evapRate_lHr]  = new PropertySchema( PropertyNames::Equipment::evapRate_lHr,  kcolEquipRealEvapRate, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Equipment::boilingPoint_c]  = new PropertySchema( PropertyNames::Equipment::boilingPoint_c,  kcolEquipBoilingPoint, QString("real"), QVariant(100.0));
   m_properties[PropertyNames::Equipment::grainAbsorption_LKg]    = new PropertySchema( PropertyNames::Equipment::grainAbsorption_LKg,    kcolEquipAbsorption, QString("real"), QVariant(1.085));

   m_properties[PropertyNames::NamedEntity::display]       = new PropertySchema( PropertyNames::NamedEntity::display,       kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]       = new PropertySchema( PropertyNames::NamedEntity::deleted,       kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]        = new PropertySchema( PropertyNames::NamedEntity::folder,        kcolFolder,  QString("text"), QString("''"));

}

void TableSchema::defineFermentableTable()
{
   m_type = BASE;
   m_className = QString("Fermentable");
   m_childTable = DatabaseConstants::FERMCHILDTABLE;
   m_inRecTable = DatabaseConstants::FERMINRECTABLE;
   m_invTable   = DatabaseConstants::FERMINVTABLE;
   m_btTable    = DatabaseConstants::BT_FERMTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntity::name]           = new PropertySchema( PropertyNames::NamedEntity::name,           kcolName, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Fermentable::notes]          = new PropertySchema( PropertyNames::Fermentable::notes,          kcolNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Fermentable::type]           = new PropertySchema( PropertyNames::Fermentable::typeString,     kcolFermType, QString("text"), QString("'Grain'"));
   m_properties[PropertyNames::Fermentable::amount_kg]       = new PropertySchema( PropertyNames::Fermentable::amount_kg,       kcolAmount, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Fermentable::yield_pct]          = new PropertySchema( PropertyNames::Fermentable::yield_pct,          kcolFermYield, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Fermentable::color_srm]          = new PropertySchema( PropertyNames::Fermentable::color_srm,          kcolFermColor, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Fermentable::addAfterBoil]   = new PropertySchema( PropertyNames::Fermentable::addAfterBoil,   kcolFermAddAfterBoil, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::Fermentable::origin]         = new PropertySchema( PropertyNames::Fermentable::origin,         kcolFermOrigin, QString("text"), QString("''"));
   m_properties[PropertyNames::Fermentable::supplier]       = new PropertySchema( PropertyNames::Fermentable::supplier,       kcolFermSupplier, QString("text"), QString("''"));
   m_properties[PropertyNames::Fermentable::coarseFineDiff_pct] = new PropertySchema( PropertyNames::Fermentable::coarseFineDiff_pct, kcolFermCoarseFineDiff, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Fermentable::moisture_pct]       = new PropertySchema( PropertyNames::Fermentable::moisture_pct,       kcolFermMoisture, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Fermentable::diastaticPower_lintner] = new PropertySchema( PropertyNames::Fermentable::diastaticPower_lintner, kcolFermDiastaticPower, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Fermentable::protein_pct]        = new PropertySchema( PropertyNames::Fermentable::protein_pct,        kcolFermProtein, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Fermentable::maxInBatch_pct]     = new PropertySchema( PropertyNames::Fermentable::maxInBatch_pct,     kcolFermMaxInBatch, QString("real"), QVariant(100.0));
   m_properties[PropertyNames::Fermentable::recommendMash]  = new PropertySchema( PropertyNames::Fermentable::recommendMash,  kcolFermRecommendMash, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::Fermentable::isMashed]       = new PropertySchema( PropertyNames::Fermentable::isMashed,       kcolFermIsMashed, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::Fermentable::ibuGalPerLb]    = new PropertySchema( PropertyNames::Fermentable::ibuGalPerLb,    kcolFermIBUGalPerLb, QString("real"), QVariant(0.0));

   m_properties[PropertyNames::NamedEntity::display]        = new PropertySchema( PropertyNames::NamedEntity::display,        kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]        = new PropertySchema( PropertyNames::NamedEntity::deleted,        kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]         = new PropertySchema( PropertyNames::NamedEntity::folder,         kcolFolder,  QString("text"), QString("''"));

   // the inventory system is getting interesting
   m_foreignKeys[kpropInventoryId]   = new PropertySchema( kpropInventoryId,    kcolInventoryId,        QString("integer"),    m_invTable);

}

void TableSchema::defineHopTable()
{
   m_type = BASE;
   m_className = QString("Hop");
   m_childTable = DatabaseConstants::HOPCHILDTABLE;
   m_inRecTable = DatabaseConstants::HOPINRECTABLE;
   m_invTable   = DatabaseConstants::HOPINVTABLE;
   m_btTable    = DatabaseConstants::BT_HOPTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // These are defined in the global file.
   m_properties[PropertyNames::NamedEntity::name]          = new PropertySchema( PropertyNames::NamedEntity::name,          kcolName, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Hop::notes]         = new PropertySchema( PropertyNames::Hop::notes,         kcolNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Hop::amount_kg]      = new PropertySchema( PropertyNames::Hop::amount_kg,      kcolAmount, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Hop::use]           = new PropertySchema( PropertyNames::Hop::useString,     kcolUse, QString("text"), QString("'Boil'"));
   m_properties[PropertyNames::Hop::time_min]          = new PropertySchema( PropertyNames::Hop::time_min,          kcolTime, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Hop::origin]        = new PropertySchema( PropertyNames::Hop::origin,        kcolOrigin, QString("text"), QString("''"));
   m_properties[PropertyNames::Hop::substitutes]   = new PropertySchema( PropertyNames::Hop::substitutes,   kcolSubstitutes, QString("text"), QString("''"));
   m_properties[PropertyNames::Hop::alpha_pct]         = new PropertySchema( PropertyNames::Hop::alpha_pct,         kcolHopAlpha, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Hop::type]          = new PropertySchema( PropertyNames::Hop::typeString,    kcolHopType, QString("text"), QString("'Boil'"));
   m_properties[PropertyNames::Hop::form]          = new PropertySchema( PropertyNames::Hop::formString,    kcolHopForm, QString("text"), QString("'Pellet'"));
   m_properties[PropertyNames::Hop::beta_pct]          = new PropertySchema( PropertyNames::Hop::beta_pct,          kcolHopBeta, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Hop::hsi_pct]           = new PropertySchema( PropertyNames::Hop::hsi_pct,           kcolHopHSI, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Hop::humulene_pct]      = new PropertySchema( PropertyNames::Hop::humulene_pct,      kcolHopHumulene, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Hop::caryophyllene_pct] = new PropertySchema( PropertyNames::Hop::caryophyllene_pct, kcolHopCaryophyllene, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Hop::cohumulone_pct]    = new PropertySchema( PropertyNames::Hop::cohumulone_pct,    kcolHopCohumulone, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Hop::myrcene_pct]       = new PropertySchema( PropertyNames::Hop::myrcene_pct,       kcolHopMyrcene, QString("real"), QVariant(0.0));

   m_properties[PropertyNames::NamedEntity::display]       = new PropertySchema( PropertyNames::NamedEntity::display,       kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]       = new PropertySchema( PropertyNames::NamedEntity::deleted,       kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]        = new PropertySchema( PropertyNames::NamedEntity::folder,        kcolFolder,  QString("text"), QString("''"));

   m_foreignKeys[kpropInventoryId]  = new PropertySchema( kpropInventoryId,   kcolInventoryId,      QString("integer"),    m_invTable);
}

void TableSchema::defineInstructionTable()
{
   m_type = BASE;
   m_className = QString("Instruction");
   m_inRecTable = DatabaseConstants::INSTINRECTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // These are defined in the global file.
   m_properties[PropertyNames::NamedEntity::name]          = new PropertySchema( PropertyNames::NamedEntity::name,          kcolName, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Instruction::directions]    = new PropertySchema( PropertyNames::Instruction::directions,    kcolInstructionDirections, QString("text"), QString("''"));
   m_properties[PropertyNames::Instruction::hasTimer]      = new PropertySchema( PropertyNames::Instruction::hasTimer,      kcolInstructionHasTimer, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::Instruction::timerValue]    = new PropertySchema( PropertyNames::Instruction::timerValue,    kcolInstructionTimerValue, QString("text"), QVariant("'00:00:00'"));
   m_properties[PropertyNames::Instruction::completed]     = new PropertySchema( PropertyNames::Instruction::completed,     kcolInstructionCompleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::Instruction::interval]      = new PropertySchema( PropertyNames::Instruction::interval,      kcolInstructionInterval, QString("real"), QVariant(0.0));

   m_properties[PropertyNames::NamedEntity::display]       = new PropertySchema( PropertyNames::NamedEntity::display,       kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]       = new PropertySchema( PropertyNames::NamedEntity::deleted,       kcolDeleted, QString("boolean"), QVariant(false));
}

void TableSchema::defineMashTable()
{
   m_type = BASE;
   m_className = QString("Mash");

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // These are defined in the global file.
   m_properties[PropertyNames::NamedEntity::name]        = new PropertySchema( PropertyNames::NamedEntity::name,        kcolName, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Mash::notes]       = new PropertySchema( PropertyNames::Mash::notes,       kcolNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Mash::grainTemp_c]   = new PropertySchema( PropertyNames::Mash::grainTemp_c,   kcolMashGrainTemp, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Mash::tunTemp_c]     = new PropertySchema( PropertyNames::Mash::tunTemp_c,     kcolMashTunTemp, QString("real"), QVariant(20.0));
   m_properties[PropertyNames::Mash::spargeTemp_c]  = new PropertySchema( PropertyNames::Mash::spargeTemp_c,  kcolMashSpargeTemp, QString("real"), QVariant(74.0));
   m_properties[PropertyNames::Mash::ph]          = new PropertySchema( PropertyNames::Mash::ph,          kcolPH, QString("real"), QVariant(7.0));
   m_properties[PropertyNames::Mash::tunWeight_kg]   = new PropertySchema( PropertyNames::Mash::tunWeight_kg,   kcolMashTunWeight, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Mash::tunSpecificHeat_calGC] = new PropertySchema( PropertyNames::Mash::tunSpecificHeat_calGC, kcolMashTunSpecHeat, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Mash::equipAdjust] = new PropertySchema( PropertyNames::Mash::equipAdjust, kcolMashEquipAdjust, QString("boolean"), QVariant(true));

   m_properties[PropertyNames::NamedEntity::display]     = new PropertySchema( PropertyNames::NamedEntity::display,     kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]     = new PropertySchema( PropertyNames::NamedEntity::deleted,     kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]      = new PropertySchema( PropertyNames::NamedEntity::folder,      kcolFolder,  QString("text"), QString("''"));
}

// property name, column name, xml property name, column type, column default, column constraint
void TableSchema::defineMashstepTable()
{
   m_type = BASE;
   m_className = QString("MashStep");

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntity::name]       = new PropertySchema( PropertyNames::NamedEntity::name,       kcolName, QString("text"), QString("''"),QString("not null"));
   m_properties[PropertyNames::MashStep::type]       = new PropertySchema( PropertyNames::MashStep::typeString, kcolMashstepType, QString("text"), QString("'Infusion'"));
   m_properties[PropertyNames::MashStep::infuseAmount_l]  = new PropertySchema( PropertyNames::MashStep::infuseAmount_l,  kcolMashstepInfuseAmt, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::MashStep::stepTemp_c]   = new PropertySchema( PropertyNames::MashStep::stepTemp_c,   kcolMashstepStepTemp, QString("real"), QVariant(67.0));
   m_properties[PropertyNames::MashStep::stepTime_min]   = new PropertySchema( PropertyNames::MashStep::stepTime_min,   kcolMashstepStepTime, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::MashStep::rampTime_min]   = new PropertySchema( PropertyNames::MashStep::rampTime_min,   kcolMashstepRampTime, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::MashStep::endTemp_c]    = new PropertySchema( PropertyNames::MashStep::endTemp_c,    kcolMashstepEndTemp, QString("real"), QVariant(67.0));
   m_properties[PropertyNames::MashStep::infuseTemp_c] = new PropertySchema( PropertyNames::MashStep::infuseTemp_c, kcolMashstepInfuseTemp, QString("real"), QVariant(67.0));
   m_properties[PropertyNames::MashStep::decoctionAmount_l]  = new PropertySchema( PropertyNames::MashStep::decoctionAmount_l,  kcolMashstepDecoctAmt, QString("real"), QVariant(67.0));
   m_properties[PropertyNames::MashStep::stepNumber] = new PropertySchema( PropertyNames::MashStep::stepNumber, kcolMashstepStepNumber, QString("integer"), QVariant(0));

   m_properties[PropertyNames::NamedEntity::display]    = new PropertySchema( PropertyNames::NamedEntity::display,    kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]    = new PropertySchema( PropertyNames::NamedEntity::deleted,    kcolDeleted, QString("boolean"), QVariant(false));

   m_foreignKeys[kpropMashId]    = new PropertySchema( kpropMashId,     kcolMashId,       QString("integer"), DatabaseConstants::MASHTABLE);

}

void TableSchema::defineMiscTable()
{
   m_type = BASE;
   m_className = QString("Misc");
   m_childTable = DatabaseConstants::MISCCHILDTABLE;
   m_inRecTable = DatabaseConstants::MISCINRECTABLE;
   m_invTable   = DatabaseConstants::MISCINVTABLE;
   m_btTable    = DatabaseConstants::BT_MISCTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // These are defined in the global file.
   m_properties[PropertyNames::NamedEntity::name]     = new PropertySchema( PropertyNames::NamedEntity::name,       kcolName, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Misc::notes]    = new PropertySchema( PropertyNames::Misc::notes,      kcolNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Misc::amount]   = new PropertySchema( PropertyNames::Misc::amount,     kcolAmount, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Misc::use]      = new PropertySchema( PropertyNames::Misc::useString,  kcolUse, QString("text"), QString("'Boil'"));
   m_properties[PropertyNames::Hop::time_min]     = new PropertySchema( PropertyNames::Hop::time_min,       kcolTime, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Misc::type]     = new PropertySchema( PropertyNames::Misc::typeString, kcolMiscType, QString("text"), QString("'Other'"));
   m_properties[PropertyNames::Misc::amountIsWeight] = new PropertySchema( PropertyNames::Misc::amountIsWeight,   kcolMiscAmtIsWgt, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::Misc::useFor]   = new PropertySchema( PropertyNames::Misc::useFor,     kcolMiscUseFor, QString("text"), QString("''"));

   m_properties[PropertyNames::NamedEntity::display]  = new PropertySchema( PropertyNames::NamedEntity::display,  kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]  = new PropertySchema( PropertyNames::NamedEntity::deleted,  kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]   = new PropertySchema( PropertyNames::NamedEntity::folder,   kcolFolder,  QString("text"), QString("''"));

   m_foreignKeys[kpropInventoryId]  = new PropertySchema( kpropInventoryId,   kcolInventoryId,      QString("integer"),    m_invTable);
}

void TableSchema::defineRecipeTable()
{
   m_type = BASE;
   m_className = QString("Recipe");
   m_childTable = DatabaseConstants::RECIPECHILDTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntity::name]        = new PropertySchema( PropertyNames::NamedEntity::name,        kcolName, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Recipe::notes]       = new PropertySchema( PropertyNames::Recipe::notes,       kcolNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Recipe::type]        = new PropertySchema( PropertyNames::Recipe::type,        kcolRecipeType, QString("text"), QString("'All Grain'"));
   m_properties[PropertyNames::Recipe::brewer]      = new PropertySchema( PropertyNames::Recipe::brewer,      kcolRecipeBrewer, QString("text"), QString("''"));
   m_properties[PropertyNames::Recipe::asstBrewer]  = new PropertySchema( PropertyNames::Recipe::asstBrewer,  kcolRecipeAsstBrewer, QString("text"), QString("'Brewken'"));
   m_properties[PropertyNames::Recipe::batchSize_l]   = new PropertySchema( PropertyNames::Recipe::batchSize_l,   kcolRecipeBatchSize, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Recipe::boilSize_l]    = new PropertySchema( PropertyNames::Recipe::boilSize_l,    kcolRecipeBoilSize, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Recipe::boilTime_min]    = new PropertySchema( PropertyNames::Recipe::boilTime_min,    kcolRecipeBoilTime, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Recipe::efficiency_pct]      = new PropertySchema( PropertyNames::Recipe::efficiency_pct,      kcolRecipeEff, QString("real"), QVariant(70.0));
   m_properties[PropertyNames::Recipe::og]          = new PropertySchema( PropertyNames::Recipe::og,          kcolRecipeOG, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::Recipe::fg]          = new PropertySchema( PropertyNames::Recipe::fg,          kcolRecipeFG, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::Recipe::fermentationStages]  = new PropertySchema( PropertyNames::Recipe::fermentationStages,  kcolRecipeFermStages, QString("int"), QVariant(0));
   m_properties[PropertyNames::Recipe::primaryAge_days] = new PropertySchema( PropertyNames::Recipe::primaryAge_days, kcolRecipePrimAgeDays, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Recipe::primaryTemp_c]    = new PropertySchema( PropertyNames::Recipe::primaryTemp_c,    kcolRecipePrimTemp, QString("real"), QVariant(20.0));
   m_properties[PropertyNames::Recipe::secondaryAge_days]  = new PropertySchema( PropertyNames::Recipe::secondaryAge_days,  kcolRecipeSecAgeDays, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Recipe::secondaryTemp_c]     = new PropertySchema( PropertyNames::Recipe::secondaryTemp_c,     kcolRecipeSecTemp, QString("real"), QVariant(20.0));
   m_properties[PropertyNames::Recipe::tertiaryAge_days] = new PropertySchema( PropertyNames::Recipe::tertiaryAge_days, kcolRecipeTertAgeDays, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Recipe::tertiaryTemp_c]    = new PropertySchema( PropertyNames::Recipe::tertiaryTemp_c,    kcolRecipeTertTemp, QString("real"), QVariant(20.0));
   m_properties[PropertyNames::Recipe::age]         = new PropertySchema( PropertyNames::Recipe::age,         kcolRecipeAge, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Recipe::ageTemp_c]     = new PropertySchema( PropertyNames::Recipe::ageTemp_c,     kcolRecipeAgeTemp, QString("real"), QVariant(20.0));
   m_properties[PropertyNames::Recipe::date]        = new PropertySchema( PropertyNames::Recipe::date,        kcolRecipeDate, QString(PropertyNames::Recipe::date), QString("CURRENT_TIMESTAMP"));
   m_properties[PropertyNames::Recipe::carbonation_vols]    = new PropertySchema( PropertyNames::Recipe::carbonation_vols,    kcolRecipeCarbVols, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Recipe::forcedCarbonation]  = new PropertySchema( PropertyNames::Recipe::forcedCarbonation,  kcolRecipeForcedCarb, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::Recipe::primingSugarName] = new PropertySchema( PropertyNames::Recipe::primingSugarName, kcolRecipePrimSugName, QString("text"), QString("''"));
   m_properties[PropertyNames::Recipe::carbonationTemp_c]    = new PropertySchema( PropertyNames::Recipe::carbonationTemp_c,    kcolRecipeCarbTemp, QString("real"), QVariant(20.0));
   m_properties[PropertyNames::Recipe::primingSugarEquiv]= new PropertySchema( PropertyNames::Recipe::primingSugarEquiv,kcolRecipePrimSugEquiv, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::Recipe::kegPrimingFactor] = new PropertySchema( PropertyNames::Recipe::kegPrimingFactor, kcolRecipeKegPrimFact, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::Recipe::tasteNotes]  = new PropertySchema( PropertyNames::Recipe::tasteNotes,  kcolRecipeTasteNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Recipe::tasteRating] = new PropertySchema( PropertyNames::Recipe::tasteRating, kcolRecipeTasteRating, QString("real"), QVariant(20.0));

   m_properties[PropertyNames::NamedEntity::display]     = new PropertySchema( PropertyNames::NamedEntity::display,     kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]     = new PropertySchema( PropertyNames::NamedEntity::deleted,     kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]      = new PropertySchema( PropertyNames::NamedEntity::folder,      kcolFolder,  QString("text"), QString("''"));
   // m_properties[kpropLocked]      = new PropertySchema( kpropLocked,      kcolLocked,             QString(),            QString("boolean"), QVariant(false));

   // enough properties, now some foreign keys

   m_foreignKeys[kpropEquipmentId] = new PropertySchema( kpropEquipmentId, kcolRecipeEquipmentId, QString("integer"), DatabaseConstants::EQUIPTABLE);
   m_foreignKeys[kpropMashId]      = new PropertySchema( kpropMashId,      kcolMashId,            QString("integer"), DatabaseConstants::MASHTABLE);
   m_foreignKeys[kpropStyleId]     = new PropertySchema( kpropStyleId,     kcolStyleId,           QString("integer"), DatabaseConstants::STYLETABLE);
   m_foreignKeys[kpropAncestorId]  = new PropertySchema( kpropAncestorId,  kcolRecipeAncestorId,  QString("integer"), DatabaseConstants::RECTABLE);
}

void TableSchema::defineYeastTable()
{
   m_type = BASE;
   m_className = QString("Yeast");
   m_childTable = DatabaseConstants::YEASTCHILDTABLE;
   m_inRecTable = DatabaseConstants::YEASTINRECTABLE;
   m_invTable   = DatabaseConstants::YEASTINVTABLE;
   m_btTable    = DatabaseConstants::BT_YEASTTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // These are defined in the global file.
   m_properties[PropertyNames::NamedEntity::name]       = new PropertySchema( PropertyNames::NamedEntity::name,       kcolName, QString("text"), QString("''"), QString("not null"));
   m_properties[PropertyNames::Yeast::notes]      = new PropertySchema( PropertyNames::Yeast::notes,      kcolNotes, QString("text"), QString("''"));
   m_properties[PropertyNames::Yeast::type]       = new PropertySchema( PropertyNames::Yeast::typeString, kcolYeastType, QString("text"), QObject::tr("'Ale'"));
   m_properties[PropertyNames::Yeast::form]       = new PropertySchema( PropertyNames::Yeast::formString, kcolYeastForm, QString("text"), QObject::tr("'Liquid'"));
   m_properties[PropertyNames::Yeast::amount]     = new PropertySchema( PropertyNames::Yeast::amount,     kcolYeastAmount, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Yeast::amountIsWeight]   = new PropertySchema( PropertyNames::Yeast::amountIsWeight,   kcolYeastAmtIsWgt, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::Yeast::laboratory]        = new PropertySchema( PropertyNames::Yeast::laboratory,        kcolYeastLab, QString("text"), QString("''"));
   m_properties[PropertyNames::Yeast::productID]  = new PropertySchema( PropertyNames::Yeast::productID,  kcolYeastProductID, QString("text"), QString("''"));
   m_properties[PropertyNames::Yeast::minTemperature_c]    = new PropertySchema( PropertyNames::Yeast::minTemperature_c,    kcolYeastMinTemp, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Yeast::maxTemperature_c]    = new PropertySchema( PropertyNames::Yeast::maxTemperature_c,    kcolYeastMaxTemp, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::Yeast::flocculation]       = new PropertySchema( PropertyNames::Yeast::flocculationString, kcolYeastFloc, QString("text"), QObject::tr("'Medium'"));
   m_properties[PropertyNames::Yeast::attenuation_pct]   = new PropertySchema( PropertyNames::Yeast::attenuation_pct,   kcolYeastAtten, QString("real"), QVariant(75.0));
   m_properties[PropertyNames::Yeast::bestFor]    = new PropertySchema( PropertyNames::Yeast::bestFor,    kcolYeastBestFor, QString("text"), QString("''"));
   m_properties[PropertyNames::Yeast::timesCultured] = new PropertySchema( PropertyNames::Yeast::timesCultured, kcolYeastTimesCultd, QString("int"), QVariant(0));
   m_properties[PropertyNames::Yeast::maxReuse]   = new PropertySchema( PropertyNames::Yeast::maxReuse,   kcolYeastMaxReuse, QString("int"), QVariant(10));
   m_properties[PropertyNames::Yeast::addToSecondary]   = new PropertySchema( PropertyNames::Yeast::addToSecondary,   kcolYeastAddToSec, QString("boolean"), QVariant(false));

   m_properties[PropertyNames::NamedEntity::display]    = new PropertySchema( PropertyNames::NamedEntity::display,    kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]    = new PropertySchema( PropertyNames::NamedEntity::deleted,    kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]     = new PropertySchema( PropertyNames::NamedEntity::folder,     kcolFolder,  QString("text"),    QString("''"));

   m_foreignKeys[kpropInventoryId]  = new PropertySchema( kpropInventoryId,   kcolInventoryId,      QString("integer"),    m_invTable);
}

void TableSchema::defineBrewnoteTable()
{
   m_type = BASE;
   m_className = QString("BrewNote");

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::BrewNote::notes]           = new PropertySchema( PropertyNames::BrewNote::notes,           kcolNotes, QString("text"),    QString("''"));

   m_properties[PropertyNames::BrewNote::brewDate]        = new PropertySchema( PropertyNames::BrewNote::brewDate,        kcolBNoteBrewDate, QString("timestamp"), QString("CURRENT_TIMESTAMP"));
   m_properties[PropertyNames::BrewNote::fermentDate]        = new PropertySchema( PropertyNames::BrewNote::fermentDate,        kcolBNoteFermDate, QString("timestamp"), QString("CURRENT_TIMESTAMP"));
   m_properties[PropertyNames::BrewNote::sg]              = new PropertySchema( PropertyNames::BrewNote::sg,              kcolBNoteSG, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::volumeIntoBK_l]     = new PropertySchema( PropertyNames::BrewNote::volumeIntoBK_l,     kcolBNoteVolIntoBoil, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::BrewNote::strikeTemp_c]      = new PropertySchema( PropertyNames::BrewNote::strikeTemp_c,      kcolBNoteStrikeTemp, QString("real"), QVariant(70.0));
   m_properties[PropertyNames::BrewNote::mashFinTemp_c]     = new PropertySchema( PropertyNames::BrewNote::mashFinTemp_c,     kcolBNoteMashFinTemp, QString("real"), QVariant(67.0));
   m_properties[PropertyNames::BrewNote::og]              = new PropertySchema( PropertyNames::BrewNote::og,              kcolBNoteOG, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::postBoilVolume_l]     = new PropertySchema( PropertyNames::BrewNote::postBoilVolume_l,     kcolBNotePostBoilVol, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::BrewNote::volumeIntoFerm_l]     = new PropertySchema( PropertyNames::BrewNote::volumeIntoFerm_l,     kcolBNoteVolIntoFerm, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::BrewNote::pitchTemp_c]       = new PropertySchema( PropertyNames::BrewNote::pitchTemp_c,       kcolBNotePitchTemp, QString("real"), QVariant(20.0));
   m_properties[PropertyNames::BrewNote::fg]              = new PropertySchema( PropertyNames::BrewNote::fg,              kcolBNoteFG, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::effIntoBK_pct]     = new PropertySchema( PropertyNames::BrewNote::effIntoBK_pct,     kcolBNoteEffIntoBoil, QString("real"), QVariant(70.0));
   m_properties[PropertyNames::BrewNote::abv]             = new PropertySchema( PropertyNames::BrewNote::abv,             kcolBNoteABV, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::BrewNote::projOg]          = new PropertySchema( PropertyNames::BrewNote::projOg,          kcolBNoteProjOG, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::brewhouseEff_pct]       = new PropertySchema( PropertyNames::BrewNote::brewhouseEff_pct,       kcolBNoteBrewhsEff, QString("real"), QVariant(70.0));
   m_properties[PropertyNames::BrewNote::projBoilGrav]    = new PropertySchema( PropertyNames::BrewNote::projBoilGrav,    kcolBNoteProjBoilGrav, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::projStrikeTemp_c]  = new PropertySchema( PropertyNames::BrewNote::projStrikeTemp_c,  kcolBNoteProjStrikeTemp, QString("real"), QVariant(70.0));
   m_properties[PropertyNames::BrewNote::projMashFinTemp_c] = new PropertySchema( PropertyNames::BrewNote::projMashFinTemp_c, kcolBNoteProjMashFinTemp, QString("real"), QVariant(67.0));
   m_properties[PropertyNames::BrewNote::projVolIntoBK_l] = new PropertySchema( PropertyNames::BrewNote::projVolIntoBK_l, kcolBNoteProjVolIntoBoil, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::projOg]          = new PropertySchema( PropertyNames::BrewNote::projOg,          kcolBNoteProjOG, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::projVolIntoFerm_l] = new PropertySchema( PropertyNames::BrewNote::projVolIntoFerm_l, kcolBNoteProjVolIntoFerm, QString("real"), QVariant(0.0));
   m_properties[PropertyNames::BrewNote::projFg]          = new PropertySchema( PropertyNames::BrewNote::projFg,          kcolBNoteProjFG, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::projEff_pct]         = new PropertySchema( PropertyNames::BrewNote::projEff_pct,         kcolBNoteProjEff, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::projABV_pct]         = new PropertySchema( PropertyNames::BrewNote::projABV_pct,         kcolBNoteProjABV, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::projAtten]       = new PropertySchema( PropertyNames::BrewNote::projAtten,       kcolBNoteProjAtten, QString("real"), QVariant(75.0));
   m_properties[PropertyNames::BrewNote::projPoints]        = new PropertySchema( PropertyNames::BrewNote::projPoints,        kcolBNoteProjPnts, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::projFermPoints]    = new PropertySchema( PropertyNames::BrewNote::projFermPoints,    kcolBNoteProjFermPnts, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::boilOff_l]         = new PropertySchema( PropertyNames::BrewNote::boilOff_l,         kcolBNoteBoilOff, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::finalVolume_l]          = new PropertySchema( PropertyNames::BrewNote::finalVolume_l,          kcolBNoteFinVol, QString("real"), QVariant(1.0));
   m_properties[PropertyNames::BrewNote::attenuation]           = new PropertySchema( PropertyNames::BrewNote::attenuation,           kcolBNoteAtten, QString("real"), QVariant(1.0));

   m_properties[PropertyNames::NamedEntity::display]         = new PropertySchema( PropertyNames::NamedEntity::display,         kcolDisplay,  QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]         = new PropertySchema( PropertyNames::NamedEntity::deleted,         kcolDeleted,  QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]          = new PropertySchema( PropertyNames::NamedEntity::folder,          kcolFolder,   QString("text"), QString("''"));

   m_foreignKeys[kpropRecipeId] = new PropertySchema( kpropRecipeId, kcolRecipeId, QString("integer"), DatabaseConstants::RECTABLE);

}

void TableSchema::defineWaterTable()
{
   m_type = BASE;
   m_className = QString("Water");
   m_childTable = DatabaseConstants::WATERCHILDTABLE;
   m_inRecTable = DatabaseConstants::WATERINRECTABLE;
   m_btTable    = DatabaseConstants::BT_WATERTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // These are defined in the global file.
   m_properties[PropertyNames::NamedEntity::name]       = new PropertySchema(PropertyNames::NamedEntity::name,       kcolName, QString("text"),    QString("''"), QString("not null"));
   m_properties[PropertyNames::Water::notes]            = new PropertySchema(PropertyNames::Water::notes,            kcolNotes, QString("text"),    QString("''"));
   m_properties[PropertyNames::Water::amount]           = new PropertySchema(PropertyNames::Water::amount,           kcolAmount, QString("real"),    QVariant(0.0));

   m_properties[PropertyNames::Water::calcium_ppm]      = new PropertySchema(PropertyNames::Water::calcium_ppm,      kcolWaterCalcium, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::bicarbonate_ppm]  = new PropertySchema(PropertyNames::Water::bicarbonate_ppm,  kcolWaterBiCarbonate, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::sulfate_ppm]      = new PropertySchema(PropertyNames::Water::sulfate_ppm,      kcolWaterSulfate, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::sodium_ppm]       = new PropertySchema(PropertyNames::Water::sodium_ppm,       kcolWaterSodium, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::chloride_ppm]     = new PropertySchema(PropertyNames::Water::chloride_ppm,     kcolWaterChloride, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::magnesium_ppm]    = new PropertySchema(PropertyNames::Water::magnesium_ppm,    kcolWaterMagnesium, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::ph]               = new PropertySchema(PropertyNames::Water::ph,               kcolPH, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::alkalinity]       = new PropertySchema(PropertyNames::Water::alkalinity,       kcolWaterAlkalinity,  QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::type]             = new PropertySchema(PropertyNames::Water::type,             kcolWaterType,        QString("int"),     QVariant(0));
   m_properties[PropertyNames::Water::mashRO]           = new PropertySchema(PropertyNames::Water::mashRO,           kcolWaterMashRO,      QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::spargeRO]         = new PropertySchema(PropertyNames::Water::spargeRO,         kcolWaterSpargeRO,    QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Water::alkalinityAsHCO3] = new PropertySchema(PropertyNames::Water::alkalinityAsHCO3, kcolWaterAsHCO3,      QString("boolean"), QVariant(true));

   m_properties[PropertyNames::NamedEntity::display]    = new PropertySchema(PropertyNames::NamedEntity::display,    kcolDisplay,          QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]    = new PropertySchema(PropertyNames::NamedEntity::deleted,    kcolDeleted,          QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]     = new PropertySchema(PropertyNames::NamedEntity::folder,     kcolFolder,           QString("text"),    QString("''"));

}

void TableSchema::defineSaltTable()
{
   m_type = BASE;
   m_className = QString("Salt");
   m_inRecTable = DatabaseConstants::SALTINRECTABLE;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // These are defined in the global file.
   m_properties[PropertyNames::NamedEntity::name]     = new PropertySchema( PropertyNames::NamedEntity::name,     kcolName, QString("text"),    QString("''"), QString("not null"));
   m_properties[PropertyNames::Salt::amount]   = new PropertySchema( PropertyNames::Salt::amount,   kcolAmount, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Salt::amountIsWeight] = new PropertySchema( PropertyNames::Salt::amountIsWeight, kcolSaltAmtIsWgt, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::Salt::percentAcid]  = new PropertySchema( PropertyNames::Salt::percentAcid,  kcolSaltPctAcid, QString("real"),    QVariant(0.0));
   m_properties[PropertyNames::Salt::isAcid]   = new PropertySchema( PropertyNames::Salt::isAcid,   kcolSaltIsAcid, QString("boolean"), QVariant(false));

   m_properties[PropertyNames::Salt::type]     = new PropertySchema( PropertyNames::Salt::type,    kcolSaltType, QString("int"),     QVariant(0));
   m_properties[PropertyNames::Salt::addTo]    = new PropertySchema( PropertyNames::Salt::addTo,   kcolSaltAddTo, QString("int"),     QVariant(0));

   m_properties[PropertyNames::NamedEntity::display]  = new PropertySchema( PropertyNames::NamedEntity::display, kcolDisplay, QString("boolean"), QVariant(true));
   m_properties[PropertyNames::NamedEntity::deleted]  = new PropertySchema( PropertyNames::NamedEntity::deleted, kcolDeleted, QString("boolean"), QVariant(false));
   m_properties[PropertyNames::NamedEntity::folder]   = new PropertySchema( PropertyNames::NamedEntity::folder,  kcolFolder,  QString("text"),    QString("''"));

   m_foreignKeys[kpropMiscId]  = new PropertySchema( kpropMiscId, kcolMiscId, QString("integer"), DatabaseConstants::MISCTABLE);
}

void TableSchema::defineChildTable(DatabaseConstants::DbTableId table)
{
   m_type = CHILD;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_foreignKeys[kpropChildId]  = new PropertySchema( kpropChildId,  kcolChildId,  QString("integer"), table);
   m_foreignKeys[kpropParentId] = new PropertySchema( kpropParentId, kcolParentId, QString("integer"), table);

}

void TableSchema::defineInRecipeTable(QString childIdx, DatabaseConstants::DbTableId table)
{
   m_type = INREC;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_foreignKeys[kpropRecipeId] = new PropertySchema( kpropRecipeId, kcolRecipeId, QString("integer"), DatabaseConstants::RECTABLE);
   m_foreignKeys[childIdx]      = new PropertySchema( childIdx,      childIdx,     QString("integer"), table);

}

// instruction in rec has an extra field. I could have cheated, but we will try
// playing it straight first.
void TableSchema::defineInstructionInRecipeTable(QString childIdx, DatabaseConstants::DbTableId table)
{
   m_type = INREC;
   m_trigger = kpropInstructionNumber;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // I am not breaking these rules any more. It makes it too annoying in the calling code to know when to use a kcol or kprop
   // so it is now kprop all the time
   m_properties[kpropInstructionNumber] = new PropertySchema( kpropInstructionNumber, kcolInstructionNumber, QString("int"), QVariant(0));

   m_foreignKeys[kpropRecipeId] = new PropertySchema( kpropRecipeId, kcolRecipeId, QString("integer"), DatabaseConstants::RECTABLE);
   m_foreignKeys[childIdx]      = new PropertySchema( childIdx,      childIdx,     QString("integer"), table);

}

void TableSchema::defineBtTable(QString childIdx, DatabaseConstants::DbTableId table)
{
   m_type = BT;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   // What good is a rule followed to well?
   m_foreignKeys[childIdx] = new PropertySchema( childIdx, childIdx, QString("integer"), table);

}

void TableSchema::defineFermInventoryTable()
{
   m_type = INV;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntityWithInventory::inventory]      = new PropertySchema( PropertyNames::NamedEntityWithInventory::inventory,     kcolAmount, QString("real"), QVariant(0.0));
}

void TableSchema::defineHopInventoryTable()
{
   m_type = INV;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntityWithInventory::inventory] = new PropertySchema( PropertyNames::NamedEntityWithInventory::inventory, kcolAmount, QString("real"), QVariant(0.0));
}

void TableSchema::defineMiscInventoryTable()
{
   m_type = INV;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntityWithInventory::inventory] = new PropertySchema( PropertyNames::NamedEntityWithInventory::inventory, kcolAmount, QString("real"), QVariant(0.0));

}

void TableSchema::defineYeastInventoryTable()
{
   m_type = INV;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[PropertyNames::NamedEntityWithInventory::inventory] = new PropertySchema( kpropQuanta,  kcolYeastQuanta, QString("real"), QVariant(0.0));

}

void TableSchema::defineSettingsTable()
{
   m_type = META;

   m_key                        = new PropertySchema();
   m_key->addProperty(kpropKey, Database::PGSQL,  kcolKey, QString("integer"), QVariant(0), 0, kPgSQLConstraint);
   m_key->addProperty(kpropKey, Database::SQLITE, kcolKey, QString("integer"), QVariant(0), 0, kSQLiteConstraint);

   m_properties[kpropSettingsVersion]    = new PropertySchema( QString(), kcolSettingsVersion,    QString("integer"), QVariant(0));
   m_properties[kpropSettingsRepopulate] = new PropertySchema( QString(), kcolSettingsRepopulate, QString("integer"), QVariant(0));
}
