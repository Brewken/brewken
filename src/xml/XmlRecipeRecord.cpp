/*======================================================================================================================
 * xml/XmlRecipeRecord.cpp is part of Brewken, and is copyright the following authors 2020-2023:
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
 =====================================================================================================================*/
#include "xml/XmlRecipeRecord.h"

#include <cstring>
#include <functional>

#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Misc.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"

namespace {
   //
   // To keep us on our toes, the various ingredients you might add to a recipe have different ways of specifying how
   // much to add and when to add them.  We'll use template specialisation to ensure we call the right member
   // functions.
   //
   template<typename CNE>
   void setAmountsEtc([[maybe_unused]] CNE & ingredient, [[maybe_unused]] NamedParameterBundle const & npb) {
      return;
   }
   template<> void setAmountsEtc(Hop & hop, NamedParameterBundle const & npb) {
      hop.setAmount_kg(npb.val<double>(PropertyNames::Hop::amount_kg));
      hop.setTime_min (npb.val<double>(PropertyNames::Hop::time_min ));
      return;
   }
   template<> void setAmountsEtc(Fermentable & fermentable, NamedParameterBundle const & npb) {
      // For Fermentable, assume amount is weight unless otherwise specified because base BeerXML does not include the
      // possibility of fermentables being measured by volume.  (It is an extension we have added as a result of
      // implementing support for BeerJSON.)
      fermentable.setAmount         (npb.val<double>(PropertyNames::Fermentable::amount        ));
      fermentable.setAmountIsWeight (npb.val<bool  >(PropertyNames::Fermentable::amountIsWeight, true));
      fermentable.setAddAfterBoil   (npb.val<bool  >(PropertyNames::Fermentable::addAfterBoil  ));
      fermentable.setIsMashed       (npb.val<bool  >(PropertyNames::Fermentable::isMashed      ));
      return;
   }
   template<> void setAmountsEtc(Misc & misc, NamedParameterBundle const & npb) {
      misc.setAmount        (npb.val<double>(PropertyNames::Misc::amount        ));
      misc.setAmountIsWeight(npb.val<bool  >(PropertyNames::Misc::amountIsWeight));
      misc.setTime          (npb.val<double>(PropertyNames::Misc::time          ));
      return;
   }
   template<> void setAmountsEtc(Yeast & yeast, NamedParameterBundle const & npb) {
      yeast.setAmount        (npb.val<double>(PropertyNames::Yeast::amount        ));
      yeast.setAmountIsWeight(npb.val<bool  >(PropertyNames::Yeast::amountIsWeight));
      return;
   }
   template<> void setAmountsEtc(Water & water, NamedParameterBundle const & npb) {
      water.setAmount(npb.val<double>(PropertyNames::Water::amount));
      return;
   }

}


template<typename CNE>
void XmlRecipeRecord::addChildren() {
   //
   // This cast is safe because we know this->namedEntity was populated with a Recipe * in the constructor of our
   // parent class (XmlNamedEntityRecord<Recipe>).
   //
   auto recipe = std::static_pointer_cast<Recipe>(this->namedEntity);

   char const * const childClassName = CNE::staticMetaObject.className();
   //
   // Previously we stored child records in a QMultiHash, which makes accessing children of a particular type easy but
   // gives an iteration order the opposite of insertion order, which is annoying when order matters (eg for Mash Steps
   // in BeerXML).  Using a list gives us a slightly less elegant loop here, but ensures that
   // normaliseAndStoreChildRecordsInDb() deals with children in the right order.
   //
   for (auto ii : this->childRecords) {
      if (ii.xmlRecord->namedEntityClassName == childClassName) {
         qDebug() <<
            Q_FUNC_INFO << "Adding " << childClassName << "#" << ii.xmlRecord->getNamedEntity()->key() << "to Recipe";

         // It would be a (pretty unexpected) coding error if the NamedEntity subclass object isn't of the class it's
         // supposed to be.
         Q_ASSERT(ii.xmlRecord->getNamedEntity()->metaObject()->className() == QString(childClassName));

         // Actually add the Hop/Yeast/etc to the Recipe
         std::shared_ptr<CNE> child{std::static_pointer_cast<CNE>(ii.xmlRecord->getNamedEntity())};
         auto added = recipe->add<CNE>(child);

         //
         // For historical reasons (specifically that early versions of Brewtarget stored data in BeerXML files, not a
         // database), the amount of each Hop/Fermentable/etc in a Recipe is stored, not in the Recipe object but in the
         // Hop/Fermentable/etc in question.  The same is true for addition times for Hops.
         //
         // When we add something to a Recipe, typically a copy is made so that we have a Hop/Fermentable/etc that is not
         // shared with any other Recipes and thus there is no ambiguity about storing the amount in it.
         //
         // However, when we read in from BeerXML, we try to avoid creating unnecessary duplicates of things.  If there's
         // a Fuggle hop in the file and we already have a Fuggle hop in the database, then we don't create another one
         // for the sake of it.  This is the right thing to do if we're reading in Hops outside the context of a Recipe.
         // But if the hop in the BeerXML file was inside a Recipe record, then we we need to make sure we captured the
         // "how much and when to add" info inside that hop record.
         //
         // So, now that we added the Hop/Fermentable/etc to the Recipe, and we have the actual object associated with the
         // Recipe, we need to set the "how much and when to add" info based on the fields we retained from XML record.
         //
         Q_ASSERT(added);
         NamedParameterBundle const & npb = ii.xmlRecord->getNamedParameterBundle();
         qDebug() <<
            Q_FUNC_INFO << "Setting amounts for" << childClassName << "#" << added->key() <<
            "to Recipe, using bundle" << npb;
         setAmountsEtc(*added, npb);
      }
   }
   return;
}


