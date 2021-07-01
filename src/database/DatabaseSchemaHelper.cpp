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
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QVariant>

#include "Brewken.h"
#include "database/Database.h"
#include "database/BrewNoteSchema.h"
#include "database/ObjectStoreWrapper.h"
#include "database/SettingsSchema.h"
#include "database/TableSchemaConst.h"
#include "database/TableSchema.h"
#include "database/WaterSchema.h"
#include "model/BrewNote.h"
#include "model/Water.h"

int const DatabaseSchemaHelper::dbVersion = 9;

namespace {
   // Commands and keywords
   QString const CREATETABLE("CREATE TABLE");
   QString const ALTERTABLE("ALTER TABLE");
   QString const DROPTABLE("DROP TABLE");
   QString const ADDCOLUMN("ADD COLUMN");
   QString const DROPCOLUMN("DROP COLUMN");
   QString const UPDATE("UPDATE");
   QString const SET("SET");
   QString const INSERTINTO("INSERT INTO");
   QString const DEFAULT("DEFAULT");
   QString const SELECT("SELECT");
   QString const SEP(" ");
   QString const COMMA(",");
   QString const OPENPAREN("(");
   QString const CLOSEPAREN(")");
   QString const END(";");
   QString const UNIQUE("UNIQUE");

   //
   // These migrate_to_Xyz functions are deliberately hard-coded.  Because we're migrating from version N to version
   // N+1, we don't need (or want) to refer to the generated table definitions from some later version of the schema,
   // which may be quite different.
   //

