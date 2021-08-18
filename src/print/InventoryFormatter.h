/*======================================================================================================================
 * print/InventoryFormatter.h is part of Brewken, and is copyright the following authors 2016-2021:
 *   • Mark de Wever <koraq@xs4all.nl>
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
#ifndef INVENTORY_FORMATTER_H
#define INVENTORY_FORMATTER_H
#pragma once

class QFile;
class QPrinter;

namespace InventoryFormatter {

   /*!
   * \brief Shows the print preview dialogue for the inventory.
   */
   void printPreview();

   /*!
   * \brief Prints the inventory.
   * \param printer The printer to print to, should not be @c NULL.
   */
   void print(QPrinter* printer);

   /*!
   * \brief Exports the inventory to a HTML document.
   * \param file The output file opened for writing.
   */
   void exportHtml(QFile* file);

}
#endif
