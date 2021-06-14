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

#include <QCryptographicHash>
#include <QDebug>
#include <QDomDocument>
#include <QDomNode>
#include <QDomNodeList>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QIODevice>
#include <QList>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QPair>
#include <QPushButton>
#include <QSqlError>
#include <QSqlField>
#include <QSqlIndex>
#include <QSqlQuery>
#include <QString>
#include <QStringBuilder>
#include <QTextCodec>
#include <QTextStream>
#include <QThread>


#include "Algorithms.h"
#include "Brewken.h"
#include "config.h"
#include "database/BrewNoteSchema.h"
#include "database/DatabaseSchema.h"
#include "database/DatabaseSchemaHelper.h"
#include "database/InstructionSchema.h"
#include "database/MashStepSchema.h"
#include "database/RecipeSchema.h"
#include "database/SaltSchema.h"
#include "database/SettingsSchema.h"
#include "database/TableSchemaConst.h"
#include "database/TableSchema.h"
#include "database/WaterSchema.h"
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
#include "QueuedMethod.h"

//
// .:TODO:. Look at BT fix https://github.com/mikfire/brewtarget/commit/e5a43c1d7babbaf9450a14e5ea1e4589235ded2c
// for incorrect inventory handling when a NE is copied
//

namespace {
   //! Helper to more easily get QMetaProperties.
   QMetaProperty metaProperty(Database const & db, const char* name) {
      return db.metaObject()->property(db.metaObject()->indexOfProperty(name));
   }

   //QThread* _thread;
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

/*
   QMap<QString, std::function<NamedEntity*(QString name)> > makeTableParams(Database & db) {
      QMap<QString, std::function<NamedEntity*(QString name)> > tmp;
      //=============================Equipment====================================

      tmp.insert(ktableEquipment,   [&](QString name) { return db.newEquipment(); } );
      tmp.insert(ktableFermentable, [&](QString name) { return db.newFermentable(); } );
      tmp.insert(ktableHop,         [&](QString name) { return db.newHop(); } );
      tmp.insert(ktableMisc,        [&](QString name) { return db.newMisc(); } );
      tmp.insert(ktableStyle,       [&](QString name) { return db.newStyle(name); } );
      tmp.insert(ktableYeast,       [&](QString name) { return db.newYeast(); } );
      tmp.insert(ktableWater,       [&](QString name) { return db.newWater(); } );
      tmp.insert(ktableSalt,        [&](QString name) { return db.newSalt(); } );

      return tmp;
   }
*/
}

//
// This private implementation class holds all private non-virtual members of BeerXML
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

   //! Helper to populate all* hashes. T should be a NamedEntity subclass.
/*   template <class T> void populateElements(Database & database, QHash<int,T*>& hash, DatabaseConstants::DbTableId table ) {
      QSqlQuery q(database.sqlDatabase());
      TableSchema* tbl = this->dbDefn.table(table);
      q.setForwardOnly(true);
      QString queryString = QString("SELECT * FROM %1").arg(tbl->tableName());
      q.prepare( queryString );

      try {
         if ( ! q.exec() )
            throw QString("%1 %2").arg(q.lastQuery()).arg(q.lastError().text());
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         q.finish();
         throw;
      }

      while( q.next() ) {
         int key = q.record().value(tbl->keyName(Brewken::dbType())).toInt();

         T* e = new T(table, key, q.record());
         if( ! hash.contains(key) )
            hash.insert(key, e);
      }

      q.finish();
   }

   //! Helper to populate the list using the given filter.
   template <class T> bool getElements( Database & database,
                                        QList<T*>& list,
                                        QString filter,
                                        DatabaseConstants::DbTableId table,
                                        QHash<int,T*> allElements,
                                        QString id=QString() ) {

      QSqlQuery q(database.sqlDatabase());
      TableSchema* tbl = this->dbDefn.table( table );
      q.setForwardOnly(true);
      QString queryString;

      if ( id.isEmpty() ) {
         id = tbl->keyName(Brewken::dbType());
      }

      if( !filter.isEmpty() ) {
         queryString = QString("SELECT %1 as id FROM %2 WHERE %3").arg(id).arg(tbl->tableName()).arg(filter);
      }
      else {
         queryString = QString("SELECT %1 as id FROM %2").arg(id).arg(tbl->tableName());
      }

      qDebug() << QString("%1 SQL: %2").arg(Q_FUNC_INFO).arg(queryString);

      try {
         if ( ! q.exec(queryString) )
            throw QString("could not execute query: %2 : %3").arg(queryString).arg(q.lastError().text());
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         q.finish();
         throw;
      }

      while( q.next() )
      {
         int key = q.record().value("id").toInt();
         if( allElements.contains(key) )
            list.append( allElements[key] );
      }

      q.finish();
      return true;
   }
*/
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

   // Note -- this has to happen on a transactional boundary. We are touching
   // something like four tables, and just sort of hoping it all works.
   /*!
    * Create a \e copy (by default) of \b ing and add the copy to \b recipe where \b ing's
    * key is \b ingKeyName and the relational table is \b relTableName.
    *
    * \tparam T the type of ingredient. Must inherit NamedEntity.
    * \param rec the recipe to add the ingredient to
    * \param ing the ingredient to add to the recipe
    * \param propName the Recipe property that will change when we add \c ing to it
    * \param relTableName the name of the relational table, perhaps "ingredient_in_recipe"
    * \param ingKeyName the name of the key in the ingredient table corresponding to \c ing
    * \param noCopy By default, we create a copy of the ingredient. If true,
    *               add the ingredient directly.
    * \param keyHash if not null, add the new (key, \c ing) pair to it
    * \param doNotDisplay if true (default), calls \c setDisplay(\c false) on the new ingredient
    * \returns the new ingredient.
    */
/*   template<class T> T* addNamedEntityToRecipe(
      Database & db,
      Recipe* rec,
      NamedEntity* ing,
      bool noCopy = false,
      QHash<int,T*>* keyHash = 0,
      bool doNotDisplay = true,
      bool transact = true
   ) {
      qDebug() <<
      Q_FUNC_INFO << "noCopy:" << (noCopy ? "true" : "false") << ", doNotDisplay:" <<
      (doNotDisplay ? "true" : "false") << ", transact" << (transact ? "true" : "false");

      T* newIng = nullptr;
      QString propName, relTableName, ingKeyName, childTableName;
      TableSchema* table;
      TableSchema* child;
      TableSchema* inrec;
      const QMetaObject* meta = ing->metaObject();
      int ndx = meta->indexOfClassInfo("signal");

      if( rec == nullptr || ing == nullptr )
         return nullptr;

      // TRANSACTION BEGIN, but only if requested. Yeah. Had to go there.
      if ( transact ) {
         db.sqlDatabase().transaction();
      }

      // Queries have to be created inside transactional boundaries
      QSqlQuery q(db.sqlDatabase());
      try {
         if ( ndx != -1 ) {
            propName  = meta->classInfo(ndx).value();
         }
         else {
            throw QString("could not locate classInfo for signal on %2").arg(meta->className());
         }

         table = this->dbDefn.table( this->dbDefn.classNameToTable(meta->className()) );
         child = this->dbDefn.table( table->childTable() );
         inrec = this->dbDefn.table( table->inRecTable() );
         // Ensure this ingredient is not already in the recipe.
         QString select = QString("SELECT %5 from %1 WHERE %2=%3 AND %5=%4")
                              .arg(inrec->tableName())
                              .arg(inrec->inRecIndexName())
                              .arg(ing->key())
                              .arg(reinterpret_cast<NamedEntity*>(rec)->key())
                              .arg(inrec->recipeIndexName());
         qDebug() << Q_FUNC_INFO << "NamedEntity in recipe search:" << select;
         if (! q.exec(select) ) {
            throw QString("Couldn't execute ingredient in recipe search: Query: %1 error: %2")
               .arg(q.lastQuery()).arg(q.lastError().text());
         }

         // this probably should just be a warning, not a throw?
         if ( q.next() ) {
            throw QString("NamedEntity already exists in recipe." );
         }

         q.finish();

         if ( noCopy ) {
            newIng = qobject_cast<T*>(ing);
            // Any ingredient part of a recipe shouldn't be visible, unless otherwise requested.
            // Not sure I like this. It's a long call stack just to end up back
            // here
            ing->setDisplay(! doNotDisplay );
            // Ensure the ingredient exists in the DB - eg if this is something that was previously deleted and we are
            // adding it back via Undo.  NB: The reinsertion will change the ingredient's key.
            ing->insertInDatabase();
         }
         else
         {
            newIng = db.pimpl->copy<T>(db, ing, keyHash, false);
            if ( newIng == nullptr ) {
               throw QString("error copying ingredient");
            }
            qDebug() << QString("%1 Copy %2 #%3 to %2 #%4").arg(Q_FUNC_INFO).arg(meta->className()).arg(ing->key()).arg(newIng->key());
            newIng->setParent(*ing);
         }

         // Put this (ing,rec) pair in the <ing_type>_in_recipe table.
         // q.setForwardOnly(true);

         // Link the ingredient to the recipe in the DB
         // Eg, for a fermentable, this is INSERT INTO fermentable_in_recipe (fermentable_id, recipe_id) VALUES (:ingredient, :recipe)
         QString insert = QString("INSERT INTO %1 (%2, %3) VALUES (:ingredient, :recipe)")
                  .arg(inrec->tableName())
                  .arg(inrec->inRecIndexName(Brewken::dbType()))
                  .arg(inrec->recipeIndexName());

         q.prepare(insert);
         q.bindValue(":ingredient", newIng->key());
         q.bindValue(":recipe", rec->key());

         qDebug() << QString("%1 Link ingredient to recipe: %2 with args %3, %4").arg(Q_FUNC_INFO).arg(insert).arg(newIng->key()).arg(rec->key());

         if ( ! q.exec() ) {
            throw QString("%2 : %1.").arg(q.lastQuery()).arg(q.lastError().text());
         }

         emit rec->changed( rec->metaProperty(propName), QVariant() );

         q.finish();

         //Put this in the <ing_type>_children table.
         // instructions and salts have no children.
         if( inrec->dbTable() != DatabaseConstants::INSTINRECTABLE && inrec->dbTable() != DatabaseConstants::SALTINRECTABLE ) {
            //
            // The parent to link to depends on where the ingredient is copied from:
            // - A fermentable from the fermentable table -> the ID of the fermentable.
            // - An ingredient from another recipe -> the ID of the ingredient's parent.
            //
            // This is required:
            // - When deleting the ingredient from the original recipe no longer fails.
            //   Else if fails due to a foreign key constrain.
            //
            int key = ing->key();
            QString parentChildSql = QString("SELECT %1 FROM %2 WHERE %3=%4")
                  .arg(child->parentIndexName())
                  .arg(child->tableName())
                  .arg(child->childIndexName())
                  .arg(key);
            q.prepare(parentChildSql);
            qDebug() << QString("%1 Parent-Child find: %2").arg(Q_FUNC_INFO).arg(parentChildSql);
            if (q.exec() && q.next()) {
               key = q.record().value(child->parentIndexName()).toInt();
            }
            q.finish();

            insert = QString("INSERT INTO %1 (%2, %3) VALUES (:parent, :child)")
                  .arg(child->tableName())
                  .arg(child->parentIndexName())
                  .arg(child->childIndexName());

            q.prepare(insert);
            q.bindValue(":parent", key);
            q.bindValue(":child", newIng->key());

            qDebug() <<
               Q_FUNC_INFO << "Parent-Child Insert:" << insert << "with args" << key << "," << newIng->key();

            if ( ! q.exec() ) {
               throw QString("%1 %2.").arg(q.lastQuery()).arg(q.lastError().text());
            }

            emit rec->changed( rec->metaProperty(propName), QVariant() );
         }
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(QString("Q_FUNC_INFO")).arg(e);
         q.finish();
         if ( transact )
            db.sqlDatabase().rollback();
         throw;
      }
      q.finish();
      if ( transact )
         db.sqlDatabase().commit();

      return newIng;
   }
*/
   // Named constructors ======================================================
   //! Create new brew note attached to \b parent.
   // maybe I should have never learned templates?
