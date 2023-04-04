/*======================================================================================================================
 * widgets/SmartLabel.cpp is part of Brewken, and is copyright the following authors 2009-2023:
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
#include "widgets/SmartLabel.h"

#include <QSettings>
#include <QDebug>
#include <QMouseEvent>

#include "BtAmountEdit.h"
#include "measurement/Measurement.h"
#include "model/Style.h"
#include "model/Recipe.h"
#include "PersistentSettings.h"
#include "utils/OptionalHelpers.h"
#include "widgets/SmartLineEdit.h"
#include "widgets/UnitAndScalePopUpMenu.h"

// This private implementation class holds all private non-virtual members of SmartLabel
class SmartLabel::impl {
public:
   impl(SmartLabel & self,
        QWidget * parent) :
      m_self         {self   },
      m_propertyName {""     },
      m_configSection{""     },
      m_parent       {parent },
      m_contextMenu  {nullptr} {
      return;
   }

   ~impl() = default;

   SmartLabel & m_self;
   QString      m_propertyName;
   QString      m_configSection;
   QWidget *    m_parent;
   QMenu *      m_contextMenu;
};

SmartLabel::SmartLabel(QWidget * parent) :
   QLabel{parent},
   pimpl{std::make_unique<impl>(*this, parent)} {
   connect(this, &QWidget::customContextMenuRequested, this, &SmartLabel::popContextMenu);
   return;
}

SmartLabel::~SmartLabel() = default;

SmartLineEdit & SmartLabel::getBuddy() const {
   // Call QLabel's built-in function to get the buddy
   QWidget * buddy = this->buddy();

   // We assert that it's a coding error for there not to be a buddy!
   Q_ASSERT(buddy);

   return static_cast<SmartLineEdit &>(*buddy);
}

void SmartLabel::enterEvent([[maybe_unused]] QEvent * event) {
   this->textEffect(true);
   return;
}

void SmartLabel::leaveEvent([[maybe_unused]] QEvent * event) {
   this->textEffect(false);
   return;
}

void SmartLabel::mouseReleaseEvent(QMouseEvent * event) {
   // For the moment, we want left-click and right-click to have the same effect, so when we get a left-click event, we
   // send ourselves the right-click signal, which will then fire SmartLabel::popContextMenu().
   emit this->QWidget::customContextMenuRequested(event->pos());
   return;
}

void SmartLabel::textEffect(bool enabled) {
   // If our buddy is an input field for a NonPhysicalQuantity, then we don't want the underline effect as there are no
   // scale choices for the user to make.
   auto const fieldType = this->getBuddy().getFieldType();
   if (std::holds_alternative<NonPhysicalQuantity>(fieldType)) {
      return;
   }

   QFont myFont = this->font();
   myFont.setUnderline(enabled);
   this->setFont(myFont);
   return;
}

void SmartLabel::initializeSection() {
   if (!this->pimpl->m_configSection.isEmpty()) {
      // We're already initialised
      return;
   }

   //
   // If the label has the pimpl->m_configSection defined, use it
   // otherwise, if the paired field has a pimpl->m_configSection, use it
   // otherwise, if the parent object has a pimpl->m_configSection, use it
   // if all else fails, get the parent's object name
   //
   if (this->property(*PropertyNames::UiAmountWithUnits::configSection).isValid()) {
      this->pimpl->m_configSection = this->property(*PropertyNames::UiAmountWithUnits::configSection).toString();
      return;
   }

   // As much as I dislike it, dynamic properties can't be referenced on initialization.
   QWidget const * mybuddy = this->buddy();
   if (mybuddy && mybuddy->property(*PropertyNames::UiAmountWithUnits::configSection).isValid() ) {
      this->pimpl->m_configSection = mybuddy->property(*PropertyNames::UiAmountWithUnits::configSection).toString();
      return;
   }

   if (this->pimpl->m_parent->property(*PropertyNames::UiAmountWithUnits::configSection).isValid() ) {
      this->pimpl->m_configSection =
         this->pimpl->m_parent->property(*PropertyNames::UiAmountWithUnits::configSection).toString();
      return;
   }

   qWarning() << Q_FUNC_INFO << "this failed" << this;
   this->pimpl->m_configSection = this->pimpl->m_parent->objectName();
   return;
}

void SmartLabel::initializeProperty() {

   if (!this->pimpl->m_propertyName.isEmpty()) {
      return;
   }

   QWidget* mybuddy = this->buddy();
   if (this->property("editField").isValid()) {
      this->pimpl->m_propertyName = this->property("editField").toString();
   } else if (mybuddy && mybuddy->property("editField").isValid()) {
      this->pimpl->m_propertyName = mybuddy->property("editField").toString();
   } else {
      qWarning() << Q_FUNC_INFO  << "That failed miserably";
   }
   return;
}

void SmartLabel::initializeMenu() {
   // TODO: Change this to a smart pointer
   //
   // If a context menu already exists, we need to delete it and recreate it.  We can't always reuse an existing menu
   // because the sub-menu for relative scale needs to change when a different unit system is selected.  (In theory we
   // could only recreate the context menu when a different unit system is selected, but that adds complication.)
   if (this->pimpl->m_contextMenu) {
      // NB: Although the existing menu is "owned" by this->pimpl->m_parent, it is fine for us to delete it here.  The Qt
      // ownership in this context merely guarantees that this->pimpl->m_parent will, in its own destructor, delete the menu if
      // it still exists.
      delete this->pimpl->m_contextMenu;
      this->pimpl->m_contextMenu = nullptr;
   }

   std::optional<Measurement::SystemOfMeasurement> forcedSystemOfMeasurement =
      Measurement::getForcedSystemOfMeasurementForField(this->pimpl->m_propertyName, this->pimpl->m_configSection);
   std::optional<Measurement::UnitSystem::RelativeScale> forcedRelativeScale =
      Measurement::getForcedRelativeScaleForField(this->pimpl->m_propertyName, this->pimpl->m_configSection);
   qDebug() <<
      Q_FUNC_INFO << "forcedSystemOfMeasurement=" << forcedSystemOfMeasurement << ", forcedRelativeScale=" <<
      forcedRelativeScale;

   auto const & buddy = this->getBuddy();
   auto fieldType = buddy.getFieldType();
   if (std::holds_alternative<NonPhysicalQuantity>(fieldType)) {
      return;
   }

   // Since fieldType is not NonPhysicalQuantity this cast should be safe
   Measurement::PhysicalQuantity const physicalQuantity = buddy.getUiAmountWithUnits().getPhysicalQuantity();
   this->pimpl->m_contextMenu = UnitAndScalePopUpMenu::create(this->pimpl->m_parent,
                                                              physicalQuantity,
                                                              forcedSystemOfMeasurement,
                                                              forcedRelativeScale);
   return;
}

void SmartLabel::popContextMenu(const QPoint& point) {
   // For the moment, at least, we do not allow people to choose date formats per-field.  (Although you might want to
   // mix and match metric and imperial systems in certain circumstances, it's less clear that there's a benefit to
   // mixing and matching date formats.)
   auto const fieldType = this->getBuddy().getFieldType();
   if (!std::holds_alternative<Measurement::PhysicalQuantity>(fieldType)) {
      return;
   }

   QObject * calledBy = this->sender();
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

   // Show the pop-up menu and get back whatever the user seleted
   QAction * invoked = this->pimpl->m_contextMenu->exec(widgie->mapToGlobal(point));
   if (invoked == nullptr) {
      return;
   }

   // Save the current settings (which may come from system-wide defaults) for the signal below
   Q_ASSERT(std::holds_alternative<Measurement::PhysicalQuantity>(fieldType));
   Measurement::PhysicalQuantity physicalQuantity = std::get<Measurement::PhysicalQuantity>(fieldType);
   PreviousScaleInfo previousScaleInfo{
      Measurement::getSystemOfMeasurementForField(this->pimpl->m_propertyName, this->pimpl->m_configSection, physicalQuantity),
      Measurement::getForcedRelativeScaleForField(this->pimpl->m_propertyName, this->pimpl->m_configSection)
   };

   qDebug() <<
      Q_FUNC_INFO << "Property Name:" << this->pimpl->m_propertyName << ", Config Section" <<
      this->pimpl->m_configSection;

   // To make this all work, we need to set ogMin and ogMax when og is set etc
   QVector<QString> fieldsToSet;
   fieldsToSet.append(this->pimpl->m_propertyName);
   if (this->pimpl->m_propertyName == "og") {
      fieldsToSet.append(QString(*PropertyNames::Style::ogMin));
      fieldsToSet.append(QString(*PropertyNames::Style::ogMax));
   } else if (this->pimpl->m_propertyName == "fg") {
      fieldsToSet.append(QString(*PropertyNames::Style::fgMin));
      fieldsToSet.append(QString(*PropertyNames::Style::fgMax));
   } else if (this->pimpl->m_propertyName == "color_srm") {
      fieldsToSet.append(QString(*PropertyNames::Style::colorMin_srm));
      fieldsToSet.append(QString(*PropertyNames::Style::colorMax_srm));
   }

   // User will either have selected a SystemOfMeasurement or a UnitSystem::RelativeScale.  We can know which based on
   // whether it's the menu or the sub-menu that it came from.
   bool isTopMenu{invoked->parentWidget() == this->pimpl->m_contextMenu};
   if (isTopMenu) {
      // It's the menu, so SystemOfMeasurement
      std::optional<Measurement::SystemOfMeasurement> whatSelected =
         UnitAndScalePopUpMenu::dataFromQAction<Measurement::SystemOfMeasurement>(*invoked);
      qDebug() << Q_FUNC_INFO << "Selected SystemOfMeasurement" << whatSelected;
      if (!whatSelected) {
         // Null means "Default", which means don't set a forced SystemOfMeasurement for this field
         for (auto field : fieldsToSet) {
            Measurement::setForcedSystemOfMeasurementForField(field, this->pimpl->m_configSection, std::nullopt);
         }
      } else {
         for (auto field : fieldsToSet) {
            Measurement::setForcedSystemOfMeasurementForField(field, this->pimpl->m_configSection, *whatSelected);
         }
      }
      // Choosing a forced SystemOfMeasurement resets any selection of forced RelativeScale
      for (auto field : fieldsToSet) {
         Measurement::setForcedRelativeScaleForField(field, this->pimpl->m_configSection, std::nullopt);
      }

      //
      // Hmm. For the color fields, we want to include the ecb or srm in the label text here.
      //
      // Assert that we already bailed above for fields that aren't a PhysicalQuantity, so we know std::get won't throw
      // here.
      //
      auto const fieldType = this->getBuddy().getFieldType();
      Q_ASSERT(std::holds_alternative<Measurement::PhysicalQuantity>(fieldType));
      if (Measurement::PhysicalQuantity::Color == std::get<Measurement::PhysicalQuantity>(fieldType)) {
         Measurement::UnitSystem const & disp =
            Measurement::getUnitSystemForField(this->pimpl->m_propertyName,
                                               this->pimpl->m_configSection,
                                               Measurement::PhysicalQuantity::Color);
         this->setText(tr("Color (%1)").arg(disp.unit()->name));
      }
   } else {
      // It's the sub-menu, so UnitSystem::RelativeScale
      std::optional<Measurement::UnitSystem::RelativeScale> whatSelected =
         UnitAndScalePopUpMenu::dataFromQAction<Measurement::UnitSystem::RelativeScale>(*invoked);
      qDebug() << Q_FUNC_INFO << "Selected RelativeScale" << whatSelected;
      if (!whatSelected) {
         // Null means "Default", which means don't set a forced RelativeScale for this field
         for (auto field : fieldsToSet) {
            Measurement::setForcedRelativeScaleForField(field, this->pimpl->m_configSection, std::nullopt);
         }
      } else {
         for (auto field : fieldsToSet) {
            Measurement::setForcedRelativeScaleForField(field, this->pimpl->m_configSection, *whatSelected);
         }
      }
   }

   // Remember, we need the original unit, not the new one.
   emit changedSystemOfMeasurementOrScale(previousScaleInfo);

   return;
}
