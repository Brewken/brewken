/**
 * database/Database.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Aidan Roberts <aidanr67@gmail.com>
 *   • A.J. Drobnich <aj.drobnich@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Chris Pavetto <chrispavetto@gmail.com>
 *   • Chris Speck <cgspeck@gmail.com>
 *   • Dan Cavanagh <dan@dancavanagh.com>
 *   • David Grundberg <individ@acc.umu.se>
 *   • Greg Greenaae <ggreenaae@gmail.com>
 *   • Jamie Daws <jdelectronics1@gmail.com>
 *   • Jean-Baptiste Wons <wonsjb@gmail.com>
 *   • Jonatan Pålsson <jonatan.p@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Luke Vincent <luke.r.vincent@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Rob Taylor <robtaylor@floopily.org>
 *   • Samuel Östling <MrOstling@gmail.com>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#include "database/Database.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QList>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QString>
#include <QThread>

#include "Brewken.h"
#include "config.h"
#include "database/DatabaseSchemaHelper.h"
#include "model/BrewNote.h"
#include "PersistentSettings.h"
#include "QueuedMethod.h"


//
// .:TODO:. Look at BT fix https://github.com/mikfire/brewtarget/commit/e5a43c1d7babbaf9450a14e5ea1e4589235ded2c
// for incorrect inventory handling when a NE is copied
//

namespace {
   // These are for SQLite databases
   QFile dbFile;
   QString dbFileName;
   QFile dataDbFile;
   QString dataDbFileName;

   // And these are for Postgres databases -- are these really required? Are
   // the sqlite ones really required?
   QString dbHostname;
   int dbPortnum;
   QString dbName;
   QString dbSchema;
   QString dbUsername;
   QString dbPassword;

   //
   // Each thread has its own connection to the database, and each connection has to have a unique name (otherwise,
   // calling QSqlDatabase::addDatabase() with the same name as an existing connection will replace that existing
   // connection with the new one created by that function).  We just create a unique connection name from the thread
   // ID in the same way that we do in the Logging module.
   //
   // We only need to store the name of the connection here.  (See header file comment for Database::sqlDatabase() for
   // more details of why it would be unhelpful to store a QSqlDatabase object in thread-local storage.)
   //
   // Since C++11, we can use thread_local to define thread-specific variables that are initialized "before first use"
   //
   thread_local QString const dbConnectionNameForThisThread {
      QString{"%1"}.arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 36)
   };

   //! \brief converts sqlite values (mostly booleans) into something postgres wants
   QVariant convertValue(Brewken::DBTypes newType, QSqlField field) {
      QVariant retVar = field.value();
      if ( field.type() == QVariant::Bool ) {
         switch(newType) {
            case Brewken::PGSQL:
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


   // May St. Stevens intercede on my behalf.
   //
   //! \brief opens an SQLite db for transfer
   QSqlDatabase openSQLite() {
      QString filePath = PersistentSettings::getUserDataDir().filePath("database.sqlite");
      QSqlDatabase newDb = QSqlDatabase::addDatabase("QSQLITE", "altdb");

      try {
         dbFile.setFileName(dbFileName);

         if ( filePath.isEmpty() )
            throw QString("Could not read the database file(%1)").arg(filePath);

         newDb.setDatabaseName(filePath);

         if (!  newDb.open() )
            throw QString("Could not open %1 : %2").arg(filePath).arg(newDb.lastError().text());
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         throw;
      }

      return newDb;
   }

   //! \brief opens a PostgreSQL db for transfer. I need
   QSqlDatabase openPostgres(QString const& Hostname, QString const& DbName,
                             QString const& Username, QString const& Password,
                             int Portnum) {
      QSqlDatabase newDb = QSqlDatabase::addDatabase("QPSQL", "altdb");

      try {
         newDb.setHostName(Hostname);
         newDb.setDatabaseName(DbName);
         newDb.setUserName(Username);
         newDb.setPort(Portnum);
         newDb.setPassword(Password);

         if ( ! newDb.open() )
            throw QString("Could not open %1 : %2").arg(Hostname).arg(newDb.lastError().text());
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         throw;
      }
      return newDb;
   }

}

//
// This private implementation class holds all private non-virtual members of Database
//
class Database::impl {
public:

   /**
    * Constructor
    */
   impl() : dbDefn{},
            dbConName{},
            loaded{false},
            loadWasSuccessful{false} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   // Don't know where to put this, so it goes here for right now
   bool loadSQLite(Database & database) {
      qDebug() << "Loading SQLITE...";

      // Set file names.
      dbFileName = PersistentSettings::getUserDataDir().filePath("database.sqlite");
      dataDbFileName = Brewken::getResourceDir().filePath("default_db.sqlite");
      qDebug() << Q_FUNC_INFO << QString("dbFileName = \"%1\"\nDatabase::loadSQLite() - dataDbFileName=\"%2\"").arg(dbFileName).arg(dataDbFileName);
      // Set the files.
      dbFile.setFileName(dbFileName);
      dataDbFile.setFileName(dataDbFileName);

      // If user restored the database from a backup, make the backup into the primary.
      {
         QFile newdb(QString("%1.new").arg(dbFileName));
         if( newdb.exists() )
         {
            dbFile.remove();
            newdb.copy(dbFileName);
            QFile::setPermissions( dbFileName, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup );
            newdb.remove();
         }
      }

      // If there's no dbFile, try to copy from dataDbFile.
      if( !dbFile.exists() )
      {
         Brewken::userDatabaseDidNotExist = true;

         // Have to wait until db is open before creating from scratch.
         if( dataDbFile.exists() )
         {
            dataDbFile.copy(dbFileName);
            QFile::setPermissions( dbFileName, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup );
         }

         // Reset the last merge request.
         Brewken::lastDbMergeRequest = QDateTime::currentDateTime();
      }

      // Open SQLite DB
      // It's a coding error if we didn't already establish that SQLite is the type of DB we're talking to, so assert
      // that and then call the generic code to get a connection
      Q_ASSERT(Brewken::dbType() == Brewken::SQLITE);
      QSqlDatabase sqldb = database.sqlDatabase();

      this->dbConName = sqldb.connectionName();
      qDebug() << Q_FUNC_INFO << "dbConName=" << this->dbConName;

      // NOTE: synchronous=off reduces query time by an order of magnitude!
      QSqlQuery pragma(sqldb);
      if ( ! pragma.exec( "PRAGMA synchronous = off" ) ) {
         qCritical() << Q_FUNC_INFO << "Could not disable synchronous writes: " << pragma.lastError().text();
         return false;
      }
      if ( ! pragma.exec( "PRAGMA foreign_keys = on")) {
         qCritical() << Q_FUNC_INFO << "Could not enable foreign keys: " << pragma.lastError().text();
         return false;
      }
      if ( ! pragma.exec( "PRAGMA locking_mode = EXCLUSIVE")) {
         qCritical() << Q_FUNC_INFO << "Could not enable exclusive locks: " << pragma.lastError().text();
         return false;
      }
      if ( ! pragma.exec("PRAGMA temp_store = MEMORY") ) {
         qCritical() << Q_FUNC_INFO << "Could not enable temporary memory: " << pragma.lastError().text();
         return false;
      }

      // older sqlite databases may not have a settings table. I think I will
      // just check to see if anything is in there.
      this->createFromScratch = sqldb.tables().size() == 0;

      return true;
   }

   bool loadPgSQL(Database & database) {

      dbHostname = PersistentSettings::value("dbHostname").toString();
      dbPortnum  = PersistentSettings::value("dbPortnum").toInt();
      dbName     = PersistentSettings::value("dbName").toString();
      dbSchema   = PersistentSettings::value("dbSchema").toString();

      dbUsername = PersistentSettings::value("dbUsername").toString();

      if ( PersistentSettings::contains("dbPassword") ) {
         dbPassword = PersistentSettings::value("dbPassword").toString();
      }
      else {
         bool isOk = false;

         // prompt for the password until we get it? I don't think this is a good
         // idea?
         while ( ! isOk ) {
            dbPassword = QInputDialog::getText(nullptr,tr("Database password"),
                  tr("Password"), QLineEdit::Password,QString(),&isOk);
            if ( isOk ) {
               isOk = verifyDbConnection( Brewken::PGSQL, dbHostname, dbPortnum, dbSchema,
                                    dbName, dbUsername, dbPassword);
            }
         }
      }

      // It's a coding error if we didn't already establish that PostgreSQL is the type of DB we're talking to, so assert
      // that and then call the generic code to get a connection
      Q_ASSERT(Brewken::dbType() == Brewken::PGSQL);
      QSqlDatabase sqldb = database.sqlDatabase();

      this->dbConName = sqldb.connectionName();
      qDebug() << Q_FUNC_INFO << "dbConName=" << this->dbConName;

      // by the time we had pgsql support, there is a settings table
      this->createFromScratch = ! sqldb.tables().contains("settings");

      return true;
   }

   // Returns true if the schema gets updated, false otherwise.
   // If err != 0, set it to true if an error occurs, false otherwise.
   bool updateSchema(Database & database, bool* err = nullptr) {
      int currentVersion = DatabaseSchemaHelper::currentVersion( database.sqlDatabase() );
      int newVersion = DatabaseSchemaHelper::dbVersion;
      bool doUpdate = currentVersion < newVersion;

      if( doUpdate )
      {
         bool success = DatabaseSchemaHelper::migrate( currentVersion, newVersion, database.sqlDatabase() );
         if( !success )
         {
            qCritical() << QString("Database migration %1->%2 failed").arg(currentVersion).arg(newVersion);
            if( err )
               *err = true;
            return false;
         }
      }

      database.sqlDatabase().transaction();

      try {
         //populate ingredient links
         int repopChild = 0;
         QSqlQuery popchildq( "SELECT repopulateChildrenOnNextStart FROM settings WHERE id=1", database.sqlDatabase() );

         if( popchildq.next() )
            repopChild = popchildq.record().value("repopulateChildrenOnNextStart").toInt();
         else
            throw QString("%1 %2").arg(popchildq.lastQuery()).arg(popchildq.lastError().text());

         if(repopChild == 1) {
            qDebug() << Q_FUNC_INFO << "calling populateChildTablesByName()";
            this->populateChildTablesByName(database);

            QSqlQuery popchildq( "UPDATE settings SET repopulateChildrenOnNextStart = 0", database.sqlDatabase() );
            if ( ! popchildq.isActive() )
               throw QString("Could not modify settings table: %1 %2").arg(popchildq.lastQuery()).arg(popchildq.lastError().text());
         }
      }
      catch (QString e ) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         database.sqlDatabase().rollback();
         throw;
      }

      database.sqlDatabase().commit();

      return doUpdate;
   }


   //! \brief does the heavy lifting to copy the contents from one db to the next
   void copyDatabase(Database & database, Brewken::DBTypes oldType, Brewken::DBTypes newType, QSqlDatabase newDb) {
      QSqlDatabase oldDb = database.sqlDatabase();
      QSqlQuery readOld(oldDb);

      // There are a lot of tables to process, and we need to make
      // sure the inventory tables go first
      foreach( TableSchema* table, this->dbDefn.allTables(true) ) {
         QString tname = table->tableName();
         QSqlField field;
         bool mustPrepare = true;
         int maxid = -1;

         // select * from [table] order by id asc
         QString findAllQuery = QString("SELECT * FROM %1 order by %2 asc")
                                    .arg(tname)
                                    .arg(table->keyName(oldType)); // make sure we specify the right db type
         qDebug() << Q_FUNC_INFO << "FIND ALL:" << findAllQuery;
         try {
            if (! readOld.exec(findAllQuery) ) {
               throw QString("Could not execute %1 : %2")
                  .arg(readOld.lastQuery())
                  .arg(readOld.lastError().text());
            }

            newDb.transaction();

            QSqlQuery upsertNew(newDb); // we will prepare this in a bit

            // Start reading the records from the old db
            while(readOld.next()) {
               int idx;
               QSqlRecord here = readOld.record();
               QString upsertQuery;

               idx = here.indexOf(table->keyName(oldType));

               // We are going to need this for resetting the indexes later. We only
               // need it for copying to postgresql, but .. meh, not worth the extra
               // work
               if ( idx != -1 && here.value(idx).toInt() > maxid ) {
                  maxid = here.value(idx).toInt();
               }

               // Prepare the insert for this table if required
               if ( mustPrepare ) {
                  upsertQuery = table->generateInsertRow(newType);
                  upsertNew.prepare(upsertQuery);
                  // but do it only once for this table
                  mustPrepare = false;
               }

               qDebug() << Q_FUNC_INFO << "INSERT:" << upsertQuery;
               // All that's left is to bind
               for(int i = 0; i < here.count(); ++i) {
                  if ( table->dbTable() == DatabaseConstants::BREWNOTETABLE
                     && here.fieldName(i) == PropertyNames::BrewNote::brewDate ) {
                     QVariant helpme(here.field(i).value().toString());
                     upsertNew.bindValue(":brewdate",helpme);
                  }
                  else {
                     upsertNew.bindValue(QString(":%1").arg(here.fieldName(i)),
                                       convertValue(newType, here.field(i)));
                  }
               }
               // and execute
               if ( ! upsertNew.exec() ) {
                  throw QString("Could not insert new row %1 : %2")
                     .arg(upsertNew.lastQuery())
                     .arg(upsertNew.lastError().text());
               }
            }
            // We need to create the increment and decrement things for the
            // instructions_in_recipe table. This seems a little weird to do this
            // here, but it makes sense to wait until after we've inserted all
            // the data. The increment trigger happens on insert, and I suspect
            // bad things would happen if it were in place before we inserted all our data.
            if ( table->dbTable() == DatabaseConstants::INSTINRECTABLE ) {
               QString trigger = table->generateIncrementTrigger(newType);
               if ( trigger.isEmpty() ) {
                  qCritical() << QString("No increment triggers found for %1").arg(table->tableName());
               }
               else {
                  qDebug() << "INC TRIGGER:" << trigger;
                  upsertNew.exec(trigger);
                  trigger =  table->generateDecrementTrigger(newType);
                  if ( trigger.isEmpty() ) {
                     qCritical() << QString("No decrement triggers found for %1").arg(table->tableName());
                  }
                  else {
                     qDebug() << "DEC TRIGGER:" << trigger;
                     if ( ! upsertNew.exec(trigger) ) {
                        throw QString("Could not insert new row %1 : %2")
                           .arg(upsertNew.lastQuery())
                           .arg(upsertNew.lastError().text());
                     }
                  }
               }
            }
            // We need to manually reset the sequences in postgresql
            if ( newType == Brewken::PGSQL ) {
               // this probably should be fixed somewhere, but this is enough for now?
               //
               // SELECT setval(hop_id_seq,(SELECT MAX(id) from hop))
               QString seq = QString("SELECT setval('%1_%2_seq',(SELECT MAX(%2) FROM %1))")
                                             .arg(table->tableName())
                                             .arg(table->keyName());
               qDebug() << "SEQ reset: " << seq;

               if ( ! upsertNew.exec(seq) )
                  throw QString("Could not reset the sequences: %1 %2")
                     .arg(seq).arg(upsertNew.lastError().text());
            }
         }
         catch (QString e) {
            qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
            newDb.rollback();
            abort();
         }

         newDb.commit();
      }
   }

   void automaticBackup() {
      int count = PersistentSettings::value("count",0,"backups").toInt() + 1;
      int frequency = PersistentSettings::value("frequency",4,"backups").toInt();
      int maxBackups = PersistentSettings::value("maximum",10,"backups").toInt();

      // The most common case is update the counter and nothing else
      // A frequency of 1 means backup every time. Which this statisfies
      if ( count % frequency != 0 ) {
         PersistentSettings::insert( "count", count, "backups");
         return;
      }

      // If the user has selected 0 max backups, we just return. There's a weird
      // case where they have a frequency of 1 and a maxBackup of 0. In that
      // case, maxBackup wins
      if ( maxBackups == 0 ) {
         return;
      }

      QString backupDir = PersistentSettings::value("directory", PersistentSettings::getUserDataDir().canonicalPath(), "backups").toString();
      QString listOfFiles = PersistentSettings::value("files", QVariant(), "backups").toString();
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
      QStringList fileNames = listOfFiles.split(",", QString::SkipEmptyParts);
#else
      QStringList fileNames = listOfFiles.split(",", Qt::SkipEmptyParts);
#endif

      QString halfName = QString("%1.%2").arg("databaseBackup").arg(QDate::currentDate().toString("yyyyMMdd"));
      QString newName = halfName;
      // Unique filenames are a pain in the ass. In the case you open Brewken
      // twice in a day, this loop makes sure we don't over write (or delete) the
      // wrong thing
      int foobar = 0;
      while ( foobar < 10000 && QFile::exists( backupDir + "/" + newName ) ) {
         foobar++;
         newName = QString("%1_%2").arg(halfName).arg(foobar,4,10,QChar('0'));
         if ( foobar > 9999 ) {
            qWarning() << QString("%1 : could not find a unique name in 10000 tries. Overwriting %2").arg(Q_FUNC_INFO).arg(halfName);
            newName = halfName;
         }
      }
      // backup the file first
      backupToDir(backupDir,newName);

      // If we have maxBackups == -1, it means never clean. It also means we
      // don't track the filenames.
      if ( maxBackups == -1 )  {
         PersistentSettings::remove("files", "backups");
         return;
      }

      fileNames.append(newName);

      // If we have too many backups. This is in a while loop because we need to
      // handle the case where a user decides they only want 4 backups, not 10.
      // The while loop will clean that up properly.
      while ( fileNames.size() > maxBackups ) {
         // takeFirst() removes the file from the list, which is important
         QString victim = backupDir + "/" + fileNames.takeFirst();
         QFile *file = new QFile(victim);
         QFileInfo *fileThing = new QFileInfo(victim);

         // Make sure it exists, and make sure it is a file before we
         // try remove it
         if ( fileThing->exists() && fileThing->isFile() ) {
            qInfo() <<
               Q_FUNC_INFO << "Removing oldest database backup file," << victim << "as more than" << maxBackups <<
               "files in" << backupDir;
            // If we can't remove it, give a warning.
            if (! file->remove() ) {
               qWarning() <<
                  Q_FUNC_INFO << "Could not remove old database backup file " << victim << ".  Error:" << file->error();
            }
         }
      }

      // re-encode the list
      listOfFiles = fileNames.join(",");

      // finally, reset the counter and save the new list of files
      PersistentSettings::insert("count", 0, "backups");
      PersistentSettings::insert("files", listOfFiles, "backups");
   }


   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   // This links ingredients with the same name.
   // The first displayed ingredient in the database is assumed to be the parent.
   void populateChildTablesByName(Database & database, DatabaseConstants::DbTableId table) {
      TableSchema* tbl = this->dbDefn.table(table);
      TableSchema* cld = this->dbDefn.childTable( table );
      qInfo() << QString("Populating Children NamedEntity Links (%1)").arg(tbl->tableName());

      try {
         // "SELECT DISTINCT name FROM [tablename]"
         QString queryString = QString("SELECT DISTINCT %1 FROM %2")
               .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::name)).arg(tbl->tableName());

         qDebug() << Q_FUNC_INFO << "DISTINCT:" << queryString;
         QSqlQuery nameq( queryString, database.sqlDatabase() );

         if ( ! nameq.exec() ) {
            throw QString("%1 %2").arg(nameq.lastQuery()).arg(nameq.lastError().text());
         }

         while (nameq.next()) {
            QString name = nameq.record().value(0).toString();
            // select id from [tablename] where ( name = :name and display = :boolean ) order by id asc
            queryString = QString( "SELECT %1 FROM %2 WHERE ( %3=:name AND %4=:boolean ) ORDER BY %1")
                        .arg(tbl->keyName())
                        .arg(tbl->tableName())
                        .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::name))
                        .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::display));
            QSqlQuery query( database.sqlDatabase() );

            qDebug() << Q_FUNC_INFO << "FIND:" << queryString;

            // find the first element with display set true (assumed parent)
            query.prepare(queryString);
            query.bindValue(":name", name);
            query.bindValue(":boolean",Brewken::dbTrue());

            if ( !query.exec() ) {
               throw QString("%1 %2").arg(query.lastQuery()).arg(query.lastError().text());
            }

            query.first();
            QString parentID = query.record().value(tbl->keyName()).toString();

            // find the every element with display set false (assumed children)
            query.bindValue(":name", name);
            query.bindValue(":boolean", Brewken::dbFalse());

            if ( !query.exec() ) {
               throw QString("%1 %2").arg(query.lastQuery()).arg(query.lastError().text());
            }
            // Postgres uses a more verbose upsert syntax. I don't like this, but
            // I'm not seeing a better way yet.
            while (query.next()) {
               QString childID = query.record().value(tbl->keyName()).toString();
               switch( Brewken::dbType() ) {
                  case Brewken::PGSQL:
                     //  INSERT INTO [child table] (parent_id, child_id) VALUES (:parentid, child_id) ON CONFLICT(child_id) DO UPDATE set parent_id = EXCLUDED.parent_id
                     queryString = QString("INSERT INTO %1 (%2, %3) VALUES (%4, %5) ON CONFLICT(%3) DO UPDATE set %2 = EXCLUDED.%2")
                           .arg(this->dbDefn.childTableName((table)))
                           .arg(cld->parentIndexName())
                           .arg(cld->childIndexName())
                           .arg(parentID)
                           .arg(childID);
                     break;
                  default:
                     // insert or replace into [child table] (parent_id, child_id) values (:parentid,:childid)
                     queryString = QString("INSERT OR REPLACE INTO %1 (%2, %3) VALUES (%4, %5)")
                                 .arg(this->dbDefn.childTableName(table))
                                 .arg(cld->parentIndexName())
                                 .arg(cld->childIndexName())
                                 .arg(parentID)
                                 .arg(childID);
               }
               qDebug() << Q_FUNC_INFO << "UPSERT:" << queryString;
               QSqlQuery insertq( queryString, database.sqlDatabase() );
               if ( !insertq.exec() ) {
                  throw QString("%1 %2").arg(insertq.lastQuery()).arg(insertq.lastError().text());
               }
            }
         }
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         abort();
      }

      return;
   }

   // populate ingredient tables
   // Runs populateChildTablesByName for each
   void populateChildTablesByName(Database & database) {

      try {
         // I really dislike this. It counts as spooky action at a distance, but
         // the populateChildTablesByName methods need these hashes populated
         // early and there is no easy way to untangle them. Yes, this results in
         // the work being done twice. Such is life.
         populateChildTablesByName(database, DatabaseConstants::FERMTABLE);
         populateChildTablesByName(database, DatabaseConstants::HOPTABLE);
         populateChildTablesByName(database, DatabaseConstants::MISCTABLE);
         populateChildTablesByName(database, DatabaseConstants::YEASTTABLE);
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         throw;
      }
      return;
   }

   DatabaseSchema dbDefn;
   QString dbConName;

   bool loaded;

   // Instance variables.
   bool loadWasSuccessful;
   bool createFromScratch;
   bool schemaUpdated;

};


