/*======================================================================================================================
 * BrewDayFormatter.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Jeff Bailey <skydvr38@verizon.net>
 *   • Mattias Måhl <mattias@kejsarsten.com>
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
#ifndef BREWDAYFORMATTER_H
#define BREWDAYFORMATTER_H
#pragma once

#include <QList>
#include <QObject>
#include <QStringList>
#include <QWidget>

#include "model/Recipe.h"

class BrewDayFormatter : public QObject {
   Q_OBJECT

public:
   /**
    * @brief Construct a new Brew Day Formatter object
    *
    * @param parent
    */
   BrewDayFormatter(QObject *parent);

   /**
    * @brief Set the Recipe object
    *
    * @param recipe
    */
   void setRecipe(Recipe *recipe);

   /**
    * @brief Builds the whole HTML page for Brewday instructions
    *
    * @return QString
    */
   QString buildHtml();

private:
   /**
    * @brief Create HTML string containing the basic information about the recipe
    *
    * @param includeImage
    * @return QString
    */
   QString buildTitleHtml(bool includeImage = true);

   /**
    * @brief Creates a list of string-lists that contains the data about the basic information about the recipe.
    *
    * @return QList<QStringList>
    */
   QList<QStringList> buildTitleList();
public:
   /**
    * @brief Create HTML string containing the instructions for the recipe
    *
    * @return QString
    */
   QString buildInstructionHtml();
private:
   /**
    * @brief Create a list of string-lists that contain the instructions on how to brew the recipe
    *
    * @return QList<QStringList>
    */
   QList<QStringList> buildInstructionList();
public:
   /**
    * @brief Builds and returns the Boil notes section for the bottom of the HTML page
    *
    * @return QString
    */
   QString buildFooterHtml();
private:
   Recipe *recObs;
   QString cssName;
};

#endif
