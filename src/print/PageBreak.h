/*======================================================================================================================
 * print/PageBreak.h is part of Brewken, and is copyright the following authors 2021:
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
#ifndef PRINT_PAGEBREAK_H
#define PRINT_PAGEBREAK_H
#pragma once

#include "print/PageChildObject.h"

namespace Print {
   class Page;
   /**
    * @brief class to handle adding in extra page beaks in the generation of printouts
    *
    */
   class PageBreak : public PageChildObject {
   public:
      /**
       * @brief Construct a new Page Break object
       *
       * @param parent
       */
      PageBreak(Page *parent);

      /**
       * @brief adds a new page to the printer.
       *
       * @param painter
       */
      void render(QPainter *painter);

      QSize getSize();

      void calculateBoundingBox(double scalex = 0.0, double scaley = 0.0);
   };
}


#endif
