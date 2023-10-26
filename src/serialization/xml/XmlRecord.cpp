/*======================================================================================================================
 * serialization/xml/XmlRecord.cpp is part of Brewken, and is copyright the following authors 2020-2023:
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
#include "serialization/xml/XmlRecord.h"

#include <QDate>
#include <QDebug>
#include <QXmlStreamWriter>

#include <xalanc/XalanDOM/XalanNodeList.hpp>
#include <xalanc/XPath/NodeRefList.hpp>
#include <xalanc/XPath/XPathEvaluator.hpp>
#include <xalanc/XalanDOM/XalanNamedNodeMap.hpp>

#include "serialization/xml/XmlCoding.h"
#include "utils/OptionalHelpers.h"
#include "utils/ObjectAddressStringMapping.h"

//
// Variables and constant definitions that we need only in this file
//
namespace {
   // See https://apache.github.io/xalan-c/api/XalanNode_8hpp_source.html for possible indexes into this array
   char const * const XALAN_NODE_TYPES[] {
      "UNKNOWN_NODE",                 // = 0,
      "ELEMENT_NODE",                 // = 1,
      "ATTRIBUTE_NODE",               // = 2,
      "TEXT_NODE",                    // = 3,
      "CDATA_SECTION_NODE",           // = 4,
      "ENTITY_REFERENCE_NODE",        // = 5,
      "ENTITY_NODE",                  // = 6,
      "PROCESSING_INSTRUCTION_NODE",  // = 7,
      "COMMENT_NODE",                 // = 8,
      "DOCUMENT_NODE",                // = 9,
      "DOCUMENT_TYPE_NODE",           // = 10,
      "DOCUMENT_FRAGMENT_NODE",       // = 11,
      "NOTATION_NODE",                // = 12
      "UNRECOGNISED!"
   };

   /**
    * \brief Helper function for writing multiple indents
    */
   void writeIndents(QTextStream & out,
                     int indentLevel,
                     char const * const indentString) {
      for (int ii = 0; ii < indentLevel; ++ii) {
         out << indentString;
      };
      return;
   }
}

XmlRecord::XmlRecord(XmlCoding           const & xmlCoding,
                     XmlRecordDefinition const & recordDefinition) :
   SerializationRecord{},
   m_coding{xmlCoding},
   m_recordDefinition{recordDefinition},
   m_childRecordSets{} {
   return;
}

XmlRecord::~XmlRecord() = default;

///QString XmlRecord::m_recordDefinition.m_recordName const {
///   return this->recordName;
///}

