/**
 * database/EquipmentSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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
#ifndef EQUIPMENTSCHEMA_H
#define EQUIPMENTSCHEMA_H

// Column names
static const QString kcolEquipBoilSize("boil_size");
static const QString kcolEquipBatchSize("batch_size");
static const QString kcolEquipTunVolume("tun_volume");
static const QString kcolEquipTunWeight("tun_weight");
static const QString kcolEquipTunSpecHeat("tun_specific_heat");
static const QString kcolEquipTopUpWater("top_up_water");
static const QString kcolEquipTrubChillLoss("trub_chiller_loss");
static const QString kcolEquipEvapRate("evap_rate");
static const QString kcolEquipBoilTime("boil_time");
static const QString kcolEquipCalcBoilVol("calc_boil_volume");
static const QString kcolEquipLauterSpace("lauter_deadspace");
static const QString kcolEquipTopUpKettle("top_up_kettle");
static const QString kcolEquipHopUtil("hop_utilization");
static const QString kcolEquipRealEvapRate("real_evap_rate");
static const QString kcolEquipBoilingPoint("boiling_point");
static const QString kcolEquipAbsorption("absorption");

#endif
