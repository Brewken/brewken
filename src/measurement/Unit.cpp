/*======================================================================================================================
 * measurement/Unit.cpp is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Rob Taylor <robtaylor@floopily.org>
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
#include "measurement/Unit.h"

#include <iostream>
#include <mutex>    // For std::once_flag etc
#include <string>

#include <QStringList>
#include <QRegExp>
#include <QDebug>

#include "Algorithms.h"
#include "Localization.h"
#include "measurement/Measurement.h"
#include "measurement/UnitSystem.h"

namespace {

   QRegExp getRegExpToMatchAmountPlusUnits() {
      QRegExp amtUnit;

      // Make sure we get the right decimal point (. or ,) and the right grouping
      // separator (, or .). Some locales write 1.000,10 and other write
      // 1,000.10. We need to catch both
      QString const decimal =  QRegExp::escape(Localization::getLocale().decimalPoint());
      QString const grouping = QRegExp::escape(Localization::getLocale().groupSeparator());

      amtUnit.setPattern("((?:\\d+" + grouping + ")?\\d+(?:" + decimal + "\\d+)?|" + decimal + "\\d+)\\s*(\\w+)?");
      amtUnit.setCaseSensitivity(Qt::CaseInsensitive);

      return amtUnit;
   }

   QString unitNameFromAmountString(QString const & qstr) {
      QRegExp amtUnit = getRegExpToMatchAmountPlusUnits();

      // if the regex dies, return ?
      if (amtUnit.indexIn(qstr) == -1) {
         return QString("?");
      }

      // Get the unit from the second capture
      return amtUnit.cap(2);
   }

   double quantityFromAmountString(QString const & qstr) {
      QRegExp amtUnit = getRegExpToMatchAmountPlusUnits();

      // if the regex dies, return 0.0
      if (amtUnit.indexIn(qstr) == -1) {
         return 0.0;
      }

      return Localization::toDouble(amtUnit.cap(1), Q_FUNC_INFO);
   }

   /**
    * \brief This is useful to allow us to initialise \c unitNameLookup and \c physicalQuantityToCanonicalUnit after
    *        all \c Unit and \c UnitSystem objects have been created.
    */
   QVector<Measurement::Unit const *> listOfAllUnits;

   /**
    * \brief This allows us to ensure that \c Unit::initialiseLookups is called exactly once
    */
   std::once_flag initFlag_Lookups;

   //
   // Note that, although Unit names (ie abbreviations) are unique within an individual UnitSystem, some are are not
   // globally unique, and some are not even unique within a PhysicalQuantity.   For example:
   //    - "L" is the abbreviation/name of both Liters and Lintner
   //    - "gal" is the abbreviation/name of the Imperial gallon and the US Customary one
   //
   // Almost all of the time when we are doing look-ups, we know the PhysicalQuantity (and it is not meaningful for the
   // user to specify units relating to a different PhysicalQuantity) so it makes sense to group look-ups by that.
   //
   struct NameLookupKey {
      Measurement::PhysicalQuantity physicalQuantity;
      QString                       lowerCaseUnitName;
      //
      // We need an ordering on the struct to use it as a key of QMap / QMultiMap.  In theory, in C++20, we should be
      // able to ask the compiler to do all the work to give a suitable default ordering, by writing:
      //
      //    friend auto operator<=>(NameLookupKey const & lhs, NameLookupKey const & rhs) = default;
      //
      // However, in practice, this requires the spaceship operator to be defined for QString, which it isn't in Qt5.
      // So, for now, define our own operator<, which will suffice for QMultiMap.
      //
      bool operator<(NameLookupKey const & rhs) const {
         NameLookupKey const & lhs = *this;
         if (lhs.physicalQuantity != rhs.physicalQuantity) {
            return (lhs.physicalQuantity < rhs.physicalQuantity);
         }
         return (lhs.lowerCaseUnitName < rhs.lowerCaseUnitName);
      }
   };
   QMultiMap<NameLookupKey, Measurement::Unit const *> unitNameLookup;

   QMap<Measurement::PhysicalQuantity, Measurement::Unit const *> physicalQuantityToCanonicalUnit;

   /**
    * \brief Get all units matching a given name and physical quantity
    *
    * \param name
    * \param physicalQuantity
    * \param caseInensitiveMatching If \c true, do a case-insensitive search.  Eg, match "ml" for milliliters, even
    *                               though the correct name is "mL".  This should always be safe to do, as AFAICT there
    *                               are no current or foreseeable units that _we_ use whose names only differ by case --
    *                               or, at least, that's the case in English...
    */
   QList<Measurement::Unit const *> getUnitsByNameAndPhysicalQuantity(QString const & name,
                                                                      Measurement::PhysicalQuantity const & physicalQuantity,
                                                                      bool const caseInensitiveMatching) {
      // Need this before we reference unitNameLookup or physicalQuantityToCanonicalUnit
      std::call_once(initFlag_Lookups, &Measurement::Unit::initialiseLookups);

      auto matches = unitNameLookup.values(NameLookupKey{physicalQuantity, name.toLower()});
      qDebug() << Q_FUNC_INFO << name << "has" << matches.length() << "case-insensitive match(es)";
      if (caseInensitiveMatching) {
         return matches;
      }

      // If we ever want to do case insensitive matching (which we think should be rare), the simplest thing is just to
      // go through all the case-insensitive matches and exclude those that aren't an exact match
      decltype(matches) filteredMatches;
      for (auto match : matches) {
         if (match->name == name) {
            filteredMatches.append(match);
         }
      }
      qDebug() << Q_FUNC_INFO << name << "has" << filteredMatches.length() << "case-sensitive match(es)";
      return filteredMatches;
   }

   /**
    * \brief Get all units matching a given name, but without knowing the physical quantity.  Pretty much the only time
    *        we need this is in \c ConverterTool to do contextless conversions.
    *
    * \param name
    * \param caseInensitiveMatching If \c true, do a case-insensitive search.  Eg, match "ml" for milliliters, even
    *                               though the correct name is "mL".
    */
   QList<Measurement::Unit const *> getUnitsOnlyByName(QString const & name,
                                                       bool const caseInensitiveMatching = true) {
      // Need this before we reference unitNameLookup or physicalQuantityToCanonicalUnit
      std::call_once(initFlag_Lookups, &Measurement::Unit::initialiseLookups);

      QList<Measurement::Unit const *> allMatches;
      for (auto key : unitNameLookup.uniqueKeys()) {
         //
         // Although it's slightly clunky to call getUnitsByNameAndPhysicalQuantity() here, it saves us re-implementing
         // all the case-insensitive logic.
         //
         auto matches = getUnitsByNameAndPhysicalQuantity(name, key.physicalQuantity, caseInensitiveMatching);
         if (matches.length() > 0) {
            allMatches.append(matches);
         }
      }
      return allMatches;
   }
}

