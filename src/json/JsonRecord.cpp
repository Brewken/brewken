/*======================================================================================================================
 * json/JsonRecord.cpp is part of Brewken, and is copyright the following authors 2020-2022:
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
#include "json/JsonRecord.h"

#include <QDate>
#include <QDebug>

#include "json/JsonCoding.h"
#include "utils/ImportRecordCount.h"

//
// Variables and constant definitions that we need only in this file
//
namespace {
}

JsonRecord::JsonRecord(QString const & recordName,
                     JsonCoding const & jsonCoding,
                     FieldDefinitions const & fieldDefinitions,
                     QString const & namedEntityClassName) :
   recordName{recordName},
   jsonCoding{jsonCoding},
   fieldDefinitions{fieldDefinitions},
   namedEntityClassName{namedEntityClassName},
   namedParameterBundle{NamedParameterBundle::NotStrict},
   namedEntity{nullptr},
   includeInStats{true},
   childRecords{} {
   return;
}

QString JsonRecord::getRecordName() const {
   return this->recordName;
}

NamedParameterBundle const & JsonRecord::getNamedParameterBundle() const {
   return this->namedParameterBundle;
}

std::shared_ptr<NamedEntity> JsonRecord::getNamedEntity() const {
   return this->namedEntity;
}

/*
bool JsonRecord::load(QTextStream & userMessage) {
   qDebug() << Q_FUNC_INFO;


   return true;
}*/

void JsonRecord::constructNamedEntity() {
   // Base class does not have a NamedEntity or a container, so nothing to do
   // Stictly, it's a coding error if this function is called, as caller should first check whether there is a
   // NamedEntity, and subclasses that do have one should override this function.
   Q_ASSERT(false && "Trying to construct named entity for base record");
   return;
}

int JsonRecord::storeNamedEntityInDb() {
   Q_ASSERT(false && "Trying to store named entity for base record");
   return -1;
}

void JsonRecord::deleteNamedEntityFromDb() {
   Q_ASSERT(false && "Trying to delete named entity for base record");
   return;
}

/*JsonRecord::ProcessingResult JsonRecord::normaliseAndStoreInDb(std::shared_ptr<NamedEntity> containingEntity,
                                                             QTextStream & userMessage,
                                                             ImportRecordCount & stats) {
   if (nullptr != this->namedEntity) {
      qDebug() <<
         Q_FUNC_INFO << "Normalise and store " << this->namedEntityClassName << "(" <<
         this->namedEntity->metaObject()->className() << "):" << this->namedEntity->name();

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
            Q_FUNC_INFO << "(Early found) duplicate" << this->namedEntityClassName <<
            (this->includeInStats ? " will" : " won't") << " be included in stats";
         if (this->includeInStats) {
            stats.skipped(this->namedEntityClassName.toLower());
         }
         return JsonRecord::ProcessingResult::FoundDuplicate;
      }

      this->normaliseName();

      // Some classes of object are owned by their containing entity and can't sensibly be saved without knowing what it
      // is.  Subclasses of JsonRecord will override setContainingEntity() to pass the info in if it is needed (or ignore
      // it if not).
      this->setContainingEntity(containingEntity);

      // Now we're ready to store in the DB
      int id = this->storeNamedEntityInDb();
      if (id <= 0) {
         userMessage << "Error storing" << this->namedEntity->metaObject()->className() <<
         "in database.  See logs for more details";
         return JsonRecord::ProcessingResult::Failed;
      }
   }

   JsonRecord::ProcessingResult processingResult;

   //
   // Finally (well, nearly) orchestrate storing any contained records
   //
   // Note, of course, that this still needs to be done, even if nullptr == this->namedEntity, because that just means
   // we're processing the root node.
   //
   if (this->normaliseAndStoreChildRecordsInDb(userMessage, stats)) {
      //
      // Now all the processing succeeded, we do that final duplicate check for any complex object such as Recipe that
      // had to be fully constructed before we could meaningfully check whether it's the same as something we already
      // have in the object store.
      //
      if (nullptr == this->namedEntity.get()) {
         // Child records OK and no duplicate check needed (root record), which also means no further processing
         // required
         return JsonRecord::ProcessingResult::Succeeded;
      }
      processingResult = this->isDuplicate() ? JsonRecord::ProcessingResult::FoundDuplicate :
                                               JsonRecord::ProcessingResult::Succeeded;
   } else {
      // There was a problem with one of our child records
      processingResult = JsonRecord::ProcessingResult::Failed;
   }

   if (nullptr != this->namedEntity.get()) {
      //
      // We potentially do stats for everything except failure
      //
      if (JsonRecord::ProcessingResult::FoundDuplicate == processingResult) {
         qDebug() <<
            Q_FUNC_INFO << "(Late found) duplicate" << this->namedEntityClassName <<
            (this->includeInStats ? " will" : " won't") << " be included in stats";
         if (this->includeInStats) {
            stats.skipped(this->namedEntityClassName.toLower());
         }
      } else if (JsonRecord::ProcessingResult::Succeeded == processingResult && this->includeInStats) {
         stats.processedOk(this->namedEntityClassName.toLower());
      }

      //
      // Clean-up
      //
      if (JsonRecord::ProcessingResult::FoundDuplicate == processingResult ||
          JsonRecord::ProcessingResult::Failed == processingResult) {
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
            Q_FUNC_INFO << "Deleting stored" << this->namedEntityClassName << "as" <<
            (JsonRecord::ProcessingResult::FoundDuplicate == processingResult ? "duplicate" : "failed to read all child records");
         this->deleteNamedEntityFromDb();
      }
   }

   return processingResult;
}*/