/*   template<class T> T* newNamedEntity(Database & database, QHash<int,T*>* all) {
      int key;
      // To quote the talking heads, my god what have I done?
      DatabaseConstants::DbTableId table = this->dbDefn.classNameToTable( T::classNameStr() );
      QString insert = QString("INSERT INTO %1 DEFAULT VALUES").arg(this->dbDefn.tableName(table));

      QSqlQuery q(database.sqlDatabase());

      // q.setForwardOnly(true);

      try {
         if ( ! q.exec(insert) )
            throw QString("could not insert a record into");

         key = q.lastInsertId().toInt();
         q.finish();
      }
      catch (QString e) {
         qCritical() << Q_FUNC_INFO << e << q.lastError().text();
         throw; // rethrow the error until somebody cares
      }

      T* tmp = new T(table, key);
      all->insert(tmp->key(),tmp);

      return tmp;
   }

   template<class T> T* newNamedEntity(Database & database, QString name, QHash<int,T*>* all) {
      int key;
      // To quote the talking heads, my god what have I done?
      TableSchema* tbl = this->dbDefn.table(this->dbDefn.classNameToTable(T::classNameStr()));
      QString insert = QString("INSERT INTO %1 (%2) VALUES (:name)")
              .arg(tbl->tableName())
              .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::name));

      QSqlQuery q(database.sqlDatabase());

      q.prepare(insert);
      q.bindValue(":name",name);

      q.setForwardOnly(true);

      try {
         if ( ! q.exec() )
            throw QString("could not insert a record into");

         key = q.lastInsertId().toInt();
         q.finish();
      }
      catch (QString e) {
         qCritical() << Q_FUNC_INFO << e << q.lastError().text();
         throw; // rethrow the error until somebody cares
      }

      T* tmp = new T(tbl->dbTable(), key);
      all->insert(tmp->key(),tmp);

      return tmp;
   }
*/
   /*!
    * \brief Create a deep copy of the \b object.
    * \em T must be a subclass of \em NamedEntity.
    * \returns a pointer to the new copy. You must manually emit the changed()
    * signal after a copy() call. Also, does not insert things magically into
    * allHop or allInstructions etc. hashes. This just simply duplicates a
    * row in a table, unless you provide \em keyHash.
    * \param object is the thing you want to copy.
    * \param displayed is true if you want the \em displayed column set to true.
    * \param keyHash if nonzero, inserts the new (key,T*) pair into the hash.
    */
/*   template<class T> T* copy(Database & database, NamedEntity const* object, QHash<int,T*>* keyHash, bool displayed = true ) {
      int newKey;
      int i;
      QString holder, fields;
      T* newOne;

      DatabaseConstants::DbTableId t = this->dbDefn.classNameToTable(object->metaObject()->className());
      TableSchema* tbl = this->dbDefn.table(t);

      QString tName = tbl->tableName();

      QSqlQuery q(database.sqlDatabase());

      try {
         QString select = QString("SELECT * FROM %1 WHERE id = %2").arg(tName).arg(object->key());

         qDebug() << Q_FUNC_INFO << "SELECT SQL:" << select;

         if( !q.exec(select) ) {
            throw QString("%1 %2").arg(q.lastQuery()).arg(q.lastError().text());
         }

         qDebug() << Q_FUNC_INFO << "Returned " << q.size() << " rows";

         q.next();

         QSqlRecord oldRecord = q.record();
         q.finish();

         // Get the field names from the oldRecord. But skip ID, because it
         // won't work to copy it
         for (i=0; i< oldRecord.count(); ++i) {
            QString name = oldRecord.fieldName(i);
            if ( name != tbl->keyName() ) {
               fields += fields.isEmpty() ? name : QString(",%1").arg(name);
               holder += holder.isEmpty() ? QString(":%1").arg(name) : QString(",:%1").arg(name);
            }
         }

         // Create a new row.
         QString prepString = QString("INSERT INTO %1 (%2) VALUES(%3)")
                              .arg(tName)
                              .arg(fields)
                              .arg(holder);

         qDebug() << Q_FUNC_INFO << "INSERT SQL:" << prepString;

         QSqlQuery insert = QSqlQuery( database.sqlDatabase() );
         insert.prepare(prepString);

         // Bind, bind like the wind! Or at least like mueslix
         for (i=0; i< oldRecord.count(); ++i)
         {
            QString name = oldRecord.fieldName(i);
            QVariant val = oldRecord.value(i);

            // We have never had an attribute called 'parent'. See the git logs to understand this comment
            if ( name == tbl->propertyToColumn(PropertyNames::NamedEntity::display) ) {
               insert.bindValue(":display", displayed ? Brewken::dbTrue() : Brewken::dbFalse() );
            }
            // Ignore ID again, for the same reasons as before.
            else if ( name != tbl->keyName() ) {
               insert.bindValue(QString(":%1").arg(name), val);
            }
         }

         // For debugging, it's useful to know what the SQL parameters were
         auto boundValues = insert.boundValues();
         for (auto ii : boundValues.keys()) {
            qDebug() << Q_FUNC_INFO << ii << "=" << boundValues.value(ii);
         }

         if (! insert.exec() )
            throw QString("could not execute %1 : %2").arg(insert.lastQuery()).arg(insert.lastError().text());

         newKey = insert.lastInsertId().toInt();
         newOne = new T(t, newKey, oldRecord);
         keyHash->insert( newKey, newOne );
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         q.finish();
         throw;
      }

      q.finish();
      return newOne;
   }
*/
   // Do an sql update.
/*   void sqlUpdate(Database & database, DatabaseConstants::DbTableId table, QString const& setClause, QString const& whereClause ) {
      QString update = QString("UPDATE %1 SET %2 WHERE %3")
                  .arg(this->dbDefn.tableName(table))
                  .arg(setClause)
                  .arg(whereClause);

      QSqlQuery q(database.sqlDatabase());
      try {
         if ( ! q.exec(update) )
            throw QString("Could not execute update %1 : %2").arg(update).arg(q.lastError().text());
      }
      catch (QString e) {
         q.finish();
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         throw QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      }

      q.finish();
   }
*/
   // Do an sql delete.
/*
   void sqlDelete(Database & database, DatabaseConstants::DbTableId table, QString const& whereClause ) {
      QString del = QString("DELETE FROM %1 WHERE %2")
                  .arg(this->dbDefn.tableName(table))
                  .arg(whereClause);

      QSqlQuery q(database.sqlDatabase());
      try {
         if ( ! q.exec(del) )
            throw QString("Could not delete %1 : %2").arg(del).arg(q.lastError().text());
      }
      catch (QString e) {
         q.finish();
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         throw QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      }

      q.finish();
   }
*/
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

/*   QHash< int, BrewNote* > allBrewNotes;
   QHash< int, Equipment* > allEquipments;
   QHash< int, Fermentable* > allFermentables;
   QHash< int, Hop* > allHops;  ////!!!!
   QHash< int, Instruction* > allInstructions;
   QHash< int, Mash* > allMashs;
   QHash< int, MashStep* > allMashSteps;
   QHash< int, Misc* > allMiscs;
   QHash< int, Recipe* > allRecipes;
   QHash< int, Style* > allStyles;
   QHash< int, Water* > allWaters;
   QHash< int, Salt* > allSalts;
   QHash< int, Yeast* > allYeasts;
*/
//   QHash<QString,QSqlQuery> selectSome;


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
///   if( QSqlDatabase::database(this->pimpl->dbConName, false ).isOpen() ) {
      this->unload();
   }

   // Delete all the ingredients floating around.
/*   qDeleteAll(this->pimpl->allBrewNotes);
   qDeleteAll(this->pimpl->allEquipments);
   qDeleteAll(this->pimpl->allFermentables);
   qDeleteAll(this->pimpl->allHops);  ////!!!
   qDeleteAll(this->pimpl->allInstructions);
   qDeleteAll(this->pimpl->allMashSteps);
   qDeleteAll(this->pimpl->allMashs);
   qDeleteAll(this->pimpl->allMiscs);
   qDeleteAll(this->pimpl->allStyles);
   qDeleteAll(this->pimpl->allWaters);
   qDeleteAll(this->pimpl->allSalts);
   qDeleteAll(this->pimpl->allYeasts);
   qDeleteAll(this->pimpl->allRecipes);
*/
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

   // Create and store all pointers.
/*   qDebug() << Q_FUNC_INFO << "Loading objects from DB";
   this->pimpl->populateElements(*this, this->pimpl->allBrewNotes, DatabaseConstants::BREWNOTETABLE );
   this->pimpl->populateElements(*this, this->pimpl->allEquipments, DatabaseConstants::EQUIPTABLE );
   this->pimpl->populateElements(*this, this->pimpl->allFermentables, DatabaseConstants::FERMTABLE );
   this->pimpl->populateElements(*this, this->pimpl->allHops, DatabaseConstants::HOPTABLE ); ////!!!!!
   this->pimpl->populateElements(*this, this->pimpl->allInstructions, DatabaseConstants::INSTRUCTIONTABLE );
   this->pimpl->populateElements(*this, this->pimpl->allMashs, DatabaseConstants::MASHTABLE );
   this->pimpl->populateElements(*this, this->pimpl->allMashSteps, DatabaseConstants::MASHSTEPTABLE );
   this->pimpl->populateElements(*this, this->pimpl->allMiscs, DatabaseConstants::MISCTABLE );
   this->pimpl->populateElements(*this, this->pimpl->allStyles, DatabaseConstants::STYLETABLE );
   this->pimpl->populateElements(*this, this->pimpl->allWaters, DatabaseConstants::WATERTABLE );
   this->pimpl->populateElements(*this, this->pimpl->allSalts, DatabaseConstants::SALTTABLE );
   this->pimpl->populateElements(*this, this->pimpl->allYeasts, DatabaseConstants::YEASTTABLE );

   this->pimpl->populateElements(*this, this->pimpl->allRecipes, DatabaseConstants::RECTABLE );
*/
   qDebug() << Q_FUNC_INFO << "Loading objects from DB - new classes";
   DbNamedEntityRecords<BrewNote>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Equipment>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Fermentable>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Hop>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Instruction>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Mash>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<MashStep>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Misc>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Recipe>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Salt>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Style>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Water>::getInstance().loadAll(this->sqlDatabase());
   DbNamedEntityRecords<Yeast>::getInstance().loadAll(this->sqlDatabase());
   qDebug() << Q_FUNC_INFO << "Objects loaded";

   Recipe::connectSignals();
   qDebug() << Q_FUNC_INFO << "Recipe signals connected";

   Mash::connectSignals();
   qDebug() << Q_FUNC_INFO << "Mash signals connected";

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

   // this->pimpl->selectSome saves context. If we close the database before we tear that
   // context down, core gets dumped
//   this->pimpl->selectSome.clear();

   // so far, it seems we only create one connection to the db. This is
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
void Database::dropInstance()
{
   static QMutex mutex;

   mutex.lock();
   Database::instance().unload();
///   delete dbInstance;
///   dbInstance=nullptr;
   mutex.unlock();
   qDebug() << Q_FUNC_INFO << "Drop Instance done";
   return;

}

char const * Database::getDefaultBackupFileName()
{
    return "database.sqlite";
}