// This private implementation class holds all private non-virtual members of Unit
class Measurement::Unit::impl {
public:
   /**
    * Constructor
    */
   impl(Unit & self,
        UnitSystem const & unitSystem,
        std::function<double(double)> convertToCanonical,
        std::function<double(double)> convertFromCanonical,
        double const boundaryValue,
        bool const isCanonical) :
      self                {self},
      unitSystem          {unitSystem},
      convertToCanonical  {convertToCanonical},
      convertFromCanonical{convertFromCanonical},
      boundaryValue       {boundaryValue},
      isCanonical         {isCanonical} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   // Member variables for impl
   Unit & self;
   UnitSystem const & unitSystem;

   std::function<double(double)> convertToCanonical;
   std::function<double(double)> convertFromCanonical;
   double const boundaryValue;
   bool const isCanonical;
};

Measurement::Unit::Unit(UnitSystem const & unitSystem,
                        QString const unitName,
                        std::function<double(double)> convertToCanonical,
                        std::function<double(double)> convertFromCanonical,
                        double boundaryValue,
                        Measurement::Unit const * canonical) :
   name{unitName},
   pimpl{std::make_unique<impl>(*this,
                                unitSystem,
                                convertToCanonical,
                                convertFromCanonical,
                                boundaryValue,
                                (canonical == nullptr) )} {
   //
   // You might think here would be a neat place to the Unit we are constructing to unitNameLookup and, if appropriate,
   // physicalQuantityToCanonicalUnit.  However, there is not guarantee that unitSystem is constructed at this point, so
   // unitSystem.getPhysicalQuantity() could result in a core dump.
   //
   // What we can do safely is add ourselves to listOfAllUnits
   //
   listOfAllUnits.append(this);
   return;
}

