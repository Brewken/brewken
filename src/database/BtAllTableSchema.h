/**
 * database/BtAllTableSchema.h is part of Brewken, and is copyright the following authors 2019-2020:
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

#ifndef BTALLTABLESCHEMA_H
#define BTALLTABLESCHEMA_H

// Columns for the btall table
static const QString kcolBtAllClassName("class_name");
static const QString kcolBtAllInventoryTableId("inventory_table_id");
static const QString kcolBtAllChildTableId("child_table_id");
static const QString kcolBtAllCreated("created");
static const QString kcolBtAllVersion("version");
static const QString kcolBtAllTableId("table_id");

// this table is a meta table. It has no object and no XML schemas. Weird,
// innit?
//
#endif // BTALLTABLESCHEMA_H
