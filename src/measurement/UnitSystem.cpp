/*======================================================================================================================
 * UnitSystem.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Théophane Martin <theophane.m@gmail.com>
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
#include "measurement/UnitSystem.h"

#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QMenu>
#include <QRegExp>

#include "Brewken.h"
#include "Localization.h"
#include "measurement/Unit.h"

namespace {
   int const fieldWidth = 0;
   char const format = 'f';
   int const defaultPrecision = 3;

   // All functions in QRegExp are reentrant, so it should be safe to use as a shared const in multi-threaded code.
   QRegExp const amtUnit {
      // Make sure we get the right decimal point (. or ,) and the right grouping separator (, or .).  Some locales
      // write 1.000,10 and others write 1,000.10.  We need to catch both.
      "((?:\\d+" + QRegExp::escape(QLocale::system().groupSeparator()) + ")?\\d+(?:" +
      QRegExp::escape(QLocale::system().decimalPoint()) + "\\d+)?|" +
      QRegExp::escape(QLocale::system().decimalPoint()) + "\\d+)\\s*(\\w+)?",
      Qt::CaseInsensitive
   };

   QMultiMap<Measurement::PhysicalQuantity, Measurement::UnitSystem const *> physicalQuantityToUnitSystems;

   // Used by UnitSystem::getInstanceByName()
   QMap<QString, Measurement::UnitSystem const *> nameToUnitSystem; /*{
      {Measurement::mass_ImperialAndUsCustomary.name,         &Measurement::mass_ImperialAndUsCustomary                   },
      {Measurement::mass_Metric.name,                         &Measurement::mass_Metric                        },

      {Measurement::volume_Imperial.name,                     &Measurement::volume_Imperial                    },
      {Measurement::volume_UsCustomary.name,                  &Measurement::volume_UsCustomary                 },
      {Measurement::volume_Metric.name,                       &Measurement::volume_Metric                      },

      {Measurement::temperature_MetricIsCelsius.name,         &Measurement::temperature_MetricIsCelsius        },
      {Measurement::temperature_UsCustomaryIsFahrenheit.name, &Measurement::temperature_UsCustomaryIsFahrenheit},

      {Measurement::time_CoordinatedUniversalTime.name,       &Measurement::time_CoordinatedUniversalTime      },

      {Measurement::color_StandardReferenceMethod.name,       &Measurement::color_StandardReferenceMethod      },
      {Measurement::color_EuropeanBreweryConvention.name,     &Measurement::color_EuropeanBreweryConvention    },

      {Measurement::density_SpecificGravity.name,             &Measurement::density_SpecificGravity            },
      {Measurement::density_Plato.name,                       &Measurement::density_Plato                      },

      {Measurement::diastaticPower_Lintner.name,              &Measurement::diastaticPower_Lintner             },
      {Measurement::diastaticPower_WindischKolbach.name,      &Measurement::diastaticPower_WindischKolbach     }
   };*/

   /**
    * \brief Used by UnitSystems::createUnitSystemMenu()
    *
    * \param menu
    * \param text
    * \param data
    * \param currentVal
    * \param actionGroup
    */
   void generateAction(QMenu * menu, QString text, QVariant data, QVariant currentVal, QActionGroup* actionGroup) {
      QAction* action = new QAction(menu);

      action->setText(text);
      action->setData(data);
      action->setCheckable(true);
      action->setChecked(currentVal == data);;
      if (actionGroup) {
         actionGroup->addAction(action);
      }

      menu->addAction(action);
      return;
   }

}

// This private implementation class holds all private non-virtual members of UnitSystem
class Measurement::UnitSystem::impl {
public:
/*   struct RelativeScaleOfUnit {
      Measurement::UnitSystem::RelativeScale const relativeScale;
      Measurement::Unit const & unit;
   };*/

   /**
    * Constructor
    */
   impl(Measurement::UnitSystem & self,
        Measurement::PhysicalQuantity const physicalQuantity,
        Measurement::Unit const * const thickness,
        Measurement::Unit const * const defaultUnit,
        std::initializer_list<std::pair<Measurement::UnitSystem::RelativeScale const,
                                        Measurement::Unit const *> > scaleToUnit) :
      self            {self},
      physicalQuantity{physicalQuantity},
      thickness       {thickness},
      defaultUnit     {defaultUnit},
      scaleToUnit     {scaleToUnit} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   /**
    * \brief Maps from a Measurement::UnitSystem::RelativeScale to a concrete Unit - eg in the US weight UnitSystem,
    *        Measurement::Unit::scaleExtraSmall maps to Measurement::Units::ounces and Measurement::Unit::scaleSmall
    *        maps to Measurement::Units::pounds
    */
   Unit const * getUnitForRelativeScale(Measurement::UnitSystem::RelativeScale const relativeScale) const {
      return this->scaleToUnit.value(relativeScale, nullptr);
   }