bool XmlRecord::load(xalanc::DOMSupport & domSupport,
                     xalanc::XalanNode * rootNodeOfRecord,
                     QTextStream & userMessage) {
   qDebug() << Q_FUNC_INFO;

   xalanc::XPathEvaluator xPathEvaluator;
   //
   // Loop through all the fields that we know/care about.  Anything else is intentionally ignored.  (We won't know
   // what to do with it, and, if it weren't allowed to be there, it would have generated an error at XSD parsing.)
   //
   for (auto & fieldDefinition : this->m_recordDefinition.fieldDefinitions) {
      //
      // NB: If we don't find a node, there's nothing for us to do.  The XSD parsing should already flagged up an error
      // if there are missing _required_ fields or if string fields that are present are not allowed to be blank.  (See
      // comments in BeerXml.xsd for why it is, in practice, plausible and acceptable for some "required" text fields
      // to be empty/blank.)
      //
      // Equally, although we only look for nodes we know about, some of these we won't use.  If there is no property
      // name/path in our field definition then it's a field we neither read nor write.  We'll parse it but we won't try
      // to pass it to the object we're creating.  But there are some fields that are "write only", such as IBU on
      // Recipe.  These have a property name in the field definition, so they will be written out in XmlRecord::toXml,
      // but the relevant object constructor ignores them when they appear in a NamedParameterBundle.  (In the case of
      // IBU on Recipe, this is because it is a calculated value.  It is helpful to some users to export it in the XML,
      // but there is no point trying to read it in from XML as the value would get overwritten by our own calculated
      // one.)
      //
      // We're not expecting multiple instances of simple fields (strings, numbers, etc) and XSD parsing should mostly
      // have flagged up errors if there were any present.  But it is often valid to have multiple child records (eg
      // Hops inside a Recipe).
      //

      //
      // If the current field is using the "Base Record" trick (descibed in serialization/json/JsonRecordDefinition.h)
      // we will have an empty xPath.  Xalan will crash if we ask it to follow an empty xPath, so we need to manually
      // do the no-op navigation (ie pretend that the current XML record is actually a child of itself for the purposes
      // of reading in a new object in our model.
      //
      // There's a bit of extra faffing around here because the XalanC "native" type `xalanc::NodeRefList` is, to all
      // intents and purposes, read-only outside of the XalanC library (eg here in our code).  We need it as an output
      // from xalanc::XPathEvaluator::selectNodeList, but, once we have it populated, it's better to copy its contents
      // into std::vector and use that.
      //
      std::vector<xalanc::XalanNode *> nodesForCurrentXPath;
      if (fieldDefinition.xPath.isEmpty()) {
         // We mark ourselves as our child - something we assert we should only be doing in the case of a Record field
         // type.  (Even then, it's only in certain cases.)
         Q_ASSERT(std::holds_alternative<XmlRecordDefinition const *>(fieldDefinition.valueDecoder));
         nodesForCurrentXPath.push_back(rootNodeOfRecord);
      } else {
         xalanc::NodeRefList tempNodesForCurrentXPath;
         xPathEvaluator.selectNodeList(tempNodesForCurrentXPath,
                                       domSupport,
                                       rootNodeOfRecord,
                                       fieldDefinition.xPath.getXalanString());
         for (xalanc::NodeRefList::size_type ii = 0; ii < tempNodesForCurrentXPath.getLength(); ++ii) {
            nodesForCurrentXPath.push_back(tempNodesForCurrentXPath.item(ii));
         }
      }
      auto numChildNodes = nodesForCurrentXPath.size();
      qDebug() << Q_FUNC_INFO << "Found" << numChildNodes << "node(s) for " << fieldDefinition.xPath;
      if (XmlRecordDefinition::FieldType::Record        == fieldDefinition.type ||
          XmlRecordDefinition::FieldType::ListOfRecords == fieldDefinition.type) {
         //
         // Depending on the context, it may or may not be valid to have multiple children of this type of record (eg
         // a Recipe might have multiple Hops but it only has one Equipment).  We don't really have to worry about that
         // here though as any rules should have been enforced in the XSD.
         //
         Q_ASSERT(std::holds_alternative<XmlRecordDefinition const *>(fieldDefinition.valueDecoder));
         Q_ASSERT(std::get              <XmlRecordDefinition const *>(fieldDefinition.valueDecoder));
         XmlRecordDefinition const & childRecordDefinition{
            *std::get<XmlRecordDefinition const *>(fieldDefinition.valueDecoder)
         };
         if (!this->loadChildRecords(domSupport,
                                     fieldDefinition,
                                     childRecordDefinition,
                                     nodesForCurrentXPath,
                                     userMessage)) {
            return false;
         }
      } else if (numChildNodes > 0) {
         //
         // If the field we're looking at is not a record, so the XSD should mostly have enforced no duplicates.  If
         // there are any though, we'll ignore them.
         //
         if (numChildNodes > 1) {
            qWarning() <<
               Q_FUNC_INFO << numChildNodes << " nodes found with path " << fieldDefinition.xPath << ".  Taking value "
               "only of the first one.";
         }
         xalanc::XalanNode * fieldContainerNode = nodesForCurrentXPath.at(0);

         // Normally the node for the tag will be type ELEMENT_NODE and will not have a value in and of itself.
         // To get the "contents", we need to look at the value of the child node, which, for strings and numbers etc,
         // should be type TEXT_NODE (and name "#text").
         XQString fieldName{fieldContainerNode->getNodeName()};
         xalanc::XalanNodeList const * fieldContents = fieldContainerNode->getChildNodes();
         int numChildrenOfContainerNode = fieldContents->getLength();
         qDebug() <<
            Q_FUNC_INFO << "Node " << fieldDefinition.xPath << "(" << fieldName << ":" <<
            XALAN_NODE_TYPES[fieldContainerNode->getNodeType()] << ") has " <<
            numChildrenOfContainerNode << " children";
         if (0 == numChildrenOfContainerNode) {
            qDebug() << Q_FUNC_INFO << "Empty!";
         } else {
            {
               //
               // The field is not a sub-record, so it must be something simple (a string, number, boolean or enum)
               //
               if (numChildrenOfContainerNode > 1) {
                  // This is probably a coding error, as it would mean the XML node had child nodes, rather than just
                  // text content, which should have already generated an error during XSD validation.
                  qWarning() <<
                     Q_FUNC_INFO << "Node " << fieldDefinition.xPath << " has " <<
                     numChildrenOfContainerNode << " children.  Taking value only of the first one.";
               }
               xalanc::XalanNode * valueNode = fieldContents->item(0);
               XQString value(valueNode->getNodeValue());
               qDebug() << Q_FUNC_INFO << "Value " << value;

               bool parsedValueOk = false;
               QVariant parsedValue;

               // A field should have an enumMapping if and only if it's of type Enum
               // Anything else is a coding error at the caller
               Q_ASSERT((XmlRecordDefinition::FieldType::Enum == fieldDefinition.type) ==
                        std::holds_alternative<EnumStringMapping const *>(fieldDefinition.valueDecoder));

               // Same applies for a unit field
               Q_ASSERT((XmlRecordDefinition::FieldType::Unit == fieldDefinition.type) ==
                        std::holds_alternative<Measurement::UnitStringMapping const *>(fieldDefinition.valueDecoder));

               //
               // We're going to need to know whether this field is "optional" in our internal data model.  If it is,
               // then, for whatever underlying type T it is, we need the parsedValue QVariant to hold std::optional<T>
               // instead of just T.
               //
               // (Note we can't do this mapping inside NamedParameterBundle, as we don't have the type information
               // there.  We could conceivably do it in the constructors that take a NamedParameterBundle parameter, but
               // I think it gets messy to have different types there than on the QProperty setters.  It's not much
               // overhead to do things here IMHO.)
               //
               // Note that:
               //    - propertyName is not actually a property name when fieldType is RequiredConstant
               //    - when propertyName is not set, there is nothing to look up (because this is a field we don't
               //      support, usually an "Extension tag")
               //
               bool const propertyIsOptional {
                  (fieldDefinition.type == XmlRecordDefinition::FieldType::RequiredConstant ||
                   fieldDefinition.propertyPath.isNull()) ?
                     false :
                     fieldDefinition.propertyPath.getTypeInfo(*this->m_recordDefinition.m_typeLookup).isOptional()
               };

               switch (fieldDefinition.type) {

                  case XmlRecordDefinition::FieldType::Bool:
                     // Unlike other XML documents, boolean fields in BeerXML are caps, so we have to accommodate that
                     if (value.toLower() == "true") {
                        parsedValue = Optional::variantFromRaw(true, propertyIsOptional);
                        parsedValueOk = true;
                     } else if (value.toLower() == "false") {
                        parsedValue = Optional::variantFromRaw(false, propertyIsOptional);
                        parsedValueOk = true;
                     } else {
                        // This is almost certainly a coding error, as we should have already validated that the field
                        // via XSD parsing.
                        qWarning() <<
                           Q_FUNC_INFO << "Ignoring " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                           fieldDefinition.xPath << "=" << value << " as could not be parsed as BOOLEAN";
                     }
                     break;

                  case XmlRecordDefinition::FieldType::Int:
                     {
                        // QString's toInt method will report success/failure of parsing straight back into our flag
                        auto const rawValue = value.toInt(&parsedValueOk);
                        parsedValue = Optional::variantFromRaw(rawValue, propertyIsOptional);
                        if (!parsedValueOk) {
                           // This is almost certainly a coding error, as we should have already validated the field via
                           // XSD parsing.
                           qWarning() <<
                              Q_FUNC_INFO << "Ignoring " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                              fieldDefinition.xPath << "=" << value << " as could not be parsed as integer";
                        }
                     }
                     break;

                  case XmlRecordDefinition::FieldType::UInt:
                     {
                        // QString's toUInt method will report success/failure of parsing straight back into our flag
                        auto const rawValue = value.toUInt(&parsedValueOk);
                        parsedValue = Optional::variantFromRaw(rawValue, propertyIsOptional);
                        if (!parsedValueOk) {
                           // This is almost certainly a coding error, as we should have already validated the field via
                           // XSD parsing.
                           qWarning() <<
                              Q_FUNC_INFO << "Ignoring " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                              fieldDefinition.xPath << "=" << value << " as could not be parsed as unsigned integer";
                        }
                     }
                     break;

                  case XmlRecordDefinition::FieldType::Double:
                     {
                        // QString's toDouble method will report success/failure of parsing straight back into our flag
                        auto rawValue = value.toDouble(&parsedValueOk);
                        if (!parsedValueOk) {
                           //
                           // Although it is not explicitly stated in the BeerXML 1.0 standard, it is clear from the
                           // sample files downloadable from www.beerxml.com that some "ignorable" percentage and decimal
                           // values can be specified as "-".  I haven't found a straightforward way to filter or
                           // transform these during XSD validation.  Nor, as yet, do I know whether it's possible from a
                           // xalanc::XalanNode to get back to the Post-Schema-Validation Infoset (PSVI) information in
                           // Xerces that might allow us to examine the XSD rules applied to the current node.
                           //
                           // For the moment, we assume that, if a "-" didn't get filtered out by XSD then it's allowed
                           // and should be interpreted as NULL, which therefore means we store 0.0.
                           //
                           qInfo() <<
                              Q_FUNC_INFO << "Treating " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                              fieldDefinition.xPath << "=" << value << " as 0.0";
                           parsedValueOk = true;
                           rawValue = 0.0;
                        }
                        parsedValue = Optional::variantFromRaw(rawValue, propertyIsOptional);
                     }
                     break;

                  case XmlRecordDefinition::FieldType::Date:
                     {
                        //
                        // Extra braces here as we have a variable (date) that is only used in this case of the switch,
                        // so we need to restrict its scope, otherwise the compiler will complain about the variable
                        // initialisation being "jumped over" in the other case labels.
                        //
                        // Dates are a bit annoying because, in some cases, fields are not restricted to using the One
                        // True Date Format™ (aka ISO 8601).  Eg, in the BeerXML 1.0 standard, for the DATE field of a
                        // Recipe, it merely says 'Date brewed in a easily recognizable format such as “3 Dec 04”', yet
                        // internally we want to store this as a date rather than just a text field.
                        //
                        // So, we make several attempts to parse a date, using various different "standard" encodings.
                        // There is a risk that certain formats are ambiguous - eg 01/04/2021 is 4 January 2021 in
                        // the USA, but 1 April 2021 in most of the rest of the world (except the enlightened countries
                        // that use the One True Date Format) - but there is little we can do about this.
                        //
                        // Start by trying ISO 8601, which is the most logical format :-)
                        //
                        QDate date = QDate::fromString(value, Qt::ISODate);
                        parsedValueOk = date.isValid();
                        if (!parsedValueOk) {
                           // If not ISO 8601, try RFC 2822 Internet Message Format, which is horrible because it
                           // assumes everyone speaks English, but (a) widely used and (b) unambiguous
                           date = QDate::fromString(value, Qt::RFC2822Date);
                           parsedValueOk = date.isValid();
                        }
                        if (!parsedValueOk) {
                           // Next we'll try Qt's "default" date format, which is good for display but not for file
                           // interchange, as it's locale-specific
                           date = QDate::fromString(value, Qt::TextDate);
                           parsedValueOk = date.isValid();
                        }
                        if (!parsedValueOk) {
                           // Now we're rolling our own formats.  See https://doc.qt.io/qt-5/qdate.html for details of
                           // the codes in the format strings.
                           //
                           // Try USA / Philippines numeric format next, though NB this could mis-parse some
                           // non-USA-format dates per example above.  (Historically we assumed USA format dates before
                           // non-USA-format ones, so we're retaining existing behaviour by trying things in this
                           // order.)
                           date = QDate::fromString(value, "M/d/yyyy");
                           parsedValueOk = date.isValid();
                        }
                        if (!parsedValueOk) {
                           // Now try the numeric version that is widely used outside the USA & the Philippines
                           date = QDate::fromString(value, "d/M/yyyy");
                           parsedValueOk = date.isValid();
                        }
                        if (!parsedValueOk) {
                           // Now try the numeric version that is widely used outside the USA & the Philippines
                           date = QDate::fromString(value, "d/M/yyyy");
                           parsedValueOk = date.isValid();
                        }
                        if (!parsedValueOk) {
                           // Now try the example "easily recognizable" format from the BeerXML 1.0 standard.
                           //
                           // Of course, this is a horrible format because it is not Y2K compliant.  So the actual date
                           // we store may be out by 100 years.  Hopefully the user will notice and correct this, and
                           // then if we export we can use a non-ambiguous format.
                           date = QDate::fromString(value, "d MMM yy");
                           parsedValueOk = date.isValid();
                        }
                        // .:TBD:. Maybe we could try some more formats here
                        parsedValue = Optional::variantFromRaw(date, propertyIsOptional);
                     }
                     if (!parsedValueOk) {
                        // This is almost certainly a coding error, as we should have already validated the field via
                        // XSD parsing.
                        qWarning() <<
                           Q_FUNC_INFO << "Ignoring " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                           fieldDefinition.xPath << "=" << value << " as could not be parsed as ISO 8601 date";
                     }
                     break;

                  case XmlRecordDefinition::FieldType::Enum:
                     // It's definitely a coding error if there is no stringToEnum mapping for a field declared as Enum!
                     Q_ASSERT(std::holds_alternative<EnumStringMapping const *>(fieldDefinition.valueDecoder));
                     Q_ASSERT(std::get              <EnumStringMapping const *>(fieldDefinition.valueDecoder));
                     {
                        auto match =
                           std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder)->stringToEnumAsInt(value);
                        if (!match) {
                           // This is probably a coding error as the XSD parsing should already have verified that the
                           // contents of the node are one of the expected values.
                           qWarning() <<
                              Q_FUNC_INFO << "Ignoring " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                              fieldDefinition.xPath << "=" << value << " as value not recognised";
                        } else {
                           auto const rawValue = match.value();
                           parsedValue = Optional::variantFromRaw(rawValue, propertyIsOptional);
                           parsedValueOk = true;
                        }
                     }
                     break;

                  case XmlRecordDefinition::FieldType::Unit:
                     // It's definitely a coding error if there is no mapping for a field declared as Unit
                     Q_ASSERT(std::holds_alternative<Measurement::UnitStringMapping const *>(fieldDefinition.valueDecoder));
                     Q_ASSERT(std::get              <Measurement::UnitStringMapping const *>(fieldDefinition.valueDecoder));
                     {
                        auto const unitMapping =
                           std::get<Measurement::UnitStringMapping const *>(fieldDefinition.valueDecoder);
                        auto match = unitMapping->stringToObjectAddress(value);
                        if (!match) {
                           // This is probably a coding error as the XSD parsing should already have verified that the
                           // contents of the node are one of the expected values.
                           qWarning() <<
                              Q_FUNC_INFO << "Ignoring " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                              fieldDefinition.xPath << "=" << value << " as value not recognised";
                        } else {
                           // We don't currently support Qt Properties holding optional Unit
                           Q_ASSERT(!propertyIsOptional);
                           // parsedValue = Optional::variantFromRaw(match, propertyIsOptional);
                           parsedValue = QVariant::fromValue<Measurement::Unit const *>(match);
                           parsedValueOk = true;
                        }
                     }
                     break;

                  case XmlRecordDefinition::FieldType::RequiredConstant:
                     //
                     // This is a field that is required to be in the XML, but whose value we don't need (and for which
                     // we always write a constant value on output).  At the moment it's only needed for the VERSION tag
                     // in BeerXML.
                     //
                     // Note that, because we abuse the propertyName field to hold the default value (ie what we write
                     // out), we can't carry on to normal processing below.  So jump straight to processing the next
                     // node in the loop (via continue).
                     //
                     qDebug() <<
                        Q_FUNC_INFO << "Skipping " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                        fieldDefinition.xPath << "=" << value << "(" << fieldDefinition.propertyPath.asXPath() <<
                        ") as not useful";
                     continue; // NB: _NOT_break here.  We want to jump straight to the next run through the for loop.

                  // By default we assume it's a string
                  case XmlRecordDefinition::FieldType::String:
                  default:
                     {
                        if (fieldDefinition.type != XmlRecordDefinition::FieldType::String) {
                           // This is almost certainly a coding error in this class as we should be able to parse all the
                           // types callers need us to.
                           qWarning() <<
                              Q_FUNC_INFO << "Treating " << this->m_recordDefinition.m_namedEntityClassName << " node " <<
                              fieldDefinition.xPath << "=" << value << " as string because did not recognise requested "
                              "parse type " << static_cast<int>(fieldDefinition.type);
                        }
                        auto const rawValue = static_cast<QString>(value);
                        parsedValue = Optional::variantFromRaw(rawValue, propertyIsOptional);
                        parsedValueOk = true;
                     }
                     break;
               }

               //
               // What we do if we couldn't parse the value depends.  If it was a value that we didn't need to set on
               // the supplied Hop/Yeast/Recipe/Etc object, then we can just ignore the problem and carry on processing.
               // But, if this was a field we were expecting to use, then it's a problem that we couldn't parse it and
               // we should bail.
               //
               if (!parsedValueOk && !fieldDefinition.propertyPath.isNull()) {
                  userMessage <<
                     "Could not parse " << this->m_recordDefinition.m_namedEntityClassName << " node " << fieldDefinition.xPath << "=" <<
                     value << " into " << fieldDefinition.propertyPath.asXPath();
                  return false;
               }

               //
               // So we've either parsed the value OK or we don't need it (or both)
               //
               // If we do need it, we now store the value
               //
               if (!fieldDefinition.propertyPath.isNull()) {
                  this->m_namedParameterBundle.insert(fieldDefinition.propertyPath, parsedValue);
               }
            }
         }
      }
   }

   //
   // For everything but the root record, we now construct a suitable object (Hop, Recipe, etc) from the
   // NamedParameterBundle (which will be empty for the root record).
   //
   if (!this->m_namedParameterBundle.isEmpty()) {
      this->constructNamedEntity();
   }

   return true;
}

