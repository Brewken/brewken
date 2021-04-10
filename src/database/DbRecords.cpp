/**
 * database/DbRecords.cpp is part of Brewken, and is copyright the following authors 2021:
 *   • Matt Young <mfsy@yahoo.com>
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
#include "database/DbRecords.h"

#include <cstring>

#include <QDebug>
#include <QHash>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "database/Database.h"

// This private implementation class holds all private non-virtual members of BeerXML
class DbRecords::impl {
public:

   /**
    * Constructor
    */
   impl(char const * const tableName,
        FieldDefinitions const & fieldDefinitions) : tableName{tableName},
                                                     fieldDefinitions{fieldDefinitions},
                                                     allObjects{} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   /**
    * \brief Append, to the supplied query string we are constructing, a comma-separated list of all the column names
    *        for the table, in the order of this->fieldDefinitions
    *
    * \param queryStringAsStream
    * \param includePrimaryKey  Usually \c true for SELECT and UPDATE, and \c false for INSERT
    * \param prependColons Set to \c true if we are appending bind values
    */
   void appendColumNames(QTextStream & queryStringAsStream, bool includePrimaryKey, bool prependColons) {
      bool skippedPrimaryKey = false;
      bool firstFieldOutput = false;
      for (auto const & fieldDefn: this->fieldDefinitions) {
         if (!includePrimaryKey && !skippedPrimaryKey) {
            // By convention the first field is the primary key
            skippedPrimaryKey = true;
         } else {
            if (!firstFieldOutput) {
               firstFieldOutput = true;
            } else {
               queryStringAsStream << ", ";
            }
            if (prependColons) {
               queryStringAsStream << ":";
            }
            queryStringAsStream << fieldDefn.columnName;
         }
      }
      return;
   }

   char const * const tableName;
   FieldDefinitions const & fieldDefinitions;
   QHash<int, std::shared_ptr<QObject> > allObjects;
};


DbRecords::DbRecords(char const * const tableName,
                     FieldDefinitions const & fieldDefinitions) : pimpl{ new impl{tableName, fieldDefinitions} } {
   return;
}

// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the
// header file)
DbRecords::~DbRecords() = default;

