/*======================================================================================================================
 * widgets/UnitAndScalePopUpMenu.h is part of Brewken, and is copyright the following authors 2012-2022:
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef WIDGETS_UNITANDSCALEPOPUPMENU_H
#define WIDGETS_UNITANDSCALEPOPUPMENU_H
#pragma once

#include <QMenu>

#include "measurement/UnitSystem.h"

class UnitAndScalePopUpMenu : public QMenu {
Q_OBJECT
public:
   /**
    * \brief For a given \c UnitSystem, creates a \c QMenu (possibly with a sub-menu) for changing the \c UnitSystem
    *        and/or \c RelativeScale used to display a particular field.  (Note that the \c UnitAndScalePopUpMenu itself
    *        has no knowledge of the field; it is for the creator to manage the effects of menu choices on how the field
    *        is displayed.)
    *
    * \param parent
    * \param unitSystem the current \c UnitSystem being used to display the field
    * \param relativeScale if set to something other than \c UnitSystem::noScale, this is the forced scale for
    *                      displaying the field
    * \param generateScale
    *
    * \return New \c QMenu owned by \c parent
    */
   UnitAndScalePopUpMenu(QWidget * parent,
                         Measurement::UnitSystem const & unitSystem,
                         Measurement::UnitSystem::RelativeScale const relativeScale = Measurement::UnitSystem::noScale,
                         bool const generateScale = true);
   virtual ~UnitAndScalePopUpMenu();
};

#endif
