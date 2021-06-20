/**
 * database/ObjectStoreWrapper.h is part of Brewken, and is copyright the following authors 2021:
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
 */
#ifndef DATABASE_OBJECTSTOREWRAPPER_H
#define DATABASE_OBJECTSTOREWRAPPER_H
#pragma once
#include "database/ObjectStoreTyped.h"

/**
 * \brief Namespace containing convenience functions for accessing member functions of appropriate ObjectStoreTyped
 *        instances via template argument deduction
 */
namespace ObjectStoreWrapper {
   template<class NE> std::shared_ptr<NE> getById(int id) {
      return ObjectStoreTyped<NE>::getInstance().getById(id);
   }

   /**
    * \brief Raw pointer version of \c getById
    */
   template<class NE> NE * getByIdRaw(int id) {
      return ObjectStoreTyped<NE>::getInstance().getById(id).get();
   }

   template<class NE> QList<std::shared_ptr<NE> > getAll() {
      return ObjectStoreTyped<NE>::getInstance().getAll();
   }

   template<class NE> QList<NE *> getAllRaw() {
      return ObjectStoreTyped<NE>::getInstance().getAllRaw();
   }

   template<class NE> std::shared_ptr<NE> copy(NE const & ne) {
      return std::make_shared<NE>(ne);
   }

   template<class NE> std::shared_ptr<NE> insert(std::shared_ptr<NE> ne) {
      return ObjectStoreTyped<NE>::getInstance().insert(ne);
   }

   template<class NE> std::shared_ptr<NE> insertCopyOf(NE const & ne) {
      return ObjectStoreTyped<NE>::getInstance().insertCopyOf(ne.key());
   }

   template<class NE> void updateProperty(NE const & ne, char const * const propertyToUpdateInDb) {
      ObjectStoreTyped<NE>::getInstance().updateProperty(ne, propertyToUpdateInDb);
      return;
   }

   template<class NE> void softDelete(NE const & ne) {
      ObjectStoreTyped<NE>::getInstance().softDelete(ne.key());
      return;
   }

   template<class NE> void hardDelete(NE const & ne) {
      ObjectStoreTyped<NE>::getInstance().hardDelete(ne.key());
      return;
   }

   template<class NE> QList<std::shared_ptr<NE> > findAllMatching(
      std::function<bool(std::shared_ptr<NE>)> const & matchFunction
   ) {
      return ObjectStoreTyped<NE>::getInstance().findAllMatching(matchFunction);
   }

}

#endif
