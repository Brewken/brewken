/**
 * database/DbNamedEntityRecords.h is part of Brewken, and is copyright the following authors 2021:
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
#ifndef DATABASE_DBNAMEDENTITYRECORDS_H
#define DATABASE_DBNAMEDENTITYRECORDS_H
#pragma once
#include <QDebug>

#include "database/DbRecords.h"
#include "model/NamedEntity.h"

/**
 * \brief Read, write and cache any subclass of \c NamedEntity in the database
 */
template<class NE>
class DbNamedEntityRecords : public DbRecords {
private:
   /**
    * \brief Constructor sets up mappings but does not read in data from DB.  Private because singleton.
    *
    * \param tableName
    * \param fieldDefinitions  First in the list should be the primary key
    */
   DbNamedEntityRecords(char const * const tableName,
                        FieldDefinitions const & fieldDefinitions) : DbRecords(tableName, fieldDefinitions) {
      return;
   }

public:

   /**
    * \brief Get the singleton instance of this class
    */
   static DbNamedEntityRecords<NE> & getInstance();

   /**
    * \brief Return an object for the specified key
    *
    *        This overrides the base-class function of the same name, enabling us (by virtue of the fact that these
    *        particular functions do NOT need to be virtual) to template the return type.
    */
   std::optional< std::shared_ptr<NE> > getById(int id) {
      if (!this->contains(id)) {
         return std::nullopt;
      }
      return std::optional< std::shared_ptr<NE> >{std::static_pointer_cast<NE>(this->DbRecords::getById(id))};
   }

   /**
    * \brief Mark an object as deleted (including in the database) and but leave it in existence (both in the database
    *        and in our local in-memory cache.
    *
    *        NB: We do not call down to \c DbRecords::softDelete() from this member function (as that would remove the
    *            object from our local in-memory cache.
    *
    * \param id ID of the object to delete
    */
   virtual void softDelete(int id) {
      this->hardOrSoftDelete(id, false);
      return;
   }

   /**
    * \brief Remove the object from our local in-memory cache, mark it as deleted, and remove its record from the DB.
    *
    * \param id ID of the object to delete
    */
   virtual void hardDelete(int id) {
      this->hardOrSoftDelete(id, true);
      return;
   }


protected:
   /**
    * \brief Create a new object of the type we are handling, using the parameters read from the DB
    */
   virtual std::shared_ptr<QObject> createNewObject(NamedParameterBundle & namedParameterBundle) {
      //
      // NB: std::static_pointer_cast actually creates a new instance of std::shared_ptr (whose stored pointer is
      // obtained from its parameter's stored pointer using a cast expression).  So there is no point creating a
      // shared_ptr of one type if we're straight away going to cast it to another type; just create the type we need
      // in the first place.
      //
      return std::shared_ptr<QObject>(new NE{namedParameterBundle});
   }

private:
   /**
    * \brief Do a hard or soft delete
    *
    * \param id ID of the object to delete
    * \param hard \c true for hard delete, \c false for soft delete
    */
   void hardOrSoftDelete(int id, bool hard) {
      if (!this->contains(id)) {
         // This is probably a coding error, but might be recoverable
         qWarning() << Q_FUNC_INFO << "Trying to delete non-existent object with ID" << id;
         return;
      }

      std::shared_ptr<NE> ne{std::static_pointer_cast<NE>(this->DbRecords::getById(id))};
      ne->setDeleted(true);
      if (hard) {
         // Base class does the heavy lifting
         this->DbRecords::hardDelete(id);
      } else {
         // Base class softDelete() actually does too much for the soft delete case; we just want to store the
         // "deleted" flag in the object's DB record
         this->updateProperty(ne, PropertyNames::NamedEntity::deleted);
      }
      return;
   }
};

#endif