   /**
    * \brief Maps from unit name (in this \c UnitSystem) to \c Unit
    */
   Unit const * getUnitFromName(QString const & name) const {
      auto const unitsForThisSystem = this->scaleToUnit.values();
      auto matchingUnit = std::find_if(
         unitsForThisSystem.begin(),
         unitsForThisSystem.end(),
         [name](Measurement::Unit const * unit) {return unit->name == name;}
      );
      if (matchingUnit != unitsForThisSystem.end()) {
         return *matchingUnit;
      }
      return nullptr;
   }

   /**
    * \brief This does most of the work for displayAmount() and amountDisplay()
    *
    * \param amount
    * \param units
    * \param scale
    *
    * \return
    */
   std::pair<double, QString> displayableAmount(double amount,
                                                Measurement::Unit const * units,
                                                Measurement::UnitSystem::RelativeScale scale) const {
      // Special cases
      if (units == nullptr || units->getPhysicalQuantity() != this->physicalQuantity) {
         return std::pair(amount, "");
      }

      // Short circuit if the 'without' key is defined
      if (this->scaleToUnit.contains(Measurement::UnitSystem::RelativeScale::scaleWithout)) {
         scale = Measurement::UnitSystem::RelativeScale::scaleWithout;
      }

      double SIAmount = units->toSI(amount);

      // If a specific scale is provided, just use that and don't loop.
      if (this->scaleToUnit.contains(scale) ) {
         Measurement::Unit const * bb = this->scaleToUnit.value(scale);
         return std::pair(bb->fromSI(SIAmount), bb->name);
      }

      // Search for the smallest measure in this system that's not too big to show the supplied value
      // QMap guarantees that we iterate in the order of its keys, thus here we'll loop from smallest to largest scale
      // (e.g., mg, g, kg).
      Measurement::Unit const * last  = nullptr;
      for (auto it : this->scaleToUnit) {
         if (last != nullptr && qAbs(SIAmount) < it->toSI(it->boundary())) {
            // Stop looping as we've found a unit that's too big to use (so we'll return the last one, ie the one smaller,
            // below)
            break;
         }
         last = it;
      }

      // It is a programming error if the map was empty (ie we didn't go through the loop at all)
      Q_ASSERT(last != nullptr);
      return std::pair(last->fromSI(SIAmount), last->name);
   }


   // Member variables for impl
   Measurement::UnitSystem & self;
   Measurement::PhysicalQuantity const physicalQuantity;
   Measurement::Unit const * const thickness;
   Measurement::Unit const * const defaultUnit;

   QMap<Measurement::UnitSystem::RelativeScale, Measurement::Unit const *> const scaleToUnit;
};

Measurement::UnitSystem::UnitSystem(Measurement::PhysicalQuantity const physicalQuantity,
                                    Measurement::Unit const * const thickness,
                                    Measurement::Unit const * const defaultUnit,
                                    std::initializer_list<std::pair<Measurement::UnitSystem::RelativeScale const,
                                                                    Measurement::Unit const *> > scaleToUnit,
                                    char const * const uniqueName,
                                    char const * const systemOfMeasurementName) :
   uniqueName{uniqueName},
   systemOfMeasurementName{systemOfMeasurementName},
   pimpl{std::make_unique<impl>(*this,
                                physicalQuantity,
                                thickness,
                                defaultUnit,
                                scaleToUnit)} {
   // We assert that no other UnitSystem has the same name as this one
   Q_ASSERT(!nameToUnitSystem.contains(uniqueName));
   nameToUnitSystem.insert(uniqueName, this);
   // Conversely, it is more often than not the case that there will be more than one UnitSystem per PhysicalQuantity
   physicalQuantityToUnitSystems.insert(physicalQuantity, this);
   return;
}

Measurement::UnitSystem::~UnitSystem() = default;

