/*======================================================================================================================
 * widgets/UnitAndScalePopUpMenu.h is part of Brewken, and is copyright the following authors 2012-2023:
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

#include <memory>
#include <optional>

#include <QMenu>

#include "measurement/SystemOfMeasurement.h"
#include "measurement/UnitSystem.h"

namespace UnitAndScalePopUpMenu {
   /**
    * \brief Creates a \c QMenu (possibly with a sub-menu) for specifying/changing a "forced" \c SystemOfMeasurement
    *        (and thus \c UnitSystem) and/or \c RelativeScale used to display a particular UI field.  This allows it to
    *        be different from the global default (eg so the user can specify a particular weight field is shown in
    *        metric even though they have set US Customary as the default for weight fields).
    *
    *        Note that the \c UnitAndScalePopUpMenu itself has no knowledge of the field; it is for the creator to
    *        manage the effects of menu choices on how the field is displayed.
    *
    * \param parent the Qt Widget that is to "own" the newly-created \c QMenu, or \c nullptr if there is none
    * \param physicalQuantities the physical quantity (or quantities) of the value(s) in the UI field.  If there is more
    *                           than one physical quantity here, it means we can measure in more than one way, eg by
    *                           Mass or by Volume
    * \param forcedSystemOfMeasurement the current \c SystemOfMeasurement, if any, specified for the UI field.  (If none
    *                                  is specified then the global default is used.)
    * \param forcedRelativeScale the forced scale, if any, for displaying the field.  (NB: Should always be
    *                            \c std::nullopt_t if \c physicalQuantity is \c Mixed2PhysicalQuantities.)
    *
    * \return New \c QMenu "owned" by \c parent, but see comment in \c widgets/SmartLabel.cpp for why we return
    *         unique pointer so that caller really owns the object -- essentially the returned object typically has a
    *         much shorter lifetime than the parent.
    */
   std::unique_ptr<QMenu> create(QWidget * parent,
                                 Measurement::PhysicalQuantities physicalQuantities,
                                 std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement,
                                 std::optional<Measurement::UnitSystem::RelativeScale> forcedRelativeScale);

   /**
    * \brief When a pop-up \c QMenu is displayed, by calling its \c exec function, the return value is a \c QAction
    *        corresponding to the menu item the user selected.  This function then retrieves the
    *        \c std::optional<Measurement::SystemOfMeasurement> or
    *        \c std::optional<Measurement::UnitSystem::RelativeScale> from that \c QAction.
    */
   template<typename T>
   std::optional<T> dataFromQAction(QAction const & action);
}

#endif
