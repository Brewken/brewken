/*======================================================================================================================
 * utils/MetaTypes.h is part of Brewken, and is copyright the following authors 2023:
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
#ifndef UTILS_METATYPES_H
#define UTILS_METATYPES_H
#pragma once

#include <optional>

#include <QDate>
#include <QString>

#include "measurement/ConstrainedAmount.h" // For MassOrVolumeAmt and MassOrVolumeConcentrationAmt;

//
// It is useful in various places to be able to store member variables in QVariant objects.
//
// Where we define a strongly-typed enum, we usually just need a corresponding Q_ENUM declaration in the same header.
// This works with generic serialisation code (eg to and from database or BeerJSON) because you can safely static_cast
// between the strongly typed enum and an integer, so the generic code can use integers (via EnumStringMapping) and the
// class-specific code can use the strongly-typed enums and everything just work.
//
// HOWEVER, when the enum is optional (ie stored in memory inside std::optional, stored in the DB as a nullable field,
// only an optional field in BeerJSON, etc) then we cannot rely on casting.  You cannot, eg, static_cast between
// std::optional<int> and std::optional<Fermentable::GrainGroup>.  So inside NamedParameterBundle, we always store
// std::optional<int> for optional enum fields inside QVariant.  We need this Q_DECLARE_METATYPE macro here to allow
// this to  happen.
//
// We then put template wrappers in NamedParameterBundle so things aren't too clunky in the class-specific code.
//
// Similarly, for other nullable fields, we need to declare that we want to store std::optional<fieldType> inside
// QVariant.  This is a convenient place to do it because this header gets included not only by all the model classes
// but also by all the different serialisation code (Database, XML, JSON).
//
// Note that Qt MOC will error if you repeat a Q_DECLARE_METATYPE() declaration for the same type, which is another
// reason to put them all in one central place rather than try to declare as needed individually.
//
Q_DECLARE_METATYPE(std::optional<bool        >)
Q_DECLARE_METATYPE(std::optional<double      >)
Q_DECLARE_METATYPE(std::optional<int         >)
Q_DECLARE_METATYPE(std::optional<QDate       >)
Q_DECLARE_METATYPE(std::optional<QString     >)
Q_DECLARE_METATYPE(std::optional<unsigned int>)

// Need these to be able to use MassOrVolumeAmt and MassOrVolumeConcentrationAmt in Qt Properties system
Q_DECLARE_METATYPE(MassOrVolumeAmt               )
Q_DECLARE_METATYPE(std::optional<MassOrVolumeAmt>)
Q_DECLARE_METATYPE(MassVolumeOrCountAmt)
Q_DECLARE_METATYPE(std::optional<MassVolumeOrCountAmt>)
Q_DECLARE_METATYPE(MassOrVolumeConcentrationAmt               )
Q_DECLARE_METATYPE(std::optional<MassOrVolumeConcentrationAmt>)

Q_DECLARE_METATYPE(Measurement::Amount               )
Q_DECLARE_METATYPE(std::optional<Measurement::Amount>)

// Normally, we would just declare enums with Q_ENUM, but that doesn't work outside of a QObject class, so we have to
// do it here and use Q_DECLARE_METATYPE instead.
Q_DECLARE_METATYPE(Measurement::PhysicalQuantity)
Q_DECLARE_METATYPE(Measurement::ChoiceOfPhysicalQuantity)

// Measurement::Unit does not inherit from QObject, so we need this for Measurement::Units::unitStringMapping to work
Q_DECLARE_METATYPE(Measurement::Unit const *)

/**
 * \brief Just to keep us on our toes, there is an additional requirement that certain new types be registered at
 *        run-time, otherwise you'll get a "Unable to handle unregistered datatype" error and eg \c QObject::property
 *        will return a \c QVariant that is not valid (ie for which \c isValid() returns \c false).
 *
 *        Again, we choose to do all this run-time registration in one place, viz this function, which should be called
 *        from \c main before invoking \c Application::run().
 */
void registerMetaTypes();

#endif
