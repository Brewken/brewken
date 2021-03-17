/**
 * database/Database.h is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Aidan Roberts <aidanr67@gmail.com>
 *   • A.J. Drobnich <aj.drobnich@gmail.com>
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Dan Cavanagh <dan@dancavanagh.com>
 *   • Jonatan Pålsson <jonatan.p@gmail.com>
 *   • Kregg Kemper <gigatropolis@yahoo.com>
 *   • Mark de Wever <koraq@xs4all.nl>
 *   • Mattias Måhl <mattias@kejsarsten.com>
 *   • Matt Young <mfsy@yahoo.com>
 *   • Maxime Lavigne <duguigne@gmail.com>
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
#ifndef DATABASE_H
#define DATABASE_H

#include <memory> // For PImpl

#include <functional>

#include <QDebug>
#include <QDomDocument>
#include <QDomNode>
#include <QFile>
#include <QHash>
#include <QList>
#include <QMap>
#include <QMetaProperty>
#include <QObject>
#include <QPair>
#include <QRegExp>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QString>
#include <QTableView>
#include <QUndoStack>
#include <QVariant>

#include "Brewken.h"
#include "database/DatabaseSchema.h"
#include "database/TableSchemaConst.h"
#include "database/TableSchema.h"
#include "model/NamedEntity.h"
#include "model/Recipe.h"

// Forward declarations
class BeerXML;
class BrewNote;
class Equipment;
class Fermentable;
class Hop;
class Instruction;
class Mash;
class MashStep;
class Misc;
//class Recipe;
class Style;
class Water;
class Yeast;

/*!
 * \class Database
 *
 *
 * \brief Model for lists of all the NamedEntity items in the database.
 *
 * This class is a singleton, meaning that there should only ever be one
 * instance of this floating around, and its purpose is to manage all of
 * the NamedEntitys in the app. The Database should be the only way
 * we ever get pointers to BeerXML ingredients and the like. This is our
 * big model class.
 *
 * .:TBD:. (MY 2020-01-03) The trouble with having such a broad purpose to this class is that it ends up being enormous
 * and very complicated.  It would be better IMHO to separate things out to:
 *  - one or more registries of NamedEntity derivatives (Hops, Fermentables, Recipes etc)
 *  - a set of mappings and classes that know how to store and retrieve each of these things in the DB
 * Because we load everything into memory, searching for a Yeast etc doesn't require us to access the DB.  We just ask
 * the relevant registry "give me Yeast X".  If we then create a new Yeast (either via the UI or by reading it in from
 * a BeerXML file) we can then ask for it to be saved in the database.
 */
class Database : public QObject
{
   Q_OBJECT

   friend class BeerXML;

public:

   //! This should be the ONLY way you get an instance.
   static Database& instance();
   //! Call this to delete the internal instance.
   static void dropInstance();
   //! \brief Should be called when we are about to close down.
   void unload();

   //! \brief Create a blank database in the given file
   bool createBlank(QString const& filename);

   static char const * getDefaultBackupFileName();

   //! backs up database to chosen file
   static bool backupToFile(QString newDbFileName);

   //! backs up database to 'dir' in chosen directory
   static bool backupToDir(QString dir, QString filename="");

   //! \brief Reverts database to that of chosen file.
   static bool restoreFromFile(QString newDbFileStr);

   static bool verifyDbConnection( Brewken::DBTypes testDb, QString const& hostname,
                                   int portnum=5432,
                                   QString const& schema="public",
                                   QString const& database="brewken",
                                   QString const& username="brewken",
                                   QString const& password="brewken");
   bool loadSuccessful();

   void updateEntry( NamedEntity* object, QString propName, QVariant value, bool notify = true, bool transact = false );

   //! \brief Get the contents of the cell specified by table/key/col_name
   QVariant get( DatabaseConstants::DbTableId table, int key, QString col_name );

   QVariant get( TableSchema* tbl, int key, QString col_name );

   //! Get a table view.
   QTableView* createView( DatabaseConstants::DbTableId table );

   BrewNote* newBrewNote(Recipe* parent, bool signal = true);
   //! Create new instruction attached to \b parent.
   Instruction* newInstruction(Recipe* parent);

   MashStep* newMashStep(Mash* parent, bool connected = true);

   Mash* newMash(Mash* other = nullptr, bool displace = true);
   Mash* newMash(Recipe* parent, bool transaction = true);

