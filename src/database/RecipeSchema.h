/**
 * database/RecipeSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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
 */
#ifndef RECIPESCHEMA_H
#define RECIPESCHEMA_H

#include <QString>
// Columns for the recipe table
static const QString kcolRecipeType("type");
static const QString kcolRecipeBrewer("brewer");
static const QString kcolRecipeBatchSize("batch_size");
static const QString kcolRecipeBoilSize("boil_size");
static const QString kcolRecipeBoilTime("boil_time");
static const QString kcolRecipeEff("efficiency");
static const QString kcolRecipeAsstBrewer("assistant_brewer");
static const QString kcolRecipeTasteNotes("taste_notes");
static const QString kcolRecipeTasteRating("taste_rating");
static const QString kcolRecipeOG("og");
static const QString kcolRecipeFG("fg");
static const QString kcolRecipeFermStages("fermentation_stages");
static const QString kcolRecipePrimAgeDays("primary_age");
static const QString kcolRecipePrimTemp("primary_temp");
static const QString kcolRecipeSecAgeDays("secondary_age");
static const QString kcolRecipeSecTemp("secondary_temp");
static const QString kcolRecipeTertAgeDays("tertiary_age");
static const QString kcolRecipeTertTemp("tertiary_temp");
static const QString kcolRecipeAge("age");
static const QString kcolRecipeAgeTemp("age_temp");
static const QString kcolRecipeDate("date");
static const QString kcolRecipeCarbVols("carb_volume");
static const QString kcolRecipeForcedCarb("forced_carb");
static const QString kcolRecipePrimSugName("priming_sugar_name");
static const QString kcolRecipeCarbTemp("carbonationtemp_c");
static const QString kcolRecipePrimSugEquiv("priming_sugar_equiv");
static const QString kcolRecipeKegPrimFact("keg_priming_factor");

// Some of the foreign keys
static const QString kcolRecipeEquipmentId("equipment_id");
static const QString kcolRecipeStyleId("style_id");
static const QString kcolRecipeAncestorId("ancestor_id");

static char const * const kpropAncestorId = "ancestor_id";
#endif
