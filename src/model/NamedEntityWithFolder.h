/*======================================================================================================================
 * model/NamedEntityWithFolder.h is part of Brewken, and is copyright the following authors 2024:
 *   • Matt Young <mfsy@yahoo.com>
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
#ifndef MODEL_NAMEDENTITYWITHFOLDER_H
#define MODEL_NAMEDENTITYWITHFOLDER_H
#pragma once

#include "model/NamedEntity.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::NamedEntityWithFolder { BtStringConst const property{#property}; }
AddPropertyName(folder)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


class NamedEntityWithFolder : public NamedEntity {
   Q_OBJECT

public:
   /**
    * \brief See comment in model/NamedEntity.h
    */
   static QString const LocalisedName;

   /**
    * \brief Mapping of names to types for the Qt properties of this class.  See \c NamedEntity::typeLookup for more
    *        info.
    */
   static TypeLookup const typeLookup;

   NamedEntityWithFolder(QString name, bool display = false, QString folder = "");
   NamedEntityWithFolder(NamedParameterBundle const & namedParameterBundle);
   NamedEntityWithFolder(NamedEntityWithFolder const & other);
   virtual ~NamedEntityWithFolder();

   virtual void swap(NamedEntityWithFolder & other) noexcept;

   //=================================================== PROPERTIES ====================================================
   Q_PROPERTY(QString folder READ folder WRITE setFolder)

   //============================================ "GETTER" MEMBER FUNCTIONS ============================================
   QString folder() const;

   //============================================ "SETTER" MEMBER FUNCTIONS ============================================
   void setFolder(QString const & var);

private:
  QString m_folder;
};

// Note that we cannot write `Q_DECLARE_METATYPE(NamedEntityWithFolder)` here, because NamedEntityWithFolder is an
// abstract class.
Q_DECLARE_METATYPE(NamedEntityWithFolder *)
Q_DECLARE_METATYPE(NamedEntityWithFolder const *)
Q_DECLARE_METATYPE(std::shared_ptr<NamedEntityWithFolder>)

#endif