void DbRecords::loadAll(QSqlDatabase databaseConnection) {
   //
   // Note the following from https://doc.qt.io/qt-5/qsqldatabase.html#database:
   //    "An instance of QSqlDatabase represents [a] connection ... to the database. ... It is highly recommended that
   //    you do not keep a copy of [a] QSqlDatabase [object] around as a member of a class, as this will prevent the
   //    instance from being correctly cleaned up on shutdown."
   //

   //
   // Using QSqlTableModel would save us having to write a SELECT statement, however it is a bit hard to use it to
   // reliably get the number of rows in a table.  Eg, QSqlTableModel::rowCount() is not implemented for all databases,
   // and there is no documented way to detect the index supplied to QSqlTableModel::record(int row) is valid.  (In
   // testing with SQLite, the returned QSqlRecord object for an index one beyond the end of he table still gave a
   // false return to QSqlRecord::isEmpty() but then returned invalid record values.)
   //
   // So, instead, we create the appropriate SELECT query from scratch.  We specify the column names rather than just
   // do SELECT * because it's small extra effort and will give us an early error if an invalid column is specified.
   //
   QString queryString{"SELECT "};
   QTextStream queryStringAsStream{&queryString};
   this->pimpl->appendColumNames(queryStringAsStream, true, false);
   queryStringAsStream << " FROM " << this->pimpl->tableName << ";";
   QSqlQuery sqlQuery{queryString, databaseConnection};
   if (!sqlQuery.exec()) {
      qCritical() <<
         Q_FUNC_INFO << "Error executing database query " << queryString << ": " << sqlQuery.lastError().text();
      return;
   }

   qDebug() << Q_FUNC_INFO << "Reading rows from database query " << queryString;

   while (sqlQuery.next()) {
      //
      // We want to pull all the fields for the current row from the database and use them to construct a new
      // object.
      //
      // Two approaches suggest themselves:
      //
      //    (i)  Create a blank object and, using Qt Properties, fill in each field using the QObject setProperty()
      //         call (as we currently do when reading in an XML file).
      //    (ii) Read all the fields for this row from the database and then use them as parameters to call a
      //         suitable constructor to get a new object.
      //
      // The problem with approach (i) is that lots of the setters called via setProperty have side-effects
      // including emitting signals and trying to update the database.  We can sort of get away with ignoring this
      // while reading an XML file, but we risk going round in circles (including being deadlocked) if we let such
      // things happen while we're still reading everything out of the DB at start-up.  A solution would be to have
      // an "initialising" flag on the object that turns off setter side-effects.  This is a small change but one
      // that needs to be made in a lot of places, including almost every setter function.
      //
      // The problem with approach (ii) is that we don't want a constructor that takes a long list of parameters as
      // it's too easy to get bugs where a call is made with the parameters in the wrong order.  We can't easily use
      // Boost Parameter to solve this because it would be hard to have parameter names as pure data (one of the
      // advantages of the Qt Property system), plus it would apparently make compile times very long.  So we would
      // have to roll our own way of passing, say, a QHash (of propertyName -> QVariant) to a constructor.  This is
      // a chunkier change but only needs to be made in a small number of places (new constructors).
      //
      // Although (i) has the further advantage of not requiring a constructor update when a new property is added
      // to a class, it feels a bit wrong to construct an object in "invalid" state and then set a "now valid" flag
      // later after calling lots of setters.  In particular, it is hard (without adding lots of complexity) for the
      // object class to enforce mandatory construction parameters with this approach.
      //
      // Method (ii) is therefore our preferred approach.  We use a simple QHash for now but note that, in future,
      // it would be possible to extend this to something akin to the Named Parameter Idiom.
      //
      NamedParameterBundle namedParameterBundle;
      int primaryKey = -1;

      //
      // Populate all the fields
      // By convention, the primary key should be listed as the first field
      //
      // NB: For now we're assuming that the primary key is always an integer, but it would not be enormous work to
      //     allow a wider range of types.
      //
      bool readPrimaryKey = false;
      for (auto const & fieldDefn : this->pimpl->fieldDefinitions) {
         qDebug() << Q_FUNC_INFO << "Reading " << fieldDefn.columnName << " into " << fieldDefn.propertyName;
         QVariant fieldValue = sqlQuery.value(fieldDefn.columnName);
         if (!fieldValue.isValid()) {
            qCritical() <<
               Q_FUNC_INFO << "Error reading column " << fieldDefn.columnName << " (" << fieldValue.toString() <<
               ") from database table " << this->pimpl->tableName << ". SQL error message: " <<
               sqlQuery.lastError().text();
            break;
         }

         // It's a coding error if we got the same parameter twice
         Q_ASSERT(!namedParameterBundle.contains(fieldDefn.propertyName));

         namedParameterBundle.insert(fieldDefn.propertyName, fieldValue);

         if (!readPrimaryKey) {
            readPrimaryKey = true;
            primaryKey = fieldValue.toInt();
         }
      }

      // Get a new object...
      auto object = this->createNewObject(namedParameterBundle);

      // ...and store it
      // It's a coding error if we have two objects with the same primary key
      Q_ASSERT(!this->pimpl->allObjects.contains(primaryKey));
      this->pimpl->allObjects.insert(primaryKey, object);
      qDebug() << Q_FUNC_INFO << "Stored #" << primaryKey;
   }

   return;
}

bool DbRecords::contains(int id) {
   return this->pimpl->allObjects.contains(id);
}

std::shared_ptr<QObject> DbRecords::getById(int id) {
   return this->pimpl->allObjects.value(id);
}

