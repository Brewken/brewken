/**
 * model/NamedEntity.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Jeff Bailey <skydvr38@verizon.net>
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
#ifndef MODEL_NAMEDENTITY_H
#define MODEL_NAMEDENTITY_H
#pragma once

#include <cstdint>

#include <QDateTime>
#include <QDomText>
#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QVariant>

#include "Brewken.h"
#include "database/DbRecords.h"
#include "database/DatabaseSchema.h"
#include "model/NamedParameterBundle.h"

namespace PropertyNames::NamedEntity { static char const * const folder = "folder"; /* previously kpropFolder */ }
namespace PropertyNames::NamedEntity { static char const * const display = "display"; /* previously kpropDisplay */ }
namespace PropertyNames::NamedEntity { static char const * const deleted = "deleted"; /* previously kpropDeleted */ }
namespace PropertyNames::NamedEntity { static char const * const name = "name"; /* previously kpropName */ }
namespace PropertyNames::NamedEntity { static char const * const key = "key"; }
namespace PropertyNames::NamedEntity { static char const * const parentKey = "parentKey"; }

// Make uintptr_t available in QVariant.
Q_DECLARE_METATYPE( uintptr_t )

/*!
 * \class NamedEntity
 *
 * \brief The base class for our substantive storable items.  There are really two sorts of storable items: ones that
 *        are freestanding and ones that are owned by other storable items.  Eg, a Hop exists in its own right and may
 *        or may not be used in one or more Recipes, but a MashStep only exists as part of a single Mash.
 *           \b BrewNote is owned by its \b Recipe
 *           \b Equipment
 *           \b Fermentable
 *           \b Hop
 *           \b Instruction is owned by its \b Recipe
 *           \b Mash
 *           \b MashStep is owned by its \b Mash
 *           \b Misc
 *           \b Recipe
 *           \b Salt
 *           \b Style
 *           \b Water
 *           \b Yeast
 *
 * Note that this class has previously been called \b Ingredient and \b BeerXMLElement.  We've changed the name to try
 * to best reflect what the class represents.  Although some of this class's subclasses (eg \b Hop, \b Fermentable,
 * \b Yeast) are ingredients in the normal sense of the word, others (eg \b Instruction, \b Equipment, \b Style,
 * \b Mash) are not really.  Equally, the fact that derived classes can be instantiated from BeerXML is not their
 * defining characteristic.
 *
 * NB: We cannot make this a template class (eg to use
 * https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) because the Qt Meta-Object Compiler (moc) cannot
 * handle templates, and we want to be able to use the Qt Property system as well as signals and slots.
 */
class NamedEntity : public QObject
{
   Q_OBJECT
   Q_CLASSINFO("version","1")

   friend class BeerXML;
public:
   NamedEntity(DatabaseConstants::DbTableId table, int key, QString t_name = QString(),
                  bool t_display = false, QString folder = QString());
   NamedEntity( NamedEntity const& other );
   NamedEntity(NamedParameterBundle & namedParameterBundle, DatabaseConstants::DbTableId table);

   // Our destructor needs to be virtual because we sometimes point to an instance of a derived class through a pointer
   // to this class -- ie NamedEntity * namedEntity = new Hop() and suchlike.  We do already get a virtual destructor by
   // virtue of inheriting from QObject, but this declaration does no harm.
   virtual ~NamedEntity() = default;

   /**
    * \brief This generic version of operator== should work for subclasses provided they correctly _override_ (NB not
    *        overload) the protected virtual isEqualTo() function.
    */
   bool operator==(NamedEntity const & other) const;

   /**
    * \brief We don't have a need to assign one NamedEntity to another, and the compiler implementation of this would
    *        be wrong, so we delete it.
    */
   NamedEntity & operator=(NamedEntity const &) = delete;

   /**
    * \brief This generic version of operator!= should work for subclasses provided they correctly _override_ (NB not
    *        overload) the protected virtual isEqualTo() function.
    */
   bool operator!=(NamedEntity const & other) const;

   //
   // TODO We should replace the following with the spaceship operator once compiler support for C++20 is more widespread
   //
   /**
    * \brief As you might expect, this ensures we order \b NamedEntity objects by name
    */
   bool operator<(NamedEntity const & other) const;
   bool operator>(NamedEntity const & other) const;

   // Everything that inherits from BeerXML has a name, delete, display and a folder
   Q_PROPERTY( QString name   READ name WRITE setName )
   Q_PROPERTY( bool deleted   READ deleted WRITE setDeleted )
   Q_PROPERTY( bool display   READ display WRITE setDisplay )
   Q_PROPERTY( QString folder READ folder WRITE setFolder )

   Q_PROPERTY( int key READ key WRITE setKey )
   Q_PROPERTY( int parentKey READ getParentKey WRITE setParentKey )
   Q_PROPERTY( DatabaseConstants::DbTableId table READ table )

   //! Convenience method to determine if we are deleted or displayed
   bool deleted() const;
   bool display() const;
   //! Access to the folder attribute.
   QString folder() const;
   //! Access to the name attribute.
   QString name() const;

   /**
    * \brief Returns a regexp that will match the " (n)" (for n some positive integer) added on the end of a name to
    *        prevent name clashes.  It will also "capture" n to allow you to extract it.
    */
   static QRegExp const & getDuplicateNameNumberMatcher();

   //! And ways to set those flags
   void setDeleted(const bool var, bool cachedOnly = false);
   void setDisplay(const bool var, bool cachedOnly = false);
   //! and a way to set the folder
   virtual void setFolder(const QString var, bool signal=true, bool cachedOnly = false);

   //!
   void setName(const QString var, bool cachedOnly = false);

   //! \returns our key in the table we are stored in.
   int key() const;

