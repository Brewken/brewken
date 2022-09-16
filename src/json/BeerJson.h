/*======================================================================================================================
 * json/BeerJson.h is part of Brewken, and is copyright the following authors 2021-2022:
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
#ifndef JSON_BEERJSON_H
#define JSON_BEERJSON_H
#pragma once

class QString;
class QTextStream;

namespace BeerJson {
   /*!
    * \brief Import ingredients, recipes, etc from a BeerJSON file
    *
    * \param filename
    * \param userMessage Where to write any (brief!) message we want to be shown to the user after the import.
    *                    Typically this is either the reason the import failed or a summary of what was imported.
    *
    * \return true if succeeded, false otherwise
    */
   bool import(QString const & filename, QTextStream & userMessage);


}

#endif
