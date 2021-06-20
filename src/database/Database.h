/**
 * database/Database.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Aidan Roberts <aidanr67@gmail.com>
 *   • A.J. Drobnich <aj.drobnich@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Dan Cavanagh <dan@dancavanagh.com>
 *   • Jonatan Pålsson <jonatan.p@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
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
#ifndef DATABASE_H
#define DATABASE_H

#include <memory> // For PImpl

#include <QDebug>
#include <QMap>
//#include <QMetaProperty>
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>

#include "Brewken.h"
#include "database/DatabaseSchema.h"
#include "database/TableSchemaConst.h"
#include "database/TableSchema.h"
#include "model/NamedEntity.h"


/*!
 * \class Database
 *
 * \brief Handles connections to the database.
 *
 * This class is a singleton.
 */
class Database : public QObject {
   Q_OBJECT

public:

   //! This should be the ONLY way you get an instance.
   static Database& instance();
   //! Call this to delete the internal instance.
   static void dropInstance();

   /*! \brief Get the right database connection for the calling thread.
    *
    *         Note the following from https://doc.qt.io/qt-5/qsqldatabase.html#database:
    *            "An instance of QSqlDatabase represents [a] connection ... to the database. ... It is highly
    *            recommended that you do not keep a copy of [a] QSqlDatabase [object] around as a member of a class,
    *            as this will prevent the instance from being correctly cleaned up on shutdown."
    *
    *         Moreover, there can be multiple instances of a QSqlDatabase object for a single connection.  (Copying
    *         the object does not create a new connection, it just creates a new object that references the same
    *         underlying connection.)
    *
    *         Per https://doc.qt.io/qt-5/qsqldatabase.html#removeDatabase, ALL QSqlDatabase objects (and QSqlQuery
    *         objects) for a given database connection MUST be destroyed BEFORE the underlying database connection is
    *         removed from Qt's list of database connections (via QSqlDatabase::removeDatabase() static function),
    *         otherwise errors of the form "QSqlDatabasePrivate::removeDatabase: connection ... is still in use, all
    *         queries will cease to work" will be logged followed by messy raw data dumps (ie where binary data is
    *         written to the logs without interpretation).
    *
    *         Thus, all this function does really is (a) generate a thread-specific name for this thread's connection,
    *         (b) have create and register a new connection for this thread if none exists, (c) return a new stack-
    *         allocated QSqlDatabase object for this thread's DB connection.
    *
    *         Callers should not copy the returned QSqlDatabase object nor retain it for longer than is necessary.
    *
    * \return A stack-allocated \c QSqlDatabase object through which this thread's database connection can be accessed.
    */
   QSqlDatabase sqlDatabase() const;

   //! \brief Should be called when we are about to close down.
   void unload();

   //! \brief Create a blank database in the given file
   bool createBlank(QString const& filename);

   static char const * getDefaultBackupFileName();

   //! backs up database to chosen file
   static bool backupToFile(QString newDbFileName);

   //! backs up database to 'dir' in chosen directory
   static bool backupToDir(QString dir, QString filename="");

   //! \brief Reverts database to that of chosen file.
   static bool restoreFromFile(QString newDbFileStr);

   static bool verifyDbConnection(Brewken::DBTypes testDb,
                                  QString const& hostname,
                                  int portnum = 5432,
                                  QString const & schema="public",
                                  QString const & database="brewken",
                                  QString const & username="brewken",
                                  QString const & password="brewken");
   bool loadSuccessful();

   /*
   //! Inserts an new inventory row in the appropriate table
   int newInventory(TableSchema* schema);

   int getInventoryId(TableSchema* tbl, int key );
   void setInventory(NamedEntity* ins, QVariant value, int invKey = 0, bool notify=true );

   //! \returns The entire inventory for a table.
   QMap<int, double> getInventory(const DatabaseConstants::DbTableId table) const;

   QVariant getInventoryAmt(QString col_name, DatabaseConstants::DbTableId table, int key);
*/
public:
   /**
   * \brief  This function is intended to be called by an ingredient that has not already cached its parent's key
   * \return Key of parent ingredient if there is one, 0 otherwise
   */
//   int getParentNamedEntityKey(NamedEntity const & ingredient);

   //! Get the file where this database was loaded from.
//   static QString getDbFileName();

   /*!
    * Updates the Brewken-provided ingredients from the given sqlite
    * database file.
    */
   void updateDatabase(QString const& filename);

   //! \brief Figures out what databases we are copying to and from, opens what
   //   needs opens and then calls the appropriate workhorse to get it done.
   void convertDatabase(QString const& Hostname, QString const& DbName,
                        QString const& Username, QString const& Password,
                        int Portnum, Brewken::DBTypes newType);

   DatabaseSchema & getDatabaseSchema();

signals:
/*   void changed(QMetaProperty prop, QVariant value);
   void newEquipmentSignal(Equipment*);
   void newFermentableSignal(Fermentable*);
   void newHopSignal(Hop*);
   void newMashSignal(Mash*);
   void newMiscSignal(Misc*);
   void newRecipeSignal(Recipe*);
   void newStyleSignal(Style*);
   void newWaterSignal(Water*);
   void newSaltSignal(Salt*);
   void newYeastSignal(Yeast*);
   // This is still experimental. Or at least mental
   void newBrewNoteSignal(BrewNote*);

   void deletedSignal(Equipment*);
   void deletedSignal(Fermentable*);
   void deletedSignal(Hop*);
   void deletedSignal(Instruction*);
   void deletedSignal(Mash*);
   void deletedSignal(Misc*);
   void deletedSignal(Recipe*);
   void deletedSignal(Style*);
   void deletedSignal(Water*);
   void deletedSignal(Salt*);
   void deletedSignal(Yeast*);
   void deletedSignal(BrewNote*);
   void deletedSignal(MashStep*);

   // MashSteps need signals too
   void newMashStepSignal(MashStep*);
*/
   // Sigh
//   void changedInventory(DatabaseConstants::DbTableId,int,QVariant);

private slots:
   //! Load database from file.
   bool load();

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;

   //! Hidden constructor.
   Database();
   //! No copy constructor, as never want anyone, not even our friends, to make copies of a singleton
   Database(Database const&) = delete;
   //! No assignment operator , as never want anyone, not even our friends, to make copies of a singleton.
   Database& operator=(Database const&) = delete;
   //! Destructor hidden.
   ~Database();


};

#endif