bool Database::backupToFile(QString newDbFileName)
{
   // Make sure the singleton exists - otherwise there's nothing to backup.
   instance();

   bool success = true;

   // Remove the files if they already exist so that
   // the copy() operation will succeed.
   QFile::remove(newDbFileName);

   success = dbFile.copy( newDbFileName );

   qDebug() << QString("Database backup to \"%1\" %2").arg(newDbFileName, success ? "succeeded" : "failed");

   return success;
}

bool Database::backupToDir(QString dir,QString filename)
{
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
   if( !newDbFile.exists() )
      return false;

   success &= newDbFile.copy(QString("%1.new").arg(dbFile.fileName()));
   QFile::setPermissions( newDbFile.fileName(), QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup );

   return success;
}

int Database::getParentNamedEntityKey(NamedEntity const & ingredient) {
   int parentKey = 0;

   const QMetaObject* meta = ingredient.metaObject();

   DatabaseConstants::DbTableId parentToChildTableId =
      this->pimpl->dbDefn.table(
         this->pimpl->dbDefn.classNameToTable(meta->className())
      )->childTable();

   // Don't do this if no child table is defined (like instructions)
   if (parentToChildTableId != DatabaseConstants::NOTABLE) {
      TableSchema * parentToChildTable = this->pimpl->dbDefn.table(parentToChildTableId);

      QString findParentNamedEntity =
         QString("SELECT %1 FROM %2 WHERE %3=%4").arg(parentToChildTable->parentIndexName())
                                                 .arg(parentToChildTable->tableName())
                                                 .arg(parentToChildTable->childIndexName())
                                                 .arg(ingredient.key());
      qDebug() << Q_FUNC_INFO << "Find Parent NamedEntity SQL: " << findParentNamedEntity;

      QSqlQuery query(sqlDatabase());
      if (!query.exec(findParentNamedEntity)) {
         throw QString("Database error trying to find parent ingredient.");
      }

      if (query.next()) {
         parentKey = query.record().value(parentToChildTable->parentIndexName()).toInt();
         qDebug() << QString("Found Parent with Key: %1").arg(parentKey);
      }
   }
   return parentKey;
}


bool Database::isStored(NamedEntity const & ingredient) {
   // Valid database keys are all positive
   if (ingredient.key() <= 0) {
      return false;
   }

   const QMetaObject* meta = ingredient.metaObject();

   TableSchema * table = this->pimpl->dbDefn.table(this->pimpl->dbDefn.classNameToTable(meta->className()));

   QString idColumnName = table->keyName(Brewken::dbType());

   QString queryString = QString("SELECT %1 AS id FROM %2 WHERE %3=%4").arg(idColumnName)
                                                                       .arg(table->tableName())
                                                                       .arg(idColumnName)
                                                                       .arg(ingredient.key());

   qDebug() << QString("%1 SQL: %2").arg(Q_FUNC_INFO).arg(queryString);

   QSqlQuery query(sqlDatabase());

   try {
      if ( ! query.exec(queryString) )
         throw QString("could not execute query: %2 : %3").arg(queryString).arg(query.lastError().text());
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      query.finish();
      throw;
   }

   bool foundInDatabase = query.next();

   query.finish();
   return foundInDatabase;
}


// removeFromRecipe ===========================================================
/*NamedEntity * Database::removeNamedEntityFromRecipe( Recipe* rec, NamedEntity* ing )
{
   NamedEntity * parentNamedEntity = ing->getParent();

   const QMetaObject* meta = ing->metaObject();
   TableSchema *table;
   TableSchema *child;
   TableSchema *inrec;

   int ndx = meta->indexOfClassInfo("signal");
   QString propName;

   sqlDatabase().transaction();
   QSqlQuery q(sqlDatabase());

   qDebug() << QString("%1 Deleting NamedEntity %2 #%3").arg(Q_FUNC_INFO).arg(meta->className()).arg(ing->key());

   try {
      if ( ndx != -1 ) {
         propName  = meta->classInfo(ndx).value();
      }
      else {
         throw QString("could not locate classInfo for signal on %2").arg(meta->className());
      }

      table = this->pimpl->dbDefn.table( this->pimpl->dbDefn.classNameToTable(meta->className()) );
      child = this->pimpl->dbDefn.table( table->childTable() );
      inrec = this->pimpl->dbDefn.table( table->inRecTable() );
      // We need to do many things -- remove the link in *in_recipe,
      // remove the entry from *_children
      // and DELETE THE COPY
      // delete from misc_in_recipe where misc_id = [misc key] and recipe_id = [rec key]
      QString deleteFromInRecipe = QString("DELETE FROM %1 WHERE %2=%3 AND %4=%5")
                                 .arg(inrec->tableName() )
                                 .arg(inrec->inRecIndexName())
                                 .arg(ing->key())
                                 .arg(inrec->recipeIndexName())
                                 .arg(rec->key());
      qDebug() << QString("Delete From In Recipe SQL: %1").arg(deleteFromInRecipe);

      // delete from misc where id = [misc key]
      QString deleteNamedEntity = QString("DELETE FROM %1 where %2=%3")
                                 .arg(table->tableName())
                                 .arg(table->keyName())
                                 .arg(ing->key());
      qDebug() << QString("Delete NamedEntity SQL: %1").arg(deleteNamedEntity);

      q.setForwardOnly(true);

      if (parentNamedEntity) {

         // delete from misc_child where child_id = [misc key]
         QString deleteFromChildren = QString("DELETE FROM %1 WHERE %2=%3")
                                    .arg(child->tableName())
                                    .arg( child->childIndexName() )
                                    .arg(ing->key());
         qDebug() << QString("Delete From Children SQL: %1").arg(deleteFromChildren);
         if ( ! q.exec( deleteFromChildren ) ) {
            throw QString("failed to delete children.");
         }
      }

      if ( ! q.exec(deleteFromInRecipe) )
         throw QString("failed to delete in_recipe.");

      if ( ! q.exec( deleteNamedEntity ) )
         throw QString("failed to delete ingredient.");

   }
   catch ( QString e ) {
      qCritical() << QString("%1 %2 %3 %4")
                           .arg(Q_FUNC_INFO)
                           .arg(e)
                           .arg(q.lastQuery())
                           .arg(q.lastError().text());
      sqlDatabase().rollback();
      q.finish();
      abort();
   }

   rec->recalcAll();
   sqlDatabase().commit();

   q.finish();
   emit rec->changed( rec->metaProperty(propName), QVariant() );

   return ing;
}

void Database::removeFromRecipe( Recipe* rec, Instruction* ins )
{
   qDebug() << QString("%1").arg(Q_FUNC_INFO);

   try {
      removeNamedEntityFromRecipe( rec, ins);
   }
   catch (QString e) {
      throw; //up the stack!
   }

   this->pimpl->allInstructions.remove(ins->key());
   emit changed( metaProperty(*this, "instructions"), QVariant() );
}
*/
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*void Database::removeFrom( Mash* mash, MashStep* step )
{
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::MASHSTEPTABLE);
   // Just mark the step as deleted.
   try {
      this->pimpl->sqlUpdate(*this, DatabaseConstants::MASHSTEPTABLE,
               QString("%1 = %2").arg(tbl->propertyToColumn(PropertyNames::NamedEntity::deleted)).arg(Brewken::dbTrue()),
               QString("%1 = %2").arg(tbl->keyName()).arg(step->key()));
   }
   catch ( QString e ) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      throw;
   }

   emit mash->mashStepsChanged();
}
*/
/*
Recipe* Database::getParentRecipe(NamedEntity const * ing) {

   QMetaObject const * meta = ing->metaObject();
   TableSchema* table = this->pimpl->dbDefn.table( this->pimpl->dbDefn.classNameToTable(meta->className()) );
   TableSchema* inrec = this->pimpl->dbDefn.table( table->inRecTable() );

   QString select = QString("SELECT %4 from %1 WHERE %2=%3")
                        .arg(inrec->tableName())
                        .arg(inrec->inRecIndexName())
                        .arg(ing->key())
                        .arg(inrec->recipeIndexName());
   qDebug() << Q_FUNC_INFO << "NamedEntity in recipe search:" << select;
   QSqlQuery q(sqlDatabase());
   if (! q.exec(select) ) {
      throw QString("Couldn't execute ingredient in recipe search: Query: %1 error: %2")
         .arg(q.lastQuery()).arg(q.lastError().text());
   }

   Recipe * parent = nullptr;

   if ( q.next() ) {
      int key = q.record().value(inrec->recipeIndexName()).toInt();
      parent = this->pimpl->allRecipes[key];
   }

   q.finish();
   return parent;
}

Recipe* Database::getParentRecipe( BrewNote const* note ) {

   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::BREWNOTETABLE);
   // SELECT recipe_id FROM brewnote WHERE id = [key]
   QString query = QString("SELECT %1 FROM %2 WHERE %3 = %4")
           .arg( tbl->recipeIndexName())
           .arg( tbl->tableName() )
           .arg( tbl->keyName() )
           .arg(note->key());

   QSqlQuery q(sqlDatabase());

   try {
      if ( ! q.exec(query) )
         throw QString("could not find recipe id");
   }
   catch ( QString e ) {
      qCritical() << QString("%1 %2 %3 %4")
                           .arg(Q_FUNC_INFO)
                           .arg(e)
                           .arg(q.lastQuery())
                           .arg(q.lastError().text());
      q.finish();
      throw;
   }

   q.next();
   int key = q.record().value(tbl->recipeIndexName()).toInt();
   q.finish();

   return this->pimpl->allRecipes[key];
}*/