   Recipe* newRecipe(QString name);
   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   // Named copy constructors==================================================
   //! \returns a copy of the given note.
   BrewNote* newBrewNote(BrewNote* other, bool signal = true);
   Equipment* newEquipment(Equipment* other = nullptr);
   Fermentable* newFermentable(Fermentable* other = nullptr);
   Hop* newHop(Hop* other = nullptr);
   //! \returns a copy of the given recipe.
   Recipe* newRecipe(Recipe* other);
   /*! \returns a copy of the given mash. Displaces the mash currently in the
    * parent recipe unless \b displace is false.
    */
   Misc* newMisc(Misc* other = nullptr);

   Style* newStyle(Style* other);
   Style* newStyle(QString name);
   Water* newWater(Water* other = nullptr);
   Salt* newSalt(Salt* other = nullptr);
   Yeast* newYeast(Yeast* other = nullptr);

   int    insertElement(NamedEntity* ins);
   int    insertEquipment(Equipment* ins);
   int    insertFermentable(Fermentable* ins);
   int    insertHop(Hop* ins);
   int    insertMash(Mash* ins);
   int    insertMisc(Misc* ins);
   int    insertRecipe(Recipe* ins);
   int    insertStyle(Style* ins);
   int    insertYeast(Yeast* ins);
   int    insertWater(Water* ins);
   int    insertSalt(Salt* ins);

   // Brewnotes, instructions and mashsteps are impossible without their parent objects
   int    insertBrewNote(BrewNote* ins, Recipe *parent);
   int    insertInstruction(Instruction* ins, Recipe *parent);
   int    insertMashStep(MashStep* ins, Mash *parent);

   //! \returns the key of the parent ingredient
   int getParentID(TableSchema* table, int childKey);

   //! \returns true if this ingredient is stored in the DB, false otherwise
   bool isStored(NamedEntity const & ingredient);

   //! Inserts an new inventory row in the appropriate table
   int newInventory(TableSchema* schema);

   int getInventoryId(TableSchema* tbl, int key );
   void setInventory(NamedEntity* ins, QVariant value, int invKey = 0, bool notify=true );

   //! \returns The entire inventory for a table.
   QMap<int, double> getInventory(const DatabaseConstants::DbTableId table) const;

   QVariant getInventoryAmt(QString col_name, DatabaseConstants::DbTableId table, int key);

   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

   //! \brief Copies all of the mashsteps from \c oldMash to \c newMash
   void duplicateMashSteps(Mash *oldMash, Mash *newMash);

   //! Get anything by key value.
   Recipe* recipe(int key);
   Equipment* equipment(int key);
   Fermentable* fermentable(int key);
   Hop* hop(int key);
   Misc* misc(int key);
   Style* style(int key);
   Yeast* yeast(int key);
   Salt* salt(int key);
   Water* water(int key);

   // Add a COPY of these ingredients to a recipe, then call the changed()
   // signal corresponding to the appropriate QList
   // of ingredients in rec. If noCopy is true, then don't copy, and set
   // the ingredient's display parameter to 0 (don't display in lists).
   void addToRecipe( Recipe* rec, Equipment* e, bool noCopy = false, bool transact = true );
   Hop * addToRecipe( Recipe* rec, Hop* hop, bool noCopy = false, bool transact = true);
   Fermentable * addToRecipe( Recipe* rec, Fermentable* ferm, bool noCopy = false, bool transact = true);
   //! Add a mash, displacing any current mash.
   Mash * addToRecipe( Recipe* rec, Mash* m, bool noCopy = false, bool transact = true );
   Misc * addToRecipe( Recipe* rec, Misc* m, bool noCopy = false, bool transact = true);
   //! Add a style, displacing any current style.
   Style * addToRecipe( Recipe* rec, Style* s, bool noCopy = false, bool transact = true );
   Water * addToRecipe( Recipe* rec, Water* w, bool noCopy = false, bool transact = true);
   Salt * addToRecipe( Recipe* rec, Salt* s,  bool noCopy = false, bool transact = true);
   Yeast * addToRecipe( Recipe* rec, Yeast* y, bool noCopy = false, bool transact = true);
   // NOTE: not possible in this format.
   //void addToRecipe( Recipe* rec, Instruction* ins );
   //
   //! \brief bulk add to a recipe.
   void addToRecipe(Recipe* rec, QList<Fermentable*> ferms, bool transact = true);
   void addToRecipe(Recipe* rec, QList<Hop*> hops, bool transact = true);
   void addToRecipe(Recipe* rec, QList<Misc*> miscs, bool transact = true);
   void addToRecipe(Recipe* rec, QList<Yeast*> yeasts, bool transact = true);

   /**
   * \brief  This function is intended to be called by an ingredient that has not already cached its parent's key
   * \return Key of parent ingredient if there is one, 0 otherwise
   */
   int getParentNamedEntityKey(NamedEntity const & ingredient);

