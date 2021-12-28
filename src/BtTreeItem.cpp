/*======================================================================================================================
 * BtTreeItem.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Daniel Pettersson <pettson81@gmail.com>
 *   • Greg Meess <Daedalus12@gmail.com>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
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
#include "BtTreeItem.h"

#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <Qt>
#include <QVariant>
#include <QVector>

#include "Brewken.h"
#include "BtFolder.h"
#include "Localization.h"
#include "measurement/Measurement.h"
#include "model/BrewNote.h"
#include "model/Equipment.h"
#include "model/Fermentable.h"
#include "model/Hop.h"
#include "model/Misc.h"
#include "model/Recipe.h"
#include "model/Style.h"
#include "model/Water.h"
#include "model/Yeast.h"
#include "PersistentSettings.h"
#include "tableModels/FermentableTableModel.h"

namespace {
   QHash<BtTreeItem::ITEMTYPE, char const *> const ItemTypeToName {
      {BtTreeItem::RECIPE,      "RECIPE"     },
      {BtTreeItem::EQUIPMENT,   "EQUIPMENT"  },
      {BtTreeItem::FERMENTABLE, "FERMENTABLE"},
      {BtTreeItem::HOP,         "HOP"        },
      {BtTreeItem::MISC,        "MISC"       },
      {BtTreeItem::YEAST,       "YEAST"      },
      {BtTreeItem::BREWNOTE,    "BREWNOTE"   },
      {BtTreeItem::STYLE,       "STYLE"      },
      {BtTreeItem::FOLDER,      "FOLDER"     },
      {BtTreeItem::WATER,       "WATER"      }
   };
}

bool operator==(BtTreeItem & lhs, BtTreeItem & rhs) {
   // Things of different types are not equal
   if (lhs._type != rhs._type) {
      return false;
   }

   return lhs.data(lhs._type, 0) == rhs.data(rhs._type, 0);
}

BtTreeItem::BtTreeItem(int _type, BtTreeItem * parent)
   : parentItem(parent), _thing(nullptr), m_showMe(false) {
   setType(_type);
}

BtTreeItem::~BtTreeItem() {
   qDeleteAll(this->childItems);
}

BtTreeItem * BtTreeItem::child(int number) {
   if (number < this->childItems.count()) {
      return this->childItems.value(number);
   }

   return nullptr;
}

BtTreeItem * BtTreeItem::parent() {
   return parentItem;
}

int BtTreeItem::type() {
   return _type;
}

int BtTreeItem::childCount() const {
   return this->childItems.count();
}

int BtTreeItem::columnCount(int _type) const {
   switch (_type) {
      case RECIPE:
         return RECIPENUMCOLS;
      case EQUIPMENT:
         return EQUIPMENTNUMCOLS;
      case FERMENTABLE:
         return FERMENTABLENUMCOLS;
      case HOP:
         return HOPNUMCOLS;
      case MISC:
         return MISCNUMCOLS;
      case YEAST:
         return YEASTNUMCOLS;
      case STYLE:
         return STYLENUMCOLS;
      case BREWNOTE:
         return BREWNUMCOLS;
      case FOLDER:
         return FOLDERNUMCOLS;
      case WATER:
         return WATERNUMCOLS;
      default:
         qWarning() << QString("BtTreeItem::columnCount Bad column: %1").arg(_type);
         return 0;
   }

}

QVariant BtTreeItem::data(int _type, int column) {

   switch (_type) {
      case RECIPE:
         return dataRecipe(column);
      case EQUIPMENT:
         return dataEquipment(column);
      case FERMENTABLE:
         return dataFermentable(column);
      case HOP:
         return dataHop(column);
      case MISC:
         return dataMisc(column);
      case YEAST:
         return dataYeast(column);
      case STYLE:
         return dataStyle(column);
      case BREWNOTE:
         return dataBrewNote(column);
      case FOLDER:
         return dataFolder(column);
      case WATER:
         return dataWater(column);
      default:
         qWarning() << QString("BtTreeItem::data Bad column: %1").arg(column);
         return QVariant();
   }
}

int BtTreeItem::childNumber() const {
   if (this->parentItem) {
      return parentItem->childItems.indexOf(const_cast<BtTreeItem *>(this));
   }
   return 0;
}

void BtTreeItem::setData(int t, QObject * d) {
   _thing = d;
   _type  = t;
}

QVariant BtTreeItem::data(int column) {
   return data(type(), column);
}

bool BtTreeItem::insertChildren(int position, int count, int _type) {
//   qDebug() <<
//      Q_FUNC_INFO << "Inserting" << count << "children of type" << _type << "(" <<
//      this->itemTypeToString(static_cast<BtTreeItem::ITEMTYPE>(_type)) << ") at position" << position;
   if (position < 0  || position > this->childItems.size()) {
      qWarning() << Q_FUNC_INFO << "Position" << position << "outside range (0, " << this->childItems.size() << ")";
      return false;
   }

   for (int row = 0; row < count; ++row) {
      BtTreeItem * newItem = new BtTreeItem(_type, this);
      this->childItems.insert(position + row, newItem);
   }

   return true;
}

bool BtTreeItem::removeChildren(int position, int count) {
   if (position < 0 || position + count > this->childItems.count()) {
      return false;
   }

   for (int row = 0; row < count; ++row) {
      delete this->childItems.takeAt(position);
   }
   // FIXME: memory leak here. With delete, it's a concurrency/memory
   // access error, due to the fact that these pointers are floating around.
   //childItems.takeAt(position);

   return true;
}

QVariant BtTreeItem::dataRecipe(int column) {
   Recipe * recipe = qobject_cast<Recipe *>(_thing);
   switch (column) {
      case RECIPENAMECOL:
         if (! _thing) {
            return QVariant(QObject::tr("Recipes"));
         } else {
            return QVariant(recipe->name());
         }
      case RECIPEANCCOUNT:
         if (recipe) {
            return QVariant(recipe->ancestors().size());
         }
         break;
      case RECIPEBREWDATECOL:
         if (recipe) {
            return Localization::displayDateUserFormated(recipe->date());
         }
         break;
      case RECIPESTYLECOL:
         if (recipe && recipe->style()) {
            return QVariant(recipe->style()->name());
         }
         break;
      default :
         qWarning() << QString("BtTreeItem::dataRecipe Bad column: %1").arg(column);
   }
   return QVariant();
}

QVariant BtTreeItem::dataEquipment(int column) {
   Equipment * kit = qobject_cast<Equipment *>(_thing);
   switch (column) {
      case EQUIPMENTNAMECOL:
         if (! kit) {
            return QVariant(QObject::tr("Equipment"));
         } else {
            return QVariant(kit->name());
         }
      case EQUIPMENTBOILTIMECOL:
         if (kit) {
            return QVariant(kit->boilTime_min());
         }
         break;
      default :
         qWarning() << QString("BtTreeItem::dataEquipment Bad column: %1").arg(column);
   }
   return QVariant();
}

QVariant BtTreeItem::dataFermentable(int column) {
   Fermentable * ferm = qobject_cast<Fermentable *>(this->_thing);

   switch (column) {
      case FERMENTABLENAMECOL:
         if (ferm) {
            return QVariant(ferm->name());
         } else {
            return QVariant(QObject::tr("Fermentables"));
         }
      case FERMENTABLETYPECOL:
         if (ferm) {
            return QVariant(ferm->typeStringTr());
         }
         break;
      case FERMENTABLECOLORCOL:
         if (ferm) {
            return QVariant(
                      Measurement::displayAmount(
                         ferm->color_srm(),
                         &Measurement::Units::srm,
                         0,
                         Measurement::getUnitSystemForField(*PropertyNames::Fermentable::color_srm, "")
                      )
                   );
         }
         break;
      default :
         qWarning() << Q_FUNC_INFO << "Bad column:" << column;
         break;
   }
   return QVariant();
}

QVariant BtTreeItem::dataHop(int column) {
   Hop * hop = qobject_cast<Hop *>(_thing);
   switch (column) {
      case HOPNAMECOL:
         if (! hop) {
            return QVariant(QObject::tr("Hops"));
         } else {
            return QVariant(hop->name());
         }
      case HOPFORMCOL:
         if (hop) {
            return QVariant(hop->formStringTr());
         }
         break;
      case HOPUSECOL:
         if (hop) {
            return QVariant(hop->useStringTr());
         }
         break;
      default :
         qWarning() << QString("BtTreeItem::dataHop Bad column: %1").arg(column);
   }
   return QVariant();
}

QVariant BtTreeItem::dataMisc(int column) {
   Misc * misc = qobject_cast<Misc *>(_thing);
   switch (column) {
      case MISCNAMECOL:
         if (! misc) {
            return QVariant(QObject::tr("Miscellaneous"));
         } else {
            return QVariant(misc->name());
         }
      case MISCTYPECOL:
         if (misc) {
            return QVariant(misc->typeStringTr());
         }
         break;
      case MISCUSECOL:
         if (misc) {
            return QVariant(misc->useStringTr());
         }
         break;
      default :
         qWarning() << QString("BtTreeItem::dataMisc Bad column: %1").arg(column);
   }
   return QVariant();
}

QVariant BtTreeItem::dataYeast(int column) {
   Yeast * yeast = qobject_cast<Yeast *>(_thing);
   switch (column) {
      case YEASTNAMECOL:
         if (! yeast) {
            return QVariant(QObject::tr("Yeast"));
         } else {
            return QVariant(yeast->name());
         }
      case YEASTTYPECOL:
         if (yeast) {
            return QVariant(yeast->typeStringTr());
         }
         break;
      case YEASTFORMCOL:
         if (yeast) {
            return QVariant(yeast->formStringTr());
         }
         break;
      default :
         qWarning() << QString("BtTreeItem::dataYeast Bad column: %1").arg(column);
   }
   return QVariant();
}

QVariant BtTreeItem::dataBrewNote(int column) {
   if (! _thing) {
      return QVariant();
   }

   BrewNote * bNote = qobject_cast<BrewNote *>(_thing);

   return bNote->brewDate_short();
}

QVariant BtTreeItem::dataStyle(int column) {
   Style * style = qobject_cast<Style *>(_thing);

   if (! style && column == STYLENAMECOL) {
      return QVariant(QObject::tr("Style"));
   } else if (style) {
      switch (column) {
         case STYLENAMECOL:
            return QVariant(style->name());
         case STYLECATEGORYCOL:
            return QVariant(style->category());
         case STYLENUMBERCOL:
            return QVariant(style->categoryNumber());
         case STYLELETTERCOL:
            return QVariant(style->styleLetter());
         case STYLEGUIDECOL:
            return QVariant(style->styleGuide());
         default :
            qWarning() << QString("BtTreeItem::dataStyle Bad column: %1").arg(column);
      }
   }
   return QVariant();
}

QVariant BtTreeItem::dataFolder(int column) {
   BtFolder * folder = qobject_cast<BtFolder *>(_thing);


   if (! folder && column == FOLDERNAMECOL) {
      return QVariant(QObject::tr("Folder"));
   }

   if (! folder) {
      return QVariant(QObject::tr("Folder"));
   } else if (column == FOLDERNAMECOL) {
      return QVariant(folder->name());
   }

   return QVariant();
}

QVariant BtTreeItem::dataWater(int column) {
   Water * water = qobject_cast<Water *>(_thing);

   if (water == nullptr && column == WATERNAMECOL) {
      return QVariant(QObject::tr("Water"));
   } else if (water) {
      switch (column) {
         case WATERNAMECOL:
            return QVariant(water->name());
         case WATERCACOL:
            return QVariant(water->calcium_ppm());
         case WATERHCO3COL:
            return QVariant(water->bicarbonate_ppm());
         case WATERSO4COL:
            return QVariant(water->sulfate_ppm());
         case WATERCLCOL:
            return QVariant(water->chloride_ppm());
         case WATERNACOL:
            return QVariant(water->sodium_ppm());
         case WATERMGCOL:
            return QVariant(water->magnesium_ppm());
         case WATERpHCOL:
            return QVariant(water->ph());
         default :
            qWarning() << QString("BtTreeItem::dataWater Bad column: %1").arg(column);
      }
   }

   return QVariant();
}

void BtTreeItem::setType(int t) {
   _type = t;
}

Recipe * BtTreeItem::recipe() {
   if (_type == RECIPE && _thing) {
      return qobject_cast<Recipe *>(_thing);
   }

   return nullptr;
}

Equipment * BtTreeItem::equipment() {
   if (_type == EQUIPMENT) {
      return qobject_cast<Equipment *>(_thing);
   }
   return nullptr;
}

Fermentable * BtTreeItem::fermentable() {
   if (_type == FERMENTABLE) {
      return qobject_cast<Fermentable *>(_thing);
   }
   return nullptr;
}

Hop * BtTreeItem::hop() {
   if (_type == HOP) {
      return qobject_cast<Hop *>(_thing);
   }
   return nullptr;
}

Misc * BtTreeItem::misc() {
   if (_type == MISC) {
      return qobject_cast<Misc *>(_thing);
   }
   return nullptr;
}

Yeast * BtTreeItem::yeast() {
   if (_type == YEAST) {
      return qobject_cast<Yeast *>(_thing);
   }
   return nullptr;
}

BrewNote * BtTreeItem::brewNote() {
   if (_type == BREWNOTE && _thing) {
      return qobject_cast<BrewNote *>(_thing);
   }

   return nullptr;
}

Style * BtTreeItem::style() {
   if (_type == STYLE && _thing) {
      return qobject_cast<Style *>(_thing);
   }

   return nullptr;
}

BtFolder * BtTreeItem::folder() {
   if (_type == FOLDER && _thing) {
      return qobject_cast<BtFolder *>(_thing);
   }

   return nullptr;
}

Water * BtTreeItem::water() {
   if (_type == WATER && _thing) {
      return qobject_cast<Water *>(_thing);
   }

   return nullptr;
}

NamedEntity * BtTreeItem::thing() {
   if (_thing) {
      return qobject_cast<NamedEntity *>(_thing);
   }

   return nullptr;
}

QString BtTreeItem::name() {
   NamedEntity * tmp;
   if (! _thing) {
      return QString();
   }
   tmp = qobject_cast<NamedEntity *>(_thing);
   return tmp->name();
}

char const * const BtTreeItem::itemTypeToString(BtTreeItem::ITEMTYPE itemType) {
   if (ItemTypeToName.contains(itemType)) {
      return ItemTypeToName.value(itemType);
   }
   return "Unknown!";
}

bool BtTreeItem::showMe() const {
   return m_showMe;
}
void BtTreeItem::setShowMe(bool val) {
   m_showMe = val;
}
