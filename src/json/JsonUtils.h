/*======================================================================================================================
 * json/JsonUtils.h is part of Brewken, and is copyright the following authors 2021:
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
#ifndef JSON_JSONUTILS_H
#define JSON_JSONUTILS_H
#pragma once

#include <boost/json/value.hpp>

class QString;

namespace JsonUtils {
   /**
    * \brief Constructor loads a JSON document from the supplied file path and parses it into a tree of Boost.JSON
    *        objects, the root of which can be accessed via the \c getParsedDocument() member function
    *
    * \param fileName is either the absolute path to a file on local storage, or the path (or alias) of a resource
    *                 packaged with the program
    *
    * \param allowComments Strictly, JSON documents are not allowed to contain comments.  In reality, it is sometimes
    *                      useful to have them.  Turning this option on will cause C/C++-style comments in the document
    *                      being opened to be ignored (rather than generate an error).
    *
    * \throw BtException containing text that can be displayed to the user
    */
   boost::json::value loadJsonDocument(QString const & fileName, bool allowComments = false);
}

#endif