/*
Recipe*      Database::recipe(int key)      { return this->pimpl->allRecipes[key]; }
Equipment*   Database::equipment(int key)   { return this->pimpl->allEquipments[key]; }
Fermentable* Database::fermentable(int key) { return this->pimpl->allFermentables[key]; }
Hop*         Database::hop(int key)         { return this->pimpl->allHops[key]; }
Hop*         Database::hop(int key)         {
   auto result = DbNamedEntityRecords<Hop>::getInstance().getById(key);
   return result ? result.value().get() : nullptr;
}

Misc*        Database::misc(int key)        { return this->pimpl->allMiscs[key]; }
Style*       Database::style(int key)       { return this->pimpl->allStyles[key]; }
Yeast*       Database::yeast(int key)       { return this->pimpl->allYeasts[key]; }
Salt*        Database::salt(int key)        { return this->pimpl->allSalts[key]; }
*/
/*void Database::swapMashStepOrder(MashStep* m1, MashStep* m2)
{
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::MASHSTEPTABLE);
   // maybe this wasn't such a good idear?
   // UPDATE mashstep SET step_number = CASE id
   //    WHEN [m1->key] then [m2->stepnumber]
   //    WHEN [m2->key] then [m1->stepnumber]
   // END
   // WHERE id IN ([m1->key],[m2->key])
   QString update = QString("UPDATE %1 SET %2 = CASE %3 "
                               "WHEN %4 then %5 "
                               "WHEN %6 then %7 "
                            "END "
                            "WHERE %3 IN (%4,%6)")
                .arg(tbl->tableName() )
                .arg(tbl->propertyToColumn(PropertyNames::MashStep::stepNumber))
                .arg(tbl->keyName())
                .arg(m1->key())
                .arg(m2->stepNumber())
                .arg(m2->key())
                .arg(m1->stepNumber());

   QSqlQuery q(sqlDatabase() );

   try {
      if ( !q.exec(update) )
         throw QString("failed to swap steps");
   }
   catch ( QString e ) {
      qCritical() << QString("%1 %2 %3 %4")
                           .arg(Q_FUNC_INFO)
                           .arg(e)
                           .arg(q.lastQuery())
                           .arg(q.lastError().text());
      q.finish();
      throw;
   }

   q.finish();

   emit m1->changed( m1->metaProperty(PropertyNames::MashStep::stepNumber) );
   emit m2->changed( m2->metaProperty(PropertyNames::MashStep::stepNumber) );
}

void Database::swapInstructionOrder(Instruction* in1, Instruction* in2)
{
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::INSTINRECTABLE);

   // UPDATE instruction_in_recipe SET instruction_number = CASE instruction_id
   //    WHEN [in1->key] THEN [in2->instructionNumber]
   //    WHEN [in2->key] THEN [in1->instructionNumber]
   // END
   // WHERE instruction_id IN ([in1->key],[in2->key])
   QString update =
      QString( "UPDATE %1 SET %2 = CASE %3 "
                  "WHEN %4 THEN %5 "
                  "WHEN %6 THEN %7 "
               "END "
               "WHERE %3 IN (%4,%6)")
      .arg(tbl->tableName())
      .arg(tbl->propertyToColumn(kpropInstructionNumber))
      .arg(tbl->inRecIndexName())
      .arg(in1->key())
      .arg(in2->instructionNumber())
      .arg(in2->key())
      .arg(in1->instructionNumber());

   QSqlQuery q( sqlDatabase());

   try {
      if ( !q.exec(update) )
         throw QString("failed to swap steps");
   }
   catch ( QString e ) {
      qCritical() << QString("%1 %2 %3 %4")
                           .arg(Q_FUNC_INFO)
                           .arg(e)
                           .arg(q.lastQuery())
                           .arg(q.lastError().text());
      q.finish();
      throw;
   }

   q.finish();

   emit in1->changed( in1->metaProperty("instructionNumber") );
   emit in2->changed( in2->metaProperty("instructionNumber") );
}

void Database::insertInstruction(Instruction* in, int pos)
{
   int parentRecipeKey;
   TableSchema* tbl = this->pimpl->dbDefn.table( DatabaseConstants::INSTINRECTABLE );
   // SELECT recipe_id FROM instruction_in_recipe WHERE instruction_id=[key]
   QString query = QString("SELECT %1 FROM %2 WHERE %3=%4")
                   .arg( tbl->recipeIndexName())
                   .arg( tbl->tableName() )
                   .arg( tbl->inRecIndexName() )
                   .arg(in->key());
   QString update;

   sqlDatabase().transaction();

   QSqlQuery q(sqlDatabase());

   try {
      if ( !q.exec(query) )
         throw QString("failed to find recipe");

      q.next();
      parentRecipeKey = q.record().value(tbl->recipeIndexName()).toInt();
      q.finish();

      // this happens in three steps --
      // 1. Bump the instruction number for any instruction >= where we need to insert the new one
      // 2. Generate all the signals for what we just did
      // 3. Insert the instruction into the desired slot.

      // Increment all instruction positions greater or equal to pos.
      // update instruction_in_recipe set instruction_number = instruction_number + 1 where recipe_id = [key] and instruction_number > [pos]
      update = QString( "UPDATE %1 SET %2=%2+1 WHERE %3=%4 AND %2>=%5")
         .arg( tbl->tableName() )
         .arg( tbl->propertyToColumn(kpropInstructionNumber) )
         .arg( tbl->recipeIndexName() )
         .arg(parentRecipeKey)
         .arg(pos);
      qDebug() << Q_FUNC_INFO << "Update 1 SQL:" << update;

      if ( !q.exec(update) )
         throw QString("failed to renumber instructions recipe");

      // Emit the signals for everything we just changed.
      // SELECT instruction_id, instruction_number FROM instruction_in_recipe WHERE recipe_id=[key] and instruction_number>[pos]")
      query = QString("SELECT %1, %2 FROM %3 WHERE %4=%5 and %2>%6")
         .arg( tbl->inRecIndexName() )
         .arg( tbl->propertyToColumn(kpropInstructionNumber) )
         .arg( tbl->tableName() )
         .arg( tbl->recipeIndexName())
         .arg(parentRecipeKey)
         .arg(pos);
      qDebug() << Q_FUNC_INFO << "Query SQL:" << query;

      if ( !q.exec(query) )
         throw QString("failed to find renumbered instructions");

      while( q.next() ) {
         Instruction* inst = this->pimpl->allInstructions[q.record().value(tbl->inRecIndexName()).toInt() ];
         int newPos = q.record().value(tbl->propertyToColumn(kpropInstructionNumber)).toInt();

         emit inst->changed( inst->metaProperty("instructionNumber"),newPos );
      }

      // Insert the instruction into the new place
      // UPDATE instruction_in_recipe set instruction_number = [pos] where instruction_id=[key]
      update = QString( "UPDATE %1 SET %2=%3 WHERE %4=%5")
         .arg(tbl->tableName())
         .arg(tbl->propertyToColumn(kpropInstructionNumber))
         .arg(pos)
         .arg( tbl->inRecIndexName() )
         .arg(in->key());
      qDebug() << Q_FUNC_INFO << "Update 2 SQL:" << update;

      if ( !q.exec(update) )
         throw QString("failed to insert new instruction recipe");
   }
   catch ( QString e ) {
      qCritical() << QString("%1 %2 %3 %4")
                           .arg(Q_FUNC_INFO)
                           .arg(e)
                           .arg(q.lastQuery())
                           .arg(q.lastError().text());
      q.finish();
      sqlDatabase().rollback();
      throw;
   }

   sqlDatabase().commit();
   q.finish();

   emit in->changed( in->metaProperty("instructionNumber"), pos );
}
*/
/*
QList<BrewNote*> Database::brewNotes(Recipe const* parent)
{
   QList<BrewNote*> ret;
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::BREWNOTETABLE);

   //  recipe_id = [parent->key] AND deleted = false
   QString filterString = QString("%1 = %2 AND %3 = %4")
           .arg( tbl->recipeIndexName() )
           .arg(parent->key())
           .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());

   this->pimpl->getElements(*this, ret, filterString, DatabaseConstants::BREWNOTETABLE, this->pimpl->allBrewNotes);

   return ret;
}

QList<Fermentable*> Database::fermentables(Recipe const* parent)
{
   QList<Fermentable*> ret;
   TableSchema* inrec = this->pimpl->dbDefn.table(DatabaseConstants::FERMINRECTABLE);
   // recipe_id = [parent->key]
   QString filter = QString("%1 = %2").arg(inrec->recipeIndexName()).arg(parent->key());

   this->pimpl->getElements(*this, ret,filter, DatabaseConstants::FERMINRECTABLE, this->pimpl->allFermentables, inrec->inRecIndexName());

   return ret;
}


QList<Hop*> Database::hops(Recipe const* parent)
{
   QList<Hop*> ret;
   TableSchema* inrec = this->pimpl->dbDefn.table(DatabaseConstants::HOPINRECTABLE);
   QString filter = QString("%1 = %2").arg(inrec->recipeIndexName()).arg(parent->key());

   this->pimpl->getElements(*this, ret,filter, DatabaseConstants::HOPINRECTABLE, this->pimpl->allHops, inrec->inRecIndexName());

   return ret;
}


QList<Misc*> Database::miscs(Recipe const* parent)
{
   QList<Misc*> ret;
   TableSchema* inrec = this->pimpl->dbDefn.table(DatabaseConstants::MISCINRECTABLE);
   QString filter = QString("%1 = %2").arg(inrec->recipeIndexName()).arg(parent->key());

   this->pimpl->getElements(*this, ret,filter, DatabaseConstants::MISCINRECTABLE, this->pimpl->allMiscs, inrec->inRecIndexName());

   return ret;
}

Equipment* Database::equipment(Recipe const* parent)
{
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE);
   int id = get( tbl, parent->key(), tbl->foreignKeyToColumn(kpropEquipmentId)).toInt();

   if( this->pimpl->allEquipments.contains(id) )
      return this->pimpl->allEquipments[id];
   else
      return nullptr;
}

Style* Database::style(Recipe const* parent)
{
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE);
   int id = get( tbl, parent->key(), tbl->foreignKeyToColumn(kpropStyleId)).toInt();

   if( this->pimpl->allStyles.contains(id) )
      return this->pimpl->allStyles[id];
   else
      return nullptr;
}

Style* Database::styleById(int styleId )
{
   if( this->pimpl->allStyles.contains(styleId) )
      return this->pimpl->allStyles[styleId];
   else
      return nullptr;
}

Mash* Database::mash( Recipe const* parent )
{
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE);
   int mashId = get( tbl, parent->key(), tbl->foreignKeyToColumn(kpropMashId)).toInt();

   if( this->pimpl->allMashs.contains(mashId) )
      return this->pimpl->allMashs[mashId];
   else
      return nullptr;
}

QList<MashStep*> Database::mashSteps(Mash const* parent)
{
   QList<MashStep*> ret;
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::MASHSTEPTABLE);

   // mash_id = [parent->key] AND deleted = false order by step_number ASC
   QString filterString = QString("%1 = %2 AND %3 = %4 order by %5 ASC")
         .arg(tbl->foreignKeyToColumn())
         .arg(parent->key())
         .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::deleted))
         .arg(Brewken::dbFalse())
         .arg(tbl->propertyToColumn(PropertyNames::MashStep::stepNumber));

   this->pimpl->getElements(*this, ret, filterString, DatabaseConstants::MASHSTEPTABLE, this->pimpl->allMashSteps);

   return ret;
}

QList<Instruction*> Database::instructions( Recipe const* parent )
{
   QList<Instruction*> ret;
   TableSchema* inrec = this->pimpl->dbDefn.table(DatabaseConstants::INSTINRECTABLE);
   // recipe_id = [parent->key] ORDER BY instruction_number ASC
   QString filter = QString("%1 = %2 ORDER BY %3 ASC")
         .arg( inrec->recipeIndexName())
         .arg(parent->key())
         .arg( inrec->propertyToColumn(kpropInstructionNumber));

   this->pimpl->getElements(*this, ret,filter,DatabaseConstants::INSTINRECTABLE,this->pimpl->allInstructions,inrec->inRecIndexName());

   return ret;
}

QList<Water*> Database::waters(Recipe const* parent)
{
   QList<Water*> ret;
   TableSchema* inrec = this->pimpl->dbDefn.table(DatabaseConstants::WATERINRECTABLE);
   QString filter = QString("%1 = %2").arg(inrec->recipeIndexName()).arg(parent->key());

   this->pimpl->getElements(*this, ret,filter,DatabaseConstants::WATERINRECTABLE,this->pimpl->allWaters,inrec->inRecIndexName());

   return ret;
}

QList<Salt*> Database::salts(Recipe const* parent)
{
   QList<Salt*> ret;
   TableSchema* inrec = this->pimpl->dbDefn.table(DatabaseConstants::SALTINRECTABLE);
   QString filter = QString("%1 = %2").arg(inrec->recipeIndexName()).arg(parent->key());

   this->pimpl->getElements(*this, ret,filter,DatabaseConstants::SALTINRECTABLE,this->pimpl->allSalts,inrec->inRecIndexName());

   return ret;
}

QList<Yeast*> Database::yeasts(Recipe const* parent)
{
   QList<Yeast*> ret;
   TableSchema* inrec = this->pimpl->dbDefn.table(DatabaseConstants::YEASTINRECTABLE);
   QString filter = QString("%1 = %2").arg(inrec->recipeIndexName()).arg(parent->key());

   this->pimpl->getElements(*this, ret,filter,DatabaseConstants::YEASTINRECTABLE,this->pimpl->allYeasts,inrec->inRecIndexName());

   return ret;
}
*/
// Named constructors =========================================================
/*
BrewNote* Database::newBrewNote(BrewNote* other, bool signal)
{
   BrewNote* tmp = this->pimpl->copy<BrewNote>(*this, other, &this->pimpl->allBrewNotes);

   if ( tmp && signal ) {
      emit changed( metaProperty(*this, "brewNotes"), QVariant() );
      emit newBrewNoteSignal(tmp);
   }

   return tmp;
}

BrewNote* Database::newBrewNote(Recipe* parent, bool signal)
{
   BrewNote* tmp;

   sqlDatabase().transaction();

   try {
      tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allBrewNotes);
      TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::BREWNOTETABLE);

      this->pimpl->sqlUpdate(*this, DatabaseConstants::BREWNOTETABLE,
               QString("%1=%2").arg(tbl->recipeIndexName()).arg(parent->key()),
               QString("%1=%2").arg(tbl->keyName()).arg(tmp->key()) );

   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   sqlDatabase().commit();
   tmp->setDisplay(true);
   if ( signal )
   {
      emit changed( metaProperty(*this, "brewNotes"), QVariant() );
      emit newBrewNoteSignal(tmp);
   }

   return tmp;
}

Equipment* Database::newEquipment(Equipment* other)
{
   Equipment* tmp;

   if (other)
      tmp = this->pimpl->copy(*this, other, &this->pimpl->allEquipments);
   else
      tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allEquipments);

   if ( tmp ) {
      emit changed( metaProperty(*this, "equipments"), QVariant() );
      emit newEquipmentSignal(tmp);
   }
   else {
      qCritical() << QString("%1 couldn't copy %2").arg(Q_FUNC_INFO).arg(other->name());
   }

   return tmp;
}

Fermentable* Database::newFermentable(Fermentable* other)
{
   Fermentable* tmp;
   bool transact = false;

   try {
      // copies automatically get their inventory_id properly set
      if (other) {
         tmp = this->pimpl->copy(*this, other, &this->pimpl->allFermentables);
      }
      else {
         // new ingredients don't. this gets ugly fast, because we are now
         // writing to two tables and need some transactional protection
         sqlDatabase().transaction();
         transact = true;
         tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allFermentables);
         int invkey = newInventory( this->pimpl->dbDefn.table(DatabaseConstants::FERMTABLE));
         tmp->setInventoryId(invkey);
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact ) sqlDatabase().rollback();
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
   }
   if ( tmp ) {
      emit changed( metaProperty(*this, "fermentables"), QVariant() );
      emit newFermentableSignal(tmp);
   }
   else {
      qCritical() << QString("%1 couldn't copy %2").arg(Q_FUNC_INFO).arg(other->name());
   }

   return tmp;
}

Hop* Database::newHop(Hop* other)
{
   Hop* tmp;
   bool transact = false;

   try {
      if ( other ) {
         tmp = this->pimpl->copy(*this, other, &this->pimpl->allHops);
      }
      else {
         sqlDatabase().transaction();
         transact = true;
         tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allHops);
         int invkey = newInventory( this->pimpl->dbDefn.table(DatabaseConstants::HOPTABLE));
         tmp->setInventoryId(invkey);
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact ) sqlDatabase().rollback();
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
   }

   if ( tmp ) {
      emit changed( metaProperty(*this, "hops"), QVariant() );
      emit newHopSignal(tmp);
   }
   else {
      qCritical() << QString("%1 could not %2 hop")
            .arg(Q_FUNC_INFO)
            .arg( other ? "copy" : "create");
   }

   return tmp;
}

Instruction* Database::newInstruction(Recipe* rec)
{
   Instruction* tmp;

   sqlDatabase().transaction();

   try {
      tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allInstructions);
      tmp->setRecipe(rec);

      // Add without copying to "instruction_in_recipe". We already have a
      // transaction open, so tell addIng to not worry about it
      tmp = this->pimpl->addNamedEntityToRecipe<Instruction>(*this, rec,tmp,true,nullptr,false,false);
   }
   catch ( QString e ) {
      qCritical() << QString("%1 %2").arg( Q_FUNC_INFO ).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   // Database's instructions have changed.
   sqlDatabase().commit();
   emit changed( metaProperty(*this, "instructions"), QVariant() );

   return tmp;
}

// needs fixed
int Database::instructionNumber(Instruction const* in)
{
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::INSTINRECTABLE);
   QString colName = tbl->propertyToColumn(kpropInstructionNumber);
   // SELECT instruction_number FROM instruction_in_recipe WHERE instruction_id=[in->key]
   QString query = QString("SELECT %1 FROM %2 WHERE %3=%4")
         .arg(colName)
         .arg(tbl->tableName())
         .arg(tbl->inRecIndexName())
         .arg(in->key());

   QSqlQuery q(query,sqlDatabase());

   if( q.next() )
      return q.record().value(colName).toInt();
   else
      return 0;
}

Mash* Database::newMash(Mash* other, bool displace)
{
   Mash* tmp;

   try {
      if ( other ) {
         sqlDatabase().transaction();
         tmp = this->pimpl->copy<Mash>(*this, other, &this->pimpl->allMashs);
      }
      else {
         tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allMashs);
      }

      if ( other ) {
         // Just copying the Mash isn't enough. We need to copy the mashsteps too
         duplicateMashSteps(other,tmp);

         // Connect tmp to parent, removing any existing mash in parent.
         // This doesn't really work. It simply orphans the old mash and its
         // steps.
         if( displace ) {
            TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE);
            this->pimpl->sqlUpdate(*this, DatabaseConstants::RECTABLE,
                       QString("%1=%2").arg(tbl->foreignKeyToColumn(kpropMashId)).arg(tmp->key()),
                       QString("%1=%2").arg(tbl->foreignKeyToColumn(kpropMashId)).arg(other->key()));
         }
      }
   }
   catch (QString e) {
      if ( other )
         sqlDatabase().rollback();
      throw;
   }

   if ( other ) {
      sqlDatabase().commit();
   }

   emit changed( metaProperty(*this, "mashs"), QVariant() );
   emit newMashSignal(tmp);

   return tmp;
}

Mash* Database::newMash(Recipe* parent, bool transact)
{
   Mash* tmp;

   if ( transact ) {
      sqlDatabase().transaction();
   }

   try {
      tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allMashs);

      // Connect tmp to parent, removing any existing mash in parent.
      TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE);
      this->pimpl->sqlUpdate(*this, DatabaseConstants::RECTABLE,
                 QString("%1=%2").arg(tbl->foreignKeyToColumn(kpropMashId)).arg(tmp->key()),
                 QString("%1=%2").arg(tbl->keyName()).arg(parent->key()));
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact )
         sqlDatabase().rollback();
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
   }

   emit changed( metaProperty(*this, "mashs"), QVariant() );
   emit newMashSignal(tmp);

   connect( tmp, SIGNAL(changed(QMetaProperty,QVariant)), parent, SLOT(acceptMashChange(QMetaProperty,QVariant)) );
   return tmp;
}

// If we are doing triggers for instructions, why aren't we doing triggers for
// mash steps?
MashStep* Database::newMashStep(Mash* mash, bool connected)
{
   // NOTE: we have unique(mash_id,step_number) constraints on this table,
   // so may have to pay special attention when creating the new record.
   MashStep* tmp;
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::MASHSTEPTABLE);
   // step_number = (SELECT COALESCE(MAX(step_number)+1,0) FROM mashstep
   // WHERE deleted=false AND mash_id=[mash->key] )
   QString coalesce = QString( "%1 = (SELECT COALESCE(MAX(%1)+1,0) FROM %2 "
                                      "WHERE %3=%4 AND %5=%6 )")
                        .arg(tbl->propertyToColumn(PropertyNames::MashStep::stepNumber))
                        .arg(tbl->tableName())
                        .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::deleted))
                        .arg(Brewken::dbFalse())
                        .arg(tbl->foreignKeyToColumn())
                        .arg(mash->key());

   sqlDatabase().transaction();

   QSqlQuery q(sqlDatabase());
   q.setForwardOnly(true);

   // mashsteps are weird, because we have to do the linking between step and
   // mash
   try {
      tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allMashSteps);

      // we need to set the mash_id first
      this->pimpl->sqlUpdate(*this, DatabaseConstants::MASHSTEPTABLE,
                 QString("%1=%2 ").arg(tbl->foreignKeyToColumn()).arg(mash->key()),
                 QString("%1=%2").arg(tbl->keyName()).arg(tmp->key())
               );

      // Just sets the step number within the mash to the next available number.
      // we need coalesce here instead of isnull. coalesce is SQL standard, so
      // should be more widely supported than isnull
      this->pimpl->sqlUpdate(*this, DatabaseConstants::MASHSTEPTABLE,
                 coalesce,
                 QString("%1=%2").arg(tbl->keyName()).arg(tmp->key()));
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   sqlDatabase().commit();

   if ( connected )
      connect( tmp, SIGNAL(changed(QMetaProperty,QVariant)), mash, SLOT(acceptMashStepChange(QMetaProperty,QVariant)) );

   emit changed( metaProperty(*this, "mashs"), QVariant() );
   emit mash->mashStepsChanged();
   return tmp;
}

Misc* Database::newMisc(Misc* other)
{
   Misc* tmp;
   bool transact = false;

   try {
      if ( other ) {
        tmp = this->pimpl->copy(*this, other, &this->pimpl->allMiscs);
      }
      else {
         sqlDatabase().transaction();
         transact = true;
         tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allMiscs);
         int invkey = newInventory( this->pimpl->dbDefn.table(DatabaseConstants::MISCTABLE));
         tmp->setInventoryId(invkey);
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact ) sqlDatabase().rollback();
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
   }

   if ( tmp ) {
      emit changed( metaProperty(*this, "miscs"), QVariant() );
      emit newMiscSignal(tmp);
   }
   else {
      qCritical() << QString("%1 could not %2 misc")
            .arg(Q_FUNC_INFO)
            .arg( other ? "copy" : "create");
   }

   return tmp;
}

Recipe* Database::newRecipe(QString name)
{
   Recipe* tmp;

   sqlDatabase().transaction();

   try {
      tmp = this->pimpl->newNamedEntity(*this, name,&this->pimpl->allRecipes);

      newMash(tmp,false);
   }
   catch (QString e ) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   try {
      // setting it in the DB doesn't set it in the cache. This makes sure the
      // name is in the cache before we throw the signal
      tmp->setName(name,true);
      tmp->setDisplay(true);
      tmp->setDeleted(false);
   }
   catch (QString e ) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      throw;
   }

   sqlDatabase().commit();
   emit changed( metaProperty(*this, "recipes"), QVariant() );
   emit newRecipeSignal(tmp);

   return tmp;
}

Recipe* Database::newRecipe(Recipe* other)
{
   Recipe* tmp;

   sqlDatabase().transaction();
   try {
      tmp = this->pimpl->copy<Recipe>(*this, other, &this->pimpl->allRecipes);

      // Copy fermentables, hops, miscs and yeasts. We've the convenience
      // methods, so use them? And now I have to instruct all of them to not do
      // transactions either. -4 SAN points!
      addToRecipe( tmp, other->fermentables(), false);
      addToRecipe( tmp, other->hops(), false);
      addToRecipe( tmp, other->miscs(), false);
      addToRecipe( tmp, other->yeasts(), false);

      // Copy style/mash/equipment
      // Style or equipment might be non-existent but these methods handle that.
      addToRecipe( tmp, other->equipment(), false, false);
      addToRecipe( tmp, other->mash(), false, false);
      addToRecipe( tmp, other->style(), false, false);
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   sqlDatabase().commit();
   emit changed( metaProperty(*this, "recipes"), QVariant() );
   emit newRecipeSignal(tmp);

   return tmp;
}

Style* Database::newStyle(Style* other)
{
   Style* tmp;

   try {
      tmp = this->pimpl->copy(*this, other, &this->pimpl->allStyles);
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   emit changed( metaProperty(*this, "styles"), QVariant() );
   emit newStyleSignal(tmp);

   return tmp;
}

Style* Database::newStyle(QString name)
{
   Style* tmp;

   try {
      tmp = this->pimpl->newNamedEntity(*this, name, &this->pimpl->allStyles);
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   try {
      // setting it in the DB doesn't set it in the cache. This makes sure the
      // name is in the cache before we throw the signal
      tmp->setName(name,true);
      tmp->setDisplay(true);
      tmp->setDeleted(false);
   }

   catch (QString e ) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      throw;
   }

   emit changed( metaProperty(*this, "styles"), QVariant() );
   emit newStyleSignal(tmp);

   return tmp;
}

Water* Database::newWater(Water* other)
{
   Water* tmp;

   try {
      if ( other )
         tmp = this->pimpl->copy(*this, other,&this->pimpl->allWaters);
      else
         tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allWaters);
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   emit changed( metaProperty(*this, "waters"), QVariant() );
   emit newWaterSignal(tmp);

   return tmp;
}

Salt* Database::newSalt(Salt* other)
{
   Salt* tmp;

   try {
      if ( other )
         tmp = this->pimpl->copy(*this, other,&this->pimpl->allSalts);
      else
         tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allSalts);
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   emit changed( metaProperty(*this, "salts"), QVariant() );
   emit newSaltSignal(tmp);

   return tmp;
}

Yeast* Database::newYeast(Yeast* other)
{
   Yeast* tmp;
   bool transact = false;

   try {
      if (other) {
         tmp = this->pimpl->copy(*this, other, &this->pimpl->allYeasts);
      }
      else {
         sqlDatabase().transaction();
         transact = true;
         tmp = this->pimpl->newNamedEntity(*this, &this->pimpl->allYeasts);
         int invkey = newInventory( this->pimpl->dbDefn.table(DatabaseConstants::YEASTTABLE));
         tmp->setInventoryId(invkey);
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
   }
   emit changed( metaProperty(*this, "yeasts"), QVariant() );
   emit newYeastSignal(tmp);

   return tmp;
}

int Database::insertElement(NamedEntity * ins)
{
   // Check whether this ingredient is already in the DB.  If so, bail here.
   if (this->isStored(*ins)) {
      qDebug() << Q_FUNC_INFO << "Already stored";
      return ins->key();
   }

   int key;
   QSqlQuery q( sqlDatabase() );

   TableSchema* schema = this->pimpl->dbDefn.table(ins->table());
   QString insertQ = schema->generateInsertProperties(Brewken::dbType());
   QStringList allProps = schema->allProperties();

   qDebug() << Q_FUNC_INFO << "SQL:" << insertQ;
   q.prepare(insertQ);

   QString sqlParameters;
   QTextStream sqlParametersConcat(&sqlParameters);
   foreach (QString prop, allProps) {
      QString pname = schema->propertyName(prop);
      QVariant val_to_ins = ins->property(pname.toUtf8().data());
      if ( ins->table() == DatabaseConstants::BREWNOTETABLE && prop == PropertyNames::BrewNote::brewDate ) {
         val_to_ins = val_to_ins.toString();
      }
      // I've arranged it such that the bindings are on the property names. It simplifies a lot
      q.bindValue( QString(":%1").arg(prop), val_to_ins);
      sqlParametersConcat << prop << " = " << val_to_ins.toString() << " || ";
   }
   qDebug() << Q_FUNC_INFO << "SQL Parameters: " << *sqlParametersConcat.string();

   try {
      if ( ! q.exec() ) {
         throw QString("could not insert a record into %1: %2")
               .arg(schema->tableName())
               .arg(insertQ);
      }

      qDebug() << Q_FUNC_INFO << "Query succeeded";
      key = q.lastInsertId().toInt();
      q.finish();
   }
   catch (QString e) {
      sqlDatabase().rollback();
      qCritical() << QString("%1 %2 %3").arg(Q_FUNC_INFO).arg(e).arg( q.lastError().text());
      abort();
   }
   ins->key() = key;

   return key;
}


// I need to break each of these out because of our signals. I will someday
// find a way to determine which signals are sent, when, from what and then
// there will come a Purge
int Database::insertStyle(Style* ins)
{
   int key = insertElement(ins);
   ins->setCacheOnly(false);

   this->pimpl->allStyles.insert(key,ins);

   emit changed( metaProperty(*this, "styles"), QVariant() );
   emit newStyleSignal(ins);

   return key;
}

int Database::insertEquipment(Equipment* ins)
{
   int key = insertElement(ins);
   ins->setCacheOnly(false);

   this->pimpl->allEquipments.insert(key,ins);
   emit changed( metaProperty(*this, "equipments"), QVariant() );
   emit newEquipmentSignal(ins);

   return key;
}

int Database::insertFermentable(Fermentable* ins)
{
   int key;
   sqlDatabase().transaction();

   try {
      key = insertElement(ins);
      ins->setCacheOnly(false);
      // I think this must go here -- we need the inventory id value written
      // to the db, and we don't have the fermentable id until now
      int invKey = newInventory(this->pimpl->dbDefn.table(DatabaseConstants::FERMTABLE));
      ins->setInventoryId(invKey);
   }
   catch( QString e ) {
      qCritical() << e;
      throw;
   }

   sqlDatabase().commit();
   this->pimpl->allFermentables.insert(key,ins);
   emit changed( metaProperty(*this, "fermentables"), QVariant() );
   emit newFermentableSignal(ins);
   return key;
}

int Database::insertHop(Hop* ins)
{
   int key;
   sqlDatabase().transaction();

   try {
      key = insertElement(ins);
      ins->setCacheOnly(false);
      int invKey = newInventory(this->pimpl->dbDefn.table(DatabaseConstants::HOPTABLE));
      ins->setInventoryId(invKey);
   }
   catch( QString e ) {
      qCritical() << e;
      throw;
   }

   sqlDatabase().commit();
   this->pimpl->allHops.insert(key,ins);
   emit changed( metaProperty(*this, "hops"), QVariant() );
   emit newHopSignal(ins);

   return key;
}

int Database::insertInstruction(Instruction* ins, Recipe* parent)
{
   int key;
   sqlDatabase().transaction();

   try {
      key = insertElement(ins);
      ins->setCacheOnly(false);

      ins = this->pimpl->addNamedEntityToRecipe<Instruction>(*this, parent,ins,true,nullptr,false,false);

   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   sqlDatabase().commit();

   this->pimpl->allInstructions.insert(key,ins);
   emit changed( metaProperty(*this, "instructions"), QVariant() );

   return key;
}

int Database::insertMash(Mash* ins)
{
   int key = insertElement(ins);
   ins->setCacheOnly(false);

   this->pimpl->allMashs.insert(key,ins);
   emit changed( metaProperty(*this, "mashs"), QVariant() );
   emit newMashSignal(ins);

   return key;
}

// this one will be harder, because we have to link the mashstep to the parent
// mash
int Database::insertMashStep(MashStep* ins, Mash* parent)
{
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::MASHSTEPTABLE);
   // step_number = (SELECT COALESCE(MAX(step_number)+1,0) FROM mashstep WHERE deleted=false AND mash_id=[key] )
   QString coalesce = QString( "%1 = (SELECT COALESCE(MAX(%1)+1,0) FROM %2 WHERE %3=%4 AND %5=%6 )")
                        .arg(tbl->propertyToColumn(PropertyNames::MashStep::stepNumber))
                        .arg(tbl->tableName())
                        .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::deleted))
                        .arg(Brewken::dbFalse())
                        .arg(tbl->foreignKeyToColumn())
                        .arg(parent->key());
   int key;

   sqlDatabase().transaction();
   try {
      // we need to insert the mashstep into the db first to get the key
      key = insertElement(ins);
      ins->setCacheOnly(false);

      this->pimpl->sqlUpdate(*this, DatabaseConstants::MASHSTEPTABLE,
                 QString("%1=%2 ").arg(tbl->foreignKeyToColumn()).arg(parent->key()),
                 QString("%1=%2").arg(tbl->keyName()).arg(ins->key())
               );

      // Just sets the step number within the mash to the next available number.
      // we need coalesce here instead of isnull. coalesce is SQL standard, so
      // should be more widely supported than isnull
      this->pimpl->sqlUpdate(*this, DatabaseConstants::MASHSTEPTABLE,
                 coalesce,
                 QString("%1=%2").arg(tbl->keyName()).arg(ins->key()));
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   sqlDatabase().commit();

   this->pimpl->allMashSteps.insert(key,ins);
   connect( ins, SIGNAL(changed(QMetaProperty,QVariant)), parent,
                 SLOT(acceptMashStepChange(QMetaProperty,QVariant)) );

   emit changed( metaProperty(*this, "mashs"), QVariant() );
   emit parent->mashStepsChanged();

   return key;
}

int Database::insertMisc(Misc* ins)
{
   int key;
   sqlDatabase().transaction();

   try {
      key = insertElement(ins);
      ins->setCacheOnly(false);

      int invKey = newInventory(this->pimpl->dbDefn.table(DatabaseConstants::MISCTABLE));
      ins->setInventoryId(invKey);
   }
   catch( QString e ) {
      qCritical() << e;
      throw;
   }

   sqlDatabase().commit();
   this->pimpl->allMiscs.insert(key,ins);
   emit changed( metaProperty(*this, "miscs"), QVariant() );
   emit newMiscSignal(ins);

   return key;
}

int Database::insertRecipe(Recipe* ins)
{
   int key = insertElement(ins);
   ins->setCacheOnly(false);

   this->pimpl->allRecipes.insert(key,ins);
   emit changed( metaProperty(*this, "recipes"), QVariant() );
   emit newRecipeSignal(ins);

   return key;
}

int Database::insertYeast(Yeast* ins)
{
   int key;
   sqlDatabase().transaction();

   try {
      key = insertElement(ins);
      ins->setCacheOnly(false);
      int invKey = newInventory(this->pimpl->dbDefn.table(DatabaseConstants::YEASTTABLE));
      ins->setInventoryId(invKey);
   }
   catch( QString e ) {
      qCritical() << e;
      throw;
   }

   sqlDatabase().commit();
   this->pimpl->allYeasts.insert(key,ins);
   emit changed( metaProperty(*this, "yeasts"), QVariant() );
   emit newYeastSignal(ins);

   return key;
}

int Database::insertWater(Water* ins)
{
   int key = insertElement(ins);
   ins->setCacheOnly(false);

   this->pimpl->allWaters.insert(key,ins);
   emit changed( metaProperty(*this, "waters"), QVariant() );
   emit newWaterSignal(ins);

   return key;
}

int Database::insertSalt(Salt* ins)
{
   int key = insertElement(ins);
   ins->setCacheOnly(false);

   this->pimpl->allSalts.insert(key,ins);
   emit changed( metaProperty(*this, "salts"), QVariant() );
   emit newSaltSignal(ins);

   return key;
}

// This is more similar to a mashstep in that we need to link the brewnote to
// the parent recipe.
int Database::insertBrewNote(BrewNote* ins, Recipe* parent) {
   // It's a coding error to try to insert a BrewNote without a Recipe
   Q_ASSERT(nullptr != parent);
   // It's a coding error to try to insert a null BrewNote!
   Q_ASSERT(nullptr != ins);

   int key;
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::BREWNOTETABLE);
   sqlDatabase().transaction();

   try {
      key = insertElement(ins);
      ins->setCacheOnly(false);

      QString const setClause = QString("%1=%2").arg(tbl->foreignKeyToColumn()).arg(parent->key());
      QString const whereClause = QString("%1=%2").arg(tbl->keyName()).arg(key);

      this->pimpl->sqlUpdate(*this, DatabaseConstants::BREWNOTETABLE, setClause, whereClause);

   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      sqlDatabase().rollback();
      throw;
   }

   qDebug() << Q_FUNC_INFO << "DB update succeeded; key =" << key;
   sqlDatabase().commit();

   this->pimpl->allBrewNotes.insert(key,ins);
   emit changed( metaProperty(*this, "brewNotes"), QVariant() );
   emit newBrewNoteSignal(ins);

   return key;
}*/
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