XmlRecord::ProcessingResult XmlRecord::normaliseAndStoreInDb(std::shared_ptr<NamedEntity> containingEntity,
                                                             QTextStream & userMessage,
                                                             ImportRecordCount & stats) {
   if (this->m_namedEntity) {
      qDebug() <<
         Q_FUNC_INFO << "Normalise and store " << this->m_recordDefinition.m_namedEntityClassName << "(" <<
         this->m_namedEntity->metaObject()->className() << "):" << this->m_namedEntity->name();

      //
      // If the object we are reading in is a duplicate of something we already have (and duplicates are not allowed)
      // then skip over this record (and any records it contains).  (This is _not_ an error, so we return true not
      // false in this event.)
      //
      // Note, however, that some objects -- in particular those such as Recipe that contain other objects -- need
      // to be further along in their construction (ie have had all their contained objects added) before we can
      // determine whether they are duplicates.  This is why we check again, after storing in the DB, below.
      //
      if (this->isDuplicate()) {
         qDebug() <<
            Q_FUNC_INFO << "(Early found) duplicate" << this->m_recordDefinition.m_namedEntityClassName <<
            (this->m_includeInStats ? " will" : " won't") << " be included in stats";
         if (this->m_includeInStats) {
            stats.skipped(*this->m_recordDefinition.m_namedEntityClassName);
         }
         return XmlRecord::ProcessingResult::FoundDuplicate;
      }

      this->normaliseName();

      // Some classes of object are owned by their containing entity and can't sensibly be saved without knowing what it
      // is.  Subclasses of XmlRecord will override setContainingEntity() to pass the info in if it is needed (or ignore
      // it if not).
      this->setContainingEntity(containingEntity);

      // Now we're ready to store in the DB
      int id = this->storeNamedEntityInDb();
      if (id <= 0) {
         userMessage << "Error storing " << this->m_namedEntity->metaObject()->className() <<
         " in database.  See logs for more details";
         return XmlRecord::ProcessingResult::Failed;
      }
   }

   XmlRecord::ProcessingResult processingResult;

   //
   // Finally (well, nearly) orchestrate storing any contained records
   //
   // Note, of course, that this still needs to be done, even if nullptr == this->m_namedEntity, because that just means
   // we're processing the root node.
   //
   if (this->normaliseAndStoreChildRecordsInDb(userMessage, stats)) {
      //
      // Now all the processing succeeded, we do that final duplicate check for any complex object such as Recipe that
      // had to be fully constructed before we could meaningfully check whether it's the same as something we already
      // have in the object store.
      //
      if (nullptr == this->m_namedEntity.get()) {
         // Child records OK and no duplicate check needed (root record), which also means no further processing
         // required
         return XmlRecord::ProcessingResult::Succeeded;
      }
      processingResult = this->isDuplicate() ? XmlRecord::ProcessingResult::FoundDuplicate :
                                               XmlRecord::ProcessingResult::Succeeded;
   } else {
      // There was a problem with one of our child records
      processingResult = XmlRecord::ProcessingResult::Failed;
   }

   if (nullptr != this->m_namedEntity.get()) {
      //
      // We potentially do stats for everything except failure
      //
      if (XmlRecord::ProcessingResult::FoundDuplicate == processingResult) {
         qDebug() <<
            Q_FUNC_INFO << "(Late found) duplicate" << this->m_recordDefinition.m_namedEntityClassName <<
            (this->m_includeInStats ? " will" : " won't") << " be included in stats";
         if (this->m_includeInStats) {
            stats.skipped(*this->m_recordDefinition.m_namedEntityClassName);
         }
      } else if (XmlRecord::ProcessingResult::Succeeded == processingResult && this->m_includeInStats) {
         stats.processedOk(*this->m_recordDefinition.m_namedEntityClassName);
      }

      //
      // Clean-up
      //
      if (XmlRecord::ProcessingResult::FoundDuplicate == processingResult ||
          XmlRecord::ProcessingResult::Failed == processingResult) {
         //
         // If we reach here, it means either there was a problem with one of our child records or we ourselves are a
         // late-detected duplicate.  We've already stored our NamedEntity record in the DB, so we need to try to undo
         // that by deleting it.  It is the responsibility of each NamedEntity subclass to take care of deleting any
         // owned stored objects, via the virtual member function NamedEntity::hardDeleteOwnedEntities().  So we don't
         // have to worry about child records that have already been stored.  (Eg if this is a Mash, and we stored it
         // and 2 MashSteps before hitting an error on the 3rd MashStep, then deleting the Mash from the DB will also
         // result in those 2 stored MashSteps getting deleted from the DB.)
         //
         qDebug() <<
            Q_FUNC_INFO << "Deleting stored" << this->m_recordDefinition.m_namedEntityClassName << "as" <<
            (XmlRecord::ProcessingResult::FoundDuplicate == processingResult ? "duplicate" : "failed to read all child records");
         this->deleteNamedEntityFromDb();
      }
   }

   return processingResult;
}

