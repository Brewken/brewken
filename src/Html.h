/*======================================================================================================================
 * Html.h is part of Brewken, and is copyright the following authors 2016-2022:
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mark de Wever <koraq@xs4all.nl>
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
#ifndef HTML_H
#define HTML_H
#pragma once

class QString;

namespace Html {

/*!
 * \return The contents of the CSS resource.
 * \param recourceName The name of the CSS resource to retreive.
 */
QString getCss(const QString& recourceName);

/*!
 * \return The header of a HTML document.
 * \param title The title of the document.
 * \param cssResourceName The name of the CSS resource for the document.
 */
QString createHeader(const QString& title, const QString& cssResourceName);

/*!
 * \return The footer of a HTML document.
 */
QString createFooter();

}
#endif