// NOTE: This really should be in a transaction, but I am going to leave that
// as the responsibility of the calling method. I am not comfortable with this
// idea.
/*void Database::duplicateMashSteps(Mash *oldMash, Mash *newMash)
{
   QList<MashStep*> tmpMS = mashSteps(oldMash);
   QList<MashStep*>::iterator ms;

   try {
      for( ms=tmpMS.begin(); ms != tmpMS.end(); ++ms)
      {
         // Copy the old mash step.
         MashStep* newStep = this->pimpl->copy<MashStep>(*this, *ms,&this->pimpl->allMashSteps);
         TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::MASHSTEPTABLE);

         // Put it in the new mash.
         this->pimpl->sqlUpdate(*this, DatabaseConstants::MASHSTEPTABLE,
                      QString("%1=%2").arg(tbl->foreignKeyToColumn()).arg(newMash->key()),
                      QString("%1=%2").arg(tbl->keyName()).arg(newStep->key())
                  );
         // Make the new mash pay attention to the new step.
         connect( newStep, &NamedEntity::changed,
                  newMash, &Mash::acceptMashStepChange );
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      throw;
   }

   emit changed( metaProperty(*this, "mashs"), QVariant() );
   emit newMash->mashStepsChanged();

}*/

QString Database::getDbFileName()
{
   // Ensure instance exists.
   instance();

   return dbFileName;
}

