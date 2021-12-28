/*======================================================================================================================
 * BtLabel.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
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
 =====================================================================================================================*/
#include "BtLabel.h"

#include <QSettings>
#include <QDebug>

#include "Brewken.h"
#include "measurement/Measurement.h"
#include "model/Style.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"


BtLabel::BtLabel(QWidget *parent,
                 LabelType lType) :
   QLabel{parent},
   whatAmI{lType},
   btParent{parent},
   _menu{nullptr} {
   connect(this, &QWidget::customContextMenuRequested, this, &BtLabel::popContextMenu);
   return;
}

BtLabel::~BtLabel() = default;

void BtLabel::initializeSection() {
   if (!this->_section.isEmpty()) {
      return;
   }

   // as much as I dislike it, dynamic properties can't be referenced on
   // initialization.
   QWidget * mybuddy = buddy();

   //
   // If the label has the configSection defined, use it
   // otherwise, if the paired field has a configSection, use it
   // otherwise, if the parent object has a configSection, use it
   // if all else fails, get the parent's object name
   //
   if ( property("configSection").isValid() ) {
      this->_section = property("configSection").toString();
   } else if (mybuddy && mybuddy->property("configSection").isValid() ) {
      this->_section = mybuddy->property("configSection").toString();
   } else if (this->btParent->property("configSection").isValid() ) {
      this->_section = this->btParent->property("configSection").toString();
   } else {
      qDebug() << Q_FUNC_INFO << "this failed" << this;
      this->_section = this->btParent->objectName();
   }
   return;
}

void BtLabel::initializeProperty() {

   if ( ! propertyName.isEmpty() ) {
      return;
   }

   QWidget* mybuddy = buddy();
   if ( property("editField").isValid() ) {
      propertyName = property("editField").toString();
   } else if ( mybuddy && mybuddy->property("editField").isValid() ) {
      propertyName = mybuddy->property("editField").toString();
   } else {
      qDebug() << Q_FUNC_INFO  << "That failed miserably";
   }
}

void BtLabel::initializeMenu() {
   // .:TBD:. At the moment, we don't initialise the menu if it already exists, however this is not quite correct as the
   // sub-menu for relative scale needs to change when a different unit system is selected
   if (this->_menu) {
      return;
   }

   Measurement::UnitSystem const * unitSystem = Measurement::getUnitSystemForField(this->propertyName, this->_section);
   Measurement::UnitSystem::RelativeScale relativeScale = Measurement::getRelativeScaleForField(this->propertyName,
                                                                                                _section);

   switch (whatAmI) {
      case COLOR:
      case DENSITY:
      case MASS:
      case TEMPERATURE:
      case VOLUME:
      case TIME:
      case DIASTATIC_POWER:
         this->_menu = unitSystem->createUnitSystemMenu(btParent, relativeScale);
         break;

      case MIXED:
         // This looks weird, but it works.
         this->_menu = unitSystem->createUnitSystemMenu(btParent, relativeScale, false); // no scale menu
         break;

/*
      case COLOR:
         this->_menu = Brewken::setupColorMenu(btParent, unitSystem);
         break;
      case DENSITY:
         this->_menu = Brewken::setupDensityMenu(btParent, unitSystem);
         break;
      case MASS:
         this->_menu = Brewken::setupMassMenu(btParent, unitSystem, relativeScale);
         break;
      case MIXED:
         // This looks weird, but it works.
         this->_menu = Brewken::setupVolumeMenu(btParent, unitSystem, relativeScale, false); // no scale menu
         break;
      case TEMPERATURE:
         this->_menu = Brewken::setupTemperatureMenu(btParent, unitSystem);
         break;
      case VOLUME:
         this->_menu = Brewken::setupVolumeMenu(btParent, unitSystem, relativeScale);
         break;
      case TIME:
         this->_menu = Brewken::setupTimeMenu(btParent, relativeScale); //scale menu only
         break;
      case DATE:
         this->_menu = Brewken::setupDateMenu(btParent, unitSystem); // unit only
         break;
      case DIASTATIC_POWER:
         this->_menu = Brewken::setupDiastaticPowerMenu(btParent, unitSystem);
         break;*/
      default:
         // Nothing to set up if we're of type NONE
         break;
   }
   return;
}

