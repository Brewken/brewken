/*======================================================================================================================
 * catalogs/YeastCatalog.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#ifndef CATALOGS_YEASTCATALOG_H
#define CATALOGS_YEASTCATALOG_H
#pragma once

#include <QDialog>
#include <QEvent>

#include "editors/YeastEditor.h"
#include "model/Yeast.h"
#include "tableModels/YeastTableModel.h"

// This needs to be the last include.  (I know, I know...)
#include "catalogs/CatalogBase.h"

/*!
 * \class YeastCatalog
 *
 * \brief View/controller class for showing/editing the list of yeasts in the database.
 */
class YeastCatalog : public QDialog, public CatalogBase<YeastCatalog,
                                                        Yeast,
                                                        YeastTableModel,
                                                        YeastSortFilterProxyModel,
                                                        YeastEditor> {
   Q_OBJECT

   CATALOG_COMMON_DECL(Yeast)

};

#endif
