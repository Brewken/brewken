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
                        FieldDefinitions const & fieldDefinitions,
                        JunctionTables const & junctionTables) :
      DbRecords(tableName, fieldDefinitions, junctionTables) {
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

   /**
    * \brief Allow searching of the set of all cached objects with a lambda.
    *
    * \param matchFunction Takes a pointer to an object and returns \c true if the object is a match or \c false otherwise.
    *
    * \return Shared pointer to the first object that gives a \c true result to \c matchFunction, or \c std::nullopt if
    *         none does
    */
   std::optional< std::shared_ptr<NE> > findMatching(std::function<bool(NE *)> const & matchFunction) {
      //
      // Caller has provided us with a lambda function that takes a pointer to NE (ie Water, Hop, Yeast, Recipe, etc)
      // and returns true or false depending on whether it's a match for whatever condition the caller requires.
      //
      // The base class findMatching() expects a lambda function that takes a std::shared_ptr<QObject> parameter.
      //
      // So, to call the base class findMatching(), we need to create our own "wrapper" lambda that receives a
      // std::shared_ptr<QObject> parameter, extracts the raw pointer from it (which we know will always be valid) and
      // downcasts it from "QObject *" to "NE *".
      //
      // We don't need shared pointers for the lambda because it's a short-lived function that isn't doing anything to
      // its parameter.  Base class uses a shared_ptr parameter to its lambda instead of just "QObject *" as otherwise
      // it would have to create its own lambda wrapper to search its internal data structure, which seems like one
      // wrapper too many!
      //
      auto result = this->DbRecords::findMatching(
         [matchFunction](std::shared_ptr<QObject> obj) {return matchFunction(static_cast<NE *>(obj.get()));}
      );
      if (!result.has_value()) {
         return std::nullopt;
      }
      return std::optional< std::shared_ptr<NE> >{std::static_pointer_cast<NE>(result.value())};
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