Measurement::Unit::~Unit() = default;

void Measurement::Unit::initialiseLookups() {
   for (auto const unit : listOfAllUnits) {
      Measurement::PhysicalQuantity const physicalQuantity = unit->pimpl->unitSystem.getPhysicalQuantity();
      unitNameLookup.insert(NameLookupKey{physicalQuantity, unit->name.toLower()}, unit);
      if (unit->pimpl->isCanonical) {
         physicalQuantityToCanonicalUnit.insert(physicalQuantity, unit);
      }
   }
   return;
}

bool Measurement::Unit::operator==(Unit const & other) const {
   // Since we're not intending to create multiple instances of any given UnitSystem, it should be enough to check
   // the addresses are equal, but, as belt-and-braces, we'll check the names & physical quantities are equal as a
   // fall-back.
   return (this == &other ||
           (this->name == other.name && this->getPhysicalQuantity() == other.getPhysicalQuantity()));
}

Measurement::Unit const & Measurement::Unit::getCanonical() const {
   return Measurement::Unit::getCanonicalUnit(this->getPhysicalQuantity());
}

bool Measurement::Unit::isCanonical() const {
   return &this->getCanonical() == this;
}

Measurement::Amount Measurement::Unit::toCanonical(double amt) const {
   return Measurement::Amount{this->pimpl->convertToCanonical(amt), this->getCanonical()};
}

double Measurement::Unit::fromCanonical(double amt) const {
   return this->pimpl->convertFromCanonical(amt);
}

Measurement::PhysicalQuantity Measurement::Unit::getPhysicalQuantity() const {
   // The PhysicalQuantity for this Unit is already stored in its UnitSystem, so we don't store it separately here
   return this->pimpl->unitSystem.getPhysicalQuantity();
}

Measurement::UnitSystem const & Measurement::Unit::getUnitSystem() const {
   return this->pimpl->unitSystem;
}

double Measurement::Unit::boundary() const {
   return this->pimpl->boundaryValue;
}

Measurement::Unit const & Measurement::Unit::getCanonicalUnit(Measurement::PhysicalQuantity const physicalQuantity) {
   // Need this before we reference unitNameLookup or physicalQuantityToCanonicalUnit
   std::call_once(initFlag_Lookups, &Measurement::Unit::initialiseLookups);

   // It's a coding error if there is no canonical unit for a real physical quantity (ie not Mixed).  (And of course
   // there should be no Unit or UnitSystem for Mixed.)
   Q_ASSERT(physicalQuantityToCanonicalUnit.contains(physicalQuantity));
   auto canonical = physicalQuantityToCanonicalUnit.value(physicalQuantity);
   return *canonical;
}

QString Measurement::Unit::convertWithoutContext(QString const & qstr, QString const & toUnitName) {

   qDebug() << Q_FUNC_INFO << "Trying to convert" << qstr << "to" << toUnitName;
   double fromQuantity = quantityFromAmountString(qstr);

   QString const fromUnitName = unitNameFromAmountString(qstr);
   auto const fromUnits = getUnitsOnlyByName(fromUnitName);
   auto const toUnits   = getUnitsOnlyByName(toUnitName);
   qDebug() <<
      Q_FUNC_INFO << "Found" << fromUnits.length() << "matches for" << fromUnitName << "and" << toUnits.length() <<
      "matches for" << toUnitName;

   if (fromUnits.length() > 0 && toUnits.length() > 0) {
      // We found at least one match for both "from" and "to" unit names.  We need to check search amongst these to find
      // one where both units relate to the same physical quantity.
      for (auto const fromUnit : fromUnits) {
         for (auto const toUnit : toUnits) {
            // Stop at the first match.  If there's more than one match then we don't have a means to disambiguate.
            if (fromUnit->getPhysicalQuantity() == toUnit->getPhysicalQuantity()) {
               double canonicalQuantity = fromUnit->toCanonical(fromQuantity).quantity();
               double toQuantity        = toUnit->fromCanonical(canonicalQuantity);
               return QString("%1 %2").arg(Measurement::displayQuantity(toQuantity, 3)).arg(toUnit->name);
            }
         }
      }
   }

   // If we didn't recognise from or to units, or we couldn't find a pair for the same PhysicalQuantity, the we return
   // the original amount with a question mark.
   return QString("%1 ?").arg(Measurement::displayQuantity(fromQuantity, 3));
}