void BtLabel::popContextMenu(const QPoint& point) {
   // For the moment, at least, we do not allow people to choose date formats per-field.  (Although you might want to
   // mix and match metric and imperial systems in certain circumstances, it's less clear that there's a benefit to
   // mixing and matching date formats.)
   if (this->whatAmI == BtLabel::DATE) {
      return;
   }

   QObject* calledBy = sender();
   if (calledBy == nullptr) {
      return;
   }

   QWidget * widgie = qobject_cast<QWidget*>(calledBy);
   if (widgie == nullptr) {
      return;
   }

   this->initializeProperty();
   this->initializeSection();
   this->initializeMenu();

   QAction * invoked = _menu->exec(widgie->mapToGlobal(point));
   if (invoked == nullptr) {
      return;
   }

   Measurement::UnitSystem const * unitSystem = Measurement::getUnitSystemForField(propertyName, _section);
   Measurement::UnitSystem::RelativeScale scale = Measurement::getRelativeScaleForField(propertyName, _section);
   QWidget* pMenu = invoked->parentWidget();
   if ( pMenu == _menu ) {
      PersistentSettings::insert(propertyName, invoked->data(), _section, PersistentSettings::UNIT);
      // reset the scale if required
      if (PersistentSettings::contains(propertyName, _section, PersistentSettings::SCALE) ) {
         PersistentSettings::insert(propertyName, Measurement::UnitSystem::noScale, _section, PersistentSettings::SCALE);
      }
   } else {
      PersistentSettings::insert(propertyName, invoked->data(), _section, PersistentSettings::SCALE);
   }

   // To make this all work, I need to set ogMin and ogMax when og is set.
   if ( propertyName == "og" ) {
      PersistentSettings::insert(PropertyNames::Style::ogMin, invoked->data(),_section, PersistentSettings::UNIT);
      PersistentSettings::insert(PropertyNames::Style::ogMax, invoked->data(),_section, PersistentSettings::UNIT);
   } else if ( propertyName == "fg" ) {
      PersistentSettings::insert(PropertyNames::Style::fgMin, invoked->data(),_section, PersistentSettings::UNIT);
      PersistentSettings::insert(PropertyNames::Style::fgMax, invoked->data(),_section, PersistentSettings::UNIT);
   } else if ( propertyName == "color_srm" ) {
      PersistentSettings::insert(PropertyNames::Style::colorMin_srm, invoked->data(),_section, PersistentSettings::UNIT);
      PersistentSettings::insert(PropertyNames::Style::colorMax_srm, invoked->data(),_section, PersistentSettings::UNIT);
   }

   // Hmm. For the color fields, I want to include the ecb or srm in the label
   // text here.
   if ( whatAmI == COLOR ) {
      Measurement::UnitSystem const * const disp = Measurement::UnitSystem::getInstanceByUniqueName(invoked->data().toString());
      setText( tr("Color (%1)").arg(disp->unit()->name));
   }

   // Remember, we need the original unit, not the new one.

   emit changedUnitSystemOrScale(unitSystem,scale);

   return;
}

BtColorLabel::BtColorLabel(QWidget *parent) :                   BtLabel(parent, COLOR)           { return; }
BtDateLabel::BtDateLabel(QWidget *parent) :                     BtLabel(parent, DATE)            { return; }
BtDensityLabel::BtDensityLabel(QWidget *parent) :               BtLabel(parent, DENSITY)         { return; }
BtMassLabel::BtMassLabel(QWidget *parent) :                     BtLabel(parent, MASS)            { return; }
BtMixedLabel::BtMixedLabel(QWidget *parent) :                   BtLabel(parent, MIXED)           { return; }
BtTemperatureLabel::BtTemperatureLabel(QWidget *parent) :       BtLabel(parent, TEMPERATURE)     { return; }
BtTimeLabel::BtTimeLabel(QWidget *parent) :                     BtLabel(parent, TIME)            { return; }
BtVolumeLabel::BtVolumeLabel(QWidget *parent) :                 BtLabel(parent, VOLUME)          { return; }
BtDiastaticPowerLabel::BtDiastaticPowerLabel(QWidget *parent) : BtLabel(parent, DIASTATIC_POWER) { return; }