bool XmlRecord::normaliseAndStoreChildRecordsInDb(QTextStream & userMessage,
                                                  ImportRecordCount & stats) {
   //
   // We are assuming it does not matter which order different children are processed in.
   //
   // Where there are several children of the same type, we need to process them in the same order as they were read in
   // from the XML document because, in some cases, this order matters.  In particular, in BeerXML, the Mash Steps
   // inside a Mash (or rather MASH_STEP tags inside a MASH_STEPS tag inside a MASH tag) are stored in order without any
   // other means of identifying order.
   //
   // So it's simplest just to process all the child records in the order they were read out of the XML document.  This
   // is the advantage of storing things in a list such as QVector.  (Alternatives such as QMultiHash iterate through
   // items that share the same key in the opposite order to which they were inserted and don't offer STL reverse
   // iterators, so going backwards would be a bit clunky.)
   //
   for (auto & childRecordSet : this->m_childRecordSets) {
      if (childRecordSet.parentFieldDefinition) {
         qDebug() <<
            Q_FUNC_INFO << childRecordSet.parentFieldDefinition->propertyPath << "has" <<
            childRecordSet.records.size() << "entries";
      } else {
         qDebug() << Q_FUNC_INFO << "Top-level record has" << childRecordSet.records.size() << "entries";
      }

      QList< std::shared_ptr<NamedEntity> > processedChildren;
      for (auto & childRecord : childRecordSet.records) {
         // The childRecord variable is a reference to a std::unique_ptr (because the vector we're looping over owns the
         // records it contains), which is why we have all the "member of pointer" (->) operators below.
         qDebug() <<
            Q_FUNC_INFO << "Storing" << childRecord->m_recordDefinition.m_namedEntityClassName << "child of" <<
            this->m_recordDefinition.m_namedEntityClassName;
         if (XmlRecord::ProcessingResult::Failed ==
            childRecord->normaliseAndStoreInDb(this->m_namedEntity, userMessage, stats)) {
            return false;
         }
         processedChildren.append(childRecord->m_namedEntity);
      }

      //
      // Now we've stored the child record (or recognised it as a duplicate of one we already hold), we want to link it
      // (or as the case may be the record it's a duplicate of) to the parent.  If this is possible via a property (eg
      // the style on a recipe), then we can just do that here.  Otherwise the work needs to be done in the appropriate
      // subclass of XmlNamedEntityRecord.
      //
      // We can't use the presence or absence of a property name to determine whether the child record can be set via
      // a property because some properties are read-only (and need to be present in the FieldDefinition for export to
      // XML to work).  Instead we distinguish between two types of records: Record, which can be set via a
      // property, and ListOfRecords, which can't.
      //
      if (childRecordSet.parentFieldDefinition) {
         auto const & propertyPath = childRecordSet.parentFieldDefinition->propertyPath;
         if (!propertyPath.isNull()) {
            // It's a coding error if we had a property defined for a record that's not trying to populate a NamedEntity
            // (ie for the root record).
            Q_ASSERT(this->m_namedEntity);

            QVariant valueToSet;
            //
            // How we set the property depends on whether this is a single child record or an array of them
            //
            if (childRecordSet.parentFieldDefinition->type != XmlRecordDefinition::FieldType::ListOfRecords) {
               // It's a coding error if we ended up with more than on child when there's only supposed to be one!
               if (processedChildren.size() > 1) {
                  qCritical() <<
                     Q_FUNC_INFO << "Only expecting one record for" << propertyPath << "property on" <<
                     this->m_recordDefinition.m_namedEntityClassName << "object, but found" << processedChildren.size();
                  Q_ASSERT(false);
               }
               // TBD: For the moment we are assuming single-item setters always take raw pointers
               valueToSet = QVariant::fromValue(processedChildren.first().get());
            } else {
               // Multi-item setters for class T all take a list of shared pointers to T, so we need to upcast from our
               // list of shared pointers to NamedEntity.
               valueToSet = this->m_recordDefinition.m_listUpcaster(processedChildren);
            }

            qDebug() <<
               Q_FUNC_INFO << "Setting" << propertyPath << "property on" <<
               this->m_recordDefinition.m_namedEntityClassName << "with" << processedChildren.size() << "value(s)";
            if (!propertyPath.setValue(*this->m_namedEntity, valueToSet)) {
               // It's a coding error if we could not set the property we use to pass in the child records
               qCritical() <<
                  Q_FUNC_INFO << "Could not write" << propertyPath << "property on" <<
                  this->m_recordDefinition.m_namedEntityClassName;
                  // TODO Reinstate this assert once all the RecipeAddition work is done!
//                  Q_ASSERT(false);
            }
         }
      }
   }

///      if (XmlRecordDefinition::FieldType::Record == ii->fieldDefinition->fieldType) {
///         char const * const propertyName = *ii->fieldDefinition->propertyName;
///         Q_ASSERT(nullptr != propertyName);
///         // It's a coding error if we had a property defined for a record that's not trying to populate a NamedEntity
///         // (ie for the root record).
///         Q_ASSERT(nullptr != this->m_namedEntity.get());
///         // It's a coding error if we're trying to set a non-existent property on the NamedEntity subclass for this
///         // record.
///         QMetaObject const * metaObject = this->m_namedEntity->metaObject();
///         int propertyIndex = metaObject->indexOfProperty(propertyName);
///         Q_ASSERT(propertyIndex >= 0);
///         QMetaProperty metaProperty = metaObject->property(propertyIndex);
///         Q_ASSERT(metaProperty.isWritable());
///         // It's a coding error if we can't create a valid QVariant from a pointer to class we are trying to "set"
///         Q_ASSERT(QVariant::fromValue(ii->xmlRecord->namedEntity.get()).isValid());
///
///         qDebug() <<
///            Q_FUNC_INFO << "Setting" << propertyName << "property (type = " <<
///            this->m_namedEntity->metaObject()->property(
///               this->m_namedEntity->metaObject()->indexOfProperty(propertyName)
///            ).typeName() << ") on" << this->m_recordDefinition.m_namedEntityClassName << "object";
///         this->m_namedEntity->setProperty(propertyName,
///                                        QVariant::fromValue(ii->xmlRecord->namedEntity.get()));
///      }
///   }
   return true;
}


