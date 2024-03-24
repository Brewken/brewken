/*======================================================================================================================
 * serialization/xml/XmlMashRecord.cpp is part of Brewken, and is copyright the following authors 2021-2022:
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
#include "serialization/xml/XmlMashRecord.h"

void XmlMashRecord::subRecordToXml(XmlRecordDefinition::FieldDefinition const & fieldDefinition,
                                   XmlRecord const & subRecord,
                                   NamedEntity const & namedEntityToExport,
                                   QTextStream & out,
                                   int indentLevel,
                                   char const * const indentString) const {
   //
   // This cast should be safe because Mash & should be what's passed to XmlRecipeRecord::toXml() (which invokes the
   // base class member function which ultimately calls this one with the same parameter).
   //
   Mash const & mash = static_cast<Mash const &>(namedEntityToExport);

   // We assert that MashStep is the only complex record inside a Mash
   Q_ASSERT(fieldDefinition.propertyPath.asXPath() == PropertyNames::Mash::mashSteps);

   auto children = mash.mashSteps();
   if (children.empty()) {
      this->writeNone(subRecord, mash, out, indentLevel, indentString);
   } else {
      for (auto child : children) {
         subRecord.toXml(*child, out, true, indentLevel, indentString);
      }
   }
   return;
}

void XmlMashRecord::setContainingEntity(std::shared_ptr<NamedEntity> containingEntity) {
   // Don't include Mash in stats is it's in a Recipe (ie if the cast below succeeds); DO include it if it's not (ie if
   // there's no containing entity or the cast below fails).
   this->m_includeInStats = (nullptr == dynamic_cast<Recipe *>(containingEntity.get()));
   qDebug() << Q_FUNC_INFO << (this->m_includeInStats ? "Included in" : "Excluded from") << "stats";
   return;
}
