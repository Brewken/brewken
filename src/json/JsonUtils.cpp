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

#include <iostream>
#include <sstream>

// We could just include <boost/json.hpp> which pulls all the Boost.JSON headers in, but that seems overkill
#include <boost/json/parse_options.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/string.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/stream_parser.hpp>

#include <QDebug>
#include <QFile>
#include <QString>

#include "utils/BtException.h"
#include "utils/BtStringStream.h"
#include "utils/ErrorCodeToStream.h"

boost::json::value JsonUtils::loadJsonDocument(QString const & fileName, bool allowComments) {

   QFile inputFile(fileName);

   if (!inputFile.open(QIODevice::ReadOnly)) {
      // Some slight duplication here but there's value in having the log messages in English and the on-screen display
      // message in the user's preferred language
      qWarning() <<
         Q_FUNC_INFO << "Could not open " << fileName << " for reading (error #" << inputFile.error() << ":" <<
         inputFile.errorString() << ")";
      QString errorMessage{
         QObject::tr("Could not open %1 for reading (error # %2)").arg(fileName).arg(inputFile.error())
      };
      throw BtException(errorMessage);
   }

   qint64 fileSize = inputFile.size();
   if (fileSize <= 0) {
      BtStringStream errorMessage;
      errorMessage << "File " << fileName << " has no data (length is " << fileSize << " bytes)";
      qWarning() << Q_FUNC_INFO << errorMessage.asString();
      throw BtException(errorMessage.asString());
   }

   //
   // A few notes on how we do the parsing:
   //
   // Using a streaming parser
   // ------------------------
   // The simplest way to have Boost.JSON parse a document is to call boost::json::parse().  However, this doesn't
   // give you the best error handling.  In particular if there is a problem with the json input, you'll just get
   // a std::error_code that says, eg, "syntax error" without giving you any clue where in the input the problem is.
   //
   // So, instead, we need to create a streaming parser and give it the source one line at a time.  That way, if we
   // hit an error we can get the line number that first caused it.
   //
   // String encodings
   // ----------------
   // Whichever way we do the parsing, we can't use QString for the input.  QString and std::string are typically
   // double-byte unicode strings, whereas JSON files are UTF-8, as required by RFC8259 (see 8.1 of
   // https://datatracker.ietf.org/doc/html/rfc8259, which says "JSON text exchanged between systems that are not part
   // of a closed ecosystem MUST be encoded using UTF-8").  Of course it's possible to convert between double-byte
   // unicode and UTF-8, but there's no need as Boost.JSON has its own string class that natively stores data in UTF-8
   // encoding.
   //
   // Similarly, per https://www.boost.org/doc/libs/1_77_0/libs/json/doc/html/json/dom/string.html, when a
   // boost::json::string is formatted to a std::ostream, the result is a valid JSON. That is, the result will be double
   // quoted and the contents properly escaped per the JSON specification.
   //
   // The boost::json::string class has a similar interface and functionality to std::basic_string, with a few
   // differences, including that access to characters in the range (size(), capacity()) is permitted, including write
   // access via the data() member function.  However, we can't use this as an efficient way to read data directly into
   // the string, because modifying the character array returned by data() does not tell the string it has changed in
   // size, and increasing the size of the string results in data being overwritten.
   //
   // Instead, we use boost::json::string_view (which is an alias either for boost::string_view or std::string_view
   // depending on how the code is compiled) to put a lightweight wrapper around a buffer of char * that Qt can provide.
   //
   // Exceptions
   // ----------
   // If you give it an error code parameter, then Boost.JSON will report its errors via that rather than by throwing
   // std::system_error exception (or boost::system::system_error if linking with Boost).  However, even when using
   // error codes, std::bad_alloc exceptions thrown from the underlying memory_resource are still possible, so it's
   // good practice to catch and recast these.
   //
   try {
      std::error_code errorCode;

      boost::json::parse_options parseOptions;
      parseOptions.allow_comments = allowComments;
      boost::json::stream_parser streamParser{
         boost::json::storage_ptr{}, // Default memory resource
         parseOptions,               // Default parse options (strict parsing)
      };
      QByteArray rawInputLine{};
      for (auto [lineNumber, bytesLeftToRead] = std::tuple{1, fileSize};
         bytesLeftToRead > 0;
         ++lineNumber, bytesLeftToRead -= rawInputLine.size()) {
         rawInputLine = inputFile.readLine();
         boost::json::string_view inputStringView{rawInputLine.data()};
         streamParser.write(inputStringView, errorCode);
         if (errorCode) {
            BtStringStream errorMessage{};
            errorMessage << "Parsing failed at line " << lineNumber << ": " << errorCode;
            qWarning() << Q_FUNC_INFO << errorMessage.asString();
            throw BtException(errorMessage.asString());
         }
      }

      streamParser.finish(errorCode);
      if (errorCode) {
         BtStringStream errorMessage;
         errorMessage << "Parsing failed after reading last line: " << errorCode;
         qWarning() << Q_FUNC_INFO << errorMessage.asString();
         throw BtException(errorMessage.asString());
      }
      boost::json::value parsedDocument = streamParser.release();

      return parsedDocument;
   } catch (std::bad_alloc const & exception) {
      // Not sure that there's a concise and user-friendly way to describe a memory allocation exception, but at
      // least we can give the user something semi-meaningful to report to a maintainer.
      BtStringStream errorMessage;
      errorMessage << "Memory allocation error (" << exception.what() << ") while parsing " << fileName;
      qWarning() << Q_FUNC_INFO << errorMessage.asString();
      throw BtException(errorMessage.asString());
   }
}

template<class S>
S & operator<<(S & stream, boost::json::kind const knd) {
   std::ostringstream output;
   output << "boost::json::kind::" << knd;
   stream << output.str().c_str();
   return stream;
}

//
// Instantiate the above template function for the types that are going to use it
// (This is all just a trick to allow the template definition to be here in the .cpp file and not in the header, so we
// don't have to pull in std::ostringstream headers in other parts of the code.)
//
template QDebug & operator<<(QDebug & stream, boost::json::kind const knd);
template QTextStream & operator<<(QTextStream & stream, boost::json::kind const knd);

template<class S,
         std::enable_if_t<(std::is_same<QDebug, S>::value || std::is_same<QTextStream, S>::value), bool> >
S & operator<<(S & stream, boost::json::value const & val) {
   // Boost.JSON already handles output to standard library output streams, so we are just piggy-backing on this to
   // provide the same output in the Qt output streams we care about.  However, we also output which value type the
   // value contains as this can sometimes be helpful for debugging.
   std::ostringstream output;
   output << "(" << val.kind() << "): " << val;
   stream << output.str().c_str();
   return stream;
}

template QDebug & operator<<(QDebug & stream, boost::json::value const & val);
template QTextStream & operator<<(QTextStream & stream, boost::json::value const & val);
