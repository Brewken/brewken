/*======================================================================================================================
 * widgets/UnitAndScalePopUpMenu.cpp is part of Brewken, and is copyright the following authors 2012-2022:
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
#include "widgets/UnitAndScalePopUpMenu.h"

#include <QApplication>

#include "measurement/Unit.h"
#include "measurement/UnitSystem.h"

namespace {
   /**
    * \brief
    *
    * \param menu
    * \param text
    * \param data
    * \param currentVal
    * \param actionGroup
    */
   void generateAction(QMenu * menu, QString text, QVariant data, QVariant currentVal, QActionGroup* actionGroup) {
      QAction * action = new QAction(menu);

      action->setText(text);
      action->setData(data);
      action->setCheckable(true);
      action->setChecked(currentVal == data);
      if (actionGroup) {
         actionGroup->addAction(action);
      }

      menu->addAction(action);
      return;
   }
}

UnitAndScalePopUpMenu::UnitAndScalePopUpMenu(QWidget * parent,
                                             Measurement::UnitSystem const & unitSystem,
                                             Measurement::UnitSystem::RelativeScale const relativeScale,
                                             bool const generateScale) :
   QMenu(parent) {

   // .:TODO:. This is mostly working but not displaying "default" as selected value when it should be
   QActionGroup * actionGroup = new QActionGroup(parent);

   // If there are other UnitSystems for this one's PhysicalQuantity then we want the user to be able to select
   // between them
   auto unitSystems = Measurement::UnitSystem::getUnitSystems(unitSystem.getPhysicalQuantity());
   if (unitSystems.size() > 1) {
      generateAction(this, QApplication::translate("UnitSystem", "Default"), QVariant(), "", actionGroup);
      for (auto system : unitSystems) {
         generateAction(this, system->systemOfMeasurementName, system->uniqueName, unitSystem.uniqueName, actionGroup);
      }
   }

   // If this UnitSystem has more than one Unit, allow the user to select a Unit for the scale
   auto relativeScales = unitSystem.getRelativeScales();
   if (relativeScales.size() > 1) {
      QMenu * subMenu = new QMenu(this);
      generateAction(subMenu,
                     QApplication::translate("UnitSystem", "Default"),
                     Measurement::UnitSystem::noScale,
                     relativeScale,
                     actionGroup);
      for (auto scale : relativeScales) {
         generateAction(subMenu, unitSystem.scaleUnit(scale)->name, scale, relativeScale, actionGroup);
      }
      subMenu->setTitle(QApplication::translate("UnitSystem", "Scale"));
      this->addMenu(subMenu);
   }

   return;
}

UnitAndScalePopUpMenu::~UnitAndScalePopUpMenu() = default;