Measurement::Unit const * Measurement::Unit::getUnit(QString const & name,
                                                     Measurement::PhysicalQuantity const & physicalQuantity,
                                                     bool const caseInensitiveMatching) {
   auto matches = getUnitsByNameAndPhysicalQuantity(name, physicalQuantity, caseInensitiveMatching);

   auto const numMatches = matches.length();
   if (0 == numMatches) {
      return nullptr;
   }

   // Under most circumstances, there is a one-to-one relationship between unit string and Unit. C will only map to
   // Measurement::Unit::Celsius, for example. If there's only one match, just return it.
   if (1 == numMatches) {
      auto unitToReturn = matches.at(0);
      if (unitToReturn->getPhysicalQuantity() != physicalQuantity) {
         qWarning() <<
            Q_FUNC_INFO << "Unit" << name << "matches a unit of type" << unitToReturn->getPhysicalQuantity() <<
            "but caller specified" << physicalQuantity;
         return nullptr;
      }
      return unitToReturn;
   }

   // That solved something like 99% of the use cases. Now we have to handle those pesky volumes.
   // Loop through the found Units, like Measurement::Unit::us_quart and
   // Measurement::Unit::imperial_quart, and try to find one that matches the global default.
   Measurement::Unit const * defUnit = nullptr;
   for (auto const unit : matches) {
      auto const & displayUnitSystem = Measurement::getDisplayUnitSystem(unit->getPhysicalQuantity());
      qDebug() <<
         Q_FUNC_INFO << "Look at" << *unit << "from" << unit->getUnitSystem() << "(Display Unit System for" <<
         unit->getPhysicalQuantity() << "is" << displayUnitSystem << ")";
      if (unit->getPhysicalQuantity() != physicalQuantity) {
         // If the caller knows the amount is, say, a Volume, don't bother trying to match against units for any other
         // physical quantity.
         qDebug() << Q_FUNC_INFO << "Ignoring match in" << unit->getPhysicalQuantity() << "as not" << physicalQuantity;
         continue;
      }

      if (displayUnitSystem == unit->getUnitSystem()) {
         // We found a match that belongs to one of the global default unit systems
         return unit;
      }

      // Save this for later if we need it - ie if we don't find a better match
      defUnit = unit;
   }

   // If we got here, we couldn't find a match. Unless something weird has
   // happened, that means you entered "qt" into a field and the system
   // default is SI. At that point, just use the USCustomary
   return defUnit;
}

Measurement::Unit const * Measurement::Unit::getUnit(QString const & name,
                                                     Measurement::UnitSystem const & unitSystem,
                                                     bool const caseInensitiveMatching) {
   auto matches = getUnitsByNameAndPhysicalQuantity(name, unitSystem.getPhysicalQuantity(), caseInensitiveMatching);

   //
   // At this point, matches is a list of all units matching the supplied name and the PhysicalQuantity of the supplied
   // UnitSystem.  If we have more than one match, then we prefer the first one we find (if any) in the supplied
   // UnitSystem, otherwise, first in the list will have to do.
   //

   auto const numMatches = matches.length();
   if (0 == numMatches) {
      return nullptr;
   }

   if (1 == numMatches) {
      return matches.at(0);
   }

   for (auto const match : matches) {
      if (match->getUnitSystem() == unitSystem) {
         return match;
      }
   }

   return matches.at(0);
}