   /**
    * \brief Set the ID (aka key) by which this object is uniquely identified in its DB table
    *
    *        This is virtual because, in some cases, subclasses are going to want to do additional work here
    */
   virtual void setKey(int key);

   int getParentKey() const;
   void setParentKey(int parentKey);

   // .:TODO:. MY 2021-03-23 Ultimately we shouldn't need to know this
   //! \returns the table we are stored in.
   DatabaseConstants::DbTableId table() const;
   //! \returns the BeerXML version of this element.
   int version() const;
   //! Convenience method to get a meta property by name.
   QMetaProperty metaProperty(const char* name) const;
   //! Convenience method to get a meta property by name.
   QMetaProperty metaProperty(QString const& name) const;

   // .:TODO:. MY 2021-03-23 These don't really belong here
   // Should be able to get rid of them when we finish refactoring BeerXml.cpp
   // Some static helpers to convert to/from text.
   static double getDouble( const QDomText& textNode );
   static bool getBool( const QDomText& textNode );
   static int getInt( const QDomText& textNode );
   static QString getString( QDomText const& textNode );
   static QDateTime getDateTime( QDomText const& textNode );
   static QDate getDate( QDomText const& textNode );
   //! Convert the string to a QDateTime according to Qt::ISODate.
   static QDateTime getDateTime(QString const& str = "");
   static QDate getDate(QString const& str = "");
   static QString text(bool val);
   static QString text(double val);
   static QString text(int val);
   //! Convert the date to string in Qt::ISODate format for storage NOT display.
   static QString text(QDate const& val);

   //! Use this to pass pointers around in QVariants.
   static inline QVariant qVariantFromPtr( NamedEntity* ptr )
   {
      uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
      return QVariant::fromValue<uintptr_t>(addr);
   }

   static inline NamedEntity* extractPtr( QVariant ptrVal )
   {
      uintptr_t addr = ptrVal.value<uintptr_t>();
      return reinterpret_cast<NamedEntity*>(addr);
   }

   /*!
    * \brief Some entities (eg Fermentable, Hop) get copied when added to a recipe, but others (eg Instruction) don't.
    *        For those that do, we think of the copy as being a child of the original NamedEntity.  This function allows
    *        us to access that parent.
    * \return Pointer to the parent NamedEntity from which this one was originally copied, or null if no such parent exists.
    */
   virtual NamedEntity * getParent() = 0;

   void setParent(NamedEntity const & parentNamedEntity);

   /*!
    * \brief When we create an NamedEntity, or undelete a deleted one, we need to put it in the database.  For the case of
    *        undelete, it's helpful for the caller not to have to know what subclass of NamedEntity we are resurrecting.
    * \return Key of element inserted in database.
    */
   int insertInDatabase();

   /*!
    * \brief If we can put something in the database, then we also need to be able to remove it.
    *        Note that, once removed from the DB, the caller is responsible for deleting this object.
    */
   void removeFromDatabase();

signals:
   /*!
    * Passes the meta property that has changed about this object.
    * NOTE: when subclassing, be \em extra careful not to create a method with
    * the same signature. Otherwise, everything will silently break.
    */
   void changed(QMetaProperty, QVariant value = QVariant());
   void changedFolder(QString);
   void changedName(QString);

protected:
   /**
    * \brief Subclasses need to overload (NB not override) this function to do the substantive work for operator==.
    *        By the time this function is called on a subclass, we will already have established that the two objects
    *        being compared are of the same class (eg we are not comparing a Hop with a Yeast) and that the names match,
    *        so subclasses do not need to repeat these tests.
    *
    *        We do not currently anticipate sub-sub-classes of \b NamedEntity but if one ever were created, it should
    *        call its parent's implementation of this function before doing its own class-specific tests.
    * \return \b true if this object is, in all the ways that matter, equal to \b other
    */
   virtual bool isEqualTo(NamedEntity const & other) const = 0;

   /**
    * \brief Subclasses need to override this function to return the appropriate instance of \c DbNamedEntityRecords.
    *        This allows us in this base class to access \c DbNamedEntityRecords<Hop> for \c Hop,
    *        \c DbNamedEntityRecords<Fermentable> for \c Fermentable, etc.
    */
   virtual DbRecords & getDbNamedEntityRecordsInstance() const = 0;

   //! The key of this entity in its table.
   int _key;
   //! The table where this entity is stored.
   DatabaseConstants::DbTableId _table;
   // This is 0 if there is no parent (or parent is not yet known)
   int parentKey;

   /*!
    * \param prop_name A meta-property name
    * \param col_name The appropriate column in the table.
    * \param value the new value
    * \param notify true to call NOTIFY method associated with \c prop_name
    * Should do the following:
    * 1) Set the appropriate value in the appropriate table row.
    * 2) Call the NOTIFY method associated with \c prop_name if \c notify == true.
    */
   /*
   void set( const char* prop_name, const char* col_name, QVariant const& value, bool notify = true );
   void set( const QString& prop_name, const QString& col_name, const QVariant& value, bool notify = true );
   */
   void setEasy(char const * const prop_name, QVariant value, bool notify = true);

   /*!
    * \param col_name - The database column of the attribute we want to get.
    * Returns the value of the attribute specified by key/table/col_name.
    *
    * .:TODO:. MY 2021-03-21 Would be nice if NamedEntity didn't need to know such DB details
    */
   QVariant get( const QString& col_name ) const;

   void setInventory( const QVariant& value, int invKey = 0, bool notify=true );
   QVariant getInventory( const QString& col_name ) const;

//   QVariantMap getColumnValueMap() const;

private:
  mutable QString _folder;
  mutable QString _name;
  mutable bool _display;
  mutable bool _deleted;

};

#endif