bool Measurement::UnitSystem::operator==(UnitSystem const & other) const {
   // Since we're not intending to create multiple instances of any given UnitSystem, it should be enough to check
   // the addresses are equal, but, as belt-and-braces, we'll check the names are equal as a fall-back.
   return (this == &other || this->uniqueName == other.uniqueName);
}

double Measurement::UnitSystem::qstringToSI(QString qstr,
                                            Unit const * defUnit,
                                            Measurement::UnitSystem::RelativeScale scale) const {

   // make sure we can parse the string
   if (amtUnit.indexIn(qstr) == -1) {
      return 0.0;
   }

   double amt = Localization::toDouble(amtUnit.cap(1), Q_FUNC_INFO);

   QString unitName = amtUnit.cap(2);

   // Look first in this unit system. If you can't find it here, find it
   // globally. I *think* this finally has all the weird magic right. If the
   // field is marked as "Imperial" and you enter "3 qt" you get 3 imperial
   // qts, 3.6 US qts, 3.41L. If you enter 3L, you get 2.64 imperial qts,
   // 3.17 US qt. If you mean 3 US qt, you are SOL unless you mark the field
   // as US Customary.

   // .:TODO:. This is exactly the sort of thing we should have a bunch of tests for!

   Unit const * unitToUse = nullptr;
   if (!unitName.isEmpty()) {
      // The supplied string specifies units, so see if they are ones we recognise in this unit system
      unitToUse = this->pimpl->getUnitFromName(unitName);
      // If we didn't find the specified units in this UnitSystem, broaden the search and look in all units
      if (!unitToUse) {
         unitToUse = Measurement::Unit::getUnit(unitName, this->pimpl->physicalQuantity);
      }
   } else if (scale != Measurement::UnitSystem::noScale) {
      // The supplied string does not specify units, so, if a scale is set, use that
      unitToUse = this->pimpl->getUnitForRelativeScale(scale);
   }

   if (!unitToUse) {
      unitToUse = defUnit;
   }

   // It is possible for unitToUse to be NULL at this point, so make sure we handle that case
   if (!unitToUse) {
      return -1.0;
   }

   return unitToUse->toSI(amt);
}

QString Measurement::UnitSystem::displayAmount(double amount,
                                               Unit const * units,
                                               int precision,
                                               Measurement::UnitSystem::RelativeScale scale) const {
   // If the precision is not specified, we take the default one
   if (precision < 0) {
      precision = defaultPrecision;
   }

   auto result = this->pimpl->displayableAmount(amount, units, scale);

   if (result.second.isEmpty()) {
      return QString("%L1").arg(this->amountDisplay(result.first, units, scale), fieldWidth, format, precision);
   }

   return QString("%L1 %2").arg(result.first, fieldWidth, format, precision).arg(result.second);
}

double Measurement::UnitSystem::amountDisplay(double amount,
                                              Unit const * units,
                                              Measurement::UnitSystem::RelativeScale scale) const {
   // Essentially we're just returning the numeric part of the displayable amount
   return this->pimpl->displayableAmount(amount, units, scale).first;
}

Measurement::Unit const * Measurement::UnitSystem::scaleUnit(Measurement::UnitSystem::RelativeScale scale) const {
   return this->pimpl->getUnitForRelativeScale(scale);
}

Measurement::Unit const * Measurement::UnitSystem::thicknessUnit() const {
   return this->pimpl->thickness;
}

Measurement::Unit const * Measurement::UnitSystem::unit() const {
   return this->pimpl->defaultUnit;
}

/*QString const & Measurement::UnitSystem::unitType() const {
   // .:TBD:. Think we don't need this!
   return this->name;
}*/

Measurement::PhysicalQuantity Measurement::UnitSystem::getPhysicalQuantity() const {
   return this->pimpl->physicalQuantity;
}

Measurement::UnitSystem const * Measurement::UnitSystem::getInstanceByUniqueName(QString const uniqueName) {
   return nameToUnitSystem.value(uniqueName, nullptr);
}

QList<Measurement::UnitSystem const *> Measurement::UnitSystem::getUnitSystems(Measurement::PhysicalQuantity physicalQuantity) {
   return physicalQuantityToUnitSystems.values(physicalQuantity);
}


