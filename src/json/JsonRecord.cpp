/*======================================================================================================================
 * json/JsonRecord.cpp is part of Brewken, and is copyright the following authors 2020-2023:
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
#include <string_view>
#include <system_error>

#include <QDate>
#include <QDebug>
#include <QMetaType>

#include "json/JsonCoding.h"
#include "json/JsonUtils.h"
#include "utils/ErrorCodeToStream.h"
#include "utils/ImportRecordCount.h"

// Need this to be able to use MassOrVolumeAmt in Qt Properties system
Q_DECLARE_METATYPE(MassOrVolumeAmt);

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
                         std::string_view & unitName) {
      // It's a coding error to supply a null pointer for recordData
      Q_ASSERT(recordData);

      // It's a coding error if we're trying to read sub-values from something that is not a JSON object
      Q_ASSERT(recordData->is_object());

      //
      // Read the value and unit fields.  We assert that they exist and are of the correct type (double and string
      // respectively) because this should have been enforced already by JSON schema validation.
      //

      // Usually leave next line commented as otherwise generates too much logging
//      qDebug() <<
//         Q_FUNC_INFO << "Reading" << valueField << "and" << unitField << "sub-fields from" << xPath << "record:" <<
//         *recordData;

      std::error_code errCode;
      boost::json::value const * valueRaw = recordData->find_pointer(valueField.asJsonPtr(), errCode);
      if (errCode) {
         // Not expecting this to happen given that we've already validated the JSON file against its schema.
         qWarning() << Q_FUNC_INFO << "Error parsing value from" << xPath << " (" << type << "): " << errCode;
         return false;
      }
      Q_ASSERT(valueRaw);
      // Usually leave next line commented as otherwise generates too much logging
//      qDebug() << Q_FUNC_INFO << "Raw Value=" << *valueRaw << "(" << valueRaw->kind() << ")";

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
      // Usually leave next line commented as otherwise generates too much logging
//      qDebug() << Q_FUNC_INFO << "Value=" << value;

      boost::json::value const * unitNameRaw = recordData->find_pointer(unitField.asJsonPtr(), errCode);
      if (errCode) {
         // Not expecting this to happen given that we've already validated the JSON file against its schema.
         qWarning() << Q_FUNC_INFO << "Error parsing units from" << xPath << " (" << type << "): " << errCode;
         return false;
      }
      Q_ASSERT(unitNameRaw);
      Q_ASSERT(unitNameRaw->is_string());
      unitName = unitNameRaw->get_string();

      // Usually leave next line commented as otherwise generates too much logging
//      qDebug() << Q_FUNC_INFO << "Read" << xPath << " (" << type << ") as" << value << " " <<
//         std::string(unitName).c_str();
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
      std::string_view unitName{""};
      if (!readValueAndUnit(fieldDefinition.type,
                            fieldDefinition.xPath,
                            std::get<JsonMeasureableUnitsMapping const *>(fieldDefinition.valueDecoder)->unitField,
                            std::get<JsonMeasureableUnitsMapping const *>(fieldDefinition.valueDecoder)->valueField,
                            recordData,
                            value,
                            unitName)) {
         return std::nullopt;
      }

      // The schema validation should have ensured that the unit name is constrained to one of the values we are
      // expecting, so it's almost certainly a coding error if it doesn't.
      if (!std::get<JsonMeasureableUnitsMapping const *>(fieldDefinition.valueDecoder)->nameToUnit.contains(unitName)) {
         qCritical() << Q_FUNC_INFO << "Unexpected unit name:" << std::string(unitName).c_str();
         // Stop here on debug build
         Q_ASSERT(false);
         return std::nullopt;
      }

      Measurement::Unit const * unit =
         std::get<JsonMeasureableUnitsMapping const *>(fieldDefinition.valueDecoder)->nameToUnit.find(unitName)->second;
      Measurement::Amount canonicalValue = unit->toSI(value);

      qDebug() <<
         Q_FUNC_INFO << "Converted" << value << " " << std::string(unitName).c_str() << "to" << canonicalValue;

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
      Q_ASSERT(std::get<ListOfJsonMeasureableUnitsMappings const *>(fieldDefinition.valueDecoder)->size() > 1);

      // Per the comment in json/JsonRecordDefinition.h, we assume that unitField and valueField are the same for each
      // JsonMeasureableUnitsMapping in the list, so we just use the first entry here.
      JsonXPath const & unitField  =
         std::get<ListOfJsonMeasureableUnitsMappings const *>(fieldDefinition.valueDecoder)->at(0)->unitField;
      JsonXPath const & valueField =
         std::get<ListOfJsonMeasureableUnitsMappings const *>(fieldDefinition.valueDecoder)->at(0)->valueField;

      double value{0};
      std::string_view unitName{""};
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
      for (auto const unitsMapping :
           *std::get<ListOfJsonMeasureableUnitsMappings const *>(fieldDefinition.valueDecoder)) {
         if (unitsMapping->nameToUnit.contains(unitName)) {
            unit = unitsMapping->nameToUnit.find(unitName)->second;
            break;
         }
      }

      // The schema validation should have ensured that the unit name is constrained to one of the values we are
      // expecting, so it's almost certainly a coding error if it doesn't.
      if (!unit) {
         qCritical() << Q_FUNC_INFO << "Unexpected unit name:" << std::string(unitName).c_str();
         // Stop here on debug build
         Q_ASSERT(false);
         return std::nullopt;
      }

      Measurement::Amount canonicalValue = unit->toSI(value);

      qDebug() <<
         Q_FUNC_INFO << "Converted" << value << " " << std::string(unitName).c_str() << "to" << canonicalValue;

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
      std::string_view unitName{""};
      if (!readValueAndUnit(fieldDefinition.type,
                            fieldDefinition.xPath,
                            std::get<JsonSingleUnitSpecifier const *>(fieldDefinition.valueDecoder)->unitField,
                            std::get<JsonSingleUnitSpecifier const *>(fieldDefinition.valueDecoder)->valueField,
                            recordData,
                            value,
                            unitName)) {
         return std::nullopt;
      }

      // The schema validation should have ensured that the unit name is what we're expecting, so it's almost certainly
      // a coding error if it doesn't.
      if (!std::get<JsonSingleUnitSpecifier const *>(fieldDefinition.valueDecoder)->validUnits.contains(unitName)) {
         qCritical() <<
            Q_FUNC_INFO << "Unit name" << std::string(unitName).c_str() << "does not match expected (" <<
            std::get<JsonSingleUnitSpecifier const *>(fieldDefinition.valueDecoder)->validUnits.first().data() <<
            "etc)";
         // Stop here on debug build
         Q_ASSERT(false);
         return std::nullopt;
      }
      return value;
   }

   /**
    * \brief Add a value to a JSON object
    *
    * \param fieldDefinition
    * \param recordDataAsObject
    * \param key
    * \param value
    */
   void insertValue(JsonRecordDefinition::FieldDefinition const & fieldDefinition,
                    boost::json::object & recordDataAsObject,
                    std::string_view const & key,
                    QVariant const & value) {
      qDebug() <<
         Q_FUNC_INFO << "Writing" << std::string(key).c_str() << "=" << value << "(type" << fieldDefinition.type << ")";
      switch(fieldDefinition.type) {
         case JsonRecordDefinition::FieldType::Bool:
            Q_ASSERT(value.canConvert<bool>());
            recordDataAsObject.emplace(key, value.toBool());
            break;

         case JsonRecordDefinition::FieldType::Int:
            Q_ASSERT(value.canConvert<int>());
            recordDataAsObject.emplace(key, value.toInt());
            break;

         case JsonRecordDefinition::FieldType::UInt:
            Q_ASSERT(value.canConvert<uint>());
            recordDataAsObject.emplace(key, value.toUInt());
            break;

         case JsonRecordDefinition::FieldType::Double:
            Q_ASSERT(value.canConvert<double>());
            recordDataAsObject.emplace(key, value.toDouble());
            break;

         case JsonRecordDefinition::FieldType::String:
            Q_ASSERT(value.canConvert<QString>());
            {
               // We have a special case where WE store Hop Year internally as an int and BeerJSON stores it as a
               // string.  If our int is negative, that means we don't have a value.
               if (value.type() == QVariant::Int && value.toInt() < 0) {
                  break;
               }

               std::string valueAsString = value.toString().toStdString();
               // On the whole, there's no benefit in writing out a field for which we don't have a value
               if (!valueAsString.empty()) {
                  recordDataAsObject.emplace(key, valueAsString);
               }
            }
            break;

         case JsonRecordDefinition::FieldType::Enum:
            // It's definitely a coding error if there is no stringToEnum mapping for a field declared as Enum!
            Q_ASSERT(nullptr != std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder));
            // An enum should always be convertible to an int
            Q_ASSERT(value.canConvert<int>());
            {
               auto match =
                  std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder)->enumAsIntToString(value.toInt());
               // It's a coding error if we couldn't find a string representation for the enum
               Q_ASSERT(match);
               recordDataAsObject.emplace(key, match->toStdString());
            }
            break;

         case JsonRecordDefinition::FieldType::EnumOpt:
            // It's also a coding error if there is no stringToEnum mapping for a field declared as EnumOpt
            Q_ASSERT(nullptr != std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder));
            // An optional enum retrieved via Qt properties should always be convertible to an std::optional<int>
            Q_ASSERT(value.canConvert< std::optional<int> >());
            {
               auto rawValue = value.value< std::optional<int> >();
               // We only add the value to the Json if it is set
               if (rawValue.has_value()) {
                  auto mapping = std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder);
                  auto match = mapping->enumAsIntToString(rawValue.value());
                  // It's a coding error if we couldn't find a string representation for the enum
                  Q_ASSERT(match);
                  recordDataAsObject.emplace(key, match->toStdString());
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
            Q_ASSERT(value.canConvert<double>());

            //
            // Ideally, value would be something we could convert to Measurement::Amount, which would give us units.
            //
            // In practice, it's usually the case that the NamedEntity property will just be a double and the rest of
            // the code "knows" the corresponding Measurement::PhysicalQuantity and therefore the canonical
            // Measurement::Unit that the measurement is in.  Eg if something is a Measurement::PhysicalQuantity::Mass,
            // we always store it in Measurement::Units::kilograms.
            //
            // One day, maybe, we might perhaps change all "measurement" double properties to Measurement::Amount, but,
            // in the meantime, we can get what we need here another way.  We have a list of possible units that could
            // be used in BeerJSON to measure the amount we're looking at.  So we grab the first Measurement::Unit in
            // the list, and, from that, we can trivially get the corresponding canonical Measurement::Unit which will,
            // by the above-mentioned convention, be the right one for the NamedEntity property.
            //
            {
               // It's definitely a coding error if there is no unit decoder mapping for a field declared to require
               // one
               JsonMeasureableUnitsMapping const * const unitsMapping =
                  std::get<JsonMeasureableUnitsMapping const *>(fieldDefinition.valueDecoder);
               Q_ASSERT(unitsMapping);
               Measurement::Unit const * const aUnit = unitsMapping->nameToUnit.cbegin()->second;
               Measurement::Unit const & canonicalUnit = aUnit->getCanonical();
               qDebug() << Q_FUNC_INFO << canonicalUnit;

               // Now we found canonical units, we need to find the right string to represent them
               auto unitName = unitsMapping->getNameForUnit(canonicalUnit);
               qDebug() << Q_FUNC_INFO << std::string(unitName).c_str();
               recordDataAsObject[key].emplace_object();
               auto & measurementWithUnits = recordDataAsObject[key].as_object();
               measurementWithUnits.emplace(unitsMapping->unitField.asKey(),  unitName);
               measurementWithUnits.emplace(unitsMapping->valueField.asKey(), value.toDouble());
            }
            break;

         case JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits:
            // .:TODO:. For the moment, I'm assuming we only use this for mass or volume.  IF that's correct then we
            // should rename JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits.  If not, we need to tweak a
            // couple of things here.
            Q_ASSERT(value.canConvert<MassOrVolumeAmt>());
            // It's definitely a coding error if there is no list of unit decoder mappings for a field declared to
            // require such
            Q_ASSERT(
               nullptr != std::get<ListOfJsonMeasureableUnitsMappings const *>(fieldDefinition.valueDecoder)
            );
            {
               //
               // This is mostly (TBD exclusively?) used to handle amounts of things that can be measured by mass or
               // volume - Yeast, Misc, Fermentable, etc
               //
               MassOrVolumeAmt amount = value.value<MassOrVolumeAmt>();

               //
               // Logic is similar to MeasurementWithUnits above, except we already have the canonical units
               //
               for (auto const unitsMapping :
                    *std::get<ListOfJsonMeasureableUnitsMappings const *>(fieldDefinition.valueDecoder)) {
                  //
                  // Each JsonMeasureableUnitsMapping in the ListOfJsonMeasureableUnitsMappings holds units for a single
                  // PhysicalQuantity -- ie we have a list of units for mass and another list of units for volume.
                  //
                  // So the first thing to do is to find the right JsonMeasureableUnitsMapping
                  //
                  if (unitsMapping->getPhysicalQuantity() == amount.unit()->getPhysicalQuantity()) {
                     // Now we have the right PhysicalQuantity, we just need the entry for our Units
                     auto unitName = unitsMapping->getNameForUnit(*amount.unit());
                     qDebug() << Q_FUNC_INFO << std::string(unitName).c_str();
                     recordDataAsObject[key].emplace_object();
                     auto & measurementWithUnits = recordDataAsObject[key].as_object();
                     measurementWithUnits.emplace(unitsMapping->unitField.asKey(),  unitName);
                     measurementWithUnits.emplace(unitsMapping->valueField.asKey(), amount.quantity());
                     break;
                  }
               }
            }
            break;

         case JsonRecordDefinition::FieldType::SingleUnitValue:
            Q_ASSERT(value.canConvert<double>());
            {
               // It's definitely a coding error if there is no unit specifier for a field declared to require one
               JsonSingleUnitSpecifier const * const jsonSingleUnitSpecifier =
                  std::get<JsonSingleUnitSpecifier const *>(fieldDefinition.valueDecoder);
               Q_ASSERT(jsonSingleUnitSpecifier);
               // There can be multiple valid (and equivalent) unit names, but we always use the first one for
               // writing.  See json/JsonSingleUnitSpecifier.h for more detail.
               recordDataAsObject[key].emplace_object();
               auto & measurementWithUnits = recordDataAsObject[key].as_object();
               measurementWithUnits.emplace(jsonSingleUnitSpecifier->unitField.asKey(),  jsonSingleUnitSpecifier->validUnits[0]);
               measurementWithUnits.emplace(jsonSingleUnitSpecifier->valueField.asKey(), value.toDouble());
            }
            break;

         //
         // From here on, we have BeerJSON-specific types.  If we ever wanted to parse some other type of JSON,
         // then we might need to make this code more generic, but, for now, we're not going to worry too much as
         // it seems unlikely there will be other JSON encodings we want to deal with in the foreseeable future.
         //
         case JsonRecordDefinition::FieldType::Date:
            Q_ASSERT(value.canConvert<QDate>());
            {
               // In BeerJSON, DateType is a string matching this regexp:
               //   "\\d{4}-\\d{2}-\\d{2}|\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}"
               // This is One True Date Format™ (aka ISO 8601), which makes our life somewhat easier
               std::string formattedDate = value.toDate().toString(Qt::ISODate).toStdString();
               recordDataAsObject.emplace(key, formattedDate);
            }
            break;

         case JsonRecordDefinition::FieldType::RequiredConstant:
            //
            // This is a field that is required to be in the JSON, but whose value we don't need, and for which we
            // always write a constant value on output.  At the moment it's only needed for the VERSION tag in
            // BeerJSON.
            //
            // Because it's such an edge case, we abuse the propertyName field to hold the default value (ie what we
            // write out).  This saves having an extra almost-never-used field on
            // JsonRecordDefinition::FieldDefinition.
            //
            recordDataAsObject.emplace(fieldDefinition.xPath.asKey(), **fieldDefinition.propertyName);
            break;

         // Don't need a default case as we want the compiler to warn us if we didn't cover everything explicitly above
      }

      return;
   }
}

