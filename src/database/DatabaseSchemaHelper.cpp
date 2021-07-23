/**
 * database/DatabaseSchemaHelper.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Jonatan Pålsson <jonatan.p@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#include "database/DatabaseSchemaHelper.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QVariant>

#include "Brewken.h"
#include "database/Database.h"
#include "database/DatabaseSchema.h"
#include "database/DbTransaction.h"
#include "database/BrewNoteSchema.h"
#include "database/ObjectStoreWrapper.h"
#include "database/SettingsSchema.h"
#include "database/TableSchemaConst.h"
#include "database/TableSchema.h"
#include "database/WaterSchema.h"
#include "model/BrewNote.h"
#include "model/Water.h"

int const DatabaseSchemaHelper::dbVersion = 10;

namespace {

   //! \brief converts sqlite values (mostly booleans) into something postgres wants
   QVariant convertValue(Database::DbType newType, QSqlField field) {
      QVariant retVar = field.value();
      if ( field.type() == QVariant::Bool ) {
         switch(newType) {
            case Database::PGSQL:
               retVar = field.value().toBool();
               break;
            default:
               retVar = field.value().toInt();
               break;
         }
      } else if ( field.name() == PropertyNames::BrewNote::fermentDate && field.value().toString() == "CURRENT_DATETIME" ) {
         retVar = "'now()'";
      }
      return retVar;
   }

   struct QueryAndParameters {
      QString sql;
      QVector<QVariant> bindValues = {};
   };

   //
   // These migrate_to_Xyz functions are deliberately hard-coded.  Because we're migrating from version N to version
   // N+1, we don't need (or want) to refer to the generated table definitions from some later version of the schema,
   // which may be quite different.
   //
   // For the moment, it mostly suffices to execute a list of queries.  A possible future enhancement might be to
   // attach to each query a (usually empty) list of bind parameters, but it's probably not necessary.
   //
   bool executeSqlQueries(QSqlQuery & q, QVector<QueryAndParameters> const & queries) {
      bool ret = true;
      for (auto & query : queries) {
         qDebug() << Q_FUNC_INFO << query.sql;
         q.prepare(query.sql);
         for (auto & bv : query.bindValues) {
            q.addBindValue(bv);
         }
         ret &= q.exec();
         if (!ret) {
            qCritical() <<
               Q_FUNC_INFO << "Error executing database upgrade/set-up query " << query.sql << ": " << q.lastError().text();
         }
      }
      return ret;
   }

   // This is when we first defined the settings table, and defined the version as a string.
   // In the new world, this will create the settings table and define the version as an int.
   // Since we don't set the version until the very last step of the update, I think this will be fine.
   bool migrate_to_202(Database & db, QSqlQuery q) {
      bool ret = true;

      // Add "projected_ferm_points" to brewnote table
      QString queryString{"ALTER TABLE brewnote ADD COLUMN projected_ferm_points "};
      QTextStream queryStringAsStream{&queryString};
      queryStringAsStream << db.getDbNativeTypeName<double>() << " DEFAULT 0.0;";
      qDebug() << Q_FUNC_INFO << queryString;
      ret &= q.exec(queryString);
      queryString = "ALTER TABLE brewnote SET projected_ferm_points = -1.0;";
      qDebug() << Q_FUNC_INFO << queryString;
      ret &= q.exec(queryString);

      // Add the settings table
      queryString = "CREATE TABLE settings ";
      queryStringAsStream << "(\n"
         "id " << db.getDbNativeTypeName<int>() << " " << db.getDbNativeIntPrimaryKeyModifier() << ",\n"
         "repopulatechildrenonnextstart " << db.getDbNativeTypeName<int>() << " DEFAULT 0,\n"
         "version " << db.getDbNativeTypeName<int>() << " DEFAULT 0);";
      qDebug() << Q_FUNC_INFO << queryString;
      ret &= q.exec(queryString);

      return ret;
   }

   bool migrate_to_210(Database & db, QSqlQuery & q) {
      QVector<QueryAndParameters> const migrationQueries{
         {QString("ALTER TABLE equipment   ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE fermentable ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE hop         ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE misc        ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE style       ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE yeast       ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE water       ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE mash        ADD COLUMN folder text DEFAULT ''")},
         //{QString("ALTER TABLE mashstep ADD COLUMN   DEFAULT ''")},
         {QString("ALTER TABLE recipe      ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE brewnote    ADD COLUMN folder text DEFAULT ''")},
         {QString("ALTER TABLE instruction ADD COLUMN   DEFAULT ''")},
         {QString("ALTER TABLE salt        ADD COLUMN folder text DEFAULT ''")},
         // Put the "Bt:.*" recipes into /brewken folder
         {QString("UPDATE recipe   SET folder='/brewken' WHERE name LIKE 'Bt:%'")},
         // Update version to 2.1.0
         {QString("UPDATE settings SET version='2.1.0' WHERE id=1")},
         // Used to trigger the code to populate the ingredient inheritance tables
         {QString("ALTER TABLE settings ADD COLUMN repopulatechildrenonnextstart %1").arg(db.getDbNativeTypeName<int>())},
         {QString("UPDATE repopulatechildrenonnextstart integer=1")},
         // Drop and re-create children tables with new UNIQUE requirement
         {QString("DROP TABLE   equipment_children")},
         {QString("CREATE TABLE equipment_children (id %1 %2, "
                                                  "child_id %1, "
                                                  "parent_id %1, "
                                                  "FOREIGN KEY(child_id) REFERENCES equipment(id), "
                                                  "FOREIGN KEY(parent_id) REFERENCES equipment(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())},
         {QString("DROP TABLE   fermentable_children")},
         {QString("CREATE TABLE fermentable_children (id %1 %2, "
                                                    "child_id %1, "
                                                    "parent_id %1, "
                                                    "FOREIGN KEY(child_id) REFERENCES fermentable(id), "
                                                    "FOREIGN KEY(parent_id) REFERENCES fermentable(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())},
         {QString("DROP TABLE   hop_children")},
         {QString("CREATE TABLE hop_children (id %1 %2, "
                                            "child_id %1, "
                                            "parent_id %1, "
                                            "FOREIGN KEY(child_id) REFERENCES hop(id), "
                                            "FOREIGN KEY(parent_id) REFERENCES hop(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())},
         {QString("DROP TABLE   misc_children")},
         {QString("CREATE TABLE misc_children (id %1 %2, "
                                             "child_id %1, "
                                             "parent_id %1, "
                                             "FOREIGN KEY(child_id) REFERENCES misc(id), "
                                             "FOREIGN KEY(parent_id) REFERENCES misc(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())},
         {QString("DROP TABLE   recipe_children")},
         {QString("CREATE TABLE recipe_children (id %1 %2, "
                                               "child_id %1, "
                                               "parent_id %1, "
                                               "FOREIGN KEY(child_id) REFERENCES recipe(id), "
                                               "FOREIGN KEY(parent_id) REFERENCES recipe(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())},
         {QString("DROP TABLE   style_children")},
         {QString("CREATE TABLE style_children (id %1 %2, "
                                              "child_id %1, "
                                              "parent_id %1, "
                                              "FOREIGN KEY(child_id) REFERENCES style(id), "
                                              "FOREIGN KEY(parent_id) REFERENCES style(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())},
         {QString("DROP TABLE   water_children")},
         {QString("CREATE TABLE water_children (id %1 %2, "
                                              "child_id %1, "
                                              "parent_id %1, "
                                              "FOREIGN KEY(child_id) REFERENCES water(id), "
                                              "FOREIGN KEY(parent_id) REFERENCES water(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())},
         {QString("DROP TABLE   yeast_children")},
         {QString("CREATE TABLE yeast_children (id %1 %2, "
                                              "child_id %1, "
                                              "parent_id %1, "
                                              "FOREIGN KEY(child_id) REFERENCES yeast(id), "
                                              "FOREIGN KEY(parent_id) REFERENCES yeast(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())},
         {QString("DROP TABLE   fermentable_in_inventory")},
         {QString("CREATE TABLE fermentable_in_inventory (id %1 %2, "
                                                        "amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>())},
         {QString("DROP TABLE   hop_in_inventory")},
         {QString("CREATE TABLE hop_in_inventory (id %1 %2, "
                                                "amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>())},
         {QString("DROP TABLE   misc_in_inventory")},
         {QString("CREATE TABLE misc_in_inventory (id %1 %2, "
                                                 "amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>())},
         {QString("DROP TABLE   yeast_in_inventory")},
         {QString("CREATE TABLE yeast_in_inventory (id %1 %2, "
                                                  "quanta %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>())},
         {QString("UPDATE settings VALUES(1,2)")}
      };
      return executeSqlQueries(q, migrationQueries);
   }

   bool migrate_to_4(Database & db, QSqlQuery & q) {
      QVector<QueryAndParameters> const migrationQueries{
         // Save old settings
         {QString("ALTER TABLE settings RENAME TO oldsettings")},
         // Create new table with integer version.
         {QString("CREATE TABLE settings (id %1 %2, "
                                        "repopulatechildrenonnextstart %1 DEFAULT 0, "
                                        "version %1 DEFAULT 0);").arg(db.getDbNativeTypeName<int>()).arg(db.getDbNativeIntPrimaryKeyModifier())},
         // Update version to 4, saving other settings
         {QString("INSERT INTO settings (id, version, repopulatechildrenonnextstart) SELECT 1, 4, repopulatechildrenonnextstart FROM oldsettings")},
         // Cleanup
         {QString("DROP TABLE oldsettings")}
      };
      return executeSqlQueries(q, migrationQueries);
   }

   bool migrate_to_5(Database & db, QSqlQuery q) {
      QVector<QueryAndParameters> const migrationQueries{
         // Drop the previous bugged TRIGGER
         {QString("DROP TRIGGER dec_ins_num")},
         // Create the good trigger
            {QString("CREATE TRIGGER dec_ins_num AFTER DELETE ON instruction_in_recipe "
                 "BEGIN "
                    "UPDATE instruction_in_recipe "
                    "SET instruction_number = instruction_number - 1 "
                    "WHERE recipe_id = OLD.recipe_id "
                    "AND instruction_number > OLD.instruction_number; "
                 "END")}
      };
      return executeSqlQueries(q, migrationQueries);
   }

   //
   bool migrate_to_6(Database & db, QSqlQuery q) {
      bool ret = true;
      // I drop this table in version 8. There is no sense doing anything here, and it breaks other things.
      return ret;
   }

   bool migrate_to_7(Database & db, QSqlQuery q) {
      QVector<QueryAndParameters> const migrationQueries{
         // Add "attenuation" to brewnote table
         {"ALTER TABLE brewnote ADD COLUMN attenuation real DEFAULT 0.0"}
      };
      return executeSqlQueries(q, migrationQueries);
   }

   bool migrate_to_8(Database & db, QSqlQuery q) {
      QString createTmpBrewnoteSql;
      QTextStream createTmpBrewnoteSqlStream(&createTmpBrewnoteSql);
      createTmpBrewnoteSqlStream <<
         "CREATE TABLE tmpbrewnote ("
            "id                      " << db.getDbNativeTypeName<int>()     << " " << db.getDbNativeIntPrimaryKeyModifier() << ", "
            "abv                     " << db.getDbNativeTypeName<double>()  << " DEFAULT 0, "
            "attenuation             " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "boil_off                " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "brewdate                " << db.getDbNativeTypeName<QDate>()   << " DEFAULT CURRENT_TIMESTAMP, "
            "brewhouse_eff           " << db.getDbNativeTypeName<double>()  << " DEFAULT 70, "
            "deleted                 " << db.getDbNativeTypeName<bool>()    << " DEFAULT 0, "
            "display                 " << db.getDbNativeTypeName<bool>()    << " DEFAULT 1, "
            "eff_into_bk             " << db.getDbNativeTypeName<double>()  << " DEFAULT 70, "
            "fermentdate             " << db.getDbNativeTypeName<QDate>()   << " DEFAULT CURRENT_TIMESTAMP, "
            "fg                      " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "final_volume            " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "folder                  " << db.getDbNativeTypeName<QString>() << " DEFAULT '', "
            "mash_final_temp         " << db.getDbNativeTypeName<double>()  << " DEFAULT 67, "
            "notes                   " << db.getDbNativeTypeName<QString>() << " DEFAULT '', "
            "og                      " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "pitch_temp              " << db.getDbNativeTypeName<double>()  << " DEFAULT 20, "
            "post_boil_volume        " << db.getDbNativeTypeName<double>()  << " DEFAULT 0, "
            "projected_abv           " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "projected_atten         " << db.getDbNativeTypeName<double>()  << " DEFAULT 75, "
            "projected_boil_grav     " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "projected_eff           " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "projected_ferm_points   " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "projected_fg            " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "projected_mash_fin_temp " << db.getDbNativeTypeName<double>()  << " DEFAULT 67, "
            "projected_og            " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "projected_points        " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "projected_strike_temp   " << db.getDbNativeTypeName<double>()  << " DEFAULT 70, "
            "projected_vol_into_bk   " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "projected_vol_into_ferm " << db.getDbNativeTypeName<double>()  << " DEFAULT 0, "
            "sg                      " << db.getDbNativeTypeName<double>()  << " DEFAULT 1, "
            "strike_temp             " << db.getDbNativeTypeName<double>()  << " DEFAULT 70, "
            "volume_into_bk          " << db.getDbNativeTypeName<double>()  << " DEFAULT 0, "
            "volume_into_fermenter   " << db.getDbNativeTypeName<double>()  << " DEFAULT 0, "
            "recipe_id               " << db.getDbNativeTypeName<int>()     << ", "
            "FOREIGN KEY(recipe_id) REFERENCES recipe(id)"
         ");";
      QVector<QueryAndParameters> const migrationQueries{
         //
         // Drop columns predicted_og and predicted_abv. They are used nowhere I can find and they are breaking things.
         //
         {createTmpBrewnoteSql},
         {QString("INSERT INTO tmpbrewnote ("
                    "id, "
                    "abv, "
                    "attenuation, "
                    "boil_off, "
                    "brewdate, "
                    "brewhouse_eff, "
                    "deleted, "
                    "display, "
                    "eff_into_bk, "
                    "fermentdate, "
                    "fg, "
                    "final_volume, "
                    "folder, "
                    "mash_final_temp, "
                    "notes, "
                    "og, "
                    "pitch_temp, "
                    "post_boil_volume, "
                    "projected_abv, "
                    "projected_atten, "
                    "projected_boil_grav, "
                    "projected_eff, "
                    "projected_ferm_points, "
                    "projected_fg, "
                    "projected_mash_fin_temp, "
                    "projected_og, "
                    "projected_points, "
                    "projected_strike_temp, "
                    "projected_vol_into_bk, "
                    "projected_vol_into_ferm, "
                    "sg, "
                    "strike_temp, "
                    "volume_into_bk, "
                    "volume_into_fermenter, "
                    "recipe_id"
                 ") SELECT id, "
                    "abv, "
                    "attenuation, "
                    "boil_off, "
                    "brewdate, "
                    "brewhouse_eff, "
                    "deleted, "
                    "display, "
                    "eff_into_bk, "
                    "fermentdate, "
                    "fg, "
                    "final_volume, "
                    "folder, "
                    "mash_final_temp, "
                    "notes, "
                    "og, "
                    "pitch_temp, "
                    "post_boil_volume, "
                    "projected_abv, "
                    "projected_atten, "
                    "projected_boil_grav, "
                    "projected_eff, "
                    "projected_ferm_points, "
                    "projected_fg, "
                    "projected_mash_fin_temp, "
                    "projected_og, "
                    "projected_points, "
                    "projected_strike_temp, "
                    "projected_vol_into_bk, "
                    "projected_vol_into_ferm, "
                    "sg, "
                    "strike_temp, "
                    "volume_into_bk, "
                    "volume_into_fermenter, "
                    "recipe_id "
                 "FROM brewnote")},
         {QString("drop table brewnote")},
         {QString("ALTER TABLE tmpbrewnote RENAME TO brewnote")},
         //
         // Rearrange inventory - fermentable
         //
         {QString("ALTER TABLE fermentable ADD COLUMN inventory_id REFERENCES fermentable_in_inventory (id)")},
         // It would seem we have kids with their own rows in the db. This is a freaking mess, but I need to delete those rows
         // before I can do anything else.
         {QString("DELETE FROM fermentable_in_inventory "
                 "WHERE fermentable_in_inventory.id in ( "
                    "SELECT fermentable_in_inventory.id "
                    "FROM fermentable_in_inventory, fermentable_children, fermentable "
                    "WHERE fermentable.id = fermentable_children.child_id "
                    "AND fermentable_in_inventory.fermentable_id = fermentable.id "
                 ")")},
         {QString("INSERT INTO fermentable_in_inventory (fermentable_id) VALUES ( "
                    // Everything has an inventory row now. This will find all the parent items that don't have an inventory row.
                    "SELECT id FROM fermentable WHERE NOT EXISTS ( "
                       "SELECT fermentable_children.id "
                       "FROM fermentable_children "
                       "WHERE fermentable_children.child_id = fermentable.id "
                    ") AND NOT EXISTS ( "
                       "SELECT fermentable_in_inventory.id "
                       "FROM fermentable_in_inventory "
                       "WHERE fermentable_in_inventory.fermentable_id = fermentable.id"
                    ") "
                 ")")},
         // Once we know all parents have inventory rows, we populate inventory_id for them
         {QString("UPDATE fermentable SET inventory_id = ("
                    "SELECT fermentable_in_inventory.id "
                    "FROM fermentable_in_inventory "
                    "WHERE fermentable.id = fermentable_in_inventory.fermentable_id"
                 ")")},
         // Finally, we update all the kids to have the same inventory_id as their dear old paw
         {QString("UPDATE fermentable SET inventory_id = ( "
                    "SELECT tmp.inventory_id "
                    "FROM fermentable tmp, fermentable_children "
                    "WHERE fermentable.id = fermentable_children.child_id "
                    "AND tmp.id = fermentable_children.parent_id"
                 ") "
                 "WHERE inventory_id IS NULL")},
         //
         // Rearrange inventory - hop
         //
         {QString("ALTER TABLE hop ADD COLUMN inventory_id REFERENCES hop_in_inventory (id)")},
         // It would seem we have kids with their own rows in the db. This is a freaking mess, but I need to delete those rows
         // before I can do anything else.
         {QString("DELETE FROM hop_in_inventory "
                 "WHERE hop_in_inventory.id in ( "
                    "SELECT hop_in_inventory.id "
                    "FROM hop_in_inventory, hop_children, hop "
                    "WHERE hop.id = hop_children.child_id "
                    "AND hop_in_inventory.hop_id = hop.id "
                 ")")},
         {QString("INSERT INTO hop_in_inventory (hop_id) VALUES ( "
                    // Everything has an inventory row now. This will find all the parent items that don't have an inventory row.
                    "SELECT id FROM hop WHERE NOT EXISTS ( "
                       "SELECT hop_children.id "
                       "FROM hop_children "
                       "WHERE hop_children.child_id = hop.id "
                    ") AND NOT EXISTS ( "
                       "SELECT hop_in_inventory.id "
                       "FROM hop_in_inventory "
                       "WHERE hop_in_inventory.hop_id = hop.id"
                    ") "
                 ")")},
         // Once we know all parents have inventory rows, we populate inventory_id for them
         {QString("UPDATE hop SET inventory_id = ("
                    "SELECT hop_in_inventory.id "
                    "FROM hop_in_inventory "
                    "WHERE hop.id = hop_in_inventory.hop_id"
                 ")")},
         // Finally, we update all the kids to have the same inventory_id as their dear old paw
         {QString("UPDATE hop SET inventory_id = ( "
                    "SELECT tmp.inventory_id "
                    "FROM hop tmp, hop_children "
                    "WHERE hop.id = hop_children.child_id "
                    "AND tmp.id = hop_children.parent_id"
                 ") "
                 "WHERE inventory_id IS NULL")},
         //
         // Rearrange inventory - misc
         //
         {QString("ALTER TABLE misc ADD COLUMN inventory_id REFERENCES misc_in_inventory (id)")},
         // It would seem we have kids with their own rows in the db. This is a freaking mess, but I need to delete those rows
         // before I can do anything else.
         {QString("DELETE FROM misc_in_inventory "
                 "WHERE misc_in_inventory.id in ( "
                    "SELECT misc_in_inventory.id "
                    "FROM misc_in_inventory, misc_children, misc "
                    "WHERE misc.id = misc_children.child_id "
                    "AND misc_in_inventory.misc_id = misc.id "
                 ")")},
         {QString("INSERT INTO misc_in_inventory (misc_id) VALUES ( "
                    // Everything has an inventory row now. This will find all the parent items that don't have an inventory row.
                    "SELECT id FROM misc WHERE NOT EXISTS ( "
                       "SELECT misc_children.id "
                       "FROM misc_children "
                       "WHERE misc_children.child_id = misc.id "
                    ") AND NOT EXISTS ( "
                       "SELECT misc_in_inventory.id "
                       "FROM misc_in_inventory "
                       "WHERE misc_in_inventory.misc_id = misc.id"
                    ") "
                 ")")},
         // Once we know all parents have inventory rows, we populate inventory_id for them
         {QString("UPDATE misc SET inventory_id = ("
                    "SELECT misc_in_inventory.id "
                    "FROM misc_in_inventory "
                    "WHERE misc.id = misc_in_inventory.misc_id"
                 ")")},
         // Finally, we update all the kids to have the same inventory_id as their dear old paw
         {QString("UPDATE misc SET inventory_id = ( "
                    "SELECT tmp.inventory_id "
                    "FROM misc tmp, misc_children "
                    "WHERE misc.id = misc_children.child_id "
                    "AND tmp.id = misc_children.parent_id"
                 ") "
                 "WHERE inventory_id IS NULL")},
         //
         // Rearrange inventory - yeast
         //
         {QString("ALTER TABLE yeast ADD COLUMN inventory_id REFERENCES yeast_in_inventory (id)")},
         // It would seem we have kids with their own rows in the db. This is a freaking mess, but I need to delete those rows
         // before I can do anything else.
         {QString("DELETE FROM yeast_in_inventory "
                 "WHERE yeast_in_inventory.id in ( "
                    "SELECT yeast_in_inventory.id "
                    "FROM yeast_in_inventory, yeast_children, yeast "
                    "WHERE yeast.id = yeast_children.child_id "
                    "AND yeast_in_inventory.yeast_id = yeast.id "
                 ")")},
         {QString("INSERT INTO yeast_in_inventory (yeast_id) VALUES ( "
                    // Everything has an inventory row now. This will find all the parent items that don't have an inventory row.
                    "SELECT id FROM yeast WHERE NOT EXISTS ( "
                       "SELECT yeast_children.id "
                       "FROM yeast_children "
                       "WHERE yeast_children.child_id = yeast.id "
                    ") AND NOT EXISTS ( "
                       "SELECT yeast_in_inventory.id "
                       "FROM yeast_in_inventory "
                       "WHERE yeast_in_inventory.yeast_id = yeast.id"
                    ") "
                 ")")},
         // Once we know all parents have inventory rows, we populate inventory_id for them
         {QString("UPDATE yeast SET inventory_id = ("
                    "SELECT yeast_in_inventory.id "
                    "FROM yeast_in_inventory "
                    "WHERE yeast.id = yeast_in_inventory.yeast_id"
                 ")")},
         // Finally, we update all the kids to have the same inventory_id as their dear old paw
         {QString("UPDATE yeast SET inventory_id = ( "
                    "SELECT tmp.inventory_id "
                    "FROM yeast tmp, yeast_children "
                    "WHERE yeast.id = yeast_children.child_id "
                    "AND tmp.id = yeast_children.parent_id"
                 ") "
                 "WHERE inventory_id IS NULL")},
         //
         // We need to drop the appropriate columns from the inventory tables
         // Scary, innit? The changes above basically reverse the relation.
         // Instead of inventory knowing about ingredients, we now have ingredients
         // knowing about inventory. I am concerned that leaving these in place
         // will cause circular references
         //
         // Dropping inventory columns - fermentable
         //
         {QString("CREATE TABLE tmpfermentable_in_inventory (id %1 %2, amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>())},
         {QString("INSERT INTO tmpfermentable_in_inventory (id, amount) SELECT id, amount FROM fermentable_in_inventory")},
         {QString("DROP TABLE fermentable_in_inventory")},
         {QString("ALTER TABLE tmpfermentable_in_inventory RENAME TO fermentable_in_inventory")},
         //
         // Dropping inventory columns - hop
         //
         {QString("CREATE TABLE tmphop_in_inventory (id %1 %2, amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>())},
         {QString("INSERT INTO tmphop_in_inventory (id, amount) SELECT id, amount FROM hop_in_inventory")},
         {QString("DROP TABLE hop_in_inventory")},
         {QString("ALTER TABLE tmphop_in_inventory RENAME TO hop_in_inventory")},
         //
         // Dropping inventory columns - misc
         //
         {QString("CREATE TABLE tmpmisc_in_inventory (id %1 %2, amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>())},
         {QString("INSERT INTO tmpmisc_in_inventory (id, amount) SELECT id, amount FROM misc_in_inventory")},
         {QString("DROP TABLE misc_in_inventory")},
         {QString("ALTER TABLE tmpmisc_in_inventory RENAME TO misc_in_inventory")},
         //
         // Dropping inventory columns - yeast
         //
         {QString("CREATE TABLE tmpyeast_in_inventory (id %1 %2, amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>())},
         {QString("INSERT INTO tmpyeast_in_inventory (id, amount) SELECT id, amount FROM yeast_in_inventory")},
         {QString("DROP TABLE yeast_in_inventory")},
         {QString("ALTER TABLE tmpyeast_in_inventory RENAME TO yeast_in_inventory")},
         //
         // Finally, the btalltables table isn't needed, so drop it
         //
         {QString("DROP TABLE IF EXISTS bt_alltables")}
      };
      return executeSqlQueries(q, migrationQueries);
   }

   // To support the water chemistry, I need to add two columns to water and to
   // create the salt and salt_in_recipe tables
   bool migrate_to_9(Database & db, QSqlQuery q) {
      QString createSaltSql;
      QTextStream createSaltSqlStream(&createSaltSql);
      createSaltSqlStream <<
         "CREATE TABLE salt ( "
            "id               " << db.getDbNativeTypeName<int>()     << " " << db.getDbNativeIntPrimaryKeyModifier() << ", "
            "addTo            " << db.getDbNativeTypeName<int>()     << "          DEFAULT 0, "
            "amount           " << db.getDbNativeTypeName<double>()  << "          DEFAULT 0, "
            "amount_is_weight " << db.getDbNativeTypeName<bool>()    << "          DEFAULT 1, "
            "deleted          " << db.getDbNativeTypeName<bool>()    << "          DEFAULT 0, "
            "display          " << db.getDbNativeTypeName<bool>()    << "          DEFAULT 1, "
            "folder           " << db.getDbNativeTypeName<QString>() << "          DEFAULT '', "
            "is_acid          " << db.getDbNativeTypeName<bool>()    << "          DEFAULT 0, "
            "name             " << db.getDbNativeTypeName<QString>() << " not null DEFAULT '', "
            "percent_acid     " << db.getDbNativeTypeName<double>()  << "          DEFAULT 0, "
            "stype            " << db.getDbNativeTypeName<int>()     << "          DEFAULT 0, "
            "misc_id          " << db.getDbNativeTypeName<int>()     << ", "
            "FOREIGN KEY(misc_id) REFERENCES misc(id)"
         ");";
      QVector<QueryAndParameters> const migrationQueries{
         {QString("ALTER TABLE water ADD COLUMN wtype      %1 DEFAULT    0").arg(db.getDbNativeTypeName<int>())},
         {QString("ALTER TABLE water ADD COLUMN alkalinity %1 DEFAULT    0").arg(db.getDbNativeTypeName<double>())},
         {QString("ALTER TABLE water ADD COLUMN as_hco3    %1 DEFAULT true").arg(db.getDbNativeTypeName<bool>())},
         {QString("ALTER TABLE water ADD COLUMN sparge_ro  %1 DEFAULT    0").arg(db.getDbNativeTypeName<double>())},
         {QString("ALTER TABLE water ADD COLUMN mash_ro    %1 DEFAULT    0").arg(db.getDbNativeTypeName<double>())},
         {createSaltSql},
         {QString("CREATE TABLE salt_in_recipe ( "
                    "id        %1 %2, "
                    "recipe_id %1, "
                    "salt_id   %1, "
                    "FOREIGN KEY(recipe_id) REFERENCES recipe(id), "
                    "FOREIGN KEY(salt_id)   REFERENCES salt(id)"
                 ");").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())}
      };
      return executeSqlQueries(q, migrationQueries);
   }

   bool migrate_to_10(Database & db, QSqlQuery q) {
      QVector<QueryAndParameters> const migrationQueries{
         {QString("ALTER TABLE recipe ADD COLUMN ancestor_id %1 REFERENCES recipe(id)").arg(db.getDbNativeTypeName<int>())},
         {QString("ALTER TABLE recipe ADD COLUMN locked %1").arg(db.getDbNativeTypeName<bool>())},
         {QString("UPDATE recipe SET locked = ?"), {QVariant{false}}},
         // By default a Recipe is its own ancestor.  So, we need to set ancestor_id = id where display = true and ancestor_id is null
         {QString("UPDATE recipe SET ancestor_id = id WHERE display = ? and ancestor_id IS NULL"), {QVariant{true}}}
      };
      return executeSqlQueries(q, migrationQueries);
   }

   /*!
    * \brief Migrate from version \c oldVersion to \c oldVersion+1
    */
   bool migrateNext(Database & database, int oldVersion, QSqlDatabase db ) {
      qDebug() << Q_FUNC_INFO << "Migrating DB schema from v" << oldVersion << "to v" << oldVersion + 1;
      QSqlQuery sqlQuery(db);
      bool ret = true;

      // NOTE: Add a new case when adding a new schema change
      switch(oldVersion)
      {
         case 1: // == '2.0.0'
            ret &= migrate_to_202(database, sqlQuery);
            break;
         case 2: // == '2.0.2'
            ret &= migrate_to_210(database, sqlQuery);
            break;
         case 3: // == '2.1.0'
            ret &= migrate_to_4(database, sqlQuery);
            break;
         case 4:
            ret &= migrate_to_5(database, sqlQuery);
            break;
         case 5:
            ret &= migrate_to_6(database, sqlQuery);
            break;
         case 6:
            ret &= migrate_to_7(database, sqlQuery);
            break;
         case 7:
            ret &= migrate_to_8(database, sqlQuery);
            break;
         case 8:
            ret &= migrate_to_9(database, sqlQuery);
            break;
         case 9:
            ret &= migrate_to_10(database, sqlQuery);
            break;
         default:
            qCritical() << QString("Unknown version %1").arg(oldVersion);
            return false;
      }

      // Set the db version
      if( oldVersion > 3 )
      {
         QString queryString{"UPDATE settings SET version=:version WHERE id=1"};
         sqlQuery.prepare(queryString);
         QVariant bindValue{QString::number(oldVersion + 1)};
         sqlQuery.bindValue(":version", bindValue);
         ret &= sqlQuery.exec();
      }

      return ret;
   }

}
bool DatabaseSchemaHelper::upgrade = false;
// Default namespace hides functions from everything outside this file.