/*bool JsonRecord::normaliseAndStoreChildRecordsInDb(QTextStream & userMessage,
                                                  ImportRecordCount & stats) {
   //
   // We are assuming it does not matter which order different children are processed in.
   //
   // Where there are several children of the same type, we need to process them in the same order as they were read in
   // from the JSON document because, in some cases, this order matters.  In particular, in BeerJSON, the Mash Steps
   // inside a Mash (or rather MASH_STEP tags inside a MASH_STEPS tag inside a MASH tag) are stored in order without any
   // other means of identifying order.
   //
   // So it's simplest just to process all the child records in the order they were read out of the JSON document.  This
   // is the advantage of storing things in a list such as QVector.  (Alternatives such as QMultiHash iterate through
   // items that share the same key in the opposite order to which they were inserted and don't offer STL reverse
   // iterators, so going backwards would be a bit clunky.)
   //
   for (auto ii = this->childRecords.begin(); ii != this->childRecords.end(); ++ii) {
      qDebug() <<
         Q_FUNC_INFO << "Storing" << ii->jsonRecord->namedEntityClassName << "child of" << this->namedEntityClassName;
      if (JsonRecord::ProcessingResult::Failed ==
         ii->jsonRecord->normaliseAndStoreInDb(this->namedEntity, userMessage, stats)) {
         return false;
      }
      //
      // Now we've stored the child record (or recognised it as a duplicate of one we already hold), we want to link it
      // (or as the case may be the record it's a duplicate of) to the parent.  If this is possible via a property (eg
      // the style on a recipe), then we can just do that here.  Otherwise the work needs to be done in the appropriate
      // subclass of JsonNamedEntityRecord.
      //
      // We can't use the presence or absence of a property name to determine whether the child record can be set via
      // a property because some properties are read-only (and need to be present in the FieldDefinition for export to
      // JSON to work).  Instead we distinguish between two types of records: RecordSimple, which can be set via a
      // property, and RecordComplex, which can't.
      //
      if (JsonRecord::FieldType::RecordSimple == ii->fieldDefinition->fieldType) {
         char const * const propertyName = *ii->fieldDefinition->propertyName;
         Q_ASSERT(nullptr != propertyName);
         // It's a coding error if we had a property defined for a record that's not trying to populate a NamedEntity
         // (ie for the root record).
         Q_ASSERT(nullptr != this->namedEntity.get());
         // It's a coding error if we're trying to set a non-existent property on the NamedEntity subclass for this
         // record.
         QMetaObject const * metaObject = this->namedEntity->metaObject();
         int propertyIndex = metaObject->indexOfProperty(propertyName);
         Q_ASSERT(propertyIndex >= 0);
         QMetaProperty metaProperty = metaObject->property(propertyIndex);
         Q_ASSERT(metaProperty.isWritable());
         // It's a coding error if we can't create a valid QVariant from a pointer to class we are trying to "set"
         Q_ASSERT(QVariant::fromValue(ii->jsonRecord->namedEntity.get()).isValid());

         qDebug() <<
            Q_FUNC_INFO << "Setting" << propertyName << "property (type = " <<
            this->namedEntity->metaObject()->property(
               this->namedEntity->metaObject()->indexOfProperty(propertyName)
            ).typeName() << ") on" << this->namedEntityClassName << "object";
         this->namedEntity->setProperty(propertyName,
                                        QVariant::fromValue(ii->jsonRecord->namedEntity.get()));
      }
   }
   return true;
}*/