Database::Database() : pimpl{ new impl{} } {
   //.setUndoLimit(100);
   return;
}

Database::~Database() {
   // Don't try and log in this function as it's called pretty close to the program exiting, at the end of main(), at
   // which point the objects used by the logging module may be in a weird state.

   // If we have not explicitly unloaded, do so now and discard changes.
   if (this->pimpl->loaded) {
      this->unload();
   }

   return;
}


QSqlDatabase Database::sqlDatabase() const {
   // Need a unique database connection for each thread.
   //http://www.linuxjournal.com/article/9602

   //
   // If we already created a valid DB connection for this thread, this call will get it, and we can just return it to
   // the caller.  Otherwise, we'll just get an invalid connection.
   //
   Q_ASSERT(!dbConnectionNameForThisThread.isEmpty());
   QSqlDatabase connection = QSqlDatabase::database(dbConnectionNameForThisThread);
   if (connection.isValid()) {
      qDebug() << Q_FUNC_INFO << "Returning connection " << dbConnectionNameForThisThread;
      return connection;
   }

   //
   // Create a new connection in Qt's register of connections.  (NB: The call to QSqlDatabase::addDatabase() is thread-
   // safe, so we don't need to worry about mutexes here.)
   //
   QString driverType{Brewken::dbType() == Brewken::PGSQL ? "QPSQL" : "QSQLITE"};
   qDebug() <<
      Q_FUNC_INFO << "Creating connection " << dbConnectionNameForThisThread << " with " << driverType << " driver";
   connection = QSqlDatabase::addDatabase(driverType, dbConnectionNameForThisThread);
   if (!connection.isValid()) {
      //
      // If the connection is not valid, it means the specified driver type is not available or could not be loaded
      // Log an error here in the knowledge that we'll also throw an exception below
      //
      qCritical() << Q_FUNC_INFO << "Unable to load " << driverType << " database driver";
   }

   //
   // Initialisation parameters depend on the DB type
   //
   if (Brewken::dbType() == Brewken::PGSQL) {
      connection.setHostName(dbHostname);
      connection.setDatabaseName(dbName);
      connection.setUserName(dbUsername);
      connection.setPort(dbPortnum);
      connection.setPassword(dbPassword);
   } else {
      connection.setDatabaseName(dbFileName);
   }

   //
   // The moment of truth is when we try to open the new connection
   //
   if (!connection.open()) {
      QString errorMessage;
      if (Brewken::dbType() == Brewken::PGSQL) {
         errorMessage = QString{
            QObject::tr("Could not open PostgreSQL DB connection to %1.\n%2")
         }.arg(dbHostname).arg(connection.lastError().text());
      } else {
         errorMessage = QString{
            QObject::tr("Could not open SQLite DB file %1.\n%2")
         }.arg(dbFileName).arg(connection.lastError().text());
      }
      qCritical() << Q_FUNC_INFO << errorMessage;

      if (Brewken::isInteractive()) {
         QMessageBox::critical(nullptr,
                               QObject::tr("Database Failure"),
                               errorMessage);
      }

      // If we can't talk to the DB, there's not much we can do to recover
      throw errorMessage;
   }

   return connection;
}