bool DatabaseSchemaHelper::create(Database & database, QSqlDatabase connection) {
   //--------------------------------------------------------------------------
   // NOTE: if you edit this function, increment dbVersion and edit
   // migrateNext() appropriately.
   //--------------------------------------------------------------------------

   // NOTE: none of the BeerXML property names should EVER change. This is to
   //       ensure backwards compatibility when rolling out ingredient updates to
   //       old versions.

   // NOTE: deleted=1 means the ingredient is "deleted" and should not be shown in
   //                 any list.
   //       deleted=0 means it isn't deleted and may or may not be shown.
   //       display=1 means the ingredient should be shown in a list, available to
   //                 be put into a recipe.
   //       display=0 means the ingredient is in a recipe already and should not
   //                 be shown in a list, available to be put into a recipe.

   // Start transaction
   // By the magic of RAII, this will abort if we exit this function (including by throwing an exception) without
   // having called dbTransaction.commit().
   DbTransaction dbTransaction{database, connection};

   // .:TODO-DATABASE:. Need to look at the default data population stuff below ALSO move repopulate stuff out of Database class into this one
   bool ret = true;
   qDebug() << Q_FUNC_INFO;
   ret &= CreateAllDatabaseTables(database, connection);

   // Create the settings table manually, since it's only used in this file
   QVector<QueryAndParameters> const setUpQueries{
      {QString("CREATE TABLE settings (id %1 %2, repopulatechildrenonnextstart %1, version %1)").arg(database.getDbNativeTypeName<int>(), database.getDbNativeIntPrimaryKeyModifier())},
      {QString("INSERT INTO settings (repopulatechildrenonnextstart, version) VALUES (?, ?)"), {QVariant(true), QVariant(dbVersion)}}

   };
   QSqlQuery sqlQuery{connection};

   ret &= executeSqlQueries(sqlQuery, setUpQueries);

   // If everything went well, we can commit the DB transaction now, otherwise it will abort when this function returns
   if (ret) {
      dbTransaction.commit();
   }

   return ret;
}