QMenu * Measurement::UnitSystem::createUnitSystemMenu(QWidget * parent,
                                                      Measurement::UnitSystem::RelativeScale const relativeScale,
                                                      bool const generateScale) const {
   QMenu * menu = new QMenu(parent);
   QActionGroup * actionGroup = new QActionGroup(parent);

   // We assert that at least one UnitSystem exists for any given PhysicalQuantity
   Q_ASSERT(physicalQuantityToUnitSystems.contains(this->pimpl->physicalQuantity));

   // If there are other UnitSystems for this one's PhysicalQuantity then we want the user to be able to select
   // between them
   auto unitSystems = physicalQuantityToUnitSystems.values(this->pimpl->physicalQuantity);
   if (unitSystems.size() > 1) {
      generateAction(menu, QApplication::translate("UnitSystem", "Default"), QVariant(), this->uniqueName, actionGroup);
      for (auto system : unitSystems) {
         generateAction(menu, system->systemOfMeasurementName, system->uniqueName, this->uniqueName, actionGroup);
      }
   }

   // If this UnitSystem has more than one Unit, allow the user to select a Unit for the scale
   if (this->pimpl->scaleToUnit.size() > 1) {
      QMenu * subMenu = new QMenu(menu);
      generateAction(subMenu,
                     QApplication::translate("UnitSystem", "Default"),
                     Measurement::UnitSystem::noScale,
                     relativeScale,
                     actionGroup);
      for (auto unitInfo = this->pimpl->scaleToUnit.begin(); unitInfo != this->pimpl->scaleToUnit.end(); ++unitInfo) {
         generateAction(subMenu, unitInfo.value()->name, unitInfo.key(), this->uniqueName, actionGroup);
      }
      subMenu->setTitle(QApplication::translate("UnitSystem", "Scale"));
      menu->addMenu(subMenu);
   }

   return menu;
}


//---------------------------------------------------------------------------------------------------------------------
//
// This is where we actually define all the different unit systems
//
//---------------------------------------------------------------------------------------------------------------------
namespace Measurement::UnitSystems {
   UnitSystem const mass_ImperialAndUsCustomary = UnitSystem(PhysicalQuantity::Mass,
                                                             &Measurement::Units::pounds,
                                                             &Measurement::Units::pounds,
                                                             {{UnitSystem::scaleExtraSmall, &Measurement::Units::ounces},
                                                                {UnitSystem::scaleSmall,      &Measurement::Units::pounds}},
                                                             "mass_ImperialAndUsCustomary",
                                                             QT_TR_NOOP("US Customary / Imperial"));

   UnitSystem const mass_Metric = UnitSystem(PhysicalQuantity::Mass,
                                             &Measurement::Units::kilograms,
                                             &Measurement::Units::kilograms,
                                             {{UnitSystem::scaleExtraSmall, &Measurement::Units::milligrams},
                                              {UnitSystem::scaleSmall,      &Measurement::Units::grams     },
                                              {UnitSystem::scaleMedium,     &Measurement::Units::kilograms }},
                                             "mass_Metric",
                                             QT_TR_NOOP("Metric (SI)"));

   UnitSystem const volume_Imperial = UnitSystem(PhysicalQuantity::Volume,
                                                 &Measurement::Units::imperial_quarts,
                                                 &Measurement::Units::imperial_gallons,
                                                 {{UnitSystem::scaleExtraSmall, &Measurement::Units::imperial_teaspoons  },
                                                  {UnitSystem::scaleSmall,      &Measurement::Units::imperial_tablespoons},
                                                  {UnitSystem::scaleMedium,     &Measurement::Units::imperial_cups       },
                                                  {UnitSystem::scaleLarge,      &Measurement::Units::imperial_quarts     },
                                                  {UnitSystem::scaleExtraLarge, &Measurement::Units::imperial_gallons    },
                                                  {UnitSystem::scaleHuge,       &Measurement::Units::imperial_barrels    }},
                                                 "volume_Imperial",
                                                 QT_TR_NOOP("Imperial"));

   UnitSystem const volume_UsCustomary = UnitSystem(PhysicalQuantity::Volume,
                                                    &Measurement::Units::us_quarts,
                                                    &Measurement::Units::us_gallons,
                                                    {{UnitSystem::scaleExtraSmall, &Measurement::Units::us_teaspoons  },
                                                     {UnitSystem::scaleSmall,      &Measurement::Units::us_tablespoons},
                                                     {UnitSystem::scaleMedium,     &Measurement::Units::us_cups       },
                                                     {UnitSystem::scaleLarge,      &Measurement::Units::us_quarts     },
                                                     {UnitSystem::scaleExtraLarge, &Measurement::Units::us_gallons    },
                                                     {UnitSystem::scaleHuge,       &Measurement::Units::us_barrels    }},
                                                    "volume_UsCustomary",
                                                    QT_TR_NOOP("US Customary"));

