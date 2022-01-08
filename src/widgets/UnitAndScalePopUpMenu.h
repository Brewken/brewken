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

namespace UnitAndScalePopUpMenu {
   /**
    * \brief Creates a \c QMenu (possibly with a sub-menu) for specifying/changing a "forced" \c UnitSystem and/or
    *        \c RelativeScale used to display a particular UI field.  This allows it to be different from the global
    *        default (eg so the user can specify a particular weight field is shown in metric even though they have set
    *        US Customary as the default for weight fields).
    *
    *        Note that the \c UnitAndScalePopUpMenu itself has no knowledge of the field; it is for the creator to
    *        manage the effects of menu choices on how the field is displayed.
    *
    * \param parent
    * \param physicalQuantity the physical quantity of the value in the UI field
    * \param forcedUnitSystem the current \c UnitSystem specified for the UI field, or \c nullptr for the global default
    * \param forcedRelativeScale if set to something other than \c UnitSystem::noScale, this is the forced scale for
    *                            displaying the field
    *
    * \return New \c QMenu owned by \c parent
    */
   QMenu * create(QWidget * parent,
                  Measurement::PhysicalQuantity physicalQuantity,
                  Measurement::UnitSystem const * forcedUnitSystem,
                  Measurement::UnitSystem::RelativeScale const forcedRelativeScale);
}

#endif
