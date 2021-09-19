/*======================================================================================================================
 * json/JsonUtils.cpp is part of Brewken, and is copyright the following authors 2021:
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
#include "json/JsonUtils.h"

// We could just include <boost/json.hpp> which pulls all the Boost.JSON headers in, but that seems overkill
#include <boost/json/parse_options.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/string.hpp>

#include <QDebug>
#include <QFile>
#include <QString>

#include "utils/BtException.h"
#include "utils/BtStringStream.h"

boost::json::value JsonUtils::loadJsonDocument(QString const & fileName, bool allowComments) {

   QFile inputFile(fileName);

   if (!inputFile.open(QIODevice::ReadOnly)) {
      // Some slight duplication here but there's value in having the log messages in English and the on-screen display
      // message in the user's preferred language
      qWarning() << Q_FUNC_INFO << "Could not open " << fileName << " for reading (error #" << inputFile.error() << ")";
      QString errorMessage{
         QObject::tr("Could not open %1 for reading (error # %2)").arg(fileName).arg(inputFile.error())
      };
      throw BtException(errorMessage);
   }

   qint64 fileSize = inputFile.size();
   if (fileSize <= 0) {
      BtStringStream errorMessage;
      errorMessage << "File " << fileName << " has no data (length is " << fileSize << " bytes)";
      qWarning() << Q_FUNC_INFO << errorMessage.string();
      throw BtException(errorMessage.asString());
   }

   //
   // The Boost.JSON library has its own string class that natively stores in UTF-8 encoding (because that's the
   // encoding that RFC8259 requires JSON to use (see 8.1 of https://datatracker.ietf.org/doc/html/rfc8259, which
   // says "JSON text exchanged between systems that are not part of a closed ecosystem MUST be encoded using
   // UTF-8").
   //
   // Similarly, per https://www.boost.org/doc/libs/1_77_0/libs/json/doc/html/json/dom/string.html, when a
   // boost::json::string is formatted to a std::ostream, the result is a valid JSON. That is, the result will be
   // double quoted and the contents properly escaped per the JSON specification.
   //
   // The boost::json::string class has a similar interface and functionality to std::basic_string, with a few
   // differences, including that access to characters in the range [size(), capacity()) is permitted, including
   // write access via the data() member function.  However, we can't use this as an efficient way to read data
   // directly into the string, because modifying the character array returned by data() does not tell the string it
   // has changed in size, and increasing the size of the string results in data being overwritten.
   //
   // Instead, we use boost::json::string_view (which is an alias either for boost::string_view or std::string_view
   // depending on how the code is compiled) to put a lightweight wrapper around a buffer of char * that Qt can
   // provide.
   //
   QByteArray rawInput;
   rawInput.resize(fileSize);
   auto bytesRead = inputFile.read(rawInput.data(), fileSize);
   if (bytesRead != fileSize) {
      BtStringStream errorMessage;
      errorMessage <<
         "Error reading " << fileName << ".  Request to read " << fileSize << " bytes returned " << bytesRead <<
         " (error #" << inputFile.error() << ")";
      qWarning() << Q_FUNC_INFO << errorMessage.string();
      throw BtException(errorMessage.asString());
   }

   //
   // Now give the raw UTF-8 data to Boost.JSON to parse as a JSON document
   //
   boost::json::string_view inputStringView{rawInput.data()};

   qDebug() << Q_FUNC_INFO <<
      "Read" << bytesRead << "bytes of" << fileName << "into string of length" << inputStringView.size();
   // Normally leave this next one commented out as it will log the entire contents of the input file!
   //qDebug() << Q_FUNC_INFO << "Total data is" << inputStringView.data();

   //
   // If you give it an error code parameter, then Boost.JSON will report its errors via that rather than by throwing
   // std::system_error exception (or boost::system::system_error if linking with Boost).  However, even when using
   // error codes, std::bad_alloc exceptions thrown from the underlying memory_resource are still possible, so it's
   // good practice to catch and recast these.
   //
   try {
      boost::json::parse_options parseOptions;
      parseOptions.allow_comments = allowComments;
      boost::json::error_code errorCode;
      boost::json::value parsedDocument = boost::json::parse(inputStringView, errorCode, {}, parseOptions);
      if (errorCode) {
         BtStringStream errorMessage;
         errorMessage << "Parsing failed: " << errorCode.message().c_str();
         qWarning() << Q_FUNC_INFO << errorMessage.string();
         throw BtException(errorMessage.asString());
      }

      if (parsedDocument.is_object()) {
         boost::json::object valueAsObject = parsedDocument.as_object();
         qDebug() << Q_FUNC_INFO << "Parsed " << valueAsObject.size() << "JSON elements";
         for (auto ii : valueAsObject) {
            qDebug() << Q_FUNC_INFO << ii.key().data();
         }
      }

      return parsedDocument;

   } catch (std::bad_alloc const & exception) {
      // Not sure that there's a concise and user-friendly way to describe a memory allocation exception, but at
      // least we can give the user something semi-meaningful to report to a maintainer.
      BtStringStream errorMessage;
      errorMessage << "Memory allocation error (" << exception.what() << ") while parsing " << fileName;
      qWarning() << Q_FUNC_INFO << errorMessage.string();
      throw BtException(errorMessage.asString());
   }
}
