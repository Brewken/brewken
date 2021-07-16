/**
 * database/BrewNoteSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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

#ifndef BREWNOTESCHEMA_H
#define BREWNOTESCHEMA_H

#include <QString>
// I am putting this here in the vain hopes I do not forget about it. There
// are two mystery columns defined in the DB for this table: predicted_abv and
// projected_fin_temp. I have no idea what they were meant for, but they are
// not exposed anywhere that I can find.
// They should be removed from the db
// Columns for the brewnote table
static const QString kcolBNoteBrewDate("brewdate");
static const QString kcolBNoteAtten("attenuation");
static const QString kcolBNoteFermDate("fermentdate");
static const QString kcolBNoteNotes("notes");
static const QString kcolBNoteSG("sg");
static const QString kcolBNoteVolIntoBoil("volume_into_bk");
static const QString kcolBNoteOG("og");
static const QString kcolBNoteVolIntoFerm("volume_into_fermenter");
static const QString kcolBNoteFG("fg");
static const QString kcolBNoteABV("abv");
static const QString kcolBNoteEffIntoBoil("eff_into_bk");
static const QString kcolBNoteBrewhsEff("brewhouse_eff");
static const QString kcolBNoteStrikeTemp("strike_temp");
static const QString kcolBNoteMashFinTemp("mash_final_temp");
static const QString kcolBNotePostBoilVol("post_boil_volume");
static const QString kcolBNotePitchTemp("pitch_temp");
static const QString kcolBNoteFinVol("final_volume");
static const QString kcolBNoteBoilOff("boil_off");

static const QString kcolBNoteProjBoilGrav("projected_boil_grav");
static const QString kcolBNoteProjVolIntoBoil("projected_vol_into_bk");
static const QString kcolBNoteProjStrikeTemp("projected_strike_temp");
static const QString kcolBNoteProjMashFinTemp("projected_mash_fin_temp");
static const QString kcolBNoteProjFinTemp("projected_fin_temp");
static const QString kcolBNoteProjOG("projected_og");
static const QString kcolBNoteProjVolIntoFerm("projected_vol_into_ferm");
static const QString kcolBNoteProjFG("projected_fg");
static const QString kcolBNoteProjEff("projected_eff");
static const QString kcolBNoteProjABV("projected_abv");
static const QString kcolBNoteProjAtten("projected_atten");
static const QString kcolBNoteProjPnts("projected_points");
static const QString kcolBNoteProjFermPnts("projected_ferm_points");

#endif
