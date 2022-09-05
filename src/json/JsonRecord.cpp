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

#include <optional>
#include <system_error>

#include <QDate>
#include <QDebug>

#include "json/JsonCoding.h"
#include "json/JsonUtils.h"
#include "utils/ErrorCodeToStream.h"
#include "utils/ImportRecordCount.h"

//
// Variables and constant definitions that we need only in this file
//
namespace {
   /**
    * \brief Read value and unit fields from a JSON record
    *
    *        We assume that the requested fields exist and are of the correct type (double and string respectively)
    *        because this should have been enforced already by JSON schema validation.
    *
    * \param type
    * \param xPath
    * \param unitField
    * \param valueField
    * \param recordData
    * \param value Variable in which value is returned
    * \param unitName Variable in which unit is returned
    * \return \c true if succeeded, \c false otherwise
    */
   bool readValueAndUnit(JsonRecordDefinition::FieldType const type,
                         JsonXPath const & xPath,
                         JsonXPath const & unitField,
                         JsonXPath const & valueField,
                         boost::json::value const * recordData,
                         double & value,
                         QString & unitName) {
      // It's a coding error to supply a null pointer for recordData
      Q_ASSERT(recordData);

      // It's a coding error if we're trying to read sub-values from something that is not a JSON object
      Q_ASSERT(recordData->is_object());

      //
      // Read the value and unit fields.  We assert that they exist and are of the correct type (double and string
      // respectively) because this should have been enforced already by JSON schema validation.
      //
      qDebug() <<
         Q_FUNC_INFO << "Reading" << valueField << "and" << unitField << "sub-fields from" << xPath << "record:" <<
         *recordData;

      std::error_code errCode;
      boost::json::value const * valueRaw = recordData->find_pointer(valueField.asJsonPtr(), errCode);
      if (errCode) {
         // Not expecting this to happen given that we've already validated the JSON file against its schema.
         qWarning() << Q_FUNC_INFO << "Error parsing value from" << xPath << " (" << type << "): " << errCode;
         return false;
      }
      Q_ASSERT(valueRaw);
      qDebug() << Q_FUNC_INFO << "Raw Value=" << *valueRaw << "(" << valueRaw->kind() << ")";
      // The JSON type should be number.  Boost.JSON will have chosen either double or int64  (or conceivably uint64) to
      // store the number, depending eg on whether it has a decimal separator.  So we cannot assert that
      // valueRaw->is_double().  Fortunately, Boost.JSON helps us with the necessary casting.
      Q_ASSERT(valueRaw->is_number());
      value = valueRaw->to_number<double>(errCode);
      if (errCode) {
         // Not expecting this to happen as doco says "If T is a floating point type and the stored value is a number,
         // the conversion is performed without error. The converted number is returned, with a possible loss of
         // precision. "
         qWarning() <<
            Q_FUNC_INFO << "Error extracting double from" << *valueRaw << "(" << valueRaw->kind() << ") for" <<
            xPath << " (" << type << "): " << errCode;
         return false;
      }
      qDebug() << Q_FUNC_INFO << "Value=" << value;

      boost::json::value const * unitNameRaw = recordData->find_pointer(unitField.asJsonPtr(), errCode);
      if (errCode) {
         // Not expecting this to happen given that we've already validated the JSON file against its schema.
         qWarning() << Q_FUNC_INFO << "Error parsing units from" << xPath << " (" << type << "): " << errCode;
         return false;
      }
      Q_ASSERT(unitNameRaw);
      Q_ASSERT(unitNameRaw->is_string());
      unitName = unitNameRaw->get_string().c_str();

      qDebug() << Q_FUNC_INFO << "Read" << xPath << " (" << type << ") as" << value << " " << unitName;
      return true;
   }