// This is where we actually define all the different units and how to convert them to/from their canonical equivalents
// Previously this was done with a huge number of subclasses, but lambdas mean that's no longer necessary
// Note that we always need to define the canonical Unit for a given PhysicalQuantity before any others
//
namespace Measurement::Units {
   // :NOTE FOR TRANSLATORS: The abbreviated name of each unit (eg "kg" for kilograms, "g" for grams, etc) must be
   //                        unique for that type of unit.  Eg you cannot have two units of weight with the same
   //                        abbreviated name.  Ideally, this should also be true on a case insensitive basis, eg it is
   //                        undesirable for "foo" and "Foo" to be the abbreviated names of two different units of
   //                        the same type.
   //

   // === Mass ===
   // See comment in measurement/UnitSystem.cpp for why we have separate entities for US Customary pounds/ounces and Imperials ones, even though they are, in fact, the same
   Unit const kilograms           {Measurement::UnitSystems::mass_Metric,                         QObject::tr("kg"),       [](double x){return x;},                 [](double y){return y;},                 1.0};
   Unit const grams               {Measurement::UnitSystems::mass_Metric,                         QObject::tr("g"),        [](double x){return x/1000.0;},          [](double y){return y*1000.0;},          1.0,  &kilograms};
   Unit const milligrams          {Measurement::UnitSystems::mass_Metric,                         QObject::tr("mg"),       [](double x){return x/1000000.0;},       [](double y){return y*1000000.0;},       1.0,  &kilograms};
   Unit const pounds              {Measurement::UnitSystems::mass_UsCustomary,                    QObject::tr("lb"),       [](double x){return x*0.45359237;},      [](double y){return y/0.45359237;},      1.0,  &kilograms};
   Unit const ounces              {Measurement::UnitSystems::mass_UsCustomary,                    QObject::tr("oz"),       [](double x){return x*0.0283495231;},    [](double y){return y/0.0283495231;},    1.0,  &kilograms};
   Unit const imperial_pounds     {Measurement::UnitSystems::mass_Imperial,                       QObject::tr("lb"),       [](double x){return x*0.45359237;},      [](double y){return y/0.45359237;},      1.0,  &kilograms};
   Unit const imperial_ounces     {Measurement::UnitSystems::mass_Imperial,                       QObject::tr("oz"),       [](double x){return x*0.0283495231;},    [](double y){return y/0.0283495231;},    1.0,  &kilograms};
   // === Volume ===
   // Where possible, the multipliers for going to and from litres come from www.conversion-metric.org as it seems to offer the most decimal places on its conversion tables
   Unit const liters              {Measurement::UnitSystems::volume_Metric     ,                  QObject::tr("L"),        [](double x){return x;},                    [](double y){return y;},                    1.0};
   Unit const milliliters         {Measurement::UnitSystems::volume_Metric     ,                  QObject::tr("mL"),       [](double x){return x/1000.0;},             [](double y){return y*1000.0;},             1.0,  &liters};
   Unit const us_barrels          {Measurement::UnitSystems::volume_UsCustomary,                  QObject::tr("bbl"),      [](double x){return x*117.34777;},          [](double y){return y/117.34777;},          1.0,  &liters};
   Unit const us_gallons          {Measurement::UnitSystems::volume_UsCustomary,                  QObject::tr("gal"),      [](double x){return x*3.7854117840007;},    [](double y){return y/3.7854117840007;},    1.0,  &liters};
   Unit const us_quarts           {Measurement::UnitSystems::volume_UsCustomary,                  QObject::tr("qt"),       [](double x){return x*0.94635294599999;},   [](double y){return y/0.94635294599999;},   1.0,  &liters};
   Unit const us_pints            {Measurement::UnitSystems::volume_UsCustomary,                  QObject::tr("qt"),       [](double x){return x*0.473176473;},        [](double y){return y/0.473176473;},        1.0,  &liters};
   Unit const us_cups             {Measurement::UnitSystems::volume_UsCustomary,                  QObject::tr("cup"),      [](double x){return x*0.23658823648491;},   [](double y){return y/0.23658823648491;},   0.25, &liters};
   Unit const us_fluidOunces      {Measurement::UnitSystems::volume_UsCustomary,                  QObject::tr("floz"),     [](double x){return x*0.029573529564112;},  [](double y){return y/0.029573529564112;},  1.0,  &liters};
   Unit const us_tablespoons      {Measurement::UnitSystems::volume_UsCustomary,                  QObject::tr("tbsp"),     [](double x){return x*0.014786764782056;},  [](double y){return y/0.014786764782056;},  1.0,  &liters};
   Unit const us_teaspoons        {Measurement::UnitSystems::volume_UsCustomary,                  QObject::tr("tsp"),      [](double x){return x*0.0049289215940186;}, [](double y){return y/0.0049289215940186;}, 1.0,  &liters};
   Unit const imperial_barrels    {Measurement::UnitSystems::volume_Imperial   ,                  QObject::tr("bbl"),      [](double x){return x*163.659;},            [](double y){return y/163.659;},            1.0,  &liters};
   Unit const imperial_gallons    {Measurement::UnitSystems::volume_Imperial   ,                  QObject::tr("gal"),      [](double x){return x*4.5460899999997;},    [](double y){return y/4.5460899999997;},    1.0,  &liters};
   Unit const imperial_quarts     {Measurement::UnitSystems::volume_Imperial   ,                  QObject::tr("qt"),       [](double x){return x*1.1365225;},          [](double y){return y/1.1365225;},          1.0,  &liters};
   Unit const imperial_pints      {Measurement::UnitSystems::volume_Imperial   ,                  QObject::tr("qt"),       [](double x){return x*0.56826125;},         [](double y){return y/0.56826125;},         1.0,  &liters};
   Unit const imperial_cups       {Measurement::UnitSystems::volume_Imperial   ,                  QObject::tr("cup"),      [](double x){return x*0.284130625;},        [](double y){return y/0.284130625;},        0.25, &liters};
   Unit const imperial_fluidOunces{Measurement::UnitSystems::volume_Imperial   ,                  QObject::tr("floz"),     [](double x){return x*0.028413075003383;},  [](double y){return y/0.028413075003383;},  1.0, &liters};
   Unit const imperial_tablespoons{Measurement::UnitSystems::volume_Imperial   ,                  QObject::tr("tbsp"),     [](double x){return x*0.0177581714;},       [](double y){return y/0.0177581714;},       1.0,  &liters};
   Unit const imperial_teaspoons  {Measurement::UnitSystems::volume_Imperial   ,                  QObject::tr("tsp"),      [](double x){return x*0.00591939047;},      [](double y){return y/0.00591939047;},      1.0,  &liters};
   // === Time ===
   // Added weeks because BeerJSON has it
   Unit const minutes             {Measurement::UnitSystems::time_CoordinatedUniversalTime,       QObject::tr("min"),      [](double x){return x;},                 [](double y){return y;},                 1.0};
   Unit const weeks               {Measurement::UnitSystems::time_CoordinatedUniversalTime,       QObject::tr("week"),     [](double x){return x*(7.0*24.0*60.0);}, [](double y){return y/(7.0*24.0*60.0);}, 1.0,  &minutes};
   Unit const days                {Measurement::UnitSystems::time_CoordinatedUniversalTime,       QObject::tr("day"),      [](double x){return x*(24.0*60.0);},     [](double y){return y/(24.0*60.0);},     1.0,  &minutes};
   Unit const hours               {Measurement::UnitSystems::time_CoordinatedUniversalTime,       QObject::tr("hr"),       [](double x){return x*60.0;},            [](double y){return y/60.0;},            2.0,  &minutes};
   Unit const seconds             {Measurement::UnitSystems::time_CoordinatedUniversalTime,       QObject::tr("s"),        [](double x){return x/60.0;},            [](double y){return y*60.0;},            90.0, &minutes};