int Database::getInventoryId(TableSchema* tbl, int key )
{
   QString query = QString("SELECT %1 from %2 where %3 = %4")
         .arg(tbl->foreignKeyToColumn())
         .arg(tbl->tableName())
         .arg(tbl->keyName())
         .arg(key);

   QSqlQuery q( query, sqlDatabase());
   q.first();

   return q.record().value(tbl->foreignKeyToColumn()).toInt();
}

// this may be bad form. After a lot of refactoring, setInventory is the only method
// that needs to update something other than the NamedEntity's table. To simplify
// other things, I merged the nastier updateEntry into here and removed that method
//
// I need one of two things here for caching to work -- either every child of a inventory capable
// thing (eg, hops) listens for the parent to signal an inventory change, or this code has to
// reach into every child and update the inventory. I am leaning towards the first.
// Turns out, both are required in some order. Still thinking signal/slot
//
void Database::setInventory(NamedEntity* ins, QVariant value, int invKey, bool notify )
{
   TableSchema* tbl = this->pimpl->dbDefn.table(ins->table());
   TableSchema* inv = this->pimpl->dbDefn.table(tbl->invTable());

   QString invProp = inv->propertyName(kpropInventory);

   int ndx = ins->metaObject()->indexOfProperty(invProp.toUtf8().data());
   // I would like to get rid of this, but I need it to properly signal
   if ( invKey == 0 ) {
      qDebug() << "bad inventory call. find it an kill it";
   }

   if ( ! value.isValid() || value.isNull() ) {
      value = 0.0;
   }

   try {
      QSqlQuery update( sqlDatabase() );
      // update hop_in_inventory set amount = [value] where hop_in_inventory.id = [invKey]
      QString command = QString("UPDATE %1 set %2=%3 where %4=%5")
                           .arg(inv->tableName())
                           .arg(inv->propertyToColumn(kpropInventory))
                           .arg(value.toString())
                           .arg(inv->keyName())
                           .arg(invKey);


      if ( ! update.exec(command) )
         throw QString("Could not update %1.%2 to %3: %4 %5")
                  .arg(inv->tableName())
                  .arg(inv->propertyToColumn(kpropInventory))
                  .arg( value.toString() )
                  .arg( update.lastQuery() )
                  .arg( update.lastError().text() );

   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      throw;
   }

   if ( notify ) {
      emit ins->changed(ins->metaObject()->property(ndx),value);
      emit changedInventory(tbl->dbTable(),invKey, value);
   }
}

