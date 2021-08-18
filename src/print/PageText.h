/*======================================================================================================================
 * print/PageText.h is part of Brewken, and is copyright the following authors 2021:
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
#ifndef PRINT_PAGETEXT_H
#define PRINT_PAGETEXT_H
#pragma once

#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QTextOption>

#include "PageChildObject.h"

namespace Print {
   /**
    * \class PageText text object that needs to have a Value (text) and a Font (Will default to Application Font if not set.)
    */
   class PageText
      : public PageChildObject
   {

   public:
      PageText(Page *parent, QString value, QFont font);
      virtual ~PageText();

      QString Value;
      QTextOption Options;

      int count();

      //Enforced by PageChildObject
      void render(QPainter *painter);
      QSize getSize();
      void calculateBoundingBox( double scalex = 0.0, double scaley = 0.0 );
   };
}
#endif