   bool executeMigrationQueries(QSqlQuery & q, QVector<QString> const & migrationQueries) {
      bool ret = true;
      for (auto queryText : migrationQueries) {
         qDebug() << Q_FUNC_INFO << queryText;
         ret &= q.exec(queryText);
         if (!ret) {
            qCritical() <<
               Q_FUNC_INFO << "Error executing database upgrade query " << queryText << ": " << q.lastError().text();
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
      QVector<QString> const migrationQueries{
         QString("ALTER TABLE equipment   ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE fermentable ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE hop         ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE misc        ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE style       ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE yeast       ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE water       ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE mash        ADD COLUMN folder text DEFAULT ''"),
         //QString("ALTER TABLE mashstep ADD COLUMN   DEFAULT ''"),
         QString("ALTER TABLE recipe      ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE brewnote    ADD COLUMN folder text DEFAULT ''"),
         QString("ALTER TABLE instruction ADD COLUMN   DEFAULT ''"),
         QString("ALTER TABLE salt        ADD COLUMN folder text DEFAULT ''"),
         // Put the "Bt:.*" recipes into /brewken folder
         QString("UPDATE recipe   SET folder='/brewken' WHERE name LIKE 'Bt:%'"),
         // Update version to 2.1.0
         QString("UPDATE settings SET version='2.1.0' WHERE id=1"),
         // Used to trigger the code to populate the ingredient inheritance tables
         QString("ALTER TABLE settings ADD COLUMN repopulatechildrenonnextstart %1").arg(db.getDbNativeTypeName<int>()),
         QString("UPDATE repopulatechildrenonnextstart integer=1"),
         // Drop and re-create children tables with new UNIQUE requirement
         QString("DROP TABLE   equipment_children"),
         QString("CREATE TABLE equipment_children (id %1 %2, "
                                                  "child_id %1, "
                                                  "parent_id %1, "
                                                  "FOREIGN KEY(child_id) REFERENCES equipment(id), "
                                                  "FOREIGN KEY(parent_id) REFERENCES equipment(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier()),
         QString("DROP TABLE   fermentable_children"),
         QString("CREATE TABLE fermentable_children (id %1 %2, "
                                                    "child_id %1, "
                                                    "parent_id %1, "
                                                    "FOREIGN KEY(child_id) REFERENCES fermentable(id), "
                                                    "FOREIGN KEY(parent_id) REFERENCES fermentable(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier()),
         QString("DROP TABLE   hop_children"),
         QString("CREATE TABLE hop_children (id %1 %2, "
                                            "child_id %1, "
                                            "parent_id %1, "
                                            "FOREIGN KEY(child_id) REFERENCES hop(id), "
                                            "FOREIGN KEY(parent_id) REFERENCES hop(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier()),
         QString("DROP TABLE   misc_children"),
         QString("CREATE TABLE misc_children (id %1 %2, "
                                             "child_id %1, "
                                             "parent_id %1, "
                                             "FOREIGN KEY(child_id) REFERENCES misc(id), "
                                             "FOREIGN KEY(parent_id) REFERENCES misc(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier()),
         QString("DROP TABLE   recipe_children"),
         QString("CREATE TABLE recipe_children (id %1 %2, "
                                               "child_id %1, "
                                               "parent_id %1, "
                                               "FOREIGN KEY(child_id) REFERENCES recipe(id), "
                                               "FOREIGN KEY(parent_id) REFERENCES recipe(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier()),
         QString("DROP TABLE   style_children"),
         QString("CREATE TABLE style_children (id %1 %2, "
                                              "child_id %1, "
                                              "parent_id %1, "
                                              "FOREIGN KEY(child_id) REFERENCES style(id), "
                                              "FOREIGN KEY(parent_id) REFERENCES style(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier()),
         QString("DROP TABLE   water_children"),
         QString("CREATE TABLE water_children (id %1 %2, "
                                              "child_id %1, "
                                              "parent_id %1, "
                                              "FOREIGN KEY(child_id) REFERENCES water(id), "
                                              "FOREIGN KEY(parent_id) REFERENCES water(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier()),
         QString("DROP TABLE   yeast_children"),
         QString("CREATE TABLE yeast_children (id %1 %2, "
                                              "child_id %1, "
                                              "parent_id %1, "
                                              "FOREIGN KEY(child_id) REFERENCES yeast(id), "
                                              "FOREIGN KEY(parent_id) REFERENCES yeast(id));").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier()),
         QString("DROP TABLE   fermentable_in_inventory"),
         QString("CREATE TABLE fermentable_in_inventory (id %1 %2, "
                                                        "amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>()),
         QString("DROP TABLE   hop_in_inventory"),
         QString("CREATE TABLE hop_in_inventory (id %1 %2, "
                                                "amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>()),
         QString("DROP TABLE   misc_in_inventory"),
         QString("CREATE TABLE misc_in_inventory (id %1 %2, "
                                                 "amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>()),
         QString("DROP TABLE   yeast_in_inventory"),
         QString("CREATE TABLE yeast_in_inventory (id %1 %2, "
                                                  "quanta %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>()),
         QString("UPDATE settings VALUES(1,2)"),
      };
      return executeMigrationQueries(q, migrationQueries);
   }

   bool migrate_to_4(Database & db, QSqlQuery & q) {
      QVector<QString> const migrationQueries{
         // Save old settings
         QString("ALTER TABLE settings RENAME TO oldsettings"),
         // Create new table with integer version.
         QString("CREATE TABLE settings (id %1 %2, "
                                        "repopulatechildrenonnextstart %1 DEFAULT 0, "
                                        "version %1 DEFAULT 0);").arg(db.getDbNativeTypeName<int>()).arg(db.getDbNativeIntPrimaryKeyModifier()),
         // Update version to 4, saving other settings
         QString("INSERT INTO settings (id, version, repopulatechildrenonnextstart) SELECT 1, 4, repopulatechildrenonnextstart FROM oldsettings"),
         // Cleanup
         QString("DROP TABLE oldsettings")
      };
      return executeMigrationQueries(q, migrationQueries);
   }

   bool migrate_to_5(Database & db, QSqlQuery q) {
      QVector<QString> const migrationQueries{
         // Drop the previous bugged TRIGGER
         QString("DROP TRIGGER dec_ins_num"),
         // Create the good trigger
         QString("CREATE TRIGGER dec_ins_num AFTER DELETE ON instruction_in_recipe "
                 "BEGIN "
                    "UPDATE instruction_in_recipe "
                    "SET instruction_number = instruction_number - 1 "
                    "WHERE recipe_id = OLD.recipe_id "
                    "AND instruction_number > OLD.instruction_number; "
                 "END")
      };
      return executeMigrationQueries(q, migrationQueries);
   }

   //
   bool migrate_to_6(Database & db, QSqlQuery q) {
      bool ret = true;
      // I drop this table in version 8. There is no sense doing anything here, and it breaks other things.
      return ret;
   }

   bool migrate_to_7(Database & db, QSqlQuery q) {
      QVector<QString> const migrationQueries{
         // Add "attenuation" to brewnote table
         "ALTER TABLE brewnote ADD COLUMN attenuation real DEFAULT 0.0"
      };
      return executeMigrationQueries(q, migrationQueries);
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
      QVector<QString> const migrationQueries{
         //
         // Drop columns predicted_og and predicted_abv. They are used nowhere I can find and they are breaking things.
         //
         createTmpBrewnoteSql,
         QString("INSERT INTO tmpbrewnote ("
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
                 "FROM brewnote"),
         QString("drop table brewnote"),
         QString("alter table tmpbrewnote rename to brewnote"),
         //
         // Rearrange inventory - fermentable
         //
         QString("ALTER TABLE fermentable ADD COLUMN inventory_id REFERENCES fermentable_in_inventory (id)"),
         // It would seem we have kids with their own rows in the db. This is a freaking mess, but I need to delete those rows
         // before I can do anything else.
         QString("DELETE FROM fermentable_in_inventory "
                 "WHERE fermentable_in_inventory.id in ( "
                    "SELECT fermentable_in_inventory.id "
                    "FROM fermentable_in_inventory, fermentable_children, fermentable "
                    "WHERE fermentable.id = fermentable_children.child_id "
                    "AND fermentable_in_inventory.fermentable_id = fermentable.id "
                 ")"),
         QString("INSERT INTO fermentable_in_inventory (fermentable_id) VALUES ( "
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
                 ")"),
         // Once we know all parents have inventory rows, we populate inventory_id for them
         QString("UPDATE fermentable SET inventory_id = ("
                    "SELECT fermentable_in_inventory.id "
                    "FROM fermentable_in_inventory "
                    "WHERE fermentable.id = fermentable_in_inventory.fermentable_id"
                 ")"),
         // Finally, we update all the kids to have the same inventory_id as their dear old paw
         QString("UPDATE fermentable SET inventory_id = ( "
                    "SELECT tmp.inventory_id "
                    "FROM fermentable tmp, fermentable_children "
                    "WHERE fermentable.id = fermentable_children.child_id "
                    "AND tmp.id = fermentable_children.parent_id"
                 ") "
                 "WHERE inventory_id IS NULL"),
         //
         // Rearrange inventory - hop
         //
         QString("ALTER TABLE hop ADD COLUMN inventory_id REFERENCES hop_in_inventory (id)"),
         // It would seem we have kids with their own rows in the db. This is a freaking mess, but I need to delete those rows
         // before I can do anything else.
         QString("DELETE FROM hop_in_inventory "
                 "WHERE hop_in_inventory.id in ( "
                    "SELECT hop_in_inventory.id "
                    "FROM hop_in_inventory, hop_children, hop "
                    "WHERE hop.id = hop_children.child_id "
                    "AND hop_in_inventory.hop_id = hop.id "
                 ")"),
         QString("INSERT INTO hop_in_inventory (hop_id) VALUES ( "
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
                 ")"),
         // Once we know all parents have inventory rows, we populate inventory_id for them
         QString("UPDATE hop SET inventory_id = ("
                    "SELECT hop_in_inventory.id "
                    "FROM hop_in_inventory "
                    "WHERE hop.id = hop_in_inventory.hop_id"
                 ")"),
         // Finally, we update all the kids to have the same inventory_id as their dear old paw
         QString("UPDATE hop SET inventory_id = ( "
                    "SELECT tmp.inventory_id "
                    "FROM hop tmp, hop_children "
                    "WHERE hop.id = hop_children.child_id "
                    "AND tmp.id = hop_children.parent_id"
                 ") "
                 "WHERE inventory_id IS NULL"),
         //
         // Rearrange inventory - misc
         //
         QString("ALTER TABLE misc ADD COLUMN inventory_id REFERENCES misc_in_inventory (id)"),
         // It would seem we have kids with their own rows in the db. This is a freaking mess, but I need to delete those rows
         // before I can do anything else.
         QString("DELETE FROM misc_in_inventory "
                 "WHERE misc_in_inventory.id in ( "
                    "SELECT misc_in_inventory.id "
                    "FROM misc_in_inventory, misc_children, misc "
                    "WHERE misc.id = misc_children.child_id "
                    "AND misc_in_inventory.misc_id = misc.id "
                 ")"),
         QString("INSERT INTO misc_in_inventory (misc_id) VALUES ( "
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
                 ")"),
         // Once we know all parents have inventory rows, we populate inventory_id for them
         QString("UPDATE misc SET inventory_id = ("
                    "SELECT misc_in_inventory.id "
                    "FROM misc_in_inventory "
                    "WHERE misc.id = misc_in_inventory.misc_id"
                 ")"),
         // Finally, we update all the kids to have the same inventory_id as their dear old paw
         QString("UPDATE misc SET inventory_id = ( "
                    "SELECT tmp.inventory_id "
                    "FROM misc tmp, misc_children "
                    "WHERE misc.id = misc_children.child_id "
                    "AND tmp.id = misc_children.parent_id"
                 ") "
                 "WHERE inventory_id IS NULL"),
         //
         // Rearrange inventory - yeast
         //
         QString("ALTER TABLE yeast ADD COLUMN inventory_id REFERENCES yeast_in_inventory (id)"),
         // It would seem we have kids with their own rows in the db. This is a freaking mess, but I need to delete those rows
         // before I can do anything else.
         QString("DELETE FROM yeast_in_inventory "
                 "WHERE yeast_in_inventory.id in ( "
                    "SELECT yeast_in_inventory.id "
                    "FROM yeast_in_inventory, yeast_children, yeast "
                    "WHERE yeast.id = yeast_children.child_id "
                    "AND yeast_in_inventory.yeast_id = yeast.id "
                 ")"),
         QString("INSERT INTO yeast_in_inventory (yeast_id) VALUES ( "
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
                 ")"),
         // Once we know all parents have inventory rows, we populate inventory_id for them
         QString("UPDATE yeast SET inventory_id = ("
                    "SELECT yeast_in_inventory.id "
                    "FROM yeast_in_inventory "
                    "WHERE yeast.id = yeast_in_inventory.yeast_id"
                 ")"),
         // Finally, we update all the kids to have the same inventory_id as their dear old paw
         QString("UPDATE yeast SET inventory_id = ( "
                    "SELECT tmp.inventory_id "
                    "FROM yeast tmp, yeast_children "
                    "WHERE yeast.id = yeast_children.child_id "
                    "AND tmp.id = yeast_children.parent_id"
                 ") "
                 "WHERE inventory_id IS NULL"),
         //
         // We need to drop the appropriate columns from the inventory tables
         // Scary, innit? The changes above basically reverse the relation.
         // Instead of inventory knowing about ingredients, we now have ingredients
         // knowing about inventory. I am concerned that leaving these in place
         // will cause circular references
         //
         // Dropping inventory columns - fermentable
         //
         QString("CREATE TABLE tmpfermentable_in_inventory (id %1 %2, amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>()),
         QString("INSERT INTO tmpfermentable_in_inventory (id, amount) SELECT id, amount FROM fermentable_in_inventory"),
         QString("DROP TABLE fermentable_in_inventory"),
         QString("alter table tmpfermentable_in_inventory rename to fermentable_in_inventory"),
         //
         // Dropping inventory columns - hop
         //
         QString("CREATE TABLE tmphop_in_inventory (id %1 %2, amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>()),
         QString("INSERT INTO tmphop_in_inventory (id, amount) SELECT id, amount FROM hop_in_inventory"),
         QString("DROP TABLE hop_in_inventory"),
         QString("alter table tmphop_in_inventory rename to hop_in_inventory"),
         //
         // Dropping inventory columns - misc
         //
         QString("CREATE TABLE tmpmisc_in_inventory (id %1 %2, amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>()),
         QString("INSERT INTO tmpmisc_in_inventory (id, amount) SELECT id, amount FROM misc_in_inventory"),
         QString("DROP TABLE misc_in_inventory"),
         QString("alter table tmpmisc_in_inventory rename to misc_in_inventory"),
         //
         // Dropping inventory columns - yeast
         //
         QString("CREATE TABLE tmpyeast_in_inventory (id %1 %2, amount %3 DEFAULT 0);").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier(), db.getDbNativeTypeName<double>()),
         QString("INSERT INTO tmpyeast_in_inventory (id, amount) SELECT id, amount FROM yeast_in_inventory"),
         QString("DROP TABLE yeast_in_inventory"),
         QString("alter table tmpyeast_in_inventory rename to yeast_in_inventory"),
         //
         // Finally, the btalltables table isn't needed, so drop it
         //
         QString("DROP TABLE IF EXISTS bt_alltables")
      };
      return executeMigrationQueries(q, migrationQueries);
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
      QVector<QString> const migrationQueries{
         QString("ALTER TABLE water ADD COLUMN wtype      %1 DEFAULT    0").arg(db.getDbNativeTypeName<int>()),
         QString("ALTER TABLE water ADD COLUMN alkalinity %1 DEFAULT    0").arg(db.getDbNativeTypeName<double>()),
         QString("ALTER TABLE water ADD COLUMN as_hco3    %1 DEFAULT true").arg(db.getDbNativeTypeName<bool>()),
         QString("ALTER TABLE water ADD COLUMN sparge_ro  %1 DEFAULT    0").arg(db.getDbNativeTypeName<double>()),
         QString("ALTER TABLE water ADD COLUMN mash_ro    %1 DEFAULT    0").arg(db.getDbNativeTypeName<double>()),
         createSaltSql,
         QString("CREATE TABLE salt_in_recipe ( "
                    "id        %1 %2, "
                    "recipe_id %1, "
                    "salt_id   %1, "
                    "FOREIGN KEY(recipe_id) REFERENCES recipe(id), "
                    "FOREIGN KEY(salt_id)   REFERENCES salt(id)"
                 ");").arg(db.getDbNativeTypeName<int>(), db.getDbNativeIntPrimaryKeyModifier())
      };
      return executeMigrationQueries(q, migrationQueries);
   }

   /*!
    * \brief Migrate from version \c oldVersion to \c oldVersion+1
    */
   bool migrateNext(Database & database, int oldVersion, QSqlDatabase db ) {
      QSqlQuery q(db);
      bool ret = true;
      DatabaseSchema* defn = new DatabaseSchema();
      TableSchema* tbl = defn->table(DatabaseConstants::SETTINGTABLE);

      // NOTE: use this to debug your migration
   #define CHECKQUERY if(!ret) qDebug() << QString("ERROR: %1\nQUERY: %2").arg(q.lastError().text()).arg(q.lastQuery());

      // NOTE: Add a new case when adding a new schema change
      switch(oldVersion)
      {
         case 1: // == '2.0.0'
            ret &= migrate_to_202(database, q);
            break;
         case 2: // == '2.0.2'
            ret &= migrate_to_210(database, q);
            break;
         case 3: // == '2.1.0'
            ret &= migrate_to_4(database, q);
            break;
         case 4:
            ret &= migrate_to_5(database, q);
            break;
         case 5:
            ret &= migrate_to_6(database, q);
            break;
         case 6:
            ret &= migrate_to_7(database, q);
            break;
         case 7:
            ret &= migrate_to_8(database, q);
            break;
         case 8:
            ret &= migrate_to_9(database, q);
            break;
         default:
            qCritical() << QString("Unknown version %1").arg(oldVersion);
            return false;
      }

      // Set the db version
      if( oldVersion > 3 )
      {
         ret &= q.exec(
            UPDATE + SEP + tbl->tableName() +
            " SET " + tbl->propertyToColumn(kpropSettingsVersion) + "=" + QString::number(oldVersion+1) + " WHERE id=1"
         );
      }

      return ret;
   #undef CHECKQUERY
   }

}
bool DatabaseSchemaHelper::upgrade = false;
// Default namespace hides functions from everything outside this file.

bool DatabaseSchemaHelper::create(Database & database, QSqlDatabase db, DatabaseSchema* defn, Brewken::DBTypes dbType )
{
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
/*
   bool hasTransaction = db.transaction();

   QSqlQuery q(db);
   bool ret = true;

   foreach( TableSchema* table, defn->allTables() ) {
      QString createTable = table->generateCreateTable(dbType);
      if ( ! q.exec(createTable) ) {
         throw QString("Could not create %1 : %2")
               .arg(table->tableName())
               .arg(q.lastError().text());
      }
      // We need to create the increment and decrement things for the instructions_in_recipe table.
      if ( table->dbTable() == DatabaseConstants::INSTINRECTABLE ) {
         q.exec(table->generateIncrementTrigger(dbType));
         q.exec(table->generateDecrementTrigger(dbType));
      }

   }
   TableSchema* settings = defn->table(DatabaseConstants::SETTINGTABLE);

   // since we create from scratch, it will always be at the most
   // recent version
   QString insSetting = QString("INSERT INTO settings (%1,%2) values (%3,%4)")
         .arg( settings->propertyToColumn(kpropSettingsRepopulate))
         .arg(settings->propertyToColumn(kpropSettingsVersion))
         .arg(Brewken::dbTrue())
         .arg(dbVersion);

   if ( !q.exec(insSetting) ) {
      throw QString("Could not insert into %1 : %2 (%3)")
               .arg(settings->tableName())
               .arg(q.lastError().text())
               .arg(q.lastQuery());
   }
   // Commit transaction
   if( hasTransaction )
      ret = db.commit();

   if ( ! ret ) {
      qCritical() << "db.commit() failed";
   }
*/

   qDebug() << Q_FUNC_INFO;
   return CreateAllDatabaseTables(database);
}

bool DatabaseSchemaHelper::migrate(Database & database, int oldVersion, int newVersion, QSqlDatabase db)
{
   if( oldVersion >= newVersion || newVersion > dbVersion )
   {
      qDebug() << Q_FUNC_INFO << QString("Requesed backwards migration from %1 to %2: You are an imbecile").arg(oldVersion).arg(newVersion);
      return false;
   }

   bool ret = true;

   // turning the foreign_keys on or off can only happen *outside* a
   // transaction boundary. Eeewwww.
   if ( Brewken::dbType() == Brewken::SQLITE ) {
      db.exec("PRAGMA foreign_keys=off");
   }

   // Start a transaction
   db.transaction();
   for( ; oldVersion < newVersion && ret; ++oldVersion )
      ret &= migrateNext(database, oldVersion, db);

   if ( Brewken::dbType() == Brewken::SQLITE ) {
      db.exec("PRAGMA foreign_keys=on");
   }
   // If any statement failed to execute, rollback database to last good state.
   if( ret )
      ret &= db.commit();
   else
   {
      qCritical() << "Rolling back";
      db.rollback();
   }

   return ret;
}

int DatabaseSchemaHelper::currentVersion(QSqlDatabase db)
{
   QVariant ver;
   TableSchema* tbl = new TableSchema(DatabaseConstants::SETTINGTABLE);
   QSqlQuery q(
      SELECT + SEP + tbl->propertyToColumn(kpropSettingsVersion) + " FROM " + tbl->tableName() + " WHERE id=1",
      db
   );

   // No settings table in version 2.0.0
   if( q.next() )
   {
      int field = q.record().indexOf(tbl->propertyToColumn(kpropSettingsVersion));
      ver = q.value(field);
   }
   else
      ver = QString("2.0.0");

   // Get the string before we kill it by convert()-ing
   QString stringVer( ver.toString() );

   // Initially, versioning was done with strings, so we need to convert
   // the old version strings to integer versions
   if( ver.convert(QVariant::Int) )
      return ver.toInt();
   else
   {
      if( stringVer == "2.0.0" )
         return 1;
      else if( stringVer == "2.0.2" )
         return 2;
      else if( stringVer == "2.1.0" )
         return 3;
   }

   qCritical() << "Could not find database version";
   return -1;
}
