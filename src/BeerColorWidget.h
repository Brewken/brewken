/*======================================================================================================================
 * BeerColorWidget.h is part of Brewken, and is copyright the following authors 2009-2014:
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
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
#ifndef BEERCOLORWIDGET_H
#define BEERCOLORWIDGET_H

class BeerColorWidget;

#include <QWidget>
#include <QColor>
#include <QPaintEvent>
#include <QImage>
#include <QMetaProperty>
#include <QVariant>
#include "model/Recipe.h"

/*!
 * \class BeerColorWidget
 *
 *
 * \brief Displays the approximate color of the beer on screen.
 */
class BeerColorWidget : public QWidget
{
   Q_OBJECT

public:
   BeerColorWidget(QWidget* parent=0);

   void setColor( QColor newColor );
   //! Observe \b rec for changes in color, and automatically update.
   void setRecipe( Recipe* rec );

public slots:
   void parseChanges(QMetaProperty prop, QVariant val);

protected:
   virtual void paintEvent(QPaintEvent *);
   QColor color;
private:
   QImage glass;
   void showColor();

   Recipe* recObs;
};

#endif   /* BEERCOLORWIDGET_H */
