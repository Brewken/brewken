/**
 * XmlNamedEntityRecord.h is part of Brewken, and is copyright the following authors 2020-2021:
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
#ifndef XML_XMLNAMEDENTITYRECORD_H
#define XML_XMLNAMEDENTITYRECORD_H
#pragma once

#include <QDebug>
#include <QString>
#include <QList>

#include "database/ObjectStoreTyped.h"
#include "model/BrewNote.h"
#include "model/Instruction.h"
#include "model/Mash.h"
#include "model/MashStep.h"
#include "model/NamedEntity.h"
#include "model/Recipe.h"
#include "xml/XmlRecord.h"
#include "xml/XQString.h"


/**
 * \brief Provides class-specific extensions to \b XmlRecord.  See comment in xml/XmlCoding.h for more details.
 */
template<class NE>
class XmlNamedEntityRecord : public XmlRecord {
public:
   /**
    * \brief This constructor doesn't have to do much more than create an appropriate new subclass of \b NamedEntity.
    *        Everything else is done in the base class.
    */
   XmlNamedEntityRecord(QString const & recordName,
                        XmlCoding const & xmlCoding,
                        XmlRecord::FieldDefinitions const & fieldDefinitions) :
   XmlRecord{recordName, xmlCoding, fieldDefinitions} {
      this->namedEntityClassName = NE::staticMetaObject.className();
      this->includeInStats = this->includedInStats();
      return;
   }


protected:
   virtual void constructNamedEntity() {
      this->namedEntityRaiiContainer.reset(new NE{this->namedParameterBundle});
      this->namedEntity = this->namedEntityRaiiContainer.get();
   }

   //
   // TODO It's a bit clunky to have the knowledge/logic in this class for whether duplicates and name clashes are
   //      allowed.  Ideally this should be part of the NamedEntity subclasses themselves and the traits used here.
   //      The same applies to whether a NamedEntity subclass is "owned" by another NamedEntity (in the sense that a
   //      MashStep is owned by a Mash.
   //

   /**
    * \brief Implementation for general case where instances are supposed to be unique.  NB: What we really mean here
    *        is that, if we find a Hop/Yeast/Fermentable/etc in an XML file that is "the same" as one that we already
    *        have stored, then we should not read it in.  This says nothing about whether we ourselves have multiple
    *        copies of such objects - eg as is currently the case when you add a Hop to a Recipe and a copy of the Hop
    *        is created.  (In the long-run we might want to change how that bit of the code works, but that's another
    *        story.)
    */
   virtual bool isDuplicate() {
      auto currentEntity = this->namedEntity;
      auto matchResult = ObjectStoreTyped<NE>::getInstance().findFirstMatching(
         [currentEntity](std::shared_ptr<NE> ne) {return *ne == *currentEntity;}
      );
      if (matchResult) {
         qDebug() << Q_FUNC_INFO << "Found a match for " << this->namedEntity->name();
         // Set our pointer to the Hop/Yeast/Fermentable/etc that we already have stored in the database, so that any
         // containing Recipe etc can refer to it.  The new object we created is still held in
         // this->namedEntityRaiiContainer and will automatically be deleted when we go out of scope.
         this->namedEntity = matchResult.value().get();
         return true;
      }
      qDebug() << Q_FUNC_INFO << "No match found for "<< this->namedEntity->name();
      return false;
   }

   /**
    * \brief Implementation for general case where name is supposed to be unique.  Before storing, we try to ensure
    *        that what we load in does not create duplicate names.  Eg, if we already have a Recipe called "Oatmeal
    *        Stout" and then read in a (different) recipe with the same name, then we will change the name of the
    *        newly read-in one to "Oatmeal Stout (1)" (or "Oatmeal Stout (2)" if "Oatmeal Stout (1)" is taken, and so
    *        on).  For those NamedEntity subclasses where we don't care about duplicate names (eg MashStep records),
    *        there is a no-op specialisation of this function.
    *
    *        See below for trivial specialisations of this function for classes where names are not unique.
    */
   virtual void normaliseName() {
      QString currentName = this->namedEntity->name();

      while (
         auto matchResult = ObjectStoreTyped<NE>::getInstance().findFirstMatching(
            [currentName](std::shared_ptr<NE> ne) {return ne->name() == currentName;}
         )
      ) {
         qDebug() << Q_FUNC_INFO << "Found existing " << this->namedEntityClassName << "named" << currentName;

         XmlRecord::modifyClashingName(currentName);

         //
         // Now the for loop will search again with the new name
         //
         qDebug() << Q_FUNC_INFO << "Trying " << currentName;
      }

      this->namedEntity->setName(currentName);

      return;
   }

   /**
    * \brief Implementation of the general case where the object is independent of its containing entity
    */
   virtual void setContainingEntity(NamedEntity * containingEntity) {
      return;
   }

private:
   /**
    *
    */
   bool includedInStats() const { return true; }

};

// Specialisations for cases where duplicates are allowed
template<> inline bool XmlNamedEntityRecord<Instruction>::isDuplicate() { return false; }
template<> inline bool XmlNamedEntityRecord<Mash>::isDuplicate() { return false; }
template<> inline bool XmlNamedEntityRecord<MashStep>::isDuplicate() { return false; }
template<> inline bool XmlNamedEntityRecord<BrewNote>::isDuplicate() { return false; }

// Specialisations for cases where name is not required to be unique
template<> inline void XmlNamedEntityRecord<Instruction>::normaliseName() { return; }
template<> inline void XmlNamedEntityRecord<Mash>::normaliseName() { return; }
template<> inline void XmlNamedEntityRecord<MashStep>::normaliseName() { return; }
template<> inline void XmlNamedEntityRecord<BrewNote>::normaliseName() { return; }

// Specialisations for cases where object is owned by its containing entity
template<> inline void XmlNamedEntityRecord<BrewNote>::setContainingEntity(NamedEntity * containingEntity) {
   qDebug() << Q_FUNC_INFO << "BrewNote * " << static_cast<void*>(this->namedEntity) << ", Recipe * " << static_cast<void*>(containingEntity);
   BrewNote * brewNote = static_cast<BrewNote *>(this->namedEntity);
   brewNote->setRecipe(static_cast<Recipe *>(containingEntity));
   return;
}

// Specialisations for cases where we don't want the objects included in the stats
template<> inline bool XmlNamedEntityRecord<Instruction>::includedInStats() const { return false; }
template<> inline bool XmlNamedEntityRecord<BrewNote>::includedInStats() const { return false; }
template<> inline bool XmlNamedEntityRecord<MashStep>::includedInStats() const { return false; }


#endif