bool DatabaseSchemaHelper::migrate(Database & database, int oldVersion, int newVersion, QSqlDatabase connection) {
   if( oldVersion >= newVersion || newVersion > dbVersion ) {
      qDebug() << Q_FUNC_INFO <<
         QString("Requested backwards migration from %1 to %2: You are an imbecile").arg(oldVersion).arg(newVersion);
      return false;
   }

   bool ret = true;
   qDebug() << Q_FUNC_INFO << "Migrating database schema from v" << oldVersion << "to v" << newVersion;

   // Start transaction
   // By the magic of RAII, this will abort if we exit this function (including by throwing an exception) without
   // having called dbTransaction.commit().  (It will also turn foreign keys back on either way -- whether the
   // transaction is committed or rolled back.)
   DbTransaction dbTransaction{database, connection, DbTransaction::DISABLE_FOREIGN_KEYS};

   for ( ; oldVersion < newVersion && ret; ++oldVersion ) {
      ret &= migrateNext(database, oldVersion, connection);
   }

   // If all statements executed OK, we can commit, otherwise the transaction will roll back when we exit this function
   if (ret) {
      ret &= dbTransaction.commit();
   }

   return ret;
}

int DatabaseSchemaHelper::currentVersion(QSqlDatabase db) {
   // Version was a string field in early versions of the code and then became an integer field
   // We'll read it into a QVariant and then work out whether it's a string or an integer
   QSqlQuery q("SELECT version FROM settings WHERE id=1", db);
   QVariant ver;
   if( q.next() ) {
      ver = q.value("version");
   } else {
      // No settings table in version 2.0.0
      ver = QString("2.0.0");
   }

   // Get the string before we kill it by convert()-ing
   QString stringVer( ver.toString() );
   qDebug() << Q_FUNC_INFO << "Database schema version" << stringVer;

   // Initially, versioning was done with strings, so we need to convert
   // the old version strings to integer versions
   if ( ver.convert(QVariant::Int) ) {
      return ver.toInt();
   }

   if( stringVer == "2.0.0" ) {
      return 1;
   }

   if( stringVer == "2.0.2" ) {
      return 2;
   }

   if( stringVer == "2.1.0" ) {
      return 3;
   }

   qCritical() << "Could not find database version";
   return -1;
}

