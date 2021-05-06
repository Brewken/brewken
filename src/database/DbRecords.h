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
   /**
    * \brief The different field types that can be stored directly in an object's DB table.
    */
   enum FieldType {
      Bool,
      Int,
      UInt,
      Double,
      String,
      Date,
      Enum   // Stored as a string in the DB
   };

   /**
    * \brief Associates an enum value with a string representation in the DB.  This is more robust than just storing
    *        the raw numerical value of the enum.
    */
   struct EnumAndItsDbString {
      QString string;
      int     native;
   };

   /**
    * \brief We don't actually bother creating hashmaps or similar between enum values and string representations
    *        because it's usually going to be a short list that we can search through pretty quickly (probably faster
    *        than calculating the hash of a key!)
    */
   typedef QVector<EnumAndItsDbString> EnumStringMapping;

   struct FieldDefinition {
      FieldType                 fieldType;
      QString                   columnName;
      char const * const        propertyName;
      EnumStringMapping const * enumMapping = nullptr; // only needed if fieldType is Enum
   };

   typedef QVector<FieldDefinition> FieldDefinitions;

   /**
    * \brief Cross-references to other objects that are stored in a junction table.  (See
    *        https://en.wikipedia.org/wiki/Associative_entity)  Eg, for a Recipe, there are several junction tables
    *        (fermentable_in_recipe, hop_in_recipe, etc) to store info where potentially many other objects
    *        (Fermentable, Hop, etc) are associated with a single recipe.
    *
    *        We assume that each junction table contains only two columns of interest to us, both of which are foreign
    *        keys to other objects, and both of which are integers.  When passing the results to-and-from the object
    *        itself, we'll normally pass a list of integers.  However, if \c assumeMaxOneEntry is set to \c true, then
    *        we'll pull at most one matching row and pass an integer (wrapped in QVariant and thus 0 if no row
    *        returned).
    *
    *        .:TBD:. For reasons that are not entirely clear, the parent-child relationship between various objects is
    *        also stored in junction tables.  Although we could change this, it's more likely we will just drop the
    *        parent-child stuff.
    *
    * \param tableName
    * \param thisPrimaryKeyColumnName
    * \param otherPrimaryKeyColumnName
    * \param propertyName
    * \param assumeMaxOneEntry
    * \param orderByColumnName  If not empty string, this is the column that orders the elements (eg instruction
    *                           number for instructions_in_recipe).  Otherwise the elements are assumed to be an
    *                           unordered set (and pulled out in ID order by default).
    */
   struct AssociativeEntity {
      char const * const tableName;
      QString thisPrimaryKeyColumnName;
      QString otherPrimaryKeyColumnName;
      char const * const propertyName;
      bool assumeMaxOneEntry = false;
      QString orderByColumnName = QString{};
   };

   typedef QVector<AssociativeEntity> AssociativeEntities;

   /**
    * \brief Constructor sets up mappings but does not read in data from DB
    *
    * \param tableName
    * \param fieldDefinitions  First in the list should be the primary key
    * \param associativeEntities
    */
   DbRecords(char const * const tableName,
             FieldDefinitions const & fieldDefinitions,
             AssociativeEntities const & associativeEntities);

   ~DbRecords();

   /**
    * \brief Create the table(s) for the objects handled by this store
    *
    * NB: THIS IS NOT YET IMPLEMENTED
    */
   void createTables();

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
   void insert(std::shared_ptr<QObject> object);

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
    *        though callers should ideally check this first via \c contains()  Subclasses are expected to provide a
    *        public override of this function to downcast the QObject shared pointer to a more specific one.
    *
    *        NB: This is private and non-virtual because we want subclasses to be able to have a different return type.
    *            You can't change the return type on a virtual function because, by definition, callers need to know
    *            the return type of the function without knowing which version of it is called - hence "invalid
    *            covariant return type" compiler errors if you try.
    *
    *            We in any case don't need virtual here anyway as external callers will not be upcasting subclasses of
    *            DbRecords.
    */
   std::shared_ptr<QObject> getById(int id);

   /**
    * \brief Allow searching of the set of all cached objects (of a given type) with a lambda.  Subclasses are
    *        expected to provide a public override of this function that implements a class-specific interface.
    *
    *        NB: This is private and non-virtual for the same reasons as \c getById()
    *
    * \param matchFunction Takes a shared pointer to object and returns \c true if it's a match or \c false otherwise.
    *
    * \return Shared pointer to the first object that gives a \c true result to \c matchFunction, or \c std::nullopt if
    *         none does
    */
   std::optional< std::shared_ptr<QObject> > findMatching(
      std::function<bool(std::shared_ptr<QObject>)> const & matchFunction
   );


private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;
};


#endif