   /*! \brief Removes the specified ingredient from the recipe, then calls the changed()
    *         signal corresponding to the appropriate QList
    *         of ingredients in rec.
    *  \param rec
    *  \param ing
    *  \returns the parent of the ingredient deleted (which is needed to be able to undo the removal)
    */
   NamedEntity * removeNamedEntityFromRecipe( Recipe* rec, NamedEntity* ing );

   // An odd ball I can't resolve quite yet. But I will.
   // This one isn't even needed. remove does it
   void removeFromRecipe( Recipe* rec, Instruction* ins );

   //! Remove \b step from \b mash.
   void removeFrom( Mash* mash, MashStep* step );

   // Or you can mark whole lists as deleted.
   // ONE METHOD TO CALL THEM ALL AND IN DARKNESS BIND THEM!
   template<class T> void remove(QList<T*> list)
   {
      if ( list.empty() )
         return;

      int ndx;
      bool emitSignal;

      foreach(T* toBeDeleted, list) {
         const QMetaObject* meta = toBeDeleted->metaObject();
         ndx = meta->indexOfClassInfo("signal");
         emitSignal = ndx != -1 ? true : false;

         remove(toBeDeleted, emitSignal);
      }
   }

   template <class T>void remove(T* ing, bool emitSignal = true)
   {
      if (!ing) return;

      const QMetaObject *meta = ing->metaObject();
      char const * propName = "";

///      DatabaseConstants::DbTableId ingTable = dbDefn->classNameToTable(meta->className());
///      if ( ingTable == DatabaseConstants::BREWNOTETABLE ) {
///         emitSignal = false;
///      }
      if ( meta->className() == QString("BrewNote") ) {
         emitSignal = false;
      }

      if ( emitSignal ) {
         int ndx = meta->indexOfClassInfo("signal");
         if ( ndx != -1 ) {
            propName = meta->classInfo(ndx).value();
         }
         else {
            throw QString("%1 cannot find signal property on %2").arg(Q_FUNC_INFO).arg(meta->className());
         }
      }

      try {
         //! Mark the \b object in \b table as deleted.
         updateEntry( ing, PropertyNames::NamedEntity::deleted, Brewken::dbTrue(), true);
      }
      catch (QString e) {
         qCritical() << QString("%1 %2").arg(Q_FUNC_INFO).arg(e);
         throw;
      }

      // Brewnotes are weird and don't emit a metapropery change
      if ( emitSignal )
         emit changed( metaObject()->property(metaObject()->indexOfProperty(propName)), QVariant() );
      // This was screaming until I needed to emit a freaking signal
///      if ( ingTable != DatabaseConstants::MASHSTEPTABLE )
///         emit deletedSignal(ing);
      if ( meta->className() == QString("MashStep") ) {
         emit deletedSignal(ing);
      }
   }

   //! Get the recipe that this \b note is part of.
   Recipe* getParentRecipe( BrewNote const* note );

   //! Interchange the step orders of the two steps. Must be in same mash.
   void swapMashStepOrder(MashStep* m1, MashStep* m2);
   //! Interchange the instruction orders. Must be in same recipe.
   void swapInstructionOrder(Instruction* in1, Instruction* in2);
   //! Insert an instruction (already in a recipe) into position \b pos.
   void insertInstruction(Instruction* in, int pos);
   //! \brief The instruction number of an instruction.
   int instructionNumber(Instruction const* in);

   Q_PROPERTY( QList<BrewNote*> brewNotes READ brewNotes /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Equipment*> equipments READ equipments /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Fermentable*> fermentables READ fermentables /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Hop*> hops READ hops /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Mash*> mashs READ mashs /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<MashStep*> mashSteps READ mashSteps /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Misc*> miscs READ miscs /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Recipe*> recipes READ recipes /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Style*> styles READ styles /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Water*> waters READ waters /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Salt*> salts READ salts /*WRITE*/ NOTIFY changed STORED false )
   Q_PROPERTY( QList<Yeast*> yeasts READ yeasts /*WRITE*/ NOTIFY changed STORED false )

   // Returns non-deleted NamedEntitys.
   QList<BrewNote*> brewNotes();
   QList<Equipment*> equipments();
   QList<Fermentable*> fermentables();
   QList<Hop*> hops();
   QList<Mash*> mashs();
   QList<MashStep*> mashSteps();
   QList<Misc*> miscs();
   QList<Recipe*> recipes();
   QList<Style*> styles();
   QList<Water*> waters();
   QList<Salt*> salts();
   QList<Yeast*> yeasts();