void DatabaseSchemaHelper::copyDatabase(Database::DbType oldType, Database::DbType newType, QSqlDatabase connectionNew) {
   Database & oldDatabase = Database::instance(oldType);
   Database & newDatabase = Database::instance(newType);

   // this is to prevent us from over-writing or doing heavens knows what to an existing db
   if( connectionNew.tables().contains(QLatin1String("settings")) ) {
      qWarning() << Q_FUNC_INFO << "It appears the database is already configured.";
      return;
   }

   // The crucial bit is creating the new tables in the new DB.  Once that is done then, assuming disabling of foreign
   // keys works OK, it should be turn-the-handle to run through all the tables and copy each record from old DB to new
   // one.
   if (!CreateAllDatabaseTables(newDatabase, connectionNew)) {
      qCritical() << Q_FUNC_INFO << "Error creating tables in new DB";
      return;
   }

   //
   // Start transaction
   // By the magic of RAII, this will abort if we exit this function (including by throwing an exception) without
   // having called dbTransaction.commit().  (It will also turn foreign keys back on either way -- whether the
   // transaction is committed or rolled back.)
   //
   DbTransaction dbTransaction{newDatabase, connectionNew, DbTransaction::DISABLE_FOREIGN_KEYS};

   QSqlDatabase connectionOld = oldDatabase.sqlDatabase();
   QSqlQuery readOld(connectionOld);
   QSqlQuery upsertNew(connectionNew); // we will prepare this in a bit

   QVector<ObjectStore const *> objectStores = GetAllObjectStores();
   for (ObjectStore const * objectStore : objectStores) {
      QList<QString> tableNames = objectStore->getAllTableNames();
      for (QString tableName : tableNames) {
         QString findAllQuery = QString("SELECT * FROM %1").arg(tableName);
         qDebug() << Q_FUNC_INFO << "FIND ALL:" << findAllQuery;
         if (! readOld.exec(findAllQuery) ) {
            qCritical() << Q_FUNC_INFO << "Error reading record from DB with SQL" << readOld.lastQuery() << ":" << readOld.lastError().text();
            return;
         }

         //
         // We do SELECT * on the old DB table and then look at the records that come back to work out what the INSERT
         // into the new DB table should look like.  Of course, we're assuming that there aren't any secret extra
         // fields on the old DB table, otherwise things will break.  But, all being well, this saves a lot of special-
         // case code either inside ObjectStore or messing with its internal data structures.
         //
         bool upsertQueryCreated{false};
         QString fieldNames;
         QTextStream fieldNamesAsStream{&fieldNames};
         QString bindNames;
         QTextStream bindNamesAsStream{&bindNames};
         QString upsertQuery{"INSERT INTO "};
         QTextStream upsertQueryAsStream{&upsertQuery};

         // Start reading the records from the old db
         while (readOld.next()) {
            QSqlRecord here = readOld.record();
            if (!upsertQueryCreated) {
               // Loop through all the fields in the record.  Order shouldn't matter.
               for (int ii = 0; ii < here.count(); ++ii) {
                  QSqlField field = here.field(ii);
                  if (ii != 0) {
                     fieldNamesAsStream << ", ";
                     bindNamesAsStream << ", ";
                  }
                  fieldNamesAsStream << field.name();
                  bindNamesAsStream << ":" << field.name();
               }
               upsertQueryAsStream << tableName << " (" << fieldNames << ") VALUES (" << bindNames << ");";
               upsertNew.prepare(upsertQuery);
               upsertQueryCreated = true;
            }

            for (int ii = 0; ii < here.count(); ++ii) {
               QSqlField field = here.field(ii);
               QString bindName = QString(":%1").arg(field.name());
               QVariant bindValue = here.value(field.name());
               //
               // QVariant should handle all the problems of different types for us here.  Eg, in SQLite, there is no
               // native bool type, so we'll get back 0 or 1 on a field we store bools in, but this should still
               // convert to the right thing in, say, PostgreSQL, when we try to insert it into a field of type
               // BOOLEAN.
               //
               upsertNew.bindValue(bindName, bindValue);
            }

            if (!upsertNew.exec()) {
               qCritical() <<
                  Q_FUNC_INFO << "Error writing record to DB with SQL" << upsertNew.lastQuery() << ":" <<
                  upsertNew.lastError().text();
               return;
            }
         }
      }
   }

   dbTransaction.commit();
   return;
}