[[nodiscard]] bool XmlRecord::loadChildRecords(xalanc::DOMSupport & domSupport,
                                               XmlRecordDefinition::FieldDefinition const & parentFieldDefinition,
                                               XmlRecordDefinition const & childRecordDefinition,
                                               std::vector<xalanc::XalanNode *> & nodesForCurrentXPath,
                                               QTextStream & userMessage) {
   //
   // This is where we have one or more substantive records of a particular type inside the one we are
   // reading - eg some Hops inside a Recipe.  So we need to loop though these "child" records and read
   // each one in with an XmlRecord object of the relevant type.
   //
   // Note an advantage of using XPaths means we can just "see through" any grouping or containing nodes.
   // For instance, in BeerXML, inside a <RECIPE>...</RECIPE> record there will be a <HOPS>...</HOPS>
   // "record set" node containing the <HOP>...</HOP> record(s) for this recipe, but we can just say in our
   // this->fieldDefinitions that we want the "HOPS/HOP" nodes inside a "RECIPE" and thus skip straight to
   // having a list of all the <HOP>...</HOP> nodes without having to explicitly parse the <HOPS>...</HOPS>
   // node.
   //
   auto constructorWrapper = childRecordDefinition.xmlRecordConstructorWrapper;
   this->m_childRecordSets.push_back(XmlRecord::ChildRecordSet{&parentFieldDefinition, {}});
   XmlRecord::ChildRecordSet & childRecordSet = this->m_childRecordSets.back();
   for (xalanc::XalanNode * childRecordNode : nodesForCurrentXPath) {
      //
      // It's a coding error if we don't recognise the type of node that we've been configured (via
      // this->fieldDefinitions) to read in.  Again, an advantage of using XPaths is that we just
      // automatically ignore nodes we're not looking for.  Eg, imagine, in a BeerXML file, there's the
      // following:
      //    <RECIPE>
      //    ...
      //       <HOPS>
      //          <FOO>...</FOO>
      //          <BAR>...</BAR>
      //          <HOP>...</HOP>
      //       </HOPS>...
      //    ...
      //    </RECIPE>
      // Requesting the HOPS/HOP subpath of RECIPE will not return FOO or BAR
      //
      XQString childRecordName{childRecordNode->getNodeName()};
      qDebug() << Q_FUNC_INFO << childRecordName;

      std::unique_ptr<XmlRecord> childRecord{
         constructorWrapper(this->m_coding, childRecordDefinition)
      };

      //
      // The return value of xalanc::XalanNode::getIndex() doesn't have an instantly obvious direct meaning, but AFAICT
      // higher values are for nodes that were later in the input file, so useful to log.
      //
      qDebug() <<
         Q_FUNC_INFO << "Loading child record" << childRecordName << "with index" << childRecordNode->getIndex() <<
         "for" << childRecordDefinition.m_namedEntityClassName;
      if (!childRecord->load(domSupport, childRecordNode, userMessage)) {
         return false;
      }
      childRecordSet.records.push_back(std::move(childRecord));
   }

   return true;
}

