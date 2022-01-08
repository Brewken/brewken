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
#include <QMouseEvent>

#include "measurement/Measurement.h"
#include "model/Style.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "widgets/UnitAndScalePopUpMenu.h"


BtLabel::BtLabel(QWidget *parent,
                 BtFieldType fieldType) :
   QLabel{parent},
   fieldType{fieldType},
   btParent{parent},
   contextMenu{nullptr} {
   connect(this, &QWidget::customContextMenuRequested, this, &BtLabel::popContextMenu);
   return;
}

BtLabel::~BtLabel() = default;

void BtLabel::enterEvent(QEvent* event) {
   this->textEffect(true);
   return;
}

void BtLabel::leaveEvent(QEvent* event) {
   this->textEffect(false);
   return;
}

void BtLabel::mouseReleaseEvent (QMouseEvent * event) {
   // For the moment, we want left-click and right-click to have the same effect, so when we get a left-click event, we
   // send ourselves the right-click signal, which will then fire BtLabel::popContextMenu().
   emit this->QWidget::customContextMenuRequested(event->pos());
   return;
}

void BtLabel::textEffect(bool enabled) {
   QFont myFont = this->font();
   myFont.setUnderline(enabled);
   this->setFont(myFont);
   return;
}

void BtLabel::initializeSection() {
   if (!this->configSection.isEmpty()) {
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
   if (this->property("configSection").isValid()) {
      this->configSection = property("configSection").toString();
   } else if (mybuddy && mybuddy->property("configSection").isValid() ) {
      this->configSection = mybuddy->property("configSection").toString();
   } else if (this->btParent->property("configSection").isValid() ) {
      this->configSection = this->btParent->property("configSection").toString();
   } else {
      qWarning() << Q_FUNC_INFO << "this failed" << this;
      this->configSection = this->btParent->objectName();
   }
   return;
}

void BtLabel::initializeProperty() {

   if (!this->propertyName.isEmpty()) {
      return;
   }

   QWidget* mybuddy = buddy();
   if (this->property("editField").isValid()) {
      this->propertyName = this->property("editField").toString();
   } else if (mybuddy && mybuddy->property("editField").isValid()) {
      this->propertyName = mybuddy->property("editField").toString();
   } else {
      qWarning() << Q_FUNC_INFO  << "That failed miserably";
   }
   return;
}

void BtLabel::initializeMenu() {
   // If a context menu already exists, we need to delete it and recreate it.  We can't always reuse an existing menu
   // because the sub-menu for relative scale needs to change when a different unit system is selected.  (In theory we
   // could only recreate the context menu when a different unit system is selected, but that adds complication.)
   if (this->contextMenu) {
      // NB: Although the existing menu is "owned" by this->btParent, it is fine for us to delete it here.  The Qt
      // ownership in this context merely guarantees that this->btParent will, in its own destructor, delete the menu if
      // it still exists.
      delete this->contextMenu;
      this->contextMenu = nullptr;
   }

   Measurement::UnitSystem const * forcedUnitSystem =
      Measurement::getUnitSystemForField(this->propertyName, this->configSection);
   Measurement::UnitSystem::RelativeScale forcedRelativeScale =
      Measurement::getRelativeScaleForField(this->propertyName, this->configSection);
   qDebug() <<
      Q_FUNC_INFO << "forcedUnitSystem=" << (nullptr == forcedUnitSystem ? "NULL" : forcedUnitSystem->uniqueName) <<
      ", forcedRelativeScale=" << forcedRelativeScale;

   if (!std::holds_alternative<Measurement::PhysicalQuantity>(this->fieldType) ||
       Measurement::PhysicalQuantity::None == std::get<Measurement::PhysicalQuantity>(this->fieldType)) {
      return;
   }

   Measurement::PhysicalQuantity physicalQuantity = std::get<Measurement::PhysicalQuantity>(this->fieldType);

   if (Measurement::PhysicalQuantity::Mixed == physicalQuantity) {

      //
      // Mixed needs some special handling because it is a "fake" physical quantity that means the user has the choice
      // to measure by mass or by volume on a per-item basis.  This is useful because, eg some Misc ingredients are best
      // measured by volume and others by mass.  Similarly, dry yeast is probably measured by mass whereas wet yeast is
      // usually measured by volume.
      //
      // For real physical quantities, there is a one-to-one correspondence between UnitSystem and the pair
      // (SystemOfMeasurement, PhysicalQuantity).  So, for any particular field holding a given PhysicalQuantity,
      // offering the user a choice of SystemOfMeasurement implies the corresponding choice of UnitSystem, and a choice
      // of RelativeScale corresponds to this UnitSystem.  So we just store the chosen UnitSystem and/or RelativeScale.
      //
      // For Mixed, where PhysicalQuantity varies per-item between two possibilities (Mass and Volume), the choice of
      // SystemOfMeasurement is going to imply a different UnitSystem per-item depending on, eg Misc::amountIsWeight,
      // Yeast::amountIsWeight, etc for that item.  Firstly, it doesn't make sense to offer the user a choice of
      // RelativeScale here, so we suppress that option.
      //
      //

      // .:TBD:. CHECK THIS STILL WORKS WITH NEW SYSTEM
      // This looks weird, but it works.
      this->contextMenu = UnitAndScalePopUpMenu::create(this->btParent, physicalQuantity, forcedUnitSystem, forcedRelativeScale/*, false*/); // no scale menu
   } else {
      this->contextMenu = UnitAndScalePopUpMenu::create(this->btParent,
                                                        physicalQuantity,
                                                        forcedUnitSystem,
                                                        forcedRelativeScale);
   }
   return;
}

void BtLabel::popContextMenu(const QPoint& point) {
   // For the moment, at least, we do not allow people to choose date formats per-field.  (Although you might want to
   // mix and match metric and imperial systems in certain circumstances, it's less clear that there's a benefit to
   // mixing and matching date formats.)
   if (!std::holds_alternative<Measurement::PhysicalQuantity>(this->fieldType)) {
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

   QAction * invoked = this->contextMenu->exec(widgie->mapToGlobal(point));
   if (invoked == nullptr) {
      return;
   }

   Measurement::UnitSystem const * unitSystem = Measurement::getUnitSystemForField(this->propertyName,
                                                                                   this->configSection);
   Measurement::UnitSystem::RelativeScale scale = Measurement::getRelativeScaleForField(this->propertyName,
                                                                                        this->configSection);
   QWidget* pMenu = invoked->parentWidget();
   if (pMenu == this->contextMenu) {
      PersistentSettings::insert(this->propertyName, invoked->data(), this->configSection, PersistentSettings::UNIT);
      // reset the scale if required
      if (PersistentSettings::contains(this->propertyName, this->configSection, PersistentSettings::SCALE) ) {
         PersistentSettings::insert(this->propertyName,
                                    Measurement::UnitSystem::noScale,
                                    this->configSection,
                                    PersistentSettings::SCALE);
      }
   } else {
      PersistentSettings::insert(this->propertyName, invoked->data(), this->configSection, PersistentSettings::SCALE);
   }

   // To make this all work, I need to set ogMin and ogMax when og is set.
   if (this->propertyName == "og") {
      PersistentSettings::insert(PropertyNames::Style::ogMin, invoked->data(), this->configSection, PersistentSettings::UNIT);
      PersistentSettings::insert(PropertyNames::Style::ogMax, invoked->data(), this->configSection, PersistentSettings::UNIT);
   } else if (this->propertyName == "fg") {
      PersistentSettings::insert(PropertyNames::Style::fgMin, invoked->data(), this->configSection, PersistentSettings::UNIT);
      PersistentSettings::insert(PropertyNames::Style::fgMax, invoked->data(), this->configSection, PersistentSettings::UNIT);
   } else if (this->propertyName == "color_srm") {
      PersistentSettings::insert(PropertyNames::Style::colorMin_srm, invoked->data(), this->configSection, PersistentSettings::UNIT);
      PersistentSettings::insert(PropertyNames::Style::colorMax_srm, invoked->data(), this->configSection, PersistentSettings::UNIT);
   }

   //
   // Hmm. For the color fields, we want to include the ecb or srm in the label text here.
   //
   // Assert that we already bailed above for fields that aren't a PhysicalQuantity, so we know std::get won't throw
   // here.
   //
   Q_ASSERT(std::holds_alternative<Measurement::PhysicalQuantity>(this->fieldType));
   if (Measurement::PhysicalQuantity::Color == std::get<Measurement::PhysicalQuantity>(this->fieldType)) {
      Measurement::UnitSystem const * const disp = Measurement::UnitSystem::getInstanceByUniqueName(invoked->data().toString());
      setText( tr("Color (%1)").arg(disp->unit()->name));
   }

   // Remember, we need the original unit, not the new one.
   emit changedUnitSystemOrScale(unitSystem, scale);

   return;
}

BtColorLabel::BtColorLabel(QWidget *parent) :                   BtLabel(parent, Measurement::PhysicalQuantity::Color)          { return; }
BtDateLabel::BtDateLabel(QWidget *parent) :                     BtLabel(parent, NonPhysicalQuantity::Date)                     { return; }
BtDensityLabel::BtDensityLabel(QWidget *parent) :               BtLabel(parent, Measurement::PhysicalQuantity::Density)        { return; }
BtMassLabel::BtMassLabel(QWidget *parent) :                     BtLabel(parent, Measurement::PhysicalQuantity::Mass)           { return; }
BtMixedLabel::BtMixedLabel(QWidget *parent) :                   BtLabel(parent, Measurement::PhysicalQuantity::Mixed)          { return; }
BtTemperatureLabel::BtTemperatureLabel(QWidget *parent) :       BtLabel(parent, Measurement::PhysicalQuantity::Temperature)    { return; }
BtTimeLabel::BtTimeLabel(QWidget *parent) :                     BtLabel(parent, Measurement::PhysicalQuantity::Time)           { return; }
BtVolumeLabel::BtVolumeLabel(QWidget *parent) :                 BtLabel(parent, Measurement::PhysicalQuantity::Volume)         { return; }
BtDiastaticPowerLabel::BtDiastaticPowerLabel(QWidget *parent) : BtLabel(parent, Measurement::PhysicalQuantity::DiastaticPower) { return; }
