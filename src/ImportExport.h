/*======================================================================================================================
 * ImportExport.h is part of Brewken, and is copyright the following authors 2013-2022:
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
#ifndef IMPORTEXPORT_H
#define IMPORTEXPORT_H
#pragma once

#include <QList>

class Equipment;
class Fermentable;
class Hop;
class Misc;
class Recipe;
class Style;
class Water;
class Yeast;

namespace ImportExport {
   /**
    * \brief Import recipes, hops, equipment, etc from BeerXML or BeerJSON files specified by the user
    */
   void importFromFiles();

   /**
    * \brief Import recipes, hops, equipment, etc to a BeerXML or BeerJSON file specified by the user
    *        (We'll work out whether it's BeerXML or BeerJSON based on the filename extension, so doesn't need to be
    *        specified in advance.)
    *
    *        Each of the parameters is allowed to be \c nullptr or an empty list, but it is the caller's responsibility
    *        to ensure that not \b all of them are!
    *
    * \param recipes
    * \param equipments
    * \param fermentables
    * \param hops
    * \param miscs
    * \param styles
    * \param waters
    * \param yeasts
    */
   void exportToFile(QList<Recipe      const *> const * recipes,
                     QList<Equipment   const *> const * equipments   = nullptr,
                     QList<Fermentable const *> const * fermentables = nullptr,
                     QList<Hop         const *> const * hops         = nullptr,
                     QList<Misc        const *> const * miscs        = nullptr,
                     QList<Style       const *> const * styles       = nullptr,
                     QList<Water       const *> const * waters       = nullptr,
                     QList<Yeast       const *> const * yeasts       = nullptr);
}

#endif
