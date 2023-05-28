/*======================================================================================================================
 * widgets/BtBoolComboBox.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef WIDGETS_BTBOOLCOMBOBOX_H
#define WIDGETS_BTBOOLCOMBOBOX_H
#pragma once

#include <memory> // For PImpl
#include <optional>

#include <QComboBox>

#include "utils/TypeLookup.h"

/**
 * \class BtBoolComboBox extends \c QComboBox to handle booleans
 */
class BtBoolComboBox : public QComboBox {
Q_OBJECT

public:
   BtBoolComboBox(QWidget * parent);
   virtual ~BtBoolComboBox();

   /**
    * \brief Post-construction initialisation.  Usually called via \c BT_BOOL_COMBO_BOX_INIT macro
    *
    *        According to https://bugreports.qt.io/browse/QTBUG-50823 it is never going to be possible to specify the
    *        data (as opposed to display text) for a combo box via the .ui file.  So we have to do it in code instead.
    *        We could use the raw enum values as the data, but it would be a bit painful to debug if we ever had to, so
    *        for small extra effort we use the same serialisation strings that we use for BeerJSON and the DB.
    *
    * \param editorName
    * \param comboBoxName
    * \param comboBoxFqName     Fully qualified name.  Usually a combination of \c editorName and \c comboBoxName
    * \param unsetDisplay       Localised displayable name for unset (ie \c false) value
    * \param setDisplay         Localised displayable name for set (ie \c true) value
    * \param typeInfo           Mainly used to determine whether this is an optional bool
    */
   void init(char const * const   editorName,
             char const * const   comboBoxName,
             char const * const   comboBoxFqName,
             QString      const & unsetDisplay,
             QString      const & setDisplay,
             TypeInfo     const & typeInfo);

   /**
    *
    */
   [[nodiscard]] bool isOptional() const;

   /**
    * \brief Set value of a combo box from a non-optional bool
    */
   void setValue(bool const value);

   /**
    * \brief Set value of a combo box from an optional bool
    */
   void setValue(std::optional<bool> const value);

   void setNull();

   /**
    * \brief Get value of a combo box for a non-optional bool
    */
   [[nodiscard]] bool getNonOptBoolValue() const;

   /**
    * \brief Get value of a combo box for an optional bool
    */
   [[nodiscard]] std::optional<bool> getOptBoolValue() const;


private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;

   //! No copy constructor, as never want anyone, not even our friends, to make copies of a label object
   BtBoolComboBox(BtBoolComboBox const&) = delete;
   //! No assignment operator , as never want anyone, not even our friends, to make copies of a label object
   BtBoolComboBox& operator=(BtBoolComboBox const&) = delete;
   //! No move constructor
   BtBoolComboBox(BtBoolComboBox &&) = delete;
   //! No move assignment
   BtBoolComboBox & operator=(BtBoolComboBox &&) = delete;
};

/**
 * \brief This macro saves a bit of copy-and-paste when invoking \c BtBoolBoolComboBox::Init.  See \c BT_COMBO_BOX_INIT
 *        for details.
 */
#define BT_BOOL_COMBO_BOX_INIT(editorClass, comboBoxName, unsetDisplay, setDisplay, modelClass, propertyName) \
   this->comboBoxName->init(#editorClass, \
                            #comboBoxName, \
                            #editorClass "->" #comboBoxName, \
                            unsetDisplay, \
                            setDisplay, \
                            modelClass::typeLookup.getType(PropertyNames::modelClass::propertyName))

#endif