   // === Temperature ===
   Unit const celsius             {Measurement::UnitSystems::temperature_MetricIsCelsius,         QObject::tr("C"),        [](double x){return x;},               [](double y){return y;},                1.0};
   Unit const fahrenheit          {Measurement::UnitSystems::temperature_UsCustomaryIsFahrenheit, QObject::tr("F"),        [](double x){return (x-32)*5.0/9.0;},  [](double y){return y * 9.0/5.0 + 32;}, 1.0,  &celsius};
   // === Color ===
   // Not sure how many people use Lovibond scale these days, but BeerJSON supports it, so we need to be able to read
   // it.  https://en.wikipedia.org/wiki/Beer_measurement#Colour= says "The Standard Reference Method (SRM) ... [gives]
   // results approximately equal to the °L."
   Unit const srm                 {Measurement::UnitSystems::color_StandardReferenceMethod,       QObject::tr("srm"),      [](double x){return x;},               [](double y){return y;},                1.0};
   Unit const ebc                 {Measurement::UnitSystems::color_EuropeanBreweryConvention,     QObject::tr("ebc"),      [](double x){return x * 12.7/25.0;},   [](double y){return y * 25.0/12.7;},    1.0,  &srm};
   Unit const lovibond            {Measurement::UnitSystems::color_Lovibond,                      QObject::tr("lovibond"), [](double x){return x;},               [](double y){return y;},                1.0,  &srm};
   // == Density ===
   // Brix isn't much used in beer brewing, but BeerJSON supports it, so we have it here.
   // Per https://en.wikipedia.org/wiki/Beer_measurement, Plato and Brix are "essentially ... the same ([both based on
   // mass fraction of sucrose) [and only] differ in their conversion from weight percentage to specific gravity in the
   // fifth and sixth decimal places"
   Unit const specificGravity             {Measurement::UnitSystems::density_SpecificGravity,             QObject::tr("sg"),       [](double x){return x;},               [](double y){return y;},                1.0};
   Unit const plato               {Measurement::UnitSystems::density_Plato,
                                   QObject::tr("P"),
                                          [](double x){return x == 0.0 ? 0.0 : Algorithms::PlatoToSG_20C20C(x);},
                                          [](double y){return y == 0.0 ? 0.0 : Algorithms::SG_20C20C_toPlato(y);},
                                          1.0,
                                          &specificGravity};
   Unit const brix                {Measurement::UnitSystems::density_Brix,
                                   QObject::tr("brix"),
                                   [](double x){return x == 0.0 ? 0.0 : Algorithms::BrixToSgAt20C(x);},
                                   [](double y){return y == 0.0 ? 0.0 : Algorithms::SgAt20CToBrix(y);},
                                   1.0,
                                   &specificGravity};
   // == Diastatic power ==
   Unit const lintner             {Measurement::UnitSystems::diastaticPower_Lintner,                  QObject::tr("L"),    [](double x){return x;},               [](double y){return y;},                1.0};
   Unit const wk                  {Measurement::UnitSystems::diastaticPower_WindischKolbach,          QObject::tr("WK"),   [](double x){return (x + 16) / 3.5;},  [](double y){return 3.5 * y - 16;},     1.0,  &lintner};
   // == Acidity ==
   Unit const pH                  {Measurement::UnitSystems::acidity_pH,                              QObject::tr("pH"),   [](double x){return x;},               [](double y){return y;},                1.0};
   // == Bitterness ==
   Unit const ibu                 {Measurement::UnitSystems::bitterness_InternationalBitternessUnits, QObject::tr("IBU"),  [](double x){return x;},               [](double y){return y;},                1.0};
   // == Carbonation ==
   // Per http://www.uigi.com/co2_conv.html, 1 cubic metre (aka 1000 litres) of CO2 at 1 atmosphere pressure and 0°C
   // temperature weighs 1.9772 kg, so 1 litre weighs 1.9772 g at this pressure and temperature.  Not clear however
   // whether we should use 0°C or 20°C or some other temperature for the conversion from volumes to grams per litre.
   // A brewing-specific source, https://byo.com/article/master-the-action-carbonation/, gives the conversion factor as
   // 1.96, so we use that.
   Unit const carbonationVolumes      {Measurement::UnitSystems::carbonation_Volumes,                 QObject::tr("vol"),  [](double x){return x;},               [](double y){return y;},                1.0};
   Unit const carbonationGramsPerLiter{Measurement::UnitSystems::carbonation_MassPerVolume,           QObject::tr("mg/L"), [](double x){return x / 1.96;},        [](double y){return y * 1.96;},         1.0,  &carbonationVolumes};
   // == Mass Concentration ==
   Unit const milligramsPerLiter  {Measurement::UnitSystems::concentration_MassPerVolume,             QObject::tr("mg/L"), [](double x){return x;},               [](double y){return y;},                1.0};
   // == Volume Concentration ==
   Unit const partsPerMillion     {Measurement::UnitSystems::concentration_PartsPer,                  QObject::tr("ppm"),  [](double x){return x;},               [](double y){return y;},                1.0};
   Unit const partsPerBillion     {Measurement::UnitSystems::concentration_PartsPer,                  QObject::tr("ppb"),  [](double x){return x * 1000.0;},      [](double y){return y/1000.0;},         1.0,  &partsPerMillion};
   // == Viscosity ==
   // Yes, 1 centipoise = 1 millipascal-second.  See comment in measurement/Unit.h for more info
   Unit const centipoise          {Measurement::UnitSystems::viscosity_Metric         ,               QObject::tr("cP"   ), [](double x){return x;},              [](double y){return y;},                1.0};
   Unit const millipascalSecond   {Measurement::UnitSystems::viscosity_MetricAlternate,               QObject::tr("mPa-s"), [](double x){return x;},              [](double y){return y;},                1.0,  &centipoise};
   // == Specific heat capacity ==
   // See comment in measurement/Unit.h for why the non-metric units are the canonical ones
   Unit const caloriesPerCelsiusPerGram{Measurement::UnitSystems::specificHeatCapacity_Calories,     QObject::tr("c/g·C"   ), [](double x){return x;},                    [](double y){return y;},                    1.0};
   Unit const joulesPerKelvinPerKg     {Measurement::UnitSystems::specificHeatCapacity_Joules  ,     QObject::tr("J/kg·K"  ), [](double x){return x / 4184.0;},           [](double y){return y * 4184.0;},           1.0, &caloriesPerCelsiusPerGram};
   Unit const btuPerFahrenheitPerPound {Measurement::UnitSystems::specificHeatCapacity_Btus    ,     QObject::tr("BTU/lb·F"), [](double x){return x;},                    [](double y){return y;},                    1.0, &caloriesPerCelsiusPerGram};
   // == Specific Volume ==
   Unit const litresPerKilogram     {Measurement::UnitSystems::specificVolume_Metric     ,           QObject::tr("L/kg"   ),  [](double x){return x;},                    [](double y){return y;},                    1.0};
   Unit const litresPerGram         {Measurement::UnitSystems::specificVolume_Metric     ,           QObject::tr("L/g"    ),  [](double x){return x * 1000;},             [](double y){return y / 1000;},             1.0, &litresPerKilogram};
   Unit const cubicMetersPerKilogram{Measurement::UnitSystems::specificVolume_Metric     ,           QObject::tr("m^3/kg" ),  [](double x){return x * 1000;},             [](double y){return y / 1000;},             1.0, &litresPerKilogram};
   Unit const us_fluidOuncesPerOunce{Measurement::UnitSystems::specificVolume_UsCustomary,           QObject::tr("floz/oz"),  [](double x){return x * 66.7632356142;},    [](double y){return y / 66.7632356142;},    1.0, &litresPerKilogram};
   Unit const us_gallonsPerPound    {Measurement::UnitSystems::specificVolume_UsCustomary,           QObject::tr("gal/lb" ),  [](double x){return x * 8.34540445177617;}, [](double y){return y / 8.34540445177617;}, 1.0, &litresPerKilogram};
   Unit const us_quartsPerPound     {Measurement::UnitSystems::specificVolume_UsCustomary,           QObject::tr("qt/lb"  ),  [](double x){return x * 2.08635111294;},    [](double y){return y / 2.08635111294;},    1.0, &litresPerKilogram};
   Unit const us_gallonsPerOunce    {Measurement::UnitSystems::specificVolume_UsCustomary,           QObject::tr("gal/oz" ),  [](double x){return x * 0.521587778236;},   [](double y){return y / 0.521587778236;},   1.0, &litresPerKilogram};
   Unit const cubicFeetPerPound     {Measurement::UnitSystems::specificVolume_UsCustomary,           QObject::tr("ft^3/lb"),  [](double x){return x * 62.4279605755126;}, [](double y){return y / 62.4279605755126;}, 1.0, &litresPerKilogram};

}