void XmlRecord::toXml(NamedEntity const & namedEntityToExport,
                      QTextStream & out,
                      bool const includeRecordNameTags,
                      int indentLevel,
                      char const * const indentString) const {
   // Callers are not allowed to supply null indent string
   Q_ASSERT(nullptr != indentString);
   qDebug() <<
      Q_FUNC_INFO << "Exporting XML for" << namedEntityToExport.metaObject()->className() << "#" <<
      namedEntityToExport.key();
   if (includeRecordNameTags) {
      writeIndents(out, indentLevel, indentString);
      out << "<" << this->m_recordDefinition.m_recordName << ">\n";
   }

   // For the moment, we are constructing XML output without using Xerces (or similar), on the grounds that, in this
   // direction (ie to XML rather than from XML), it's a pretty simple algorithm and we don't need to validate anything
   // (because we assume that our own data is valid).

   // BeerXML doesn't care about field order, so we don't either (though it would be relatively small additional work
   // to control field order precisely).
   for (auto & fieldDefinition : this->m_recordDefinition.fieldDefinitions) {
      // If there isn't a property name that means this is not a field we support so there's nothing to write out.
      if (fieldDefinition.propertyPath.isNull()) {
         // At the moment at least, we support all XmlRecord::Record and XmlRecord::ListOfRecords fields, so it's
         // a coding error if one of them does not have a property name.
         Q_ASSERT(XmlRecordDefinition::FieldType::Record  != fieldDefinition.type);
         Q_ASSERT(XmlRecordDefinition::FieldType::ListOfRecords != fieldDefinition.type);
         continue;
      }

      // Nested record fields are of two types.  XmlRecord::Record can be handled generically.
      // XmlRecord::ListOfRecords need to be handled in part by subclasses.
      if (XmlRecordDefinition::FieldType::Record  == fieldDefinition.type ||
          XmlRecordDefinition::FieldType::ListOfRecords == fieldDefinition.type) {
         //
         // Some of the work is generic, so we do it here.  In particular, we can work out what tags are needed to
         // contain the record (from the XPath, if any, prior to the last slash), but also what type of XmlRecord(s) we
         // will need by looking at the end of the XPath for this field.
         //
         // (In BeerXML, these contained XPaths are only 1-2 elements, so numContainingTags is always 0 or 1.  If and
         // when we support a different XML coding, we might need to look at this code more closely.)
         //
         // In certain circumstances, the XPath will be "" for essentially the same reasons as described in the "base
         // records" comment in serialization/json/JsonRecordDefinition.h on JsonRecordDefinition::FieldType::Record.
         // In this case, numContainingTags will be -1.
         //
         QStringList xPathElements = fieldDefinition.xPath.split("/");
         Q_ASSERT(xPathElements.size() >= 1);
         int numContainingTags = xPathElements.size() - 1;
         for (int ii = 0; ii < numContainingTags; ++ii) {
            writeIndents(out, indentLevel + 1 + ii, indentString);
            out << "<" << xPathElements.at(ii) << ">\n";
         }
         qDebug() << Q_FUNC_INFO << xPathElements;
         qDebug() << Q_FUNC_INFO << xPathElements.last();
///         std::shared_ptr<XmlRecord> subRecord = this->m_coding.getNewXmlRecord(xPathElements.last());
         std::unique_ptr<XmlRecord> subRecord{this->m_recordDefinition.makeRecord(this->m_coding)};

         if (XmlRecordDefinition::FieldType::Record == fieldDefinition.type) {
            NamedEntity * childNamedEntity =
               fieldDefinition.propertyPath.getValue(namedEntityToExport).value<NamedEntity *>();
            if (childNamedEntity) {
               // For a "base record", having numContainingTags == -1 means the fourth parameter here is still correct!
               subRecord->toXml(*childNamedEntity,
                                out,
                                numContainingTags >= 0,
                                indentLevel + numContainingTags + 1,
                                indentString);
            } else {
               this->writeNone(*subRecord, namedEntityToExport, out, indentLevel + numContainingTags + 1, indentString);
            }
         } else {
            //
            // In theory we could get a list of the contained records via the Qt Property system.  However, the
            // different things we would get back inside the QVariant (QList<BrewNote *>, QList<Hop *> etc) have no
            // common base class, so we can't safely treat them as, or upcast them to, QList<NamedEntity *>.
            //
            // Instead, we get the subclass of this class (eg XmlRecipeRecord) to do the work
            //
            this->subRecordToXml(fieldDefinition,
                                 *subRecord,
                                 namedEntityToExport,
                                 out,
                                 indentLevel + numContainingTags + 1,
                                 indentString);
         }

         // Obviously closing tags need to be written out in reverse order
         for (int ii = numContainingTags - 1; ii >= 0 ; --ii) {
            writeIndents(out, indentLevel + 1 + ii, indentString);
            out << "</" << xPathElements.at(ii) << ">\n";
         }
         continue;
      }

      QString valueAsText;
      if (fieldDefinition.type == XmlRecordDefinition::FieldType::RequiredConstant) {
         //
         // This is a field that is required to be in the XML, but whose value we don't need, and for which we always
         // write a constant value on output.  At the moment it's only needed for the VERSION tag in BeerXML.
         //
         // Because it's such an edge case, we abuse the propertyName field to hold the default value (ie what we
         // write out).  This saves having an extra almost-never-used field on XmlRecordDefinition::FieldDefinition.
         //
         valueAsText = fieldDefinition.propertyPath.asXPath();
      } else {
         QVariant value = fieldDefinition.propertyPath.getValue(namedEntityToExport);
         Q_ASSERT(value.isValid());

         // It's a coding error if we are trying here to write out some field with a complex XPath
         if (fieldDefinition.xPath.contains("/")) {
            qCritical() << Q_FUNC_INFO <<
               "Invalid use of non-trivial XPath (" << fieldDefinition.xPath << ") for output of property" <<
               fieldDefinition.propertyPath.asXPath() << "of" << namedEntityToExport.metaObject()->className();
            Q_ASSERT(false); // Stop here on a debug build
            continue;        // Soldier on in a prod build
         }

         //
         // If the Qt property is an optional value, we need to unwrap it from std::optional and then, if it's null,
         // skip writing it out.  Strong typing of std::optional makes this a bit more work here (but it helps us in
         // other ways elsewhere).
         //
         // Note that:
         //    - propertyName is not actually a property name when fieldType is RequiredConstant
         //    - when propertyName is not set, there is nothing to look up (because this is a field we don't support,
         //      usually an "Extension tag")
         //
         bool const propertyIsOptional {
            (fieldDefinition.type == XmlRecordDefinition::FieldType::RequiredConstant) ?
               false : fieldDefinition.propertyPath.getTypeInfo(*this->m_recordDefinition.m_typeLookup).isOptional()
         };
         switch (fieldDefinition.type) {

            case XmlRecordDefinition::FieldType::Bool:
               if (Optional::removeOptionalWrapperIfPresent<bool>(value, propertyIsOptional)) {
                  // Unlike other XML documents, boolean fields in BeerXML are caps, so we have to accommodate that
                  valueAsText = value.toBool() ? "TRUE" : "FALSE";
               }
               break;

            case XmlRecordDefinition::FieldType::Int:
               if (Optional::removeOptionalWrapperIfPresent<int>(value, propertyIsOptional)) {
                  // QVariant knows how to convert a number to a string
                  valueAsText = value.toString();
               }
               break;

            case XmlRecordDefinition::FieldType::UInt:
               if (Optional::removeOptionalWrapperIfPresent<unsigned int>(value, propertyIsOptional)) {
                  // QVariant knows how to convert a number to a string
                  valueAsText = value.toString();
               }
               break;

            case XmlRecordDefinition::FieldType::Double:
               if (Optional::removeOptionalWrapperIfPresent<double>(value, propertyIsOptional)) {
                  // QVariant knows how to convert a number to a string.  However, for a double, we want to have a bit
                  // more control over the conversion.  In particular, we want to avoid the number coming out in
                  // scientific notation.

                  valueAsText = QString::number(value.toDouble(), 'f', QLocale::FloatingPointShortest);
               }
               break;

            case XmlRecordDefinition::FieldType::Date:
               if (Optional::removeOptionalWrapperIfPresent<QDate>(value, propertyIsOptional)) {
                  // There is only one true date format :-)
                  valueAsText = value.toDate().toString(Qt::ISODate);
               }
               break;

            case XmlRecordDefinition::FieldType::Enum:
               // It's definitely a coding error if there is no enumMapping for a field declared as Enum!
               Q_ASSERT(std::holds_alternative<EnumStringMapping const *>(fieldDefinition.valueDecoder));
               Q_ASSERT(std::get              <EnumStringMapping const *>(fieldDefinition.valueDecoder));
               // A non-optional enum should always be convertible to an int; and we always ensure that an optional one is
               // returned as std::optional<int> when accessed via the Qt property system.
               if (Optional::removeOptionalWrapperIfPresent<int>(value, propertyIsOptional)) {
                  auto match =
                     std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder)->enumAsIntToString(value.toInt());
                  // It's a coding error if we couldn't find a string representation for the enum
                  Q_ASSERT(match && !match->isEmpty());
                  valueAsText = *match;
               }
               break;

            case XmlRecordDefinition::FieldType::Unit:
               // It's definitely a coding error if there is no mapping for a field declared as Unit!
               Q_ASSERT(std::holds_alternative<Measurement::UnitStringMapping const *>(fieldDefinition.valueDecoder));
               Q_ASSERT(std::get              <Measurement::UnitStringMapping const *>(fieldDefinition.valueDecoder));
               // We don't currently support Qt Properties holding optional Unit
               Q_ASSERT(!propertyIsOptional);
               //if (Optional::removeOptionalWrapperIfPresent<Measurement::Unit const *>(value, propertyIsOptional)) {
               {
                  auto const unitMapping =
                     std::get<Measurement::UnitStringMapping const *>(fieldDefinition.valueDecoder);
                  auto match = unitMapping->objectAddressToString(value.value<Measurement::Unit const *>());
                  // It's a coding error if we couldn't find a string representation for the unit
                  Q_ASSERT(!match.isEmpty());
                  valueAsText = match;
               }
               break;

            // By default we assume it's a string
            case XmlRecordDefinition::FieldType::String:
            default:
               if (Optional::removeOptionalWrapperIfPresent<QString>(value, propertyIsOptional)) {
                  // We use this to escape "&" to "&amp;" and so on in string content.  (Other data types should not
                  // have anything in their string representation that needs escaping in XML.)
                  QXmlStreamWriter qXmlStreamWriter(&valueAsText);
                  qXmlStreamWriter.writeCharacters(value.toString());
               }
               break;
         }

         if (propertyIsOptional && value.isNull()) {
            qDebug() <<
               Q_FUNC_INFO << "Not writing XPath" << fieldDefinition.xPath << "as property" <<
               fieldDefinition.propertyPath.asXPath() << "is unset, ie set to std::nullopt";
            continue;
         }
      }

      writeIndents(out, indentLevel + 1, indentString);
      out << "<" << fieldDefinition.xPath << ">" << valueAsText << "</" << fieldDefinition.xPath << ">\n";
   }

   if (includeRecordNameTags) {
      writeIndents(out, indentLevel, indentString);
      out << "</" << this->m_recordDefinition.m_recordName << ">\n";
   }
   return;
}