XmlRecord::ProcessingResult XmlRecipeRecord::normaliseAndStoreInDb(std::shared_ptr<NamedEntity> containingEntity,
                                                                   QTextStream & userMessage,
                                                                   ImportRecordCount & stats) {
   // This call to the base class function will store the Recipe and all the objects it contains, as well as link the
   // Recipe to its Style and Equipment.
   XmlRecord::ProcessingResult result = XmlRecord::normaliseAndStoreInDb(containingEntity, userMessage, stats);
   if (result != XmlRecord::ProcessingResult::Succeeded) {
      // The result was either Failed (= abort) or FoundDuplicate (= stop trying to process the current record), so we
      // bail here.
      return result;
   }

   //
   // We now need to tie some other things together
   //
   this->addChildren<Hop>();
   this->addChildren<Fermentable>();
   this->addChildren<Misc>();
   this->addChildren<Yeast>();
   this->addChildren<Water>();

   this->addChildren<Instruction>();


   // BrewNotes are a bit different than some of the other fields.  Each BrewNote relates to only one Recipe, but the
   // Recipe class does not (currently) have an interface for adding BrewNotes.  It suffices to tell each BrewNote what
   // its Recipe is, something we achieve via template specialisation of XmlNamedEntityRecord::setContainingEntity

   return XmlRecord::ProcessingResult::Succeeded;
}

template<typename CNE>
bool XmlRecipeRecord::childrenToXml(XmlRecord::FieldDefinition const & fieldDefinition,
                                    XmlRecord const & subRecord,
                                    Recipe const & recipe,
                                    QTextStream & out,
                                    int indentLevel,
                                    char const * const indentString,
                                    BtStringConst const & propertyNameForGetter,
                                    RecipeChildGetter<CNE> getter) const {
   if (fieldDefinition.propertyName != propertyNameForGetter) {
      return false;
   }
   QList<CNE *> children = std::invoke(getter, recipe);
   if (children.size() == 0) {
      this->writeNone(subRecord, recipe, out, indentLevel, indentString);
   } else {
      for (CNE * child : children) {
         subRecord.toXml(*child, out, indentLevel, indentString);
      }
   }
   return true;
}

void XmlRecipeRecord::subRecordToXml(XmlRecord::FieldDefinition const & fieldDefinition,
                                     XmlRecord const & subRecord,
                                     NamedEntity const & namedEntityToExport,
                                     QTextStream & out,
                                     int indentLevel,
                                     char const * const indentString) const {
   //
   // This cast should be safe because Recipe & should be what's passed to XmlRecipeRecord::toXml() (which invokes the
   // base class member function which ultimately calls this one with the same parameter).
   //
   Recipe const & recipe = static_cast<Recipe const &>(namedEntityToExport);

   if (this->childrenToXml(fieldDefinition, subRecord, recipe, out, indentLevel, indentString, PropertyNames::Recipe::hops        , &Recipe::hops        )) { return; }
   if (this->childrenToXml(fieldDefinition, subRecord, recipe, out, indentLevel, indentString, PropertyNames::Recipe::fermentables, &Recipe::fermentables)) { return; }
   if (this->childrenToXml(fieldDefinition, subRecord, recipe, out, indentLevel, indentString, PropertyNames::Recipe::miscs       , &Recipe::miscs       )) { return; }
   if (this->childrenToXml(fieldDefinition, subRecord, recipe, out, indentLevel, indentString, PropertyNames::Recipe::yeasts      , &Recipe::yeasts      )) { return; }
   if (this->childrenToXml(fieldDefinition, subRecord, recipe, out, indentLevel, indentString, PropertyNames::Recipe::waters      , &Recipe::waters      )) { return; }
   if (this->childrenToXml(fieldDefinition, subRecord, recipe, out, indentLevel, indentString, PropertyNames::Recipe::instructions, &Recipe::instructions)) { return; }
   if (this->childrenToXml(fieldDefinition, subRecord, recipe, out, indentLevel, indentString, PropertyNames::Recipe::brewNotes   , &Recipe::brewNotes   )) { return; }

   // It's a coding error if we get here
   qCritical() << Q_FUNC_INFO << "Don't know how to export Recipe property " << fieldDefinition.propertyName;
   Q_ASSERT(false); // Stop in a debug build
   return;          // Soldier on in a production build
}