/*void Database::updateEntry( NamedEntity* object, QString propName, QVariant value, bool notify, bool transact )
{
   TableSchema* schema =this->pimpl->dbDefn.table( object->table() );
   int idx = object->metaObject()->indexOfProperty(propName.toUtf8().data());
   QMetaProperty mProp = object->metaObject()->property(idx);
   QString colName = schema->propertyToColumn(propName);

   if ( colName.isEmpty() ) {
      colName = schema->foreignKeyToColumn(propName);
   }

   if ( colName.isEmpty() ) {
      qCritical() << Q_FUNC_INFO << "Could not translate " << propName << " to a column name";
      throw  QString("Could not translate %1 to a column name").arg(propName);
   }
   if ( transact )
      sqlDatabase().transaction();

   try {
      QSqlQuery update( sqlDatabase() );
      QString command = QString("UPDATE %1 set %2=:value where id=%3")
                           .arg(schema->tableName())
                           .arg(colName)
                           .arg(object->key());

      update.prepare( command );
      update.bindValue(":value", value);

      if ( ! update.exec() )
         throw QString("Could not update %1.%2 to %3: %4 %5")
                  .arg( schema->tableName() )
                  .arg( colName )
                  .arg( value.toString() )
                  .arg( update.lastQuery() )
                  .arg( update.lastError().text() );

   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact )
         sqlDatabase().rollback();
      abort();
   }

   if ( transact )
      sqlDatabase().commit();

   if ( notify )
      emit object->changed(mProp,value);

}*/


/*QVariant Database::get( DatabaseConstants::DbTableId table, int key, QString col_name )
{
   QSqlQuery q;
   TableSchema* tbl = this->pimpl->dbDefn.table(table);

   QString index = QString("%1_%2").arg(tbl->tableName()).arg(col_name);

   if ( ! this->pimpl->selectSome.contains(index) ) {
      QString query = QString("SELECT %1 from %2 WHERE %3=:id")
                        .arg(col_name)
                        .arg(tbl->tableName())
                        .arg(tbl->keyName());
      q = QSqlQuery( sqlDatabase() );
      q.prepare(query);
      this->pimpl->selectSome.insert(index,q);
   }

   q = this->pimpl->selectSome.value(index);
   q.bindValue(":id", key);

   q.exec();
   if( !q.next() ) {
      q.finish();
      return QVariant();
   }

   QVariant ret( q.record().value(col_name) );
   q.finish();
   return ret;
}


QVariant Database::get( TableSchema* tbl, int key, QString col_name )
{
   return get( tbl->dbTable(), key, col_name.toUtf8().data());
}*/


// Inventory functions ========================================================



QVariant Database::getInventoryAmt(QString col_name, DatabaseConstants::DbTableId table, int key)
{
   QVariant val = QVariant(0.0);
   TableSchema* tbl = this->pimpl->dbDefn.table(table);
   TableSchema* inv = this->pimpl->dbDefn.table(tbl->invTable());

   // select hop_in_inventory.amount from hop_in_inventory,hop where hop.id = key and hop_in_inventory.id = hop.inventory_id
   QString query = QString("select %1.%2 from %1,%3 where %3.%4 = %5 and %1.%6 = %3.%7")
         .arg(inv->tableName())
         .arg(inv->propertyToColumn(kpropInventory))
         .arg(tbl->tableName())
         .arg(tbl->keyName())
         .arg(key)
         .arg(inv->keyName())
         .arg(tbl->foreignKeyToColumn(kpropInventoryId));


   QSqlQuery q( query, sqlDatabase() );

   if ( q.first() ) {
      val = q.record().value(inv->propertyToColumn(col_name));
   }
   return val;
}

//create a new inventory row
int Database::newInventory(TableSchema* schema) {
   TableSchema* inv = this->pimpl->dbDefn.table(schema->invTable());
   int newKey;

   // not sure why we were doing an upsert earlier. We already know there is no
   // inventory row for this element. So doesn't this just need an insert?
   // insert into hop_in_inventory DEFAULT VALUES
   QString queryString = QString("INSERT INTO %1 DEFAULT VALUES").arg(inv->tableName());
   QSqlQuery q( queryString, sqlDatabase() );
   newKey = q.lastInsertId().toInt();

   return newKey;
}

QMap<int, double> Database::getInventory(const DatabaseConstants::DbTableId table) const
{
   QMap<int, double> result;
   TableSchema* tbl = this->pimpl->dbDefn.table(table);
   TableSchema* inv = this->pimpl->dbDefn.invTable(table);

   // select fermentable.id as id,fermentable_in_inventory.amount as amount from
   //   fermentable_in_inventory where amount > 0 and fermentable.inventory_id = fermentable_in_inventory.id
   QString query = QString("SELECT %1.%2 as id,%3.%4 as amount FROM %1,%3 WHERE %3.%4 > 0 and %1.%5=%3.%6 and %1.%7=%8 and %1.%9=%10")
         .arg(tbl->tableName())
         .arg(tbl->keyName())
         .arg(inv->tableName())
         .arg(inv->propertyToColumn(kpropInventory))
         .arg(tbl->foreignKeyToColumn())
         .arg(inv->keyName())
         .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::display))
         .arg(Brewken::dbTrue())
         .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::deleted))
         .arg(Brewken::dbFalse());

   QSqlQuery sql(query, this->sqlDatabase());
   if (! sql.isActive()) {
      throw QString("Failed to get the inventory.\nQuery:\n%1\nError:\n%2")
            .arg(sql.lastQuery())
            .arg(sql.lastError().text());
   }

   while (sql.next()) {
      result[sql.value("id").toInt()] = sql.value("amount").toDouble();
   }

   return result;
}

