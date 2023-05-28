/*======================================================================================================================
 * widgets/BtComboBox.cpp is part of Brewken, and is copyright the following authors 2023:
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
#include "widgets/BtComboBox.h"

// This private implementation class holds all private non-virtual members of BtComboBox
class BtComboBox::impl {
public:
   impl(BtComboBox & self) :
      m_self              {self },
      m_initialised       {false},
      m_editorName        {"Uninitialised m_editorName!"    },
      m_comboBoxName      {"Uninitialised m_comboBoxName!"  },
      m_comboBoxFqName    {"Uninitialised m_comboBoxFqName!"},
      m_nameMapping       {nullptr},
      m_displayNameMapping{nullptr},
      m_typeInfo          {nullptr} {
      return;
   }

   ~impl() = default;

   BtComboBox              & m_self              ;
   bool                      m_initialised       ;
   char const *              m_editorName        ;
   char const *              m_comboBoxName      ;
   char const *              m_comboBoxFqName    ;
   EnumStringMapping const * m_nameMapping       ;
   EnumStringMapping const * m_displayNameMapping;
   TypeInfo          const * m_typeInfo          ;

};

BtComboBox::BtComboBox(QWidget * parent) :
   QComboBox{parent},
   pimpl {std::make_unique<impl>(*this)} {
   return;
}

BtComboBox::~BtComboBox() = default;

void BtComboBox::init(char const * const        editorName        ,
                      char const * const        comboBoxName      ,
                      char const * const        comboBoxFqName    ,
                      EnumStringMapping const & nameMapping       ,
                      EnumStringMapping const & displayNameMapping,
                      TypeInfo          const & typeInfo          ) {
   qDebug() << Q_FUNC_INFO << comboBoxFqName << ":" << typeInfo;

   // It's a coding error to call init twice
   Q_ASSERT(!this->pimpl->m_initialised);

   // It's a coding error if the type we're displaying is not an enum
   Q_ASSERT(typeInfo.isEnum());

   this->pimpl->m_editorName         =  editorName        ;
   this->pimpl->m_comboBoxName       =  comboBoxName      ;
   this->pimpl->m_comboBoxFqName     =  comboBoxFqName    ;
   this->pimpl->m_nameMapping        = &nameMapping       ;
   this->pimpl->m_displayNameMapping = &displayNameMapping;
   this->pimpl->m_typeInfo           = &typeInfo          ;

   // If this is an optional enum, then we need a blank value
   if (typeInfo.isOptional()) {
      this->addItem("", "");
   }

   // It's a coding error if the two enum mappings do not have the same number of entries
   auto const numEnumVals = this->pimpl->m_nameMapping->size();
   Q_ASSERT(this->pimpl->m_displayNameMapping->size() == numEnumVals);
   for (auto ii = 0; ii < numEnumVals; ++ii) {
      this->addItem(*this->pimpl->m_displayNameMapping->enumAsIntToString(ii),
                    *this->pimpl->m_nameMapping       ->enumAsIntToString(ii));
   }

   this->pimpl->m_initialised = true;
   return;
}

[[nodiscard]] bool BtComboBox::isOptional() const {
   Q_ASSERT(this->pimpl->m_initialised);
   return this->pimpl->m_typeInfo->isOptional();
}

void BtComboBox::setNull() {
   this->setCurrentIndex(0);
   // For an optional field, it's a coding error if the first value is not empty string
   Q_ASSERT(this->currentData().toString().isEmpty());
   return;
}

void BtComboBox::setValue(int value) {
   Q_ASSERT(this->pimpl->m_initialised);
   this->setCurrentIndex(this->findData(this->pimpl->m_nameMapping->enumToString(value)));
   // It's a coding error if we have an empty string here
   Q_ASSERT(!this->currentData().toString().isEmpty());
   return;
}

[[nodiscard]] std::optional<int> BtComboBox::getOptIntValue() const {
   Q_ASSERT(this->pimpl->m_initialised);
   QString const rawValue = this->currentData().toString();
   if (rawValue.isEmpty()) {
      Q_ASSERT(this->isOptional());
      return std::nullopt;
   }

   // It's a coding error if we don't recognise the values in our own combo boxes
   auto value = this->pimpl->m_nameMapping->stringToEnumAsInt(rawValue);
   Q_ASSERT(value);
   return value;
}

[[nodiscard]] int BtComboBox::getNonOptIntValue() const {
   Q_ASSERT(this->pimpl->m_initialised);
   QString const rawValue = this->currentData().toString();
   Q_ASSERT(!rawValue.isEmpty());

   // It's a coding error if we don't recognise the values in our own combo boxes
   auto value = this->pimpl->m_nameMapping->stringToEnumAsInt(rawValue);
   Q_ASSERT(value);
   return *value;
}