namespace {
   // updateDatabase is ugly enough. This takes 20-ish lines out of it that do
   // not really enhance understanding
   void bindForUpdateDatabase(TableSchema* tbl, QSqlQuery qry, QSqlRecord rec) {
      foreach( QString prop, tbl->allProperties() ) {
         // we need to specify the database here. The default database might be
         // postgres, but the new ingredients are always shipped in sqlite
         QString col = tbl->propertyToColumn(prop, Database::SQLITE);
         QVariant bindVal;

         // deleted is always false, but spell 'false' properly for
         // the database
         if ( prop == PropertyNames::NamedEntity::deleted ) {
            bindVal = QVariant(false);
         }
         // boolean values suck, so make sure we spell them properly
         else if ( tbl->propertyColumnType(prop) == "boolean" ) {
            // makes the lines short enough
            bindVal = QVariant(rec.value(col).toBool());
         }
         // otherwise, just grab the value
         else {
            bindVal = rec.value(col);
         }
         // and bind it.
         qry.bindValue(QString(":%1").arg(prop), bindVal);
      }
   }
}

/*******
 *
 * I will be using hop as my example, because it is easy to type.  You should
 * be able to substitute any of the base tables and it will work the same.
 *
 * We maintain a table named bt_hop. The bt_hop table has two columns: id and
 * hop_id. id is the standard autosequence we use. hop_id is the id of a row
 * in the hop table for a hop that we shipped. In the default database, the
 * two values will almost always be equal. In all databases, hop_id will point
 * to a parent hop.
 *
 * When a new hop is added to the default-db.sqlite, a new row has to be
 * inserted into bt_hop pointing to the new hop.
 *
 * When the user gets the dialog saying "There are new ingredients, would you
 * like to merge?", updateDatabase() is called and it works like this:
 *     1. We get all the rows from bt_hop from default_db.sqlite
 *     2. We search for each bt.id in the user's database.
 *     3. If we do not find the bt.id, it means the hop is new to the user and
 *        we need to add it to their database.
 *     4. We do the necessary binding and inserting to add the new hop to the
 *        user's database
 *     5. We put a new entry in the user's bt_hop table, pointing to the
 *        record we just added.
 *     6. Repeat steps 3 - 5 until we run out of rows.
 *
 * It is really important that we DO NOTHING if the user already has the hop.
 * We should NEVER over write user data without explicit permission. I have no
 * interest in working up a diff mechanism, a display mechanism, etc. to show
 * the user what would be done. For now, then, we don't over write any
 * existing records.
 *
 * A few other notes. Any use of TableSchema on the default_db.sqlite must
 * specify the database type as SQLite. We cannot be sure the user's database
 * is SQLite. There's no real difference yet, but I am considering tackling
 * mysql again.
 */