// Add to recipe ==============================================================
/*
void Database::addToRecipe( Recipe* rec, Equipment* e, bool noCopy, bool transact )
{
   Equipment* newEquip = e;
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE);

   if( e == nullptr )
      return;

   if ( transact )
      sqlDatabase().transaction();

   try {
      // Make a copy of equipment.
      if ( ! noCopy ) {
         newEquip = this->pimpl->copy<Equipment>(*this, e, &this->pimpl->allEquipments, false);
      }

      // Update equipment_id
      this->pimpl->sqlUpdate(*this, DatabaseConstants::RECTABLE,
                QString("%1=%2").arg(tbl->foreignKeyToColumn(kpropEquipmentId)).arg(newEquip->key()),
                QString("%1=%2").arg(tbl->keyName()).arg(rec->key()));

   }
   catch (QString e ) {
      if ( transact )
         sqlDatabase().rollback();
      throw;
   }

   // This is likely illadvised. But if you are telling me to not transact it,
   // it is up to you to commit the changes
   if ( transact ) {
      sqlDatabase().commit();
   }
   // NOTE: need to disconnect the recipe's old equipment?
   connect( newEquip, &NamedEntity::changed, rec, &Recipe::acceptEquipChange );
   // NOTE: If we don't reconnect these signals, bad things happen when
   // changing boil times on the mainwindow
   connect( newEquip, &Equipment::changedBoilSize_l, rec, &Recipe::setBoilSize_l);
   connect( newEquip, &Equipment::changedBoilTime_min, rec, &Recipe::setBoilTime_min);

   // Emit a changed signal.
   emit rec->changed( rec->metaProperty("equipment"), NamedEntity::qVariantFromPtr(newEquip) );

   // If we are already wrapped in a transaction boundary, do not call
   // recaclAll(). Weirdness ensues. But I want this after all the signals are
   // attached, etc.
   if ( transact )
      rec->recalcAll();
}

Fermentable * Database::addToRecipe( Recipe* rec, Fermentable* ferm, bool noCopy, bool transact )
{
   if ( ferm == nullptr )
      return nullptr;

   try {
      Fermentable* newFerm = this->pimpl->addNamedEntityToRecipe<Fermentable>(*this, rec,ferm,noCopy,&this->pimpl->allFermentables,true,transact );
      connect( newFerm, SIGNAL(changed(QMetaProperty,QVariant)), rec, SLOT(acceptFermChange(QMetaProperty,QVariant)) );

      // If somebody upstream is doing the transaction, let them call recalcAll
      if ( transact && ! noCopy )
         rec->recalcAll();

      return newFerm;
   }
   catch (QString e) {
      throw;
   }
}


void Database::addToRecipe( Recipe* rec, QList<Fermentable*>ferms, bool transact )
{
   if ( ferms.size() == 0 )
      return;

   if ( transact ) {
      sqlDatabase().transaction();
   }

   try {
      foreach (Fermentable* ferm, ferms )
      {
         Fermentable* newFerm = this->pimpl->addNamedEntityToRecipe<Fermentable>(*this, rec,ferm,false,&this->pimpl->allFermentables,true,false);
         connect( newFerm, SIGNAL(changed(QMetaProperty,QVariant)), rec, SLOT(acceptFermChange(QMetaProperty,QVariant)) );
      }
   }
   catch ( QString e  ) {
      if ( transact ) {
         sqlDatabase().rollback();
      }
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
      rec->recalcAll();
   }
}


Hop * Database::addToRecipe( Recipe* rec, Hop* hop, bool noCopy, bool transact )
{
   try {
      Hop* newHop = this->pimpl->addNamedEntityToRecipe<Hop>(*this, rec, hop, noCopy, &this->pimpl->allHops, true, transact );
      // it's slightly dirty pool to put this all in the try block. Sue me.
      connect( newHop, SIGNAL(changed(QMetaProperty,QVariant)), rec, SLOT(acceptHopChange(QMetaProperty,QVariant)));
      if ( transact ) {
         rec->recalcIBU();
      }
      return newHop;
   }
   catch (QString e) {
      throw;
   }
}

void Database::addToRecipe( Recipe* rec, QList<Hop*>hops, bool transact )
{
   if ( hops.size() == 0 )
      return;

   if ( transact ) {
      sqlDatabase().transaction();
   }

   try {
      foreach (Hop* hop, hops ) {
         Hop* newHop = this->pimpl->addNamedEntityToRecipe<Hop>(*this, rec, hop, false, &this->pimpl->allHops, true, false );
         connect( newHop, SIGNAL(changed(QMetaProperty,QVariant)), rec, SLOT(acceptHopChange(QMetaProperty,QVariant)));
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact ) {
         sqlDatabase().rollback();
      }
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
      rec->recalcIBU();
   }
}


Mash * Database::addToRecipe( Recipe* rec, Mash* m, bool noCopy, bool transact )
{
   Mash* newMash = m;
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE);

   if ( transact )
      sqlDatabase().transaction();
   // Make a copy of mash.
   // Making a copy of the mash isn't enough. We need a copy of the mashsteps
   // too.
   try {
      if ( ! noCopy ) {
         newMash = this->pimpl->copy<Mash>(*this, m, &this->pimpl->allMashs, false);
         duplicateMashSteps(m,newMash);
      }

      // Update mash_id
      this->pimpl->sqlUpdate(*this, DatabaseConstants::RECTABLE,
               QString("%1=%2").arg(tbl->foreignKeyToColumn(kpropMashId) ).arg(newMash->key()),
               QString("%1=%2").arg(tbl->keyName()).arg(rec->key()));
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact )
         sqlDatabase().rollback();
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
   }
   connect( newMash, SIGNAL(changed(QMetaProperty,QVariant)), rec, SLOT(acceptMashChange(QMetaProperty,QVariant)));
   emit rec->changed( rec->metaProperty("mash"), NamedEntity::qVariantFromPtr(newMash) );
   // And let the recipe recalc all?
   if ( !noCopy && transact )
      rec->recalcAll();

   return newMash;
}

Misc * Database::addToRecipe( Recipe* rec, Misc* m, bool noCopy, bool transact )
{

   try {
      Misc * newMisc = this->pimpl->addNamedEntityToRecipe(*this, rec, m, noCopy, &this->pimpl->allMiscs, true, transact );
      if ( transact && ! noCopy )
         rec->recalcAll();
      return newMisc;
   }
   catch (QString e) {
      throw;
   }
}

void Database::addToRecipe( Recipe* rec, QList<Misc*>miscs, bool transact )
{
   if ( miscs.size() == 0 )
      return;

   if ( transact )
      sqlDatabase().transaction();

   try {
      foreach (Misc* misc, miscs ) {
         this->pimpl->addNamedEntityToRecipe(*this, rec, misc, false, &this->pimpl->allMiscs,true,false );
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact ) {
         sqlDatabase().rollback();
      }
      throw;
   }
   if ( transact ) {
      sqlDatabase().commit();
      rec->recalcAll();
   }
}

Water * Database::addToRecipe( Recipe* rec, Water* w, bool noCopy, bool transact )
{

   try {
      return this->pimpl->addNamedEntityToRecipe(*this, rec, w, noCopy, &this->pimpl->allWaters,true,transact );
   }
   catch (QString e) {
      throw;
   }
}

Salt * Database::addToRecipe( Recipe* rec, Salt* s, bool noCopy, bool transact )
{

   try {
      return this->pimpl->addNamedEntityToRecipe(*this, rec, s, noCopy, &this->pimpl->allSalts,true,transact );
   }
   catch (QString e) {
      throw;
   }
}

Style * Database::addToRecipe( Recipe* rec, Style* s, bool noCopy, bool transact )
{
   Style* newStyle = s;
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE);

   if ( s == nullptr )
      return nullptr;

   if ( transact )
      sqlDatabase().transaction();

   try {
      if ( ! noCopy )
         newStyle = this->pimpl->copy<Style>(*this, s, &this->pimpl->allStyles, false);

      this->pimpl->sqlUpdate(*this, DatabaseConstants::RECTABLE,
                QString("%1=%2").arg(tbl->foreignKeyToColumn(kpropStyleId)).arg(newStyle->key()),
                QString("%1=%2").arg(tbl->keyName()).arg(rec->key()));
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact )
         sqlDatabase().rollback();
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
   }
   // Emit a changed signal.
   rec->styleId = newStyle->key();
   emit rec->changed( rec->metaProperty("style"), NamedEntity::qVariantFromPtr(newStyle) );
   return newStyle;
}

Yeast * Database::addToRecipe( Recipe* rec, Yeast* y, bool noCopy, bool transact )
{
   try {
      Yeast* newYeast = this->pimpl->addNamedEntityToRecipe<Yeast>(*this, rec, y, noCopy, &this->pimpl->allYeasts, true, transact );
      connect( newYeast, SIGNAL(changed(QMetaProperty,QVariant)), rec, SLOT(acceptYeastChange(QMetaProperty,QVariant)));
      if ( transact && ! noCopy )
      {
         rec->recalcOgFg();
         rec->recalcABV_pct();
      }
      return newYeast;
   }
   catch (QString e) {
      throw;
   }
}

void Database::addToRecipe( Recipe* rec, QList<Yeast*>yeasts, bool transact )
{
   if ( yeasts.size() == 0 )
      return;

   if ( transact )
      sqlDatabase().transaction();

   try {
      foreach (Yeast* yeast, yeasts )
      {
         Yeast* newYeast = this->pimpl->addNamedEntityToRecipe(*this, rec, yeast, false, &this->pimpl->allYeasts,true,false );
         connect( newYeast, SIGNAL(changed(QMetaProperty,QVariant)), rec, SLOT(acceptYeastChange(QMetaProperty,QVariant)));
      }
   }
   catch (QString e) {
      qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
      if ( transact )
         sqlDatabase().rollback();
      throw;
   }

   if ( transact ) {
      sqlDatabase().commit();
      rec->recalcOgFg();
      rec->recalcABV_pct();
   }
}
*/



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/*
QList<BrewNote*> Database::brewNotes()
{
   QList<BrewNote*> tmp;

   this->pimpl->getElements(*this,  tmp, QString("deleted=%1").arg(Brewken::dbFalse()), DatabaseConstants::BREWNOTETABLE, this->pimpl->allBrewNotes );
   return tmp;
}

QList<Equipment*> Database::equipments()
{
   QList<Equipment*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::EQUIPTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::EQUIPTABLE, this->pimpl->allEquipments);
   return tmp;
}

QList<Fermentable*> Database::fermentables()
{
   QList<Fermentable*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::FERMTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::FERMTABLE, this->pimpl->allFermentables);
   return tmp;
}

QList<Hop*> Database::hops()
{
   QList<Hop*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::HOPTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::HOPTABLE, this->pimpl->allHops);
   return tmp;
}

QList<Mash*> Database::mashs()
{
   QList<Mash*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::MASHTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   //! Mashs and mashsteps are the odd balls.
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::MASHTABLE, this->pimpl->allMashs);
   return tmp;
}

QList<MashStep*> Database::mashSteps()
{
   QList<MashStep*> tmp;
   TableSchema* tbl = this->pimpl->dbDefn.table(DatabaseConstants::MASHSTEPTABLE);
   QString query = QString("%1=%2 order by %3")
           .arg(tbl->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse())
           .arg(tbl->propertyToColumn(PropertyNames::MashStep::stepNumber));
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::MASHSTEPTABLE, this->pimpl->allMashSteps);
   return tmp;
}

QList<Misc*> Database::miscs()
{
   QList<Misc*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::MISCTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::MISCTABLE, this->pimpl->allMiscs );
   return tmp;
}

QList<Recipe*> Database::recipes()
{
   QList<Recipe*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::RECTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   // This is gonna kill me.
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::RECTABLE, this->pimpl->allRecipes );
   return tmp;
}

QList<Style*> Database::styles()
{
   QList<Style*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::STYLETABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::STYLETABLE, this->pimpl->allStyles );
   return tmp;
}

QList<Water*> Database::waters()
{
   QList<Water*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::WATERTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::WATERTABLE, this->pimpl->allWaters );
   return tmp;
}

QList<Salt*> Database::salts()
{
   QList<Salt*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::SALTTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::SALTTABLE, this->pimpl->allSalts );
   return tmp;
}

QList<Yeast*> Database::yeasts()
{
   QList<Yeast*> tmp;
   QString query = QString("%1=%2")
           .arg(this->pimpl->dbDefn.table(DatabaseConstants::YEASTTABLE)->propertyToColumn(PropertyNames::NamedEntity::deleted))
           .arg(Brewken::dbFalse());
   this->pimpl->getElements(*this,  tmp, query, DatabaseConstants::YEASTTABLE, this->pimpl->allYeasts );
   return tmp;
}


//
// These templated wrappers of the member functions make it easier for callers to use templates to do generic
// processing of NamedEntity derivatives (ie Hop, Yeast, Equipment, etc objects).
//
template<> QList<BrewNote*>    Database::getAll<BrewNote>()    { return this->brewNotes();    }
template<> QList<Equipment*>   Database::getAll<Equipment>()   { return this->equipments();   }
template<> QList<Fermentable*> Database::getAll<Fermentable>() { return this->fermentables(); }
//template<> QList<Hop*>         Database::getAll<Hop>()         { return this->hops();         }
template<> QList<Mash*>        Database::getAll<Mash>()        { return this->mashs();        }
template<> QList<MashStep*>    Database::getAll<MashStep>()    { return this->mashSteps();    }
template<> QList<Misc*>        Database::getAll<Misc>()        { return this->miscs();        }
template<> QList<Recipe*>      Database::getAll<Recipe>()      { return this->recipes();      }
template<> QList<Style*>       Database::getAll<Style>()       { return this->styles();       }
template<> QList<Water*>       Database::getAll<Water>()       { return this->waters();       }
template<> QList<Salt*>        Database::getAll<Salt>()        { return this->salts();        }
template<> QList<Yeast*>       Database::getAll<Yeast>()       { return this->yeasts();       }
*/




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