   /**
    * Templated static versions of the above functions, so other parts of the code can call Database::getAll<Hop>,
    * Database::getAll<Yeast>, etc.
    *
    * This is a template where we _only_ use the specialisations - ie there isn't a general definition.  The
    * specialisations are trivial functions and, in theory, since C++17, we should be able to define them here, eg
    * immediately after the template declaration:
    *   template<class S> static QList<S *> getAll();
    *   template<> QList<BrewNote*>  getAll<BrewNote>()   { return Database::instance().brewNotes(); }
    *   template<> QList<Equipment*> getAll<Equipment>() { return Database::instance().equipments(); }
    *   etc
    * However, due to bug https://gcc.gnu.org/bugzilla/show_bug.cgi?id=85282, this won't compile in gcc.  So we
    * we therefore put them in the cpp file.  (This is fine because callers to getAll<T>() just need the generic bit
    * here in the header file to compile, and the specific implementations of getAll<BrewNote>(), getAll<Equipment>()
    * are only required by the linker.
    */
   template<class S> QList<S *> getAll();

   //! \b returns a list of the brew notes in a recipe.
   QList<BrewNote*> brewNotes(Recipe const* parent);
   //! Return a list of all the fermentables in a recipe.
   QList<Fermentable*> fermentables(Recipe const* parent);
   //! Return a list of all the hops in a recipe.
   QList<Hop*> hops( Recipe const* parent );
   //! Return a list of all the instructions in a recipe.
   QList<Instruction*> instructions( Recipe const* parent );
   //! Return a list of all the miscs in a recipe.
   QList<Misc*> miscs( Recipe const* parent );
   //! Return a list of all the waters in a recipe.
   QList<Water*> waters( Recipe const* parent );
   //! Return a list of all the salts in a recipe.
   QList<Salt*> salts( Recipe const* parent );
   //! Return a list of all the yeasts in a recipe.
   QList<Yeast*> yeasts( Recipe const* parent );
   //! Get recipe's equipment.
   Equipment* equipment(Recipe const* parent);
   //! Get the recipe's mash.
   Mash* mash( Recipe const* parent );
   //! Get recipe's style.
   Style* style(Recipe const* parent);
   Style* styleById(int styleId );
   //! Return a list of all the steps in a mash.
   QList<MashStep*> mashSteps(Mash const* parent);

   QString textFromValue(QVariant value, QString type);

   //! Get the file where this database was loaded from.
   static QString getDbFileName();

   /*!
    * Updates the Brewken-provided ingredients from the given sqlite
    * database file.
    */
   void updateDatabase(QString const& filename);

   //! \brief Figures out what databases we are copying to and from, opens what
   //   needs opens and then calls the appropriate workhorse to get it done.
   void convertDatabase(QString const& Hostname, QString const& DbName,
                        QString const& Username, QString const& Password,
                        int Portnum, Brewken::DBTypes newType);

   DatabaseSchema & getDatabaseSchema();

signals:
   void changed(QMetaProperty prop, QVariant value);
   void newEquipmentSignal(Equipment*);
   void newFermentableSignal(Fermentable*);
   void newHopSignal(Hop*);
   void newMashSignal(Mash*);
   void newMiscSignal(Misc*);
   void newRecipeSignal(Recipe*);
   void newStyleSignal(Style*);
   void newWaterSignal(Water*);
   void newSaltSignal(Salt*);
   void newYeastSignal(Yeast*);
   // This is still experimental. Or at least mental
   void newBrewNoteSignal(BrewNote*);

   void deletedSignal(Equipment*);
   void deletedSignal(Fermentable*);
   void deletedSignal(Hop*);
   void deletedSignal(Instruction*);
   void deletedSignal(Mash*);
   void deletedSignal(Misc*);
   void deletedSignal(Recipe*);
   void deletedSignal(Style*);
   void deletedSignal(Water*);
   void deletedSignal(Salt*);
   void deletedSignal(Yeast*);
   void deletedSignal(BrewNote*);
   void deletedSignal(MashStep*);

   // MashSteps need signals too
   void newMashStepSignal(MashStep*);

   // Sigh
   void changedInventory(DatabaseConstants::DbTableId,int,QVariant);

private slots:
   //! Load database from file.
   bool load();

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;

   //! Hidden constructor.
   Database();
   //! No copy constructor, as never want anyone, not even our friends, to make copies of a singleton
   Database(Database const&) = delete;
   //! No assignment operator , as never want anyone, not even our friends, to make copies of a singleton.
   Database& operator=(Database const&) = delete;
   //! Destructor hidden.
   ~Database();


};

#endif