/*bool JsonRecord::loadChildRecords(xalanc::DOMSupport & domSupport,
                                 JsonRecord::FieldDefinition const * fieldDefinition,
                                 xalanc::NodeRefList & nodesForCurrentXPath,
                                 QTextStream & userMessage) {
   //
   // This is where we have one or more substantive records of a particular type inside the one we are
   // reading - eg some Hops inside a Recipe.  So we need to loop though these "child" records and read
   // each one in with an JsonRecord object of the relevant type.
   //
   // Note an advantage of using XPaths means we can just "see through" any grouping or containing nodes.
   // For instance, in BeerJSON, inside a <RECIPE>...</RECIPE> record there will be a <HOPS>...</HOPS>
   // "record set" node containing the <HOP>...</HOP> record(s) for this recipe, but we can just say in our
   // this->fieldDefinitions that we want the "HOPS/HOP" nodes inside a "RECIPE" and thus skip straight to
   // having a list of all the <HOP>...</HOP> nodes without having to explicitly parse the <HOPS>...</HOPS>
   // node.
   //
   for (xalanc::NodeRefList::size_type ii = 0; ii < nodesForCurrentXPath.getLength(); ++ii) {
      //
      // It's a coding error if we don't recognise the type of node that we've been configured (via
      // this->fieldDefinitions) to read in.  Again, an advantage of using XPaths is that we just
      // automatically ignore nodes we're not looking for.  Eg, imagine, in a BeerJSON file, there's the
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
      xalanc::XalanNode * childRecordNode = nodesForCurrentXPath.item(ii);
      XQString childRecordName{childRecordNode->getNodeName()};
      Q_ASSERT(this->jsonCoding.isKnownJsonRecordType(childRecordName));

      std::shared_ptr<JsonRecord> jsonRecord = this->jsonCoding.getNewJsonRecord(childRecordName);
      this->childRecords.append(JsonRecord::ChildRecord{fieldDefinition, jsonRecord});
      //
      // The return value of xalanc::XalanNode::getIndex() doesn't have an instantly obvious direct meaning, but AFAICT
      // higher values are for nodes that were later in the input file, so useful to log.
      //
      qDebug() <<
         Q_FUNC_INFO << "Loading child record" << childRecordName << "with index" << childRecordNode->getIndex();
      if (!jsonRecord->load(domSupport, childRecordNode, userMessage)) {
         return false;
      }
   }

   return true;
}*/


bool JsonRecord::isDuplicate() {
   // Base class does not have a NamedEntity so nothing to check
   // Stictly, it's a coding error if this function is called, as caller should first check whether there is a
   // NamedEntity, and subclasses that do have one should override this function.
   Q_ASSERT(false && "Trying to check for duplicate NamedEntity when there is none");
   return false;
}

void JsonRecord::normaliseName() {
   // Base class does not have a NamedEntity so nothing to normalise
   // Stictly, it's a coding error if this function is called, as caller should first check whether there is a
   // NamedEntity, and subclasses that do have one should override this function.
   Q_ASSERT(false && "Trying to normalise name of NamedEntity when there is none");
   return;
}

void JsonRecord::setContainingEntity(std::shared_ptr<NamedEntity> containingEntity) {
   // Base class does not have a NamedEntity or a container, so nothing to do
   // Stictly, it's a coding error if this function is called, as caller should first check whether there is a
   // NamedEntity, and subclasses that do have one should override this function.
   Q_ASSERT(false && "Trying to set containing entity when there is none");
   return;
}