void DatabaseSchemaHelper::updateDatabase(Database & database, QString const& filename) {
   // In the naming here "old" means the user's database, and
   // "new" means the database coming from 'filename'.
   QVariant btid, newid, oldid;
   /*
    *  TODO-DATABASE:
   ¥¥¥¥ ADD FOLDER FIELD
   ¥¥¥¥ GET DATA TO COME FROM XML
    *
    *
    */

   // .:TODO:. Change this to read in BeerXML and to put NEW recipes in brewtarget folder (but how?)
   // .:TODO:. Change where we test equality of newly read-in things from BeerXML

   // Start transaction
   // By the magic of RAII, this will abort if we exit this function (including by throwing an exception) without
   // having called dbTransaction.commit().
   QSqlDatabase connectionOld = database.sqlDatabase();
   DbTransaction dbTransaction{database, connectionOld};

   try {
      // connect to the new database
      QString newCon("newSqldbCon");
      QSqlDatabase newSqldb = QSqlDatabase::addDatabase("QSQLITE", newCon);
      newSqldb.setDatabaseName(filename);
      if( ! newSqldb.open() ) {
         QMessageBox::critical(nullptr,
                              QObject::tr("Database Failure"),
                              QString(QObject::tr("Failed to open the database '%1'.").arg(filename)));
         throw QString("Could not open %1 for reading.\n%2").arg(filename).arg(newSqldb.lastError().text());
      }

      // For each (id, hop_id) in newSqldb.bt_hop...

      // SELECT * FROM newSqldb.hop WHERE id=<hop_id>

      // INSERT INTO hop SET name=:name, alpha=:alpha,... WHERE id=(SELECT hop_id FROM bt_hop WHERE id=:bt_id)

      // Bind :bt_id from <id>
      // Bind :name, :alpha, ..., from newRecord.

      // Execute.
      DatabaseSchema & dbDefn = database.getDatabaseSchema();
      for ( TableSchema* tbl : dbDefn.baseTables() )
      {
         TableSchema* btTbl = dbDefn.btTable(tbl->dbTable());
         // skip any table that doesn't have a bt_ table
         if ( btTbl == nullptr ) {
            continue;
         }

         // build and prepare all the queries once per table.

         // get the new hop referenced by bt_hop.hop_id
         QSqlQuery qNewIng(newSqldb);
         QString   newIngString = QString("SELECT * FROM %1 WHERE %2=:id")
                                    .arg(tbl->tableName())
                                    .arg(tbl->keyName(Database::SQLITE));
         qNewIng.prepare(newIngString);
         qDebug() << Q_FUNC_INFO << newIngString;

         // get the same row from the old bt_hop.
         QSqlQuery qOldBtIng(connectionOld);
         QString   oldBtIngString = QString("SELECT * FROM %1 WHERE %2=:btid")
                                    .arg(btTbl->tableName())
                                    .arg(btTbl->keyName());
         qOldBtIng.prepare(oldBtIngString);
         qDebug() << Q_FUNC_INFO << oldBtIngString;

         // insert the new bt_hop row into the old database.
         QSqlQuery qOldBtIngInsert(connectionOld);
         QString   oldBtIngInsert = QString("INSERT INTO %1 (%2,%3) values (:id,:%3)")
                                          .arg(btTbl->tableName())
                                          .arg(btTbl->keyName())
                                          .arg(btTbl->childIndexName());
         qOldBtIngInsert.prepare(oldBtIngInsert);
         qDebug() << Q_FUNC_INFO << oldBtIngInsert;

         // Create in insert statement for new records. We will bind this
         // later
         QSqlQuery qInsertOldIng(connectionOld);
         QString   insertString = tbl->generateInsertProperties();
         qInsertOldIng.prepare(insertString);
         qDebug() << Q_FUNC_INFO << insertString;

         // get the bt_hop rows from the new database
         QSqlQuery qNewBtIng(newSqldb);
         QString   newBtIngString = QString("SELECT * FROM %1").arg(btTbl->tableName());
         qDebug() << Q_FUNC_INFO << newBtIngString;

         if ( ! qNewBtIng.exec(newBtIngString) ) {
            throw QString("Could not find btID (%1): %2 %3")
                     .arg(btid.toInt())
                     .arg(qNewBtIng.lastQuery())
                     .arg(qNewBtIng.lastError().text());
         }

         // start processing the ingredients from the new db
         while ( qNewBtIng.next() ) {

            // get the bt.id and bt.hop_id. Note we specify the db type here
            btid  = qNewBtIng.record().value(btTbl->keyName(Database::SQLITE));
            newid = qNewBtIng.record().value(btTbl->childIndexName(Database::SQLITE));

            // bind the id to find the hop in the new db
            qNewIng.bindValue(":id", newid);

            // if we can't execute the search
            if ( ! qNewIng.exec() ) {
               throw QString("Could not retrieve new ingredient: %1 %2")
                        .arg(qNewIng.lastQuery())
                        .arg(qNewIng.lastError().text());
            }

            // if we can't read/find the hop
            if ( ! qNewIng.next() ) {
               throw QString("Could not advance query: %1 %2")
                        .arg(qNewIng.lastQuery())
                        .arg(qNewIng.lastError().text());
            }

            // Find the bt_hop record in the old database.
            qOldBtIng.bindValue( ":btid", btid );
            if ( ! qOldBtIng.exec() ) {
               throw QString("Could not find btID (%1): %2 %3")
                        .arg(btid.toInt())
                        .arg(qOldBtIng.lastQuery())
                        .arg(qOldBtIng.lastError().text());
            }

            // If the new bt_hop.id isn't in the old bt_hop
            if( ! qOldBtIng.next() ) {
               // bind the values from the new hop to the insert query
               bindForUpdateDatabase(tbl,qInsertOldIng,qNewIng.record());
               // execute the insert
               if ( ! qInsertOldIng.exec() ) {
                  throw QString("Could not insert new btID (%1): %2 %3")
                           .arg(oldid.toInt())
                           .arg(qInsertOldIng.lastQuery())
                           .arg(qInsertOldIng.lastError().text());
               }

               // get the id from the last insert
               oldid = qInsertOldIng.lastInsertId().toInt();

               // Insert an entry into the old bt_hop table.
               qOldBtIngInsert.bindValue( ":id", btid);
               qOldBtIngInsert.bindValue( QString(":%1").arg(btTbl->childIndexName()), oldid);

               if ( ! qOldBtIngInsert.exec() ) {
                  throw QString("Could not insert btID (%1): %2 %3")
                           .arg(btid.toInt())
                           .arg(qOldBtIngInsert.lastQuery())
                           .arg(qOldBtIngInsert.lastError().text());
               }
            }
         }
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      abort();
   }

   // If we made it this far, everything was OK and we can commit the transaction
   dbTransaction.commit();
   return;
}