JsonRecord::JsonRecord(JsonCoding const & jsonCoding,
                       boost::json::value & recordData,
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

[[nodiscard]] bool JsonRecord::load(QTextStream & userMessage) {
   Q_ASSERT(this->recordData.is_object());
   qDebug() <<
      Q_FUNC_INFO << "Loading" << this->recordDefinition.recordName << "record containing" <<
      this->recordData.as_object().size() << "elements";

   //
   // Loop through all the fields that we know/care about.  Anything else is intentionally ignored.  (We won't know
   // what to do with it, and, if it weren't allowed to be there, it would have generated an error at schema parsing.)
   //
   qDebug() <<
      Q_FUNC_INFO << "Examining" << this->recordDefinition.fieldDefinitions.size() << "field definitions";
   for (auto & fieldDefinition : this->recordDefinition.fieldDefinitions) {
      //
      // NB: As with XML processing in XmlRecord::load, if we don't find a node, there's nothing for us to do.  The
      // schema validation should already flagged up an error if there are missing _required_ fields.  Equally,
      // although we only look for nodes we know about, some of these we won't use for one reason or another.
      //
      std::error_code errorCode;
      boost::json::value * container = this->recordData.find_pointer(fieldDefinition.xPath.asJsonPtr(), errorCode);
      if (!container) {
         // As noted above this is usually not an error, but _sometimes_ useful to log for debugging.  Usually leave
         // this logging commented out though as otherwise it fills up the log files
         qDebug() <<
            Q_FUNC_INFO << fieldDefinition.xPath << " (" << fieldDefinition.type << ") not present (error code " <<
            errorCode.value() << ":" << errorCode.message().c_str() << ")";
      } else {
         // Again, it can be useful to uncomment this logging statement for debugging, but usually we don't want it
         // taking up space in the log files.
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
            boost::json::array & childRecordsData = container->get_array();
            if (!this->loadChildRecords(fieldDefinition,
                                        this->jsonCoding.getJsonRecordDefinitionByName(fieldDefinition.xPath.asXPath_c_str()),
                                        childRecordsData,
                                        userMessage)) {
               return false;
            }
         } else {
            //
            // If it's not an array then it's fields on the object we're currently populating
            //

            // It should not be possible for propertyName to be a null pointer.  (It may well be a pointer to
            // BtString::NULL_STR, in which case propertyName->isNull() will return true, but that's fine.)
            Q_ASSERT(fieldDefinition.propertyName);

            bool parsedValueOk = false;
            QVariant parsedValue;

            //
            // JSON Schema validation should have ensured this field really is what we're expecting, so it's a coding
            // error if it's not, which is what most of the asserts below are saying.
            //
            // HOWEVER, note that we need to take care with numeric types.  JSON only has one base numeric type
            // (number).  Boost.JSON handles this correctly but also offers access to the underlying type it has used to
            // store the number (std::int64_t, std::uint64_t or double).  So, eg, you can first call
            // container->is_double() to check whether the underlying storage is double and then, if that returns true,
            // call container->get_double() to get the value.  This seems like an attractive short-cut (which it is when
            // you have full control over the JSON input) but in can catch you out.  Eg if a field that usually has a
            // decimal point happens to be an integer and was stored (validly) without the decimal point in the JSON
            // file, then Boost.JSON will put it in eg std::int64_t rather than double, and get_double() will barf an
            // assertion failure.
            //
            // The correct thing to do for general purpose handling is to assert is_number() and use the templated
            // to_number() function to get back the type WE want rather than Boost.JSON's internal storage type.
            //
            switch(fieldDefinition.type) {

               case JsonRecordDefinition::FieldType::Bool:
                  Q_ASSERT(container->is_bool());
                  parsedValue.setValue(container->get_bool());
                  parsedValueOk = true;
                  break;

               case JsonRecordDefinition::FieldType::Int:
                  Q_ASSERT(container->is_number());
                  parsedValue.setValue(container->to_number<std::int64_t>());
                  parsedValueOk = true;
                  break;

               case JsonRecordDefinition::FieldType::UInt:
                  Q_ASSERT(container->is_number());
                  parsedValue.setValue(container->to_number<std::uint64_t>());
                  parsedValueOk = true;
                  break;

               case JsonRecordDefinition::FieldType::Double:
                  Q_ASSERT(container->is_number());
                  parsedValue.setValue(container->to_number<double>());
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
                  Q_ASSERT(nullptr != std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder));
                  {
                     Q_ASSERT(container->is_string());
                     QString value{container->get_string().c_str()};

                     auto match =
                        std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder)->stringToEnumAsInt(value);
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

               case JsonRecordDefinition::FieldType::EnumOpt:
                  // It's also a coding error if there is no stringToEnum mapping for a field declared as EnumOpt
                  Q_ASSERT(nullptr != std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder));
                  {
                     Q_ASSERT(container->is_string());
                     QString value{container->get_string().c_str()};

                     // Normally we would expect the value to be valid if it's present, as the JSON Schema should have
                     // enforced this.  We shouldn't have to handle the std::nullopt case as it's implied by the field
                     // not being present at all (and handled by the default value in the relevant constructor (eg of
                     // Fermentable).
                     auto match =
                        std::get<EnumStringMapping const *>(fieldDefinition.valueDecoder)->stringToEnumAsInt(value);
                     if (!match) {
                        // This is probably a coding error as the JSON Schema should already have verified that the
                        // value is one of the expected ones.
                        qWarning() <<
                           Q_FUNC_INFO << "Ignoring " << this->recordDefinition.namedEntityClassName << " node " <<
                           fieldDefinition.xPath << "=" << value << " as value not recognised";
                     } else {
                        parsedValue.setValue(std::optional<int>(match.value()));
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
                  Q_ASSERT(nullptr != std::get<JsonMeasureableUnitsMapping const *>(fieldDefinition.valueDecoder));
                  // JSON schema validation should have ensured that the field is actually one with subfields for value
                  // and unit
                  Q_ASSERT(container->is_object());
                  {
                     std::optional<Measurement::Amount> canonicalValue = readMeasurementWithUnits(fieldDefinition,
                                                                                                  container);
                     if (canonicalValue) {
                        parsedValue.setValue(canonicalValue->quantity());
                        parsedValueOk = true;
                     }
                  }
                  break;

               case JsonRecordDefinition::FieldType::OneOfMeasurementsWithUnits:
                  // It's definitely a coding error if there is no list of unit decoder mappings for a field declared to
                  // require such
                  Q_ASSERT(
                     nullptr != std::get<ListOfJsonMeasureableUnitsMappings const *>(fieldDefinition.valueDecoder)
                  );
                  // JSON schema validation should have ensured that the field is actually one with subfields for value
                  // and unit
                  Q_ASSERT(container->is_object());
                  {
                     // Logic similar to that for MeasurementWithUnits.  We rely on the NamedEntity subclass
                     // (Fermentable, Yeast, Misc, etc) to know what to do with the MassOrVolumeAmt
                     std::optional<Measurement::Amount> canonicalValue = readOneOfMeasurementsWithUnits(fieldDefinition,
                                                                                                        container);
                     if (canonicalValue) {
                        parsedValue.setValue(static_cast<MassOrVolumeAmt>(canonicalValue.value()));
                        parsedValueOk = true;
                     }
                  }
                  break;

               case JsonRecordDefinition::FieldType::SingleUnitValue:
                  // It's definitely a coding error if there is no unit specifier for a field declared to require one
                  Q_ASSERT(nullptr != std::get<JsonSingleUnitSpecifier const *>(fieldDefinition.valueDecoder));
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

               // Don't need a default case.  Compiler should warn us if we didn't have a case for one of the
               // JsonRecordDefinition::FieldType values.  This is one of the benefits of strongly-typed enums
            }

            //
            // What we do if we couldn't parse the value depends.  If it was a value that we didn't need to set on
            // the supplied Hop/Yeast/Recipe/Etc object, then we can just ignore the problem and carry on processing.
            // But, if this was a field we were expecting to use, then it's a problem that we couldn't parse it and
            // we should bail.
            //
            if (!parsedValueOk && !fieldDefinition.propertyName->isNull()) {
               userMessage <<
                  "Could not parse " << this->recordDefinition.namedEntityClassName << " node " <<
                  fieldDefinition.xPath << "=" << *container << " into " << *fieldDefinition.propertyName;
               return false;
            }

            //
            // So we've either parsed the value OK or we don't need it (or both)
            //
            // If we do need it, we now store the value
            //
            if (!fieldDefinition.propertyName->isNull()) {
               this->namedParameterBundle.insert(*fieldDefinition.propertyName, parsedValue);
            }
         }
      }
   }

   //
   // For everything but the root record, we now construct a suitable object (Hop, Recipe, etc) from the
   // NamedParameterBundle (which will be empty for the root record).
   //
   if (!this->namedParameterBundle.isEmpty()) {
      this->constructNamedEntity();
   }

   return true;
}

