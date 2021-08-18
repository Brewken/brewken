/*======================================================================================================================
 * print/PageBreak.cpp is part of Brewken, and is copyright the following authors 2021:
 *   • Mattias Måhl <mattias@kejsarsten.com>
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
#include "print/PageBreak.h"
#include "print/Page.h"

namespace Print {
   PageBreak::PageBreak(Page *parent) :
      PageChildObject{parent} {
      return;
   }

   void PageBreak::render(QPainter *painter) {
      this->parent->printer->newPage();
   }

   QSize PageBreak::getSize() {
      return QSize();
   }

   // .:TBD:. Not sure what this is for...?
   void PageBreak::calculateBoundingBox(double scalex, double scaley) {
      return;
   }

}
