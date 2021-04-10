/**
 * database/DbRecords.h is part of Brewken, and is copyright the following authors 2021:
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
#ifndef DATABASE_DBRECORDS_H
#define DATABASE_DBRECORDS_H
#pragma once

#include <memory> // For PImpl

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "model/NamedParameterBundle.h"

/**
 * \brief Base class for storing objects (of a given class) in (a) the database and (b) a local in-memory cache.
 *
 *        This class does all the generic work and, by virtue of being a non-template class, can have most of its
 *        implementation private.  The template class \c DbNamedEntityRecords then does the class-specific work (eg
 *        call constructor for the right type of object) and provides a class- specific interface (so that callers
 *        don't have to downcast return values etc).
 *
 *        Inheritance from QObject is to allow this class to send signals
 */
class DbRecords : QObject {
   // We also need the Q_OBJECT macro to use signals and/or slots
   Q_OBJECT

public:
   enum FieldType {
      Bool,
      Int,
      UInt,
      Double,
      String,
      Date,
      Enum,
      Record
   };

   struct FieldDefinition {
      FieldType           fieldType;
      QString             columnName;
      char const * const  propertyName;
   };

   typedef QVector<FieldDefinition> FieldDefinitions;

   /**
    * \brief Constructor sets up mappings but does not read in data from DB
    *
    * \param tableName
    * \param fieldDefinitions  First in the list should be the primary key
    */
   DbRecords(char const * const tableName, FieldDefinitions const & fieldDefinitions);

   ~DbRecords();

   /**
    * \brief Load from database all objects handled by this store
    */
   void loadAll(QSqlDatabase databaseConnection);

   /**
    * \brief Create a new object of the type we are handling, using the parameters read from the DB.  Subclass needs to
    *        implement.
    */
   virtual std::shared_ptr<QObject> createNewObject(NamedParameterBundle & namedParameterBundle) = 0;

   /**
    * \brief Insert a new object in the DB (and in our cache list)
    */
   void insert(std::shared_ptr<QObject> newObject);

   /**
    * \brief Update an existing object in the DB
    */
   void update(std::shared_ptr<QObject> object);

   /**
    * \brief Update a single property of an existing object in the DB
    */
   void updateProperty(std::shared_ptr<QObject> object, char const * const propertyToUpdateInDb);

   /**
    * \brief Remove the object from our local in-memory cache
    *
    *        Subclasses can do additional or different work, eg \c DbNamedEntityRecords will mark the object as deleted
    *        both in memory and in the database (via the \c "deleted" property of \c NamedEntity which is also stored
    *        in the DB) but will leave the object in the local cache (ie will not call down to this base class member
    *        function).
    *
    * \param id ID of the object to delete
    *
    *        (We take the ID of the object to delete rather than, say, std::shared_ptr<QObject> because it's almost
    *        certainly simpler for the caller to extract the ID than for us.)
    */
   virtual void softDelete(int id);

   /**
    * \brief Remove the object from our local in-memory cache and remove its record from the DB.
    *
    *        Subclasses can do additional work, eg \c DbNamedEntityRecords will also mark the in-memory object as
    *        deleted (via the \c "deleted" property of \c NamedEntity).
    *
    *        .:TODO:. Need to work out where to do "is this object used elsewhere" checks - eg should a Hop be deletable if it's used in a Recipe
    *
    * \param id ID of the object to delete
    */
   virtual void hardDelete(int id);

   /**
    * \brief Return \c true if an object with the supplied ID is stored in the cache or \c false otherwise
    */
   bool contains(int id);

signals:
   /**
    * \brief Signal emitted when a new object is inserted in the database.  Parts of the UI that need to display all
    *        objects of this type should connect this signal to a slot.  NB: Replaces the following signals:
    *            void newBrewNoteSignal(BrewNote*);
    *            void newEquipmentSignal(Equipment*);
    *            void newFermentableSignal(Fermentable*);
    *            void newHopSignal(Hop*);
    *            void newMashSignal(Mash*);
    *            void newMashStepSignal(MashStep*);
    *            void newMiscSignal(Misc*);
    *            void newRecipeSignal(Recipe*);
    *            void newSaltSignal(Salt*);
    *            void newStyleSignal(Style*);
    *            void newWaterSignal(Water*);
    *            void newYeastSignal(Yeast*);
    *
    *        Note that Qt Signals and Slots can't be in templated classes because part of the code to implement them is
    *        generated by the Meta-Object Compiler (MOC), which runs before main compilation, and therefore before
    *        template instantiation.  Also, when calling QObject::connect() to connect a signal to a slot, you need the
    *        \b object that emits the signal (not the class).
    *
    *        So, we emit the signal here in the virtual base class, and it will be received in slot(s) that have
    *        connected to the relevant singleton instance of the subclass.  (Eg, if you connect a slot to
    *        \c DbNamedEntityRecords<Water>::getInstance() then its going to receive a signal whenever a new Water
    *        object is inserted in the database.
    *
    * \param id The primary key of the newly inserted object.  (For the moment we assume all primary keys are integers.
    *           If we want to extend this in future then we'd change this param to a QVariant.)
    */
   void signalObjectInserted(int id);

protected:
   /**
    * \brief Return pointer to the object with the specified key (or pointer to null if no object exists for the key,
    *        though callers should ideally check this first via \c contains()  Subclasses are expected to override this
    *        to downcast the QObject shared pointer to a more specific one.
    *
    *        NB: This is not virtual because we want subclasses to be able to have a different return type.  (We don't
    *            need virtual here anyway as external callers will not be upcasting subclasses of DbRecords, and even,
    *            if they did, they'd still get the "right" result by calling this base class function.)
    */
   std::shared_ptr<QObject> getById(int id);

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};


#endif
