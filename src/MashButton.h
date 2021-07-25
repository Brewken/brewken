/**
 * MashButton.h is part of Brewken, and is copyright the following authors 2009-2014:
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
#ifndef MASHBUTTON_H
#define MASHBUTTON_H
#pragma once

#include <QPushButton>
#include <QMetaProperty>
#include <QVariant>

// Forward declarations.
class Mash;
class Recipe;
class QWidget;

/*!
 * \class MashButton
 *
 * \brief This is a view class that displays a named mash
 */
class MashButton : public QPushButton
{
   Q_OBJECT
public:
   MashButton(QWidget* parent = 0);
   virtual ~MashButton(){}

   //! \brief Observe \c recipe
   void setRecipe(Recipe* recipe);
   //! \brief Observe \c mash.
   void setMash(Mash* mash);
   //! \brief \return the observed mash
   Mash* mash();

private slots:
   void recChanged(QMetaProperty,QVariant);
   void mashChanged(QMetaProperty,QVariant);

private:
   Recipe* m_rec;
   Mash* _mash;
};

#endif
