/**
 * database/DatabaseSchemaHelper.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Jonatan Pålsson <jonatan.p@gmail.com>
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
#ifndef DATABASESCHEMAHELPER_H
#define DATABASESCHEMAHELPER_H
#pragma once

#include <QSqlDatabase>

#include "Brewken.h"
#include "Database.h"

class DatabaseSchema;

/*!
 * \brief Helper functions to manage Database schema upgrades etc
 */
namespace DatabaseSchemaHelper {

   //! \brief Database version. Increment on any schema change.
   extern int const dbVersion;

   extern bool upgrade;

   /*!
    * \brief Create a blank database whose schema version is \c dbVersion
    */
   bool create(Database & database, QSqlDatabase db);

   /*!
    * \brief Migrate schema from \c oldVersion to \c newVersion
    */
   bool migrate(Database & database, int oldVersion, int newVersion, QSqlDatabase connection);

   //! \brief Current schema version of the given database
   int currentVersion(QSqlDatabase db = QSqlDatabase());

   //! \brief does the heavy lifting to copy the contents from one db to the next
   void copyDatabase(Database::DbType oldType, Database::DbType newType, QSqlDatabase connectionNew);

   //! \brief Populates (or updates) default Recipes, Hops, Styles, etc in the DB
   void updateDatabase(Database & database, QString const& filename);
}

#endif