void DbRecords::insert(std::shared_ptr<QObject> newObject) {
   //
   // Construct the SQL, which will be of the form
   //
   //    INSERT INTO tablename (firstColumn, secondColumn, ...)
   //    VALUES (:firstColumn, :secondColumn, ...);
   //
   // We omit the primary key column because we can't know its value in advance.  We'll find out what value the DB
   // assigned to it after the query was run -- see below.
   //
   // .:TBD:. A small optimisation might be to construct this just once rather than every time this function is called
   //
   QString queryString{"INSERT INTO "};
   QTextStream queryStringAsStream{&queryString};
   queryStringAsStream << this->pimpl->tableName << " (";
   this->pimpl->appendColumNames(queryStringAsStream, false, false);
   queryStringAsStream << ") VALUES (";
   this->pimpl->appendColumNames(queryStringAsStream, false, true);
   queryStringAsStream << ");";

   //
   // Bind the values
   //
   QSqlQuery sqlQuery{queryString, Database::instance().sqlDatabase()};
   bool skippedPrimaryKey = false;
   char const * primaryKeyParameter{nullptr};
   for (auto const & fieldDefn: this->pimpl->fieldDefinitions) {
      if (!skippedPrimaryKey) {
         // By convention the first field is the primary key
         skippedPrimaryKey = true;
         primaryKeyParameter = fieldDefn.propertyName;
      } else {
         QString bindName = QString{":%1"}.arg(fieldDefn.columnName);
         sqlQuery.bindValue(bindName, newObject->property(fieldDefn.propertyName));
      }
   }

   //
   // Run the query
   //
   if (!sqlQuery.exec()) {
      qCritical() <<
         Q_FUNC_INFO << "Error executing database query " << queryString << ": " << sqlQuery.lastError().text();
      return;
   }

   //
   // Get the ID of the row we just inserted and put it in the object
   //
   // Assert that we are only using database drivers that support returning the last insert ID.  (It is frustratingly
   // hard to find documentation about this, as, eg, https://doc.qt.io/qt-5/sql-driver.html does not explicitly list
   // which supplied drivers support which features.  However, in reality, we know SQLite and PostgreSQL drivers both
   // support this, so it would likely only be a problem if a new type of DB were introduced.)
   //
   Q_ASSERT(sqlQuery.driver()->hasFeature(QSqlDriver::LastInsertId));
   auto primaryKey = sqlQuery.lastInsertId();

   newObject->setProperty(primaryKeyParameter, primaryKey);
   qDebug() << Q_FUNC_INFO << "Object with ID" << primaryKey.toInt() << "inserted in database using" << queryString;

   //
   // Add the object to our list of all objects of this type (asserting that it should be impossible for an object with
   // this ID to already exist in that list).
   //
   Q_ASSERT(!this->pimpl->allObjects.contains(primaryKey.toInt()));
   this->pimpl->allObjects.insert(primaryKey.toInt(), newObject);

   //
   // Tell any bits of the UI that need to know that there's a new object
   //
   emit this->signalObjectInserted(primaryKey.toInt());

   return;
}

void DbRecords::update(std::shared_ptr<QObject> object) {
   //
   // Construct the SQL, which will be of the form
   //
   //    UPDATE tablename
   //    SET firstColumn = :firstColumn, secondColumn = :secondColumn, ...
   //    WHERE primaryKeyColumn = :primaryKeyColumn;
   //
   // .:TBD:. A small optimisation might be to construct this just once rather than every time this function is called
   //
   QString queryString{"UPDATE "};
   QTextStream queryStringAsStream{&queryString};
   queryStringAsStream << this->pimpl->tableName << " SET ";

   QString primaryKeyColumn;
   bool skippedPrimaryKey = false;
   bool firstFieldOutput = false;
   for (auto const & fieldDefn: this->pimpl->fieldDefinitions) {
      if (!skippedPrimaryKey) {
         // By convention the first field is the primary key
         skippedPrimaryKey = true;
         primaryKeyColumn = fieldDefn.columnName;
      } else {
         if (!firstFieldOutput) {
            firstFieldOutput = true;
         } else {
            queryStringAsStream << ", ";
         }
         queryStringAsStream << " " << fieldDefn.columnName << " = :" << fieldDefn.columnName;
      }
   }

   queryStringAsStream << " WHERE " << primaryKeyColumn << " = :" << primaryKeyColumn << ";";

   //
   // Bind the values
   //
   QSqlQuery sqlQuery{queryString, Database::instance().sqlDatabase()};
   for (auto const & fieldDefn: this->pimpl->fieldDefinitions) {
      QString bindName = QString{":%1"}.arg(fieldDefn.columnName);
      sqlQuery.bindValue(bindName, object->property(fieldDefn.propertyName));
   }

   //
   // Run the query
   //
   if (!sqlQuery.exec()) {
      qCritical() <<
         Q_FUNC_INFO << "Error executing database query " << queryString << ": " << sqlQuery.lastError().text();
      return;
   }

   return;
}