void JsonRecord::modifyClashingName(QString & candidateName) {
   //
   // First, see whether there's already a (n) (ie "(1)", "(2)" etc) at the end of the name (with or without
   // space(s) preceding the left bracket.  If so, we want to replace this with " (n+1)".  If not, we try " (1)".
   //
   int duplicateNumber = 1;
   QRegExp const & nameNumberMatcher = NamedEntity::getDuplicateNameNumberMatcher();
   int positionOfMatch = nameNumberMatcher.indexIn(candidateName);
   if (positionOfMatch > -1) {
      // There's already some integer in brackets at the end of the name, extract it, add one, and truncate the
      // name.
      duplicateNumber = nameNumberMatcher.cap(1).toInt() + 1;
      candidateName.truncate(positionOfMatch);
   }
   candidateName += QString(" (%1)").arg(duplicateNumber);
   return;
}

void JsonRecord::toJson(NamedEntity const & namedEntityToExport,
                      QTextStream & out) const {
   qDebug() <<
      Q_FUNC_INFO << "Exporting JSON for" << namedEntityToExport.metaObject()->className() << "#" << namedEntityToExport.key();
/*
      out << "<" << this->recordName << ">\n";

   // For the moment, we are constructing JSON output without using Xerces (or similar), on the grounds that, in this
   // direction (ie to JSON rather than from JSON), it's a pretty simple algorithm and we don't need to validate anything
   // (because we assume that our own data is valid).

   // BeerJSON doesn't care about field order, so we don't either (though it would be relatively small additional work
   // to control field order precisely).
   for (auto & fieldDefinition : this->fieldDefinitions) {
      // If there isn't a property name that means this is not a field we support so there's nothing to write out.
      if (fieldDefinition.propertyName.isNull()) {
         // At the moment at least, we support all JsonRecord::RecordSimple and JsonRecord::RecordComplex fields, so it's
         // a coding error if one of them does not have a property name.
         Q_ASSERT(JsonRecord::FieldType::RecordSimple != fieldDefinition.fieldType);
         Q_ASSERT(JsonRecord::FieldType::RecordComplex != fieldDefinition.fieldType);
         continue;
      }

      // Nested record fields are of two types.  JsonRecord::RecordSimple can be handled generically.
      // JsonRecord::RecordComplex need to be handled in part by subclasses.
      if (JsonRecord::FieldType::RecordSimple == fieldDefinition.fieldType ||
          JsonRecord::FieldType::RecordComplex == fieldDefinition.fieldType) {
         //
         // Some of the work is generic, so we do it here.  In particular, we can work out what tags are needed to
         // contain the record (from the XPath, if any, prior to the last slash), but also what type of JsonRecord(s) we
         // will need by looking at the end of the XPath for this field.
         //
         // (In BeerJSON, these contained XPaths are only 1-2 elements, so numContainingTags is always 0 or 1.  If and
         // when we support a different JSON coding, we might need to look at this code more closely.)
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
         std::shared_ptr<JsonRecord> subRecord = this->jsonCoding.getNewJsonRecord(xPathElements.last());

         if (JsonRecord::FieldType::RecordSimple == fieldDefinition.fieldType) {
            NamedEntity * childNamedEntity =
               namedEntityToExport.property(*fieldDefinition.propertyName).value<NamedEntity *>();
            if (childNamedEntity) {
               subRecord->toJson(*childNamedEntity, out, indentLevel + numContainingTags + 1, indentString);
            } else {
               this->writeNone(*subRecord, namedEntityToExport, out, indentLevel + numContainingTags + 1, indentString);
            }
         } else {
            //
            // In theory we could get a list of the contained records via the Qt Property system.  However, the
            // different things we would get back inside the QVariant (QList<BrewNote *>, QList<Hop *> etc) have no
            // common base class, so we can't safely treat them as, or upcast them to, QList<NamedEntity *>.
            //
            // Instead, we get the subclass of this class (eg JsonRecipeRecord) to do the work
            //
            this->subRecordToJson(fieldDefinition,
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
      if (fieldDefinition.fieldType == JsonRecord::FieldType::RequiredConstant) {
         //
         // This is a field that is required to be in the JSON, but whose value we don't need, and for which we always
         // write a constant value on output.  At the moment it's only needed for the VERSION tag in BeerJSON.
         //
         // Because it's such an edge case, we abuse the propertyName field to hold the default value (ie what we
         // write out).  This saves having an extra almost-never-used field on JsonRecord::FieldDefinition.
         //
         valueAsText = *fieldDefinition.propertyName;
      } else {
         QVariant value = namedEntityToExport.property(*fieldDefinition.propertyName);
         Q_ASSERT(value.isValid());
         // It's a coding error if we are trying here to write out some field with a complex XPath
         if (fieldDefinition.xPath.contains("/")) {
            qCritical() << Q_FUNC_INFO <<
               "Invalid use of non-trivial XPath (" << fieldDefinition.xPath << ") for output of property" <<
               fieldDefinition.propertyName << "of" << namedEntityToExport.metaObject()->className();
            Q_ASSERT(false); // Stop here on a debug build
            continue;        // Soldier on in a prod build
         }

         switch (fieldDefinition.fieldType) {

            case JsonRecord::FieldType::Bool:
               // Unlike other JSON documents, boolean fields in BeerJSON are caps, so we have to accommodate that
               valueAsText = value.toBool() ? "TRUE" : "FALSE";
               break;

            case JsonRecord::FieldType::Int:
            case JsonRecord::FieldType::UInt:
            case JsonRecord::FieldType::Double:
               // QVariant knows how to convert a number to a string
               valueAsText = value.toString();
               break;

            case JsonRecord::FieldType::Date:
               // There is only one true date format :-)
               valueAsText = value.toDate().toString(Qt::ISODate);
               break;

            case JsonRecord::FieldType::Enum:
               // It's definitely a coding error if there is no enumMapping for a field declared as Enum!
               Q_ASSERT(nullptr != fieldDefinition.enumMapping);
               {
                  auto match = fieldDefinition.enumMapping->enumToString(value.toInt());
                  if (!match) {
                  // It's a coding error if we couldn't find the enum value the enum mapping
                  qCritical() << Q_FUNC_INFO <<
                     "Could not find string representation of enum property" << fieldDefinition.propertyName <<
                     "value " << value.toString() << "when writing <" << fieldDefinition.xPath << "> field of" <<
                     namedEntityToExport.metaObject()->className();
                  Q_ASSERT(false); // Stop here on a debug build
                  continue;        // Soldier on in a prod build
               }
                  valueAsText = match.value();
               }
               break;

            // By default we assume it's a string
            case JsonRecord::FieldType::String:
            default:
               {
                  // We use this to escape "&" to "&amp;" and so on in string content.  (Other data types should not
                  // have anything in their string representation that needs escaping in JSON.)
                  QJsonStreamWriter qJsonStreamWriter(&valueAsText);
                  qJsonStreamWriter.writeCharacters(value.toString());
               }
               break;
         }
      }
      writeIndents(out, indentLevel + 1, indentString);
      out << "<" << fieldDefinition.xPath << ">" << valueAsText << "</" << fieldDefinition.xPath << ">\n";
   }

   writeIndents(out, indentLevel, indentString);
   out << "</" << this->recordName << ">\n";
   */
   return;
}

void JsonRecord::subRecordToJson(JsonRecord::FieldDefinition const & fieldDefinition,
                               JsonRecord const & subRecord,
                               NamedEntity const & namedEntityToExport,
                               QTextStream & out,
                               int indentLevel,
                               char const * const indentString) const {
   // Base class does not know how to handle nested records
   // It's a coding error if we get here as this virtual member function should be overridden classes that have nested records
   qCritical() << Q_FUNC_INFO <<
      "Coding error: cannot export" << namedEntityToExport.metaObject()->className() << "(" <<
      this->namedEntityClassName << ") property" << fieldDefinition.propertyName << "to <" << fieldDefinition.xPath <<
      "> from base class JsonRecord";
   Q_ASSERT(false);
   return;
}