void JsonRecord::constructNamedEntity() {
   // Base class does not have a NamedEntity or a container, so nothing to do
   // Stictly, it's a coding error if this function is called, as caller should first check whether there is a
   // NamedEntity, and subclasses that do have one should override this function.
   qCritical() <<
      Q_FUNC_INFO << this->recordDefinition.namedEntityClassName << "this->namedParameterBundle:" <<
      this->namedParameterBundle;
   qDebug().noquote() << Q_FUNC_INFO << Logging::getStackTrace();
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

[[nodiscard]] JsonRecord::ProcessingResult JsonRecord::normaliseAndStoreInDb(
   std::shared_ptr<NamedEntity> containingEntity,
   QTextStream & userMessage,
   ImportRecordCount & stats
) {
   qDebug() << Q_FUNC_INFO;
   if (nullptr != this->namedEntity) {
      qDebug() <<
         Q_FUNC_INFO << "Normalise and store " << this->recordDefinition.namedEntityClassName << "(" <<
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
            Q_FUNC_INFO << "(Early found) duplicate" << this->recordDefinition.namedEntityClassName <<
            (this->includeInStats ? " will" : " won't") << " be included in stats";
         if (this->includeInStats) {
            stats.skipped(*this->recordDefinition.namedEntityClassName);
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
            Q_FUNC_INFO << "(Late found) duplicate" << this->recordDefinition.namedEntityClassName <<
            (this->includeInStats ? " will" : " won't") << " be included in stats";
         if (this->includeInStats) {
            stats.skipped(*this->recordDefinition.namedEntityClassName);
         }
      } else if (JsonRecord::ProcessingResult::Succeeded == processingResult && this->includeInStats) {
         stats.processedOk(*this->recordDefinition.namedEntityClassName);
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
            Q_FUNC_INFO << "Deleting stored" << this->recordDefinition.namedEntityClassName << "as" <<
            (JsonRecord::ProcessingResult::FoundDuplicate == processingResult ? "duplicate" : "failed to read all child records");
         this->deleteNamedEntityFromDb();
      }
   }

   return processingResult;
}


[[nodiscard]] bool JsonRecord::normaliseAndStoreChildRecordsInDb(QTextStream & userMessage,
                                                                 ImportRecordCount & stats) {
   qDebug() << Q_FUNC_INFO << this->childRecords.size() << "child records";
   //
   // We are assuming it does not matter which order different children are processed in.
   //
   // Where there are several children of the same type, we need to process them in the same order as they were read in
   // from the JSON document because, in some cases, this order matters.  In particular, in BeerJSON, the Mash Steps
   // inside a Mash (or rather MASH_STEP tags inside a MASH_STEPS tag inside a MASH tag) are stored in order without any
   // other means of identifying order. *** TODO Not sure this is true for BeerJSON ***
   //
   // So it's simplest just to process all the child records in the order they were read out of the JSON document.  This
   // is the advantage of storing things in a list such as std::vector.  (Alternatives such as QMultiHash iterate
   // through items that share the same key in the opposite order to which they were inserted and don't offer STL
   // reverse iterators, so going backwards would be a bit clunky.)
   //
   for (auto child = this->childRecords.begin(); child != this->childRecords.end(); ++child) {
      qDebug() <<
         Q_FUNC_INFO << "Storing" << child->record->recordDefinition.namedEntityClassName << "child of" <<
         this->recordDefinition.namedEntityClassName;
      if (JsonRecord::ProcessingResult::Failed ==
         child->record->normaliseAndStoreInDb(this->namedEntity, userMessage, stats)) {
         return false;
      }
      //
      // Now we've stored the child record (or recognised it as a duplicate of one we already hold), we want to link it
      // (or as the case may be the record it's a duplicate of) to the parent.  If this is possible via a property (eg
      // the style on a recipe), then we can just do that here.  Otherwise the work needs to be done in the appropriate
      // subclass of JsonNamedEntityRecord.
      //
      // We can't just use the presence or absence of a property name to determine whether the child record can be set
      // via a property.  It's a necessary but not sufficient condition.  This is because some properties are read-only
      // in the code (eg because they are calculated values) but need to be present in the FieldDefinition for export to
      // JSON to work.  However, we can tell whether a property is read-only by calling QMetaProperty::isWritable().
      //
      BtStringConst const & propertyName = *child->parentFieldDefinition->propertyName;
      if (!propertyName.isNull()) {
         // It's a coding error if we had a property defined for a record that's not trying to populate a NamedEntity
         // (ie for the root record).
         Q_ASSERT(nullptr != this->namedEntity.get());
         // It's a coding error if we're trying to set a non-existent property on the NamedEntity subclass for this
         // record.
         QMetaObject const * metaObject = this->namedEntity->metaObject();
         int propertyIndex = metaObject->indexOfProperty(*propertyName);
         Q_ASSERT(propertyIndex >= 0);
         QMetaProperty metaProperty = metaObject->property(propertyIndex);
         if (metaProperty.isWritable()) {
            // It's a coding error if we can't create a valid QVariant from a pointer to class we are trying to "set"
            Q_ASSERT(QVariant::fromValue(child->record->namedEntity.get()).isValid());

            qDebug() <<
               Q_FUNC_INFO << "Setting" << propertyName << "property (type = " <<
               this->namedEntity->metaObject()->property(
                  this->namedEntity->metaObject()->indexOfProperty(*propertyName)
               ).typeName() << ") on" << this->recordDefinition.namedEntityClassName << "object";
            this->namedEntity->setProperty(*propertyName, QVariant::fromValue(child->record->namedEntity.get()));
         } else {
            qDebug() << Q_FUNC_INFO << "Skipping non-writeable" << propertyName << "property (type = " <<
               this->namedEntity->metaObject()->property(
                  this->namedEntity->metaObject()->indexOfProperty(*propertyName)
               ).typeName() << ") on" << this->recordDefinition.namedEntityClassName << "object";
         }
      }
   }
   return true;
}


[[nodiscard]] bool JsonRecord::loadChildRecords(JsonRecordDefinition::FieldDefinition const & parentFieldDefinition,
                                                JsonRecordDefinition const & childRecordDefinition,
                                                boost::json::array & childRecordsData,
                                                QTextStream & userMessage) {
   qDebug() << Q_FUNC_INFO;
   //
   // This is where we have a list of one or more substantive records of a particular type, which may be either at top
   // level (eg hop_varieties) or inside another record that we are in the process of reading (eg hop_additions inside a
   // recipe).  Either way, we need to loop though these "child" records and read each one in with an JsonRecord object
   // of the relevant type.
   //
   for (auto & recordData : childRecordsData) {
      // Iterating through an array gives us boost::json::value objects
      // We assert that these are boost::json::object key:value containers (because we don't use arrays of other types)
      Q_ASSERT(recordData.is_object());


      auto constructorWrapper = childRecordDefinition.jsonRecordConstructorWrapper;

      JsonRecord::ChildRecord childRecord{&parentFieldDefinition,
                                          constructorWrapper(this->jsonCoding, recordData, childRecordDefinition)};

      if (!childRecord.record->load(userMessage)) {
         return false;
      }
      this->childRecords.push_back(std::move (childRecord));
   }

   return true;
}


[[nodiscard]] bool JsonRecord::isDuplicate() {
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

void JsonRecord::setContainingEntity([[maybe_unused]] std::shared_ptr<NamedEntity> containingEntity) {
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

// TODO Finish this!
void JsonRecord::toJson(NamedEntity const & namedEntityToExport) {
   Q_ASSERT(this->recordData.is_object());
   qDebug() <<
      Q_FUNC_INFO << "Exporting JSON for" << namedEntityToExport.metaObject()->className() << "#" <<
      namedEntityToExport.key();

   boost::json::object & recordDataAsObject = this->recordData.get_object();

   // BeerJSON doesn't care about field order, so we don't either (though it would be relatively small additional work
   // to control field order precisely).
   for (auto & fieldDefinition : this->recordDefinition.fieldDefinitions) {
      // If there isn't a property name that means this is not a field we support so there's nothing to write out.
      Q_ASSERT(fieldDefinition.propertyName);
      if (fieldDefinition.propertyName->isNull()) {
         // At the moment at least, we support all sub-record fields, so it's a coding error if one of them does not
         // have a property name.
         Q_ASSERT(JsonRecordDefinition::FieldType::Array != fieldDefinition.type);
         continue;
      }

      if (JsonRecordDefinition::FieldType::Array == fieldDefinition.type) {
         // .:TODO:.
         qCritical() << Q_FUNC_INFO << "NOT YET IMPLEMENTED";
         /*
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

          */
      } else {
         //
         // If it's not an array then it's fields on the object we're currently exporting to JSON
         //

         // It should not be possible for propertyName to be a null pointer.  (It may well be a pointer to
         // BtString::NULL_STR, in which case propertyName->isNull() will return true, but that's fine.)
         Q_ASSERT(fieldDefinition.propertyName);

         QVariant value = namedEntityToExport.property(**fieldDefinition.propertyName);
         Q_ASSERT(value.isValid());
         // If we have a non-trivial XPath then we'll need to create a sub-object
         //
         // In C++23, we'll be able to write key.contains('/"), but until then the alternative
         // is only slightly longer.
         auto key = fieldDefinition.xPath.asKey();
         if (key.find('/') != key.npos) {
            qDebug() << Q_FUNC_INFO <<
               "Splitting non-trivial XPath (" << fieldDefinition.xPath << ") for output of property" <<
               *fieldDefinition.propertyName << "of" << namedEntityToExport.metaObject()->className();
            auto keyList = fieldDefinition.xPath.getElements();
            // Ensure sub-objects exist.
            boost::json::object * currentObject = &recordDataAsObject;
            auto const * currentKey = &keyList[0]; // Strictly we don't need this initialisation, but it
                                                   // prevents a compiler warning.
            for (auto const & subKey : keyList) {
               qDebug() << Q_FUNC_INFO << "Sub-key" << std::string(subKey).c_str();
               // We want to loop over all but the last items in the vector, as the last subKey is what gets passed to
               // insertValue.  Easiest way to do this is loop over everything and bail out when we reach the last
               // element.
               if (&subKey != &keyList.back()) {
                  if (!currentObject->if_contains(subKey)) {
                     qDebug() << Q_FUNC_INFO << "Making sub-object for" << std::string(subKey).c_str();
                     (*currentObject)[subKey].emplace_object();
                  }
                  currentObject = (*currentObject)[subKey].if_object();
                  Q_ASSERT(currentObject);
               }

               currentKey = &subKey;
            }
            insertValue(fieldDefinition, *currentObject, *currentKey, value);
            continue;
         }

         insertValue(fieldDefinition, recordDataAsObject, key, value);
      }

   }
   return;
}

void JsonRecord::subRecordToJson(JsonRecordDefinition::FieldDefinition const & fieldDefinition,
                                 [[maybe_unused]] JsonRecord const & subRecord,
                                 NamedEntity const & namedEntityToExport,
                                 [[maybe_unused]] QTextStream & out,
                                 [[maybe_unused]] int indentLevel,
                                 [[maybe_unused]] char const * const indentString) const {
   // Base class does not know how to handle nested records
   // It's a coding error if we get here as this virtual member function should be overridden classes that have nested
   // records
   qCritical() << Q_FUNC_INFO <<
      "Coding error: cannot export" << namedEntityToExport.metaObject()->className() << "(" <<
      this->recordDefinition.namedEntityClassName << ") property" << fieldDefinition.propertyName << "to <" <<
      fieldDefinition.xPath << "> from base class JsonRecord";
   Q_ASSERT(false);
   return;
}