void DbRecords::updateProperty(std::shared_ptr<QObject> object, char const * const propertyToUpdateInDb) {
   //
   // Construct the SQL, which will be of the form
   //
   //    UPDATE tablename
   //    SET columnName = :columnName
   //    WHERE primaryKeyColumn = :primaryKeyColumn;
   //
   QString queryString{"UPDATE "};
   QTextStream queryStringAsStream{&queryString};
   queryStringAsStream << this->pimpl->tableName << " SET ";

   QString const &    primaryKeyColumn   = this->pimpl->fieldDefinitions[0].columnName;
   char const * const primaryKeyProperty = this->pimpl->fieldDefinitions[0].propertyName;

   auto matchingFieldDefn = std::find_if(
      this->pimpl->fieldDefinitions.begin(),
      this->pimpl->fieldDefinitions.end(),
      [propertyToUpdateInDb](FieldDefinition const & fd) {return 0 == std::strcmp(fd.propertyName, propertyToUpdateInDb);}
   );

   // It's a coding error if the property isn't one we store in the DB for this type of object
   Q_ASSERT(matchingFieldDefn != this->pimpl->fieldDefinitions.end());
   QString const & columnToUpdateInDb = matchingFieldDefn->columnName;

   queryStringAsStream << " " << columnToUpdateInDb << " = :" << columnToUpdateInDb;
   queryStringAsStream << " WHERE " << primaryKeyColumn << " = :" << primaryKeyColumn << ";";

   //
   // Bind the values
   //
   QSqlQuery sqlQuery{queryString, Database::instance().sqlDatabase()};
   sqlQuery.bindValue(QString{":%1"}.arg(columnToUpdateInDb), object->property(propertyToUpdateInDb));
   sqlQuery.bindValue(QString{":%1"}.arg(primaryKeyColumn), object->property(primaryKeyProperty));

   //
   // Run the query
   //
   if (!sqlQuery.exec()) {
      qCritical() <<
         Q_FUNC_INFO << "Error executing database query " << queryString << ": " << sqlQuery.lastError().text();
      return;
   }

   return;
}


void DbRecords::softDelete(int id) {
   this->pimpl->allObjects.remove(id);
   return;
}

//
void DbRecords::hardDelete(int id) {
   //
   // Construct the SQL, which will be of the form
   //
   //    DELETE FROM tablename
   //    WHERE primaryKeyColumn = :primaryKeyColumn;
   //
   // .:TBD:. A small optimisation might be to construct this just once rather than every time this function is called
   //
   QString queryString{"DELETE FROM "};
   QTextStream queryStringAsStream{&queryString};
   queryStringAsStream << this->pimpl->tableName;
   QString const & primaryKeyColumn = this->pimpl->fieldDefinitions[0].columnName;
   queryStringAsStream << " WHERE " << primaryKeyColumn << " = :" << primaryKeyColumn << ";";

   //
   // Bind the value
   //
   QSqlQuery sqlQuery{queryString, Database::instance().sqlDatabase()};
   QString bindName = QString{":%1"}.arg(primaryKeyColumn);
   sqlQuery.bindValue(bindName, QVariant(id));

   //
   // Run the query
   //
   if (!sqlQuery.exec()) {
      qCritical() <<
         Q_FUNC_INFO << "Error executing database query " << queryString << ": " << sqlQuery.lastError().text();
      return;
   }

   //
   // Remove the object from the cache
   //
   this->pimpl->allObjects.remove(id);

   return;
}