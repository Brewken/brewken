/**
 * StyleRangeWidget.h is part of Brewken, and is copyright the following authors 2009-2014:
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
 */

#ifndef STYLERANGEWIDGET_H
#define STYLERANGEWIDGET_H

#include <QWidget>
#include <QSize>
#include <QString>
#include "RangedSlider.h"
class QPaintEvent;
class QMouseEvent;

/*!
 * \brief Widget to display a recipe statistic with "in-range" context from the style.
 *
 */
class StyleRangeWidget : public RangedSlider
{
   Q_OBJECT
public:
   StyleRangeWidget(QWidget* parent=0);
};

#endif /*STYLERANGEWIDGET_H*/
