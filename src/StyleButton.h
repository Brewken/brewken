/**
 * StyleButton.h is part of Brewken, and is copyright the following authors 2009-2014:
 *   • Mik Firestone <mikfire@gmail.com>
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
#ifndef STYLEBUTTON_H
#define STYLEBUTTON_H

#include <QPushButton>
#include <QMetaProperty>
#include <QVariant>

// Forward declarations.
class Style;
class Recipe;
class QWidget;

/*!
 * \class StyleButton
 *
 * \brief This is a view class that displays the name of a style
 */
class StyleButton : public QPushButton
{
   Q_OBJECT
public:
   StyleButton(QWidget* parent = nullptr);
   virtual ~StyleButton(){}

   //! Observe a recipe's equipment.
   void setRecipe(Recipe* recipe);
   //! Observe a particular equipment.
   void setStyle(Style* style);

private slots:
   void recChanged(QMetaProperty,QVariant);
   void styleChanged(QMetaProperty,QVariant);

private:
   Recipe* m_rec;
   Style* _style;
};

#endif