   UnitSystem const volume_Metric = UnitSystem(PhysicalQuantity::Volume,
                                               &Measurement::Units::liters,
                                               &Measurement::Units::liters,
                                               {{UnitSystem::scaleExtraSmall, &Measurement::Units::milliliters},
                                                {UnitSystem::scaleSmall,      &Measurement::Units::liters     }},
                                               "volume_Metric",
                                               QT_TR_NOOP("Metric (SI)"));

   UnitSystem const temperature_MetricIsCelsius = UnitSystem(PhysicalQuantity::Temperature,
                                                             nullptr,
                                                             &Measurement::Units::celsius,
                                                             {{UnitSystem::scaleWithout, &Measurement::Units::celsius}},
                                                             "temperature_MetricIsCelsius",
                                                             QT_TR_NOOP("Celsius"));

   UnitSystem const temperature_UsCustomaryIsFahrenheit = UnitSystem(PhysicalQuantity::Temperature,
                                                                     nullptr,
                                                                     &Measurement::Units::fahrenheit,
                                                                     {{UnitSystem::scaleWithout, &Measurement::Units::fahrenheit}},
                                                                     "temperature_UsCustomaryIsFahrenheit",
                                                                     QT_TR_NOOP("Fahrenheit"));

   UnitSystem const time_CoordinatedUniversalTime = UnitSystem(PhysicalQuantity::Time,
                                                               nullptr,
                                                               &Measurement::Units::minutes,
                                                               {{UnitSystem::scaleExtraSmall, &Measurement::Units::seconds},
                                                                {UnitSystem::scaleSmall,      &Measurement::Units::minutes},
                                                                {UnitSystem::scaleMedium,     &Measurement::Units::hours  },
                                                                {UnitSystem::scaleLarge,      &Measurement::Units::days   }},
                                                               "time_CoordinatedUniversalTime",
                                                               QT_TR_NOOP("Coordinated Universal Time"));

   UnitSystem const color_EuropeanBreweryConvention = UnitSystem(PhysicalQuantity::Color,
                                                                 nullptr,
                                                                 &Measurement::Units::ebc,
                                                                 {{UnitSystem::scaleWithout, &Measurement::Units::ebc}},
                                                                 "color_EuropeanBreweryConvention",
                                                                 QT_TR_NOOP("EBC (European Brewery Convention)"));

   UnitSystem const color_StandardReferenceMethod = UnitSystem(PhysicalQuantity::Color,
                                                               nullptr,
                                                               &Measurement::Units::srm,
                                                               {{UnitSystem::scaleWithout, &Measurement::Units::srm}},
                                                               "color_StandardReferenceMethod",
                                                               QT_TR_NOOP("SRM (Standard Reference Method)"));

   UnitSystem const density_SpecificGravity = UnitSystem(PhysicalQuantity::Density,
                                                         nullptr,
                                                         &Measurement::Units::sp_grav,
                                                         {{UnitSystem::scaleWithout, &Measurement::Units::sp_grav}},
                                                         "density_SpecificGravity",
                                                         QT_TR_NOOP("Specific Gravity"));

   UnitSystem const density_Plato = UnitSystem(PhysicalQuantity::Density,
                                               nullptr,
                                               &Measurement::Units::plato,
                                               {{UnitSystem::scaleWithout, &Measurement::Units::plato}},
                                               "density_Plato",
                                               QT_TR_NOOP("Plato"));

   UnitSystem const diastaticPower_Lintner = UnitSystem(PhysicalQuantity::DiastaticPower,
                                                        nullptr,
                                                        &Measurement::Units::lintner,
                                                        {{UnitSystem::scaleWithout, &Measurement::Units::lintner}},
                                                        "diastaticPower_Lintner",
                                                        QT_TR_NOOP("Lintner"));

   UnitSystem const diastaticPower_WindischKolbach = UnitSystem(PhysicalQuantity::DiastaticPower,
                                                                nullptr,
                                                                &Measurement::Units::wk,
                                                                {{UnitSystem::scaleWithout, &Measurement::Units::wk}},
                                                                "diastaticPower_WindischKolbach",
                                                                QT_TR_NOOP("WK (Windisch Kolbach)"));

}