   /**
    * \brief Read value and unit fields from a JSON record with a single mapping (ie relating to a single physical
    *        quantity) and convert to canonical units
    *
    *        We assume that the requested fields exist and are of the correct type (double and string respectively)
    *        because this should have been enforced already by JSON schema validation.
    *
    * \param fieldDefinition
    * \param recordData
    * \return The value, converted to canonical scale, or \c std::nullopt if there was an error
    */
   std::optional<Measurement::Amount> readMeasurementWithUnits(
      JsonRecordDefinition::FieldDefinition const & fieldDefinition,
      boost::json::value const * recordData
   ) {
      // It's a coding error to supply a null pointer for recordData
      Q_ASSERT(recordData);

      double value{0};
      QString unitName{};
      if (!readValueAndUnit(fieldDefinition.type,
                            fieldDefinition.xPath,
                            fieldDefinition.valueDecoder.unitsMapping->unitField,
                            fieldDefinition.valueDecoder.unitsMapping->valueField,
                            recordData,
                            value,
                            unitName)) {
         return std::nullopt;
      }

      // The schema validation should have ensured that the unit name is constrained to one of the values we are
      // expecting, so it's almost certainly a coding error if it doesn't.
      if (!fieldDefinition.valueDecoder.unitsMapping->nameToUnit.contains(unitName)) {
         qCritical() << Q_FUNC_INFO << "Unexpected unit name:" << unitName;
         // Stop here on debug build
         Q_ASSERT(false);
         return std::nullopt;
      }

      Measurement::Unit const * unit = fieldDefinition.valueDecoder.unitsMapping->nameToUnit.value(unitName);
      Measurement::Amount canonicalValue = unit->toSI(value);

      qDebug() <<
         Q_FUNC_INFO << "Converted" << value << " " << unitName << "to" << canonicalValue.quantity << " " <<
         canonicalValue.unit;

      return canonicalValue;
   }

   /**
    * \brief Read value and unit fields from a JSON record with a multiple mappings (eg one for mass and one for volume)
    *        and convert to canonical units
    *
    *        We assume that the requested fields exist and are of the correct type (double and string respectively)
    *        because this should have been enforced already by JSON schema validation.
    *
    * \param fieldDefinition
    * \param recordData
    * \return The value, converted to canonical scale, or \c std::nullopt if there was an error
    */
   std::optional<Measurement::Amount> readOneOfMeasurementsWithUnits(
      JsonRecordDefinition::FieldDefinition const & fieldDefinition,
      boost::json::value const * recordData
   ) {
      // It's a coding error to supply a null pointer for recordData
      Q_ASSERT(recordData);

      // It's a coding error if the list of JsonMeasureableUnitsMapping objects has less than two elements.  (For
      // one element you should use JsonRecordDefinition::FieldType::MeasurementWithUnits instead of
      // JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits.)
      Q_ASSERT(fieldDefinition.valueDecoder.listOfUnitsMappings->size() > 1);

      // Per the comment in json/JsonRecordDefinition.h, we assume that unitField and valueField are the same for each
      // JsonMeasureableUnitsMapping in the list, so we just use the first entry here.
      JsonXPath const & unitField  = fieldDefinition.valueDecoder.listOfUnitsMappings->at(0)->unitField;
      JsonXPath const & valueField = fieldDefinition.valueDecoder.listOfUnitsMappings->at(0)->valueField;

      double value{0};
      QString unitName{};
      if (!readValueAndUnit(fieldDefinition.type,
                            fieldDefinition.xPath,
                            unitField,
                            valueField,
                            recordData,
                            value,
                            unitName)) {
         return std::nullopt;
      }

      Measurement::Unit const * unit = nullptr;
      for (auto const unitsMapping : *fieldDefinition.valueDecoder.listOfUnitsMappings) {
         if (unitsMapping->nameToUnit.contains(unitName)) {
            unit = unitsMapping->nameToUnit.value(unitName);
            break;
         }
      }

      // The schema validation should have ensured that the unit name is constrained to one of the values we are
      // expecting, so it's almost certainly a coding error if it doesn't.
      if (!unit) {
         qCritical() << Q_FUNC_INFO << "Unexpected unit name:" << unitName;
         // Stop here on debug build
         Q_ASSERT(false);
         return std::nullopt;
      }

      Measurement::Amount canonicalValue = unit->toSI(value);

      qDebug() <<
         Q_FUNC_INFO << "Converted" << value << " " << unitName << "to" << canonicalValue.quantity << " " <<
         canonicalValue.unit;

      return canonicalValue;
   }

   /**
    * \brief Read value and unit fields where the units are expected to always be the same (eg "%")
    *
    *        We assume that the requested fields exist and are of the correct type (double and string respectively)
    *        because this should have been enforced already by JSON schema validation.
    *
    * \param fieldDefinition
    * \param recordData
    * \return The value, or \c std::nullopt if there was an error
    */
   std::optional<double> readSingleUnitValue(JsonRecordDefinition::FieldDefinition const & fieldDefinition,
                                             boost::json::value const * recordData) {
      // It's a coding error to supply a null pointer for recordData
      Q_ASSERT(recordData);

      double value{0};
      QString unitName{};
      if (!readValueAndUnit(fieldDefinition.type,
                            fieldDefinition.xPath,
                            fieldDefinition.valueDecoder.singleUnitSpecifier->unitField,
                            fieldDefinition.valueDecoder.singleUnitSpecifier->valueField,
                            recordData,
                            value,
                            unitName)) {
         return std::nullopt;
      }

      // The schema validation should have ensured that the unit name is what we're expecting, so it's almost certainly
      // a coding error if it doesn't.
      if (!fieldDefinition.valueDecoder.singleUnitSpecifier->validUnits.contains(unitName)) {
         qCritical() <<
            Q_FUNC_INFO << "Unit name" << unitName << "does not match expected (" <<
            fieldDefinition.valueDecoder.singleUnitSpecifier->validUnits << ")";
         // Stop here on debug build
         Q_ASSERT(false);
         return std::nullopt;
      }
      return value;
   }

}

