/**
 * database/DbTransaction.cpp is part of Brewken, and is copyright the following authors 2021:
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
#include "database/DbTransaction.h"

#include <QDebug>
#include "database/Database.h"


DbTransaction::DbTransaction(QSqlDatabase & connection, DbTransaction::SpecialBehaviours specialBehaviours) :
   connection(connection),
   committed(false),
   specialBehaviours(specialBehaviours) {
   // Note that, on SQLite at least, turning foreign keys on and off has to happen outside a transaction, so we have to
   // be careful about the order in which we do things.
   if (this->specialBehaviours & DISABLE_FOREIGN_KEYS) {
      Database::setForeignKeysEnabled(false, connection);
   }

   bool succeeded = this->connection.transaction();
   qDebug() << Q_FUNC_INFO << "Database transaction begin: " << (succeeded ? "succeeded" : "failed");
   return;
}

DbTransaction::~DbTransaction() {
   qDebug() << Q_FUNC_INFO;
   if (!committed) {
      bool succeeded = this->connection.rollback();
      qWarning() << Q_FUNC_INFO << "Database transaction rollback: " << (succeeded ? "succeeded" : "failed");
   }

   // See comment above about why we need to do this _after_ the transaction has finished
   if (this->specialBehaviours & DISABLE_FOREIGN_KEYS) {
      Database::setForeignKeysEnabled(true, connection);
   }
   return;
}

bool DbTransaction::commit() {
   this->committed = connection.commit();
   qDebug() << Q_FUNC_INFO << "Database transaction commit: " << (this->committed ? "succeeded" : "failed");
   return this->committed;
}