bool Database::load() {
   bool dbIsOpen;

   this->pimpl->createFromScratch=false;
   this->pimpl->schemaUpdated=false;
   this->pimpl->loadWasSuccessful = false;

   if ( Brewken::dbType() == Brewken::PGSQL ) {
      dbIsOpen = this->pimpl->loadPgSQL(*this);
   }
   else {
      dbIsOpen = this->pimpl->loadSQLite(*this);
   }

   if ( ! dbIsOpen ) {
      return false;
   }

   this->pimpl->loaded = true;

   QSqlDatabase sqldb = this->sqlDatabase();

   // This should work regardless of the db being used.
   if( this->pimpl->createFromScratch ) {
      bool success = DatabaseSchemaHelper::create(sqldb,&this->pimpl->dbDefn,Brewken::dbType());
      if( !success ) {
         qCritical() << "DatabaseSchemaHelper::create() failed";
         return false;
      }
   }

   // Update the database if need be. This has to happen before we do anything
   // else or we dump core
   bool schemaErr = false;
   this->pimpl->schemaUpdated = this->pimpl->updateSchema(*this, &schemaErr);

   if( schemaErr ) {
      if (Brewken::isInteractive()) {
         QMessageBox::critical(
            nullptr,
            QObject::tr("Database Failure"),
            QObject::tr("Failed to update the database")
         );
      }
      return false;
   }

   // See if there are new ingredients that we need to merge from the data-space db.
   // Don't do this if we JUST copied the dataspace database.
   if (dataDbFile.fileName() != dbFile.fileName() &&
       !Brewken::userDatabaseDidNotExist &&
       QFileInfo(dataDbFile).lastModified() > Brewken::lastDbMergeRequest) {
      if( Brewken::isInteractive() &&
         QMessageBox::question(
            nullptr,
            tr("Merge Database"),
            tr("There may be new ingredients and recipes available. Would you like to add these to your database?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
         )
         == QMessageBox::Yes
      ) {
         updateDatabase(dataDbFile.fileName());
      }

      // Update this field.
      Brewken::lastDbMergeRequest = QDateTime::currentDateTime();
   }

   this->pimpl->loadWasSuccessful = true;
   return this->pimpl->loadWasSuccessful;
}

bool Database::createBlank(QString const& filename)
{
   {
      QSqlDatabase sqldb = QSqlDatabase::addDatabase("QSQLITE", "blank");
      sqldb.setDatabaseName(filename);
      bool dbIsOpen = sqldb.open();
      if( ! dbIsOpen )
      {
         qWarning() << QString("Database::createBlank(): could not open '%1'").arg(filename);
         return false;
      }

      DatabaseSchemaHelper::create(sqldb,&this->pimpl->dbDefn,Brewken::SQLITE);

      sqldb.close();
   } // sqldb gets destroyed as it goes out of scope before removeDatabase()

   QSqlDatabase::removeDatabase( "blank" );
   return true;
}

bool Database::loadSuccessful()
{
   return this->pimpl->loadWasSuccessful;
}


void Database::unload() {

   // So far, it seems we only create one connection to the db. This is
   // likely overkill
   QStringList allConnectionNames{QSqlDatabase::connectionNames()};
   for (QString conName : allConnectionNames) {
      qDebug() << Q_FUNC_INFO << "Closing connection " << conName;
      {
         //
         // Extra braces here are to ensure that this QSqlDatabase object is out of scope before the call to
         // QSqlDatabase::removeDatabase() below
         //
         QSqlDatabase connectionToClose = QSqlDatabase::database(conName, false);
         if (connectionToClose.isOpen()) {
            connectionToClose.rollback();
            connectionToClose.close();
         }
      }
      QSqlDatabase::removeDatabase(conName);
   }

   qDebug() << Q_FUNC_INFO << "DB connections all closed";

   if (this->pimpl->loadWasSuccessful && Brewken::dbType() == Brewken::SQLITE ) {
      dbFile.close();
      this->pimpl->automaticBackup();
   }

   this->pimpl->loaded = false;
   this->pimpl->loadWasSuccessful = false;

   return;
}


Database& Database::instance() {

   //
   // As of C++11, simple "Meyers singleton" is now thread-safe -- see
   // https://www.modernescpp.com/index.php/thread-safe-initialization-of-a-singleton#h3-guarantees-of-the-c-runtime
   //
   static Database dbSingleton;

   //
   // And C++11 also provides a thread-safe way to ensure a function is called exactly once
   //
   // (See http://www.aristeia.com/Papers/DDJ_Jul_Aug_2004_revised.pdf for why user-implemented efforts to do this via
   // double-checked locking often come unstuck in the face of compiler optimisations, especially on multi-processor
   // platforms, back in the days when the C++ language had "no notion of threading (or any other form of
   // concurrency)".
   //
   static std::once_flag initFlag;
   std::call_once(initFlag, &Database::load, &dbSingleton);

   return dbSingleton;
}

// TBD Why don't we just have callers invoke unload directly?
void Database::dropInstance() {
   static QMutex mutex;

   mutex.lock();
   Database::instance().unload();
   mutex.unlock();
   qDebug() << Q_FUNC_INFO << "Drop Instance done";
   return;
}

char const * Database::getDefaultBackupFileName() {
    return "database.sqlite";
}

bool Database::backupToFile(QString newDbFileName) {
   // Make sure the singleton exists - otherwise there's nothing to backup.
   instance();

   // Remove the files if they already exist so that
   // the copy() operation will succeed.
   QFile::remove(newDbFileName);

   bool success = dbFile.copy( newDbFileName );

   qDebug() << QString("Database backup to \"%1\" %2").arg(newDbFileName, success ? "succeeded" : "failed");

   return success;
}

bool Database::backupToDir(QString dir, QString filename) {
   bool success = true;
   QString prefix = dir + "/";
   QString newDbFileName = prefix + getDefaultBackupFileName();

   if ( !filename.isEmpty() ) {
      newDbFileName = prefix + filename;
   }

   success = backupToFile( newDbFileName );

   return success;
}

bool Database::restoreFromFile(QString newDbFileStr)
{
   bool success = true;

   QFile newDbFile(newDbFileStr);
   // Fail if we can't find file.
   if( !newDbFile.exists() ) {
      return false;
   }
   success &= newDbFile.copy(QString("%1.new").arg(dbFile.fileName()));
   QFile::setPermissions( newDbFile.fileName(), QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup );

   return success;
}

// .:TBD:. Discuss with other folks whether this is worth fixing
void Database::updateDatabase(QString const& filename)
{
   throw QString("Not implemented");
/*
   // In the naming here "old" means our local database, and
   // "new" means the database coming from 'filename'.

   QVariant btid, newid, oldid;
   QMap<QString, std::function<NamedEntity*(QString name)> >  makeObject = makeTableParams(*this);

   try {
      QString newCon("newSqldbCon");
      QSqlDatabase newSqldb = QSqlDatabase::addDatabase("QSQLITE", newCon);
      newSqldb.setDatabaseName(filename);
      if( ! newSqldb.open() ) {
         QMessageBox::critical(nullptr,
                              QObject::tr("Database Failure"),
                              QString(QObject::tr("Failed to open the database '%1'.").arg(filename)));
         throw QString("Could not open %1 for reading.\n%2").arg(filename).arg(newSqldb.lastError().text());
      }

      // This is the basic gist...
      // For each (id, hop_id) in newSqldb.bt_hop...

      // Call this newRecord
      // SELECT * FROM newSqldb.hop WHERE id=<hop_id>

      // UPDATE hop SET name=:name, alpha=:alpha,... WHERE id=(SELECT hop_id FROM bt_hop WHERE id=:bt_id)

      // Bind :bt_id from <id>
      // Bind :name, :alpha, ..., from newRecord.

      // Execute.

      foreach( TableSchema* tbl, this->pimpl->dbDefn.baseTables() )
      {
         TableSchema* btTbl = this->pimpl->dbDefn.btTable(tbl->dbTable());
         // not all tables have bt* tables
         if ( btTbl == nullptr ) {
            continue;
         }
         QSqlQuery qNewBtIng( QString("SELECT * FROM %1").arg(btTbl->tableName()), newSqldb );

         QSqlQuery qNewIng( newSqldb );
         qNewIng.prepare(QString("SELECT * FROM %1 WHERE %2=:id").arg(tbl->tableName()).arg(tbl->keyName()));

         // Construct the big update query.
         QSqlQuery qUpdateOldIng( sqlDatabase() );
         QString updateString = tbl->generateUpdateRow();
         qUpdateOldIng.prepare(updateString);

         QSqlQuery qOldBtIng( sqlDatabase() );
         qOldBtIng.prepare( QString("SELECT * FROM %1 WHERE %2=:btid").arg(btTbl->tableName()).arg(btTbl->keyName()) );

         QSqlQuery qOldBtIngInsert( sqlDatabase() );
         qOldBtIngInsert.prepare( QString("INSERT INTO %1 (%2,%3) values (:id,:%3)")
                                  .arg(btTbl->tableName())
                                  .arg(btTbl->keyName())
                                  .arg(btTbl->childIndexName()));

         while( qNewBtIng.next() ) {
            btid = qNewBtIng.record().value(btTbl->keyName());
            newid = qNewBtIng.record().value(btTbl->childIndexName());

            qNewIng.bindValue(":id", newid);
            // if we can't run the query
            if ( ! qNewIng.exec() )
               throw QString("Could not retrieve new ingredient: %1 %2").arg(qNewIng.lastQuery()).arg(qNewIng.lastError().text());

            // if we can't get the result from the query
            if( !qNewIng.next() )
               throw QString("Could not advance query: %1 %2").arg(qNewIng.lastQuery()).arg(qNewIng.lastError().text());

            foreach( QString pn, tbl->allColumnNames()) {
               // Bind the old values to the new unless it is deleted, which we always set to false
               if ( pn == kcolDeleted ) {
                  qUpdateOldIng.bindValue( QString(":%1").arg(pn), Brewken::dbFalse());
               }
               qUpdateOldIng.bindValue( QString(":%1").arg(pn), qNewIng.record().value(pn));
            }

            // Done retrieving new ingredient data.
            qNewIng.finish();

            // Find the bt_<ingredient> record in the local table.
            qOldBtIng.bindValue( ":btid", btid );
            if ( ! qOldBtIng.exec() ) {
               throw QString("Could not find btID (%1): %2 %3")
                        .arg(btid.toInt())
                        .arg(qOldBtIng.lastQuery())
                        .arg(qOldBtIng.lastError().text());
            }

            // If the btid exists in the old bt_hop table, do an update.
            if( qOldBtIng.next() ) {
               oldid = qOldBtIng.record().value( btTbl->keyName() );
               qOldBtIng.finish();

               qUpdateOldIng.bindValue( ":id", oldid );

               if ( ! qUpdateOldIng.exec() )
                  throw QString("Could not update old btID (%1): %2 %3")
                           .arg(oldid.toInt())
                           .arg(qUpdateOldIng.lastQuery())
                           .arg(qUpdateOldIng.lastError().text());

            }
            // If the btid doesn't exist in the old bt_ table, do an insert into
            // the new table, then into the new bt_ table.
            else {
               // Create a new ingredient.
               oldid = makeObject.value(tbl->tableName())(qNewBtIng.record().value(PropertyNames::NamedEntity::name).toString())->key();

               // Copy in the new data.
               qUpdateOldIng.bindValue( ":id", oldid );

               if ( ! qUpdateOldIng.exec() )
                  throw QString("Could not insert new btID (%1): %2 %3")
                           .arg(oldid.toInt())
                           .arg(qUpdateOldIng.lastQuery())
                           .arg(qUpdateOldIng.lastError().text());


               // Insert an entry into our bt_<ingredient> table.
               qOldBtIngInsert.bindValue( ":id", btid );
               qOldBtIngInsert.bindValue( QString(":%1").arg(btTbl->childIndexName()), oldid );

               if ( !  qOldBtIngInsert.exec() )
                  throw QString("Could not insert btID (%1): %2 %3")
                           .arg(btid.toInt())
                           .arg(qOldBtIngInsert.lastQuery())
                           .arg(qOldBtIngInsert.lastError().text());
            }
         }
      }
      // If we, by some miracle, get here, commit
      sqlDatabase().commit();
      // I think
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      abort();
   }
   */
}

bool Database::verifyDbConnection(Brewken::DBTypes testDb, QString const& hostname, int portnum, QString const& schema,
                              QString const& database, QString const& username, QString const& password)
{
   QString driverName;
   QSqlDatabase connDb;
   bool results;

   switch( testDb )
   {
      case Brewken::PGSQL:
         driverName = "QPSQL";
         break;
      default:
         driverName = "QSQLITE";
   }
   connDb = QSqlDatabase::addDatabase(driverName,"testConnDb");

   switch( testDb )
   {
      case Brewken::PGSQL:
         connDb.setHostName(hostname);
         connDb.setPort(portnum);
         connDb.setDatabaseName(database);
         connDb.setUserName(username);
         connDb.setPassword(password);
         break;
      default:
         connDb.setDatabaseName(hostname);
   }

   results = connDb.open();

   if ( results )
      connDb.close();
   else
      QMessageBox::critical(nullptr, tr("Connection failed"),
               QString(tr("Could not connect to %1 : %2")).arg(hostname).arg(connDb.lastError().text())
            );
   return results;

}



void Database::convertDatabase(QString const& Hostname, QString const& DbName,
                               QString const& Username, QString const& Password,
                               int Portnum, Brewken::DBTypes newType)
{
   QSqlDatabase newDb;

   Brewken::DBTypes oldType = static_cast<Brewken::DBTypes>(PersistentSettings::value("dbType", Brewken::SQLITE).toInt());

   try {
      if ( newType == Brewken::NODB ) {
         throw QString("No type found for the new database.");
      }

      if ( oldType == Brewken::NODB ) {
         throw QString("No type found for the old database.");
      }

      switch( newType ) {
         case Brewken::PGSQL:
            newDb = openPostgres(Hostname, DbName,Username, Password, Portnum);
            break;
         default:
            newDb = openSQLite();
      }

      if ( ! newDb.isOpen() ) {
         throw QString("Could not open new database: %1").arg(newDb.lastError().text());
      }

      // this is to prevent us from over-writing or doing heavens knows what to an existing db
      if( newDb.tables().contains(QLatin1String("settings")) ) {
         qWarning() << QString("It appears the database is already configured.");
         return;
      }

      newDb.transaction();

      // make sure we get the inventory tables first
      foreach( TableSchema* table, this->pimpl->dbDefn.allTables(true) ) {
         QString createTable = table->generateCreateTable(newType);
         QSqlQuery results( newDb );
         if ( ! results.exec(createTable) ) {
            throw QString("Could not create %1 : %2").arg(table->tableName()).arg(results.lastError().text());
         }
      }
      newDb.commit();

      this->pimpl->copyDatabase(*this, oldType, newType, newDb);
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      throw;
   }
}

DatabaseSchema & Database::getDatabaseSchema() {
   return this->pimpl->dbDefn;
}
