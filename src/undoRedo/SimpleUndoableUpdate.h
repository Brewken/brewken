/*======================================================================================================================
 * undoRedo/SimpleUndoableUpdate.h is part of Brewken, and is copyright the following authors 2020-2023:
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
#ifndef SIMPLE_UNDOABLE_UPDATE_H
#define SIMPLE_UNDOABLE_UPDATE_H
#pragma once

#include <QMetaProperty>
#include <QMetaType>
#include <QString>
#include <QUndoCommand>
#include <QVariant>

#include "model/NamedEntity.h"
#include "utils/BtStringConst.h"
#include "utils/OptionalHelpers.h"
#include "utils/TypeTraits.h"
#include "utils/TypeLookup.h"

/*!
 * \class SimpleUndoableUpdate
 *
 * \brief Each instance of this class is an undoable update to a 'simple' editable field of a recipe, style, etc.
 *        By simple, we mean that there is one of them and that it is non-relational (ie can be passed and set by value).
 *        The thing being updated needs to inherit from Q_OBJECT and the field being changed needs to have been
 *        declared as a Q_PROPERTY.
 */
class SimpleUndoableUpdate : public QUndoCommand {
public:
   /*!
    * \brief The template wrappers below around this constructor cover the cases where compiler doesn't know a priori
    *        how to (correctly) convert the newValue argument to a \c QVariant.
    *
    * \param updatee The entity (eg recipe) we are updating
    * \param propertyName Which property we are updating - needs to have been declared as a Q_PROPERTY in the class header file
    * \param newValue The new value to assign
    * \param description Short text we can show on undo/redo menu to describe this update eg "Change Recipe Name"
    * \param parent This is for grouping updates together.  We don't currently use it.
    */
   SimpleUndoableUpdate(NamedEntity & updatee,
                        TypeInfo const & typeInfo,
                        QVariant newValue,
                        QString const & description,
                        QUndoCommand * parent = nullptr);

   template<typename E, typename = std::enable_if_t<is_non_optional_enum<E>::value> >
   SimpleUndoableUpdate(NamedEntity & updatee,
                        TypeInfo const & typeInfo,
                        E newValue,
                        QString const & description,
                        QUndoCommand * parent = nullptr) :
      SimpleUndoableUpdate(updatee, typeInfo, static_cast<int>(newValue), description, parent) {
      Q_ASSERT(typeInfo.isEnum());
      Q_ASSERT(!typeInfo.isOptional());
      return;
   }

   template<typename E, typename = std::enable_if_t<is_non_optional_enum<E>::value> >
   SimpleUndoableUpdate(NamedEntity & updatee,
                        TypeInfo const & typeInfo,
                        std::optional<E> newValue,
                        QString const & description,
                        QUndoCommand * parent = nullptr) :
      SimpleUndoableUpdate(updatee, typeInfo, Optional::toOptInt(newValue), description, parent) {
      Q_ASSERT(typeInfo.isEnum());
      Q_ASSERT(typeInfo.isOptional());
      return;
   }

   template<IsOptionalOther T>
   SimpleUndoableUpdate(NamedEntity & updatee,
                        TypeInfo const & typeInfo,
                        T newValue,
                        QString const & description,
                        QUndoCommand * parent = nullptr) :
      SimpleUndoableUpdate(updatee, typeInfo, QVariant::fromValue<T>(newValue), description, parent) {
      Q_ASSERT(typeInfo.isOptional());
      return;
   }

   ~SimpleUndoableUpdate();

   /*!
    * \brief Apply the update (including for the first time)
    */
   void redo();

   /*!
    * \brief Undo applying the update
    */
   void undo();

private:
   /*!
    * \brief Undo or redo applying the update
    * \param isUndo true for undo, false for redo
    * \return true if succeeded, false otherwise - not currently used but potentially useful for a derived class
    */
   bool undoOrRedo(bool const isUndo);

   NamedEntity & updatee;

   /**
    * \brief Because \c QVariant isn't fantastic at handling null values (although it looks like this may be improved
    *        in Qt 6), we need to know a bit about the type we are storing.
    */
   TypeInfo const & typeInfo;

   QVariant oldValue;
   QVariant newValue;
};

/**
 * \brief Convenience macros for the second parameter to the constructor.  Instead of writing:
 *           Misc::typeLookup.getType(PropertyNames::Misc::use)
 *        You write:
 *           TYPE_INFO(Misc, use)
 *
 *        For inherited properties, we need an extra parameter.  Instead of writing:
 *           Misc::typeLookup.getType(PropertyNames::NamedEntity::name)
 *        You write:
 *           TYPE_INFO(Misc, NamedEntity, name)
 *
 *        Using TYPE_INFO_GET_OVERLOAD is a standard macro "trick" to allow us to have two "overloads" of TYPE_INFO that
 *        actually resolve down to TYPE_INFO_2 and TYPE_INFO_3 (where the subscript is the number of parameters).
 */
#define TYPE_INFO_2(className, property) className::typeLookup.getType(PropertyNames::className::property)
#define TYPE_INFO_3(className, baseClassName, property) className::typeLookup.getType(PropertyNames::baseClassName::property)
#define TYPE_INFO_GET_OVERLOAD(param1, param2, param3, NAME, ...) NAME
#define TYPE_INFO(...) TYPE_INFO_GET_OVERLOAD(__VA_ARGS__, TYPE_INFO_3, TYPE_INFO_2)(__VA_ARGS__)

#endif