JsonRecord::JsonRecord(JsonCoding const & jsonCoding,
                       boost::json::value const & recordData,
                       JsonRecordDefinition const & recordDefinition) :
   jsonCoding{jsonCoding},
   recordData{recordData},
   recordDefinition{recordDefinition},
   namedParameterBundle{NamedParameterBundle::NotStrict},
   namedEntity{nullptr},
   includeInStats{true},
   childRecords{} {
   return;
}

JsonRecord::~JsonRecord() = default;

NamedParameterBundle const & JsonRecord::getNamedParameterBundle() const {
   return this->namedParameterBundle;
}

std::shared_ptr<NamedEntity> JsonRecord::getNamedEntity() const {
   return this->namedEntity;
}


bool JsonRecord::load(QTextStream & userMessage) {
   Q_ASSERT(this->recordData.is_object());
   qDebug() <<
      Q_FUNC_INFO << "Loading" << this->recordDefinition.recordName << "record containing" <<
      this->recordData.as_object().size() << "elements";

   //
   // Loop through all the fields that we know/care about.  Anything else is intentionally ignored.  (We won't know
   // what to do with it, and, if it weren't allowed to be there, it would have generated an error at XSD parsing.)
   //
   for (auto & fieldDefinition : this->recordDefinition.fieldDefinitions) {
      //
      // NB: As with XML processing in XmlRecord::load, if we don't find a node, there's nothing for us to do.  The
      // schema validation should already flagged up an error if there are missing _required_ fields.  Equally,
      // although we only look for nodes we know about, some of these we won't use for one reason or another.
      //
      std::error_code errorCode;
      boost::json::value const * container = this->recordData.find_pointer(fieldDefinition.xPath.asJsonPtr(),
                                                                           errorCode);
      if (!container) {
         // As noted above this is usually not an error, but sometimes useful to log for debugging
         qDebug() <<
            Q_FUNC_INFO << fieldDefinition.xPath << " (" << fieldDefinition.type << ") not present (error code " <<
            errorCode.value() << ":" << errorCode.message().c_str() << ")";
      } else {
         qDebug() <<
            Q_FUNC_INFO << "Found" << fieldDefinition.xPath << " (" << fieldDefinition.type << "/" <<
            container->kind() << ")";

         if (JsonRecordDefinition::FieldType::Array == fieldDefinition.type) {
            //
            // One difference between XML and JSON when it comes to arrays is that the latter has one less layer of
            // tags.  In XML (eg BeerXML), we have "<HOPS><HOP>...</HOP><HOP>...</HOP>...</HOPS>".  In JSON (eg
            // BeerJSON) we have hop_varieties: [{...},{...},...].
            //
            // Schema should have already enforced that this field is an array, so we assert that here
            //
            Q_ASSERT(container->is_array());
            boost::json::array const & childRecordsData = container->get_array();
            if (!this->loadChildRecords(this->jsonCoding.getJsonRecordDefinitionByName(fieldDefinition.xPath.asXPath_c_str()),
                                        childRecordsData,
                                        userMessage)) {
               return false;
            }
         } else {
            //
            // If it's not an array then it's fields on the object we're currently populating
            //

            bool parsedValueOk = false;
            QVariant parsedValue;

            // JSON Schema validation should have ensured this field really is what we're expecting, so it's a coding
            // error if it's not, which is what most of the asserts below are saying.
            switch(fieldDefinition.type) {

               case JsonRecordDefinition::FieldType::Bool:
                  Q_ASSERT(container->is_bool());
                  parsedValue.setValue(container->get_bool());
                  parsedValueOk = true;
                  break;

               case JsonRecordDefinition::FieldType::Int:
                  Q_ASSERT(container->is_int64());
                  parsedValue.setValue(container->get_int64());
                  parsedValueOk = true;
                  break;

               case JsonRecordDefinition::FieldType::UInt:
                  Q_ASSERT(container->is_uint64());
                  parsedValue.setValue(container->get_uint64());
                  parsedValueOk = true;
                  break;

               case JsonRecordDefinition::FieldType::Double:
                  Q_ASSERT(container->is_double());
                  parsedValue.setValue(container->get_double());
                  parsedValueOk = true;
                  break;

               case JsonRecordDefinition::FieldType::String:
                  Q_ASSERT(container->is_string());
                  {
                     QString value{container->get_string().c_str()};
                     parsedValue.setValue(value);
                     parsedValueOk = true;
                  }
                  break;

               case JsonRecordDefinition::FieldType::Enum:
                  // It's definitely a coding error if there is no stringToEnum mapping for a field declared as Enum!
                  Q_ASSERT(nullptr != fieldDefinition.valueDecoder.enumMapping);
                  {
                     Q_ASSERT(container->is_string());
                     QString value{container->get_string().c_str()};
                     parsedValue.setValue(value);

                     auto match = fieldDefinition.valueDecoder.enumMapping->stringToEnum(value);
                     if (!match) {
                        // This is probably a coding error as the JSON Schema should already have verified that the
                        // value is one of the expected ones.
                        qWarning() <<
                           Q_FUNC_INFO << "Ignoring " << this->recordDefinition.namedEntityClassName << " node " <<
                           fieldDefinition.xPath << "=" << value << " as value not recognised";
                     } else {
                        parsedValue.setValue(match.value());
                        parsedValueOk = true;
                     }
                  }
                  break;

               case JsonRecordDefinition::FieldType::Array:
                  // This should be unreachable as we dealt with this case separately above, but having an case
                  // statement for it eliminates a compiler warning whilst still retaining the useful warning if we
                  // have ever omitted processing for another field type.
                  Q_ASSERT(false);
                  break;

               case JsonRecordDefinition::FieldType::MeasurementWithUnits:
                  // It's definitely a coding error if there is no unit decoder mapping for a field declared to require
                  // one
                  Q_ASSERT(nullptr != fieldDefinition.valueDecoder.unitsMapping);
                  // JSON schema validation should have ensured that the field is actually one with subfields for value
                  // and unit
                  Q_ASSERT(container->is_object());
                  {
                     std::optional<Measurement::Amount> canonicalValue = readMeasurementWithUnits(fieldDefinition,
                                                                                                  container);
                     if (canonicalValue) {
                        parsedValue.setValue(canonicalValue->quantity);
                        parsedValueOk = true;
                     }
                  }
                  break;

               case JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits:
                  // It's definitely a coding error if there is no list of unit decoder mappings for a field declared to
                  // require such
                  Q_ASSERT(nullptr != fieldDefinition.valueDecoder.listOfUnitsMappings);
                  // JSON schema validation should have ensured that the field is actually one with subfields for value
                  // and unit
                  Q_ASSERT(container->is_object());
                  {
                     // Logic is the same as for MeasurementWithUnits, but just loop through all the mappings
                     std::optional<Measurement::Amount> canonicalValue = readOneOfMeasurementsWithUnits(fieldDefinition,
                                                                                                        container);
                     if (canonicalValue) {
                        parsedValue.setValue(canonicalValue->quantity);
                        parsedValueOk = true;
                     }
                  }
                  break;

               case JsonRecordDefinition::FieldType::SingleUnitValue:
                  // It's definitely a coding error if there is no unit specifier for a field declared to require one
                  Q_ASSERT(nullptr != fieldDefinition.valueDecoder.singleUnitSpecifier);
                  // JSON schema validation should have ensured that the field is actually one with subfields for value
                  // and unit
                  Q_ASSERT(container->is_object());
                  {
                     std::optional<double> value = readSingleUnitValue(fieldDefinition, container);
                     if (value) {
                        parsedValue.setValue(*value);
                        parsedValueOk = true;
                     }
                  }
                  break;

               //
               // From here on, we have BeerJSON-specific types.  If we ever wanted to parse some other type of JSON,
               // then we might need to make this code more generic, but, for now, we're not going to worry too much as
               // it seems unlikely there will be other JSON encodings we want to deal with in the foreseeable future.
               //

               case JsonRecordDefinition::FieldType::Date:
                  // In BeerJSON, DateType is a string matching this regexp:
                  //   "\\d{4}-\\d{2}-\\d{2}|\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}"
                  // This is One True Date Format™ (aka ISO 8601), which makes our life somewhat easier
                  Q_ASSERT(!container->is_string());
                  {
                     QString value{container->get_string().c_str()};
                     QDate date = QDate::fromString(value, Qt::ISODate);
                     parsedValueOk = date.isValid();
                     if (!parsedValueOk) {
                        // The JSON schema validation doesn't guarantee the date is valid, just that it's the right
                        // digit groupings.  So, we do need to handle cases such as 2022-13-13 which are the right
                        // format but not valid dates.
                        qWarning() <<
                           Q_FUNC_INFO << "Ignoring " << this->recordDefinition.namedEntityClassName << " node " <<
                           fieldDefinition.xPath << "=" << value << " as could not be parsed as ISO 8601 date";
                     }
                  }
                  break;

               case JsonRecordDefinition::FieldType::RequiredConstant:
                  //
                  // This is a field that is required to be in the JSON, but whose value we don't need (and for which
                  // we always write a constant value on output).  At the moment it's only needed for the VERSION tag
                  // in BeerJSON.
                  //
                  // Note that, because we abuse the propertyName field to hold the default value (ie what we write
                  // out), we can't carry on to normal processing below.  So jump straight to processing the next
                  // node in the loop (via continue).
                  //
                  qDebug() <<
                     Q_FUNC_INFO << "Skipping " << this->recordDefinition.namedEntityClassName << " node " <<
                     fieldDefinition.xPath << "=" << *container << "(" << *fieldDefinition.propertyName <<
                     ") as not useful";
                  continue; // NB: _NOT_break here.  We want to jump straight to the next run through the for loop.
            }
            ///*********************TODO FINISH THIS!**************************
            /*
             */
         }
      }
   }

   return true;
}

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
      if (JsonRecord::FieldType::RecordSimple == ii->fieldDefinition->type) {
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


bool JsonRecord::loadChildRecords(JsonRecordDefinition const & childRecordDefinition,
                                  boost::json::array const & childRecordsData,
                                  QTextStream & userMessage) {
   //
   // This is where we have a list of one or more substantive records of a particular type, which may be either at top
   // level (eg hop_varieties) or inside another record that we are in the process of reading (eg hop_additions inside a
   // recipe).  Either way, we need to loop though these "child" records and read each one in with an JsonRecord object
   // of the relevant type.
   //
   for (auto & value : childRecordsData) {
      // Iterating through an array gives us boost::json::value objects
      // We assert that these are boost::json::object key:value containers (because we don't use arrays of other types)
      Q_ASSERT(value.is_object());
//      boost::json::object const & recordData = value.get_object();
      JsonRecord childRecord{this->jsonCoding, value, childRecordDefinition};
      if (!childRecord.load(userMessage)) {
         return false;
      }
   }

   return true;
}


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
         Q_ASSERT(JsonRecord::FieldType::RecordSimple != fieldDefinition.type);
         Q_ASSERT(JsonRecord::FieldType::RecordComplex != fieldDefinition.type);
         continue;
      }

      // Nested record fields are of two types.  JsonRecord::RecordSimple can be handled generically.
      // JsonRecord::RecordComplex need to be handled in part by subclasses.
      if (JsonRecord::FieldType::RecordSimple == fieldDefinition.type ||
          JsonRecord::FieldType::RecordComplex == fieldDefinition.type) {
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

         if (JsonRecord::FieldType::RecordSimple == fieldDefinition.type) {
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
      if (fieldDefinition.type == JsonRecord::FieldType::RequiredConstant) {
         //
         // This is a field that is required to be in the JSON, but whose value we don't need, and for which we always
         // write a constant value on output.  At the moment it's only needed for the VERSION tag in BeerJSON.
         //
         // Because it's such an edge case, we abuse the propertyName field to hold the default value (ie what we
         // write out).  This saves having an extra almost-never-used field on JsonRecordDefinition::FieldDefinition.
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

         switch (fieldDefinition.type) {

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

void JsonRecord::subRecordToJson(JsonRecordDefinition::FieldDefinition const & fieldDefinition,
                               JsonRecord const & subRecord,
                               NamedEntity const & namedEntityToExport,
                               QTextStream & out,
                               int indentLevel,
                               char const * const indentString) const {
   // Base class does not know how to handle nested records
   // It's a coding error if we get here as this virtual member function should be overridden classes that have nested records
   qCritical() << Q_FUNC_INFO <<
      "Coding error: cannot export" << namedEntityToExport.metaObject()->className() << "(" <<
      this->recordDefinition.namedEntityClassName << ") property" << fieldDefinition.propertyName << "to <" << fieldDefinition.xPath <<
      "> from base class JsonRecord";
   Q_ASSERT(false);
   return;
}
