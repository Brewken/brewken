/**
 * IbuMethods.h is part of Brewken, and is copyright the following authors 2009-2014:
 *   • Daniel Pettersson <pettson81@gmail.com>
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
#ifndef IBUMETHODS_H
#define IBUMETHODS_H
#pragma once

/*!
 * \class IbuMethods
 *
 * \brief Make IBU calculations.
 */
class IbuMethods
{
public:
   IbuMethods();
   ~IbuMethods();

   /*!
    * \return ibus according to selected algorithm.
    * \param AArating in [0,1] (0.04 means 4% AA for example)
    * \param hops_grams - mass of hops in grams
    * \param finalVolume_liters - self explanatory
    * \param wort_grav in specific gravity at around 60F I guess.
    * \param minutes - minutes that the hops are in the boil
    */
   static double getIbus(double AArating, double hops_grams, double finalVolume_liters, double wort_grav, double minutes);
private:
   static double tinseth(double AArating, double hops_grams, double finalVolume_liters, double wort_grav, double minutes);
   static double rager(double AArating, double hops_grams, double finalVolume_liters, double wort_grav, double minutes);

   /*!
    *
    *
    * \brief Calculates the IBU by Greg Noonans formula
    */
   static double noonan(double AARating, double hops_grams, double finalVolume_liters, double wort_grav, double minutes);
};

#endif
