/**
 * model/NamedEntity.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Mik Firestone <mikfire@gmail.com>
 *   • Philip Greggory Lee <rocketman768@gmail.com>
 *   • Samuel Östling <MrOstling@gmail.com>
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
#include "model/NamedEntity.h"

#include <typeinfo>

#include <QMetaProperty>

#include "Brewken.h"
#include "database/ObjectStore.h"

namespace {
 char const * const kVersion = "version";
}

NamedEntity::NamedEntity(int key,  bool cache, QString t_name, bool t_display, QString folder) :
   QObject    {nullptr  },
   _key       {key      },
   parentKey  {-1       },
   m_cacheOnly{cache    },
   _folder    {folder   },
   _name      {t_name   },
   _display   {t_display},
   _deleted   {false    } {
   return;
}

NamedEntity::NamedEntity(NamedEntity const & other) :
   QObject    {nullptr          }, // QObject doesn't have a copy constructor, so just make a new one
   _key       {-1               }, // We don't want to copy the other object's key/ID
   parentKey  {other.parentKey  },
   m_cacheOnly{other.m_cacheOnly},
   _folder    {other._folder    },
   _name      {other._name      },
   _display   {other._display   },
   _deleted   {other._deleted   } {
   return;
}

NamedEntity::NamedEntity(NamedParameterBundle const & namedParameterBundle) :
   QObject    {nullptr                                                            },
   _key       {namedParameterBundle(PropertyNames::NamedEntity::key).toInt()      },
   // Not all subclasses have parents so parentKey should be optional in the NamedParameterBundle
   // .:TBD:. For the moment, parent IDs are actually stored outside the main object table (eg in equipment_children
   //         rather than equipment), so this will always set parentKey to -1, but we could envisage changing that in
   //         future.
   parentKey  {namedParameterBundle(PropertyNames::NamedEntity::parentKey, -1)    },
   m_cacheOnly{false                                                              },
   _folder    {namedParameterBundle(PropertyNames::NamedEntity::folder, QString{})}, // Not all subclasses have folders
   _name      {namedParameterBundle(PropertyNames::NamedEntity::name, QString{})  }, // One subclass, BrewNote, does not have a name
   _display   {namedParameterBundle(PropertyNames::NamedEntity::display).toBool() },
   _deleted   {namedParameterBundle(PropertyNames::NamedEntity::deleted).toBool() } {
   return;
}

void NamedEntity::makeChild(NamedEntity const & copiedFrom) {
   // It's a coding error if we're not starting out with objects that are copies of each other
   Q_ASSERT(*this == copiedFrom);
   Q_ASSERT(this->parentKey == copiedFrom.parentKey);

   // We also assume that this newly-created object has not yet been put in the database (so we don't need to call
   // down to the ObjectStore to update fields in the DB).
   Q_ASSERT(this->_key <= 0);

   // By default, we have the same parent as the object from which we were copied.  But, if that means we have no
   // parent, then we take object from which we were copied as our parent, on the assumption that it is the master
   // version of this Hop/Fermentable/etc.
   if (this->parentKey <= 0) {
      this->parentKey = copiedFrom._key;
   }

   // So, now, we should have some plausible parent ID, and in particular we should not be our own parent!
   Q_ASSERT(this->parentKey != this->_key);
   return;
}

QRegExp const & NamedEntity::getDuplicateNameNumberMatcher() {
   //
   // Note that, in the regexp, to match a bracket, we need to escape it, thus "\(" instead of "(".  However, we
   // must also escape the backslash so that the C++ compiler doesn't think we want a special character (such as
   // '\n') and barf a "unknown escape sequence" warning at us.  So "\\(" is needed in the string literal here to
   // pass "\(" to the regexp to match literal "(" (and similarly for close bracket).
   //
   static QRegExp const duplicateNameNumberMatcher{" *\\(([0-9]+)\\)$"};
   return duplicateNameNumberMatcher;
}


// See https://zpz.github.io/blog/overloading-equality-operator-in-cpp-class-hierarchy/ (and cross-references to
// http://www.gotw.ca/publications/mill18.htm) for good discussion on implementation of operator== in a class
// hierarchy.  Our implementation differs slightly for a couple of reasons:
//   - This class is already abstract so it's good to force subclasses to implement isEqualTo() by making it pure
//     virtual here.  (Of course that doesn't help us for sub-sub-classes etc, but it's better than nothing)
//   - We want to do the type comparison first, as this saves us repeating this test in each subclass
//
bool NamedEntity::operator==(NamedEntity const & other) const {
   // The first thing to do is check we are even comparing two objects of the same class.  A Hop is never equal to
   // a Recipe etc.
   if (typeid(*this) != typeid(other)) {
//      qDebug() << Q_FUNC_INFO << "No type id match (" << typeid(*this).name() << "/" << typeid(other).name() << ")";
      return false;
   }

   //
   // For the base class attributes, we deliberately don't compare _key, parentKey, table or _folder.  If we've read
   // in an object from a file and want to  see if it's the same as one in the database, then the DB-related info and
   // folder classification are not a helpful part of that comparison.  Similarly, we do not compare _display and
   // _deleted as they are more related to the UI than whether, in essence, two objects are the same.
   //
   if (this->_name != other._name) {
//      qDebug() << Q_FUNC_INFO << "No name match (" << this->_name << "/" << other._name << ")";
      //
      // If the names don't match, let's check it's not for a trivial reason.  Eg, if you have one Hop called
      // "Tettnang" and another called "Tettnang (1)" we wouldn't say they are different just because of the names.
      // So we want to strip off any number in brackets at the ends of the names and then compare again.
      //
      QRegExp const & duplicateNameNumberMatcher = NamedEntity::getDuplicateNameNumberMatcher();
      QString names[2] {this->_name, other._name};
      for (auto ii = 0; ii < 2; ++ii) {
         int positionOfMatch = duplicateNameNumberMatcher.indexIn(names[ii]);
         if (positionOfMatch > -1) {
            // There's some integer in brackets at the end of the name.  Chop it off.
            names[ii].truncate(positionOfMatch);
         }
      }
//      qDebug() << Q_FUNC_INFO << "Adjusted names to " << names[0] << " & " << names[1];
      if (names[0] != names[1]) {
         return false;
      }
   }

   return this->isEqualTo(other);
}

bool NamedEntity::operator!=(NamedEntity const & other) const {
   // Don't reinvent the wheel '!=' should just be the opposite of '=='
   return !(*this == other);
}

bool NamedEntity::operator<(const NamedEntity & other) const { return (this->_name < other._name); }
bool NamedEntity::operator>(const NamedEntity & other) const { return (this->_name > other._name); }

bool NamedEntity::deleted() const {
   return this->_deleted;
}

bool NamedEntity::display() const {
   return this->_display;
}

// Sigh. New databases, more complexity
void NamedEntity::setDeleted(const bool var, bool cachedOnly) {
   this->_deleted = var;
   if ( ! cachedOnly ) {
      setEasy(PropertyNames::NamedEntity::deleted, var ? Brewken::dbTrue() : Brewken::dbFalse());
   }
   return;
}

void NamedEntity::setDisplay(bool var, bool cachedOnly) {
   this->_display = var;
   if ( ! cachedOnly ) {
      setEasy(PropertyNames::NamedEntity::display, var ? Brewken::dbTrue() : Brewken::dbFalse());
   }
   return;
}

QString NamedEntity::folder() const {
   return this->_folder;
}

void NamedEntity::setFolder(const QString var, bool signal, bool cachedOnly) {
   this->_folder = var;
   if ( ! cachedOnly )
      // set( kFolder, kFolder, var );
      setEasy( PropertyNames::NamedEntity::folder, var );
   // not sure if I should only signal when not caching?
   if ( signal ) {
      emit changedFolder(var);
   }
   return;
}

QString NamedEntity::name() const {
   return this->_name;
}

void NamedEntity::setName(const QString var, bool cachedOnly) {
   this->_name = var;
   if ( ! cachedOnly ) {
      setEasy( PropertyNames::NamedEntity::name, var );
      emit changedName(var);
   }
   return;
}

int NamedEntity::key() const {
   return this->_key;
}

void NamedEntity::setKey(int key) {
   this->_key = key;
   return;
}

int NamedEntity::getParentKey() const {
   return this->parentKey;
}

bool NamedEntity::cacheOnly() const {
   return this->m_cacheOnly;
}

void NamedEntity::setParentKey(int parentKey) {
   this->parentKey = parentKey;

   //
   // If the data is obviously messed up then let's at least log it.  (It doesn't necessarily mean there is a bug in
   // the current version of the code.  It could be the result of a bug in an earlier version.  If so, a manual data
   // fix is needed in the database.)
   //
   // Something should not be its own parent for instance
   //
   if (this->parentKey == this->_key) {
      qCritical() << Q_FUNC_INFO << this->metaObject()->className() << "#" << this->_key << "is its own parent!";
   }

   return;
}

void NamedEntity::setCacheOnly(bool cache) {
   this->m_cacheOnly = cache;
   return;
}

QVector<int> NamedEntity::getParentAndChildrenIds() const {
   QVector<int> results;
   NamedEntity const * parent = this->getParent();
   if (nullptr == parent) {
      parent = this;
   }

   // We are assuming that grandparents do not exist - ie that it's a coding error if they do
   // We want more than just an assert in that case as debugging would be a lot harder without knowing which
   // NamedEntity has the unexpected data.
   if (parent->parentKey > 0) {
      qCritical() <<
         Q_FUNC_INFO << this->metaObject()->className() << "#" << this->_key << "has parent #" << this->parentKey <<
         "with parent #" << parent->parentKey;
      Q_ASSERT(false);
   }

   // We've got the parent ingredient...
   results.append(parent->_key);

   // ...now find all the children, ie all the other ingredients of this type whose parent is the ingredient we just
   // found
   QList<std::shared_ptr<QObject> > children = this->getObjectStoreTypedInstance().findAllMatching(
      [parent](std::shared_ptr<QObject> obj) { return std::static_pointer_cast<NamedEntity>(obj)->getParentKey() == parent->key(); }
   );
   for (auto child : children) {
      results.append(std::static_pointer_cast<NamedEntity>(child)->key());
   }
   return results;
}


int NamedEntity::version() const {
   return QString(metaObject()->classInfo(metaObject()->indexOfClassInfo(kVersion)).value()).toInt();
}

QMetaProperty NamedEntity::metaProperty(const char* name) const
{
   return metaObject()->property(metaObject()->indexOfProperty(name));
}

QMetaProperty NamedEntity::metaProperty(QString const& name) const
{
   return metaObject()->property(metaObject()->indexOfProperty(name.toStdString().c_str()));
}

// getVal =====================================================================
double NamedEntity::getDouble(const QDomText& textNode)
{
   bool ok;
   double ret;

   QString text = textNode.nodeValue();

   // ret = text.toDouble(&ok);
   ret = Brewken::toDouble(text,&ok);
   if( !ok )
      qCritical() << QString("NamedEntity::getDouble: %1 is not a number. Line %2").arg(text).arg(textNode.lineNumber());

   return ret;
}

bool NamedEntity::getBool(const QDomText& textNode)
{
   QString text = textNode.nodeValue();

   if( text == "TRUE" )
      return true;
   else if( text == "FALSE" )
      return false;
   else
      qCritical() << QString("NamedEntity::getBool: %1 is not a boolean value. Line %2").arg(text).arg(textNode.lineNumber());

   return false;
}

int NamedEntity::getInt(const QDomText& textNode)
{
   bool ok;
   int ret;
   QString text = textNode.nodeValue();

   ret = text.toInt(&ok);
   if( !ok )
      qCritical() << QString("NamedEntity::getInt: %1 is not an integer. Line %2").arg(text).arg(textNode.lineNumber());

   return ret;
}

QString NamedEntity::getString( QDomText const& textNode )
{
   return textNode.nodeValue();
}

QDateTime NamedEntity::getDateTime( QDomText const& textNode )
{
   bool ok = true;
   QDateTime ret;
   QString text = textNode.nodeValue();

   ret = QDateTime::fromString(text, Qt::ISODate);
   ok = ret.isValid();
   if( !ok )
      qCritical() << QString("NamedEntity::getDateTime: %1 is not a date. Line %2").arg(text).arg(textNode.lineNumber());

   return ret;
}

QDate NamedEntity::getDate( QDomText const& textNode )
{
   bool ok = true;
   QDate ret;
   QString text = textNode.nodeValue();

   ret = QDate::fromString(text, "M/d/yyyy");
   ok = ret.isValid();
   // Dates have some odd inconsistencies.
   if( !ok )
   {
      ret = QDate::fromString(text,"d/M/yyyy");
      ok = ret.isValid();
   }

   if ( !ok )
      qCritical() << QString("NamedEntity::getDate: %1 is not an ISO date-time. Line %2").arg(text).arg(textNode.lineNumber());

   return ret;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

QDateTime NamedEntity::getDateTime(QString const& str)
{
   QDateTime temp;

   if ( str != "" && (temp = QDateTime::fromString(str, Qt::ISODate)).isValid() )
      return temp;
   else
      return QDateTime::currentDateTime();
}

QString NamedEntity::text(bool val)
{
   if( val )
      return QString("TRUE");
   else
      return QString("FALSE");
}

QString NamedEntity::text(double val)
{
   return QString("%1").arg(val, 0, 'f', 8);
}

QString NamedEntity::text(int val)
{
   return QString("%1").arg(val);
}

QString NamedEntity::text(QDate const& val)
{
   return val.toString(Qt::ISODate);
}

void NamedEntity::setEasy(char const * const prop_name, QVariant value, bool notify) {

   // NB: It's the caller's responsibility to have actually updated the property here, we're just telling the object
   //     store it got updated.  (You might think we can just call this->setProperty(prop_name, value), but that's
   //     going to result in an object setter function getting called, which in turn is going to call this function,
   //     which will then lead us to infinite recursion.)

   // Update the object store, but only if we're already stored there
   // (We don't pass the value as it will get read out of the object via prop_name)
   if (this->_key > 0) {
      this->getObjectStoreTypedInstance().updateProperty(*this, prop_name);
   }

   // Send a signal if needed
   if (notify) {
      int idx = this->metaObject()->indexOfProperty(prop_name);
      QMetaProperty mProp = this->metaObject()->property(idx);
      emit this->changed(mProp, value);
   }

   return;
}


NamedEntity * NamedEntity::getParent() const {
   if (this->parentKey <=0) {
      return nullptr;
   }

   return static_cast<NamedEntity *>(this->getObjectStoreTypedInstance().getById(this->parentKey).get());
}


void NamedEntity::setParent(NamedEntity const & parentNamedEntity) {
   this->parentKey = parentNamedEntity._key;
   this->setEasy(PropertyNames::NamedEntity::parentKey, this->parentKey);
   return;
}

int NamedEntity::insertInDatabase() {
   return this->getObjectStoreTypedInstance().insertOrUpdate(this);
}

void NamedEntity::removeFromDatabase() {
   this->getObjectStoreTypedInstance().softDelete(this->_key);
   return;
}
