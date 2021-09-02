/*======================================================================================================================
 * BtHorizontalTabs.h is part of Brewken, and is Copyright the following authors 2021:
 *   • Mik Firestone <mikfire@gmail.com>
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
#ifndef BTHORIZONTALTABS_H
#define BTHORIZONTALTABS_H
#pragma once

#include <QProxyStyle>
#include <QStyleOption>
#include <QStyleOptionTab>
#include <QSize>

/**
 * \brief A quick little bit to handle a tab bar with the tabs on the west.
 */
class BtHorizontalTabs : public QProxyStyle
{
public:
   QSize sizeFromContents( ContentsType type, const QStyleOption* option,
                           const QSize& size, const QWidget* widget ) const
   {
      QSize s = QProxyStyle::sizeFromContents(type,option,size,widget);
      if ( type == QStyle::CT_TabBarTab ) {
         s.transpose();
      }
      return s;
   }

   void drawControl( ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
   {
      if ( element == CE_TabBarTabLabel ) {
         if ( const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option)) {
            QStyleOptionTab opt(*tab);
            // this looks wrong, but it isn't. No idea why.
            opt.shape = QTabBar::RoundedNorth;
            QProxyStyle::drawControl(element,&opt,painter,widget);
            return;
         }
      }
      QProxyStyle::drawControl(element,option,painter,widget);
   }
};
#endif