void XmlRecord::subRecordToXml(XmlRecordDefinition::FieldDefinition const & fieldDefinition,
                               [[maybe_unused]] XmlRecord const & subRecord,
                               NamedEntity const & namedEntityToExport,
                               [[maybe_unused]] QTextStream & out,
                               [[maybe_unused]] int indentLevel,
                               [[maybe_unused]] char const * const indentString) const {
   // Base class does not know how to handle nested records
   // It's a coding error if we get here as this virtual member function should be overridden classes that have nested
   // records.
   qCritical() << Q_FUNC_INFO <<
      "Coding error: cannot export" << namedEntityToExport.metaObject()->className() << "(" <<
      this->m_recordDefinition.m_namedEntityClassName << ") property" << fieldDefinition.propertyPath.asXPath() <<
      "to <" << fieldDefinition.xPath << "> from base class XmlRecord";
   Q_ASSERT(false);
   return;
}

void XmlRecord::writeNone(XmlRecord const & subRecord,
                          NamedEntity const & namedEntityToExport,
                          QTextStream & out,
                          int indentLevel,
                          char const * const indentString) const {
   //
   // The fact that we don't have anything to write for a particular subrecord may or may not be a problem in a given
   // XML coding.  Eg, we allow a recipe to exist without a style, equipment or mash, but, in BeerXML, only the latter
   // two of these three are optional.  For the moment we just log what's going on.
   //
   qInfo() <<
      Q_FUNC_INFO << "Skipping" << subRecord.m_recordDefinition.m_recordName << "tag while exporting" <<
      this->m_recordDefinition.m_recordName << "XML record for" << namedEntityToExport.metaObject()->className() <<
      "as no data to write";
   writeIndents(out, indentLevel, indentString);
   out <<
      "<!-- No " << subRecord.m_recordDefinition.m_recordName << " in this " <<
      this->m_recordDefinition.m_recordName << " -->\n";
   return;
}
