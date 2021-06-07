/**
 * model/Instruction.cpp is part of Brewken, and is copyright the following authors 2009-2021:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Matt Young <mfsy@yahoo.com>
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
 */
#include "model/Instruction.h"

#include "Brewken.h"
#include "database/Database.h"
#include "database/InstructionSchema.h"
#include "database/TableSchemaConst.h"

// This private implementation class holds all private non-virtual members of Instruction
class Instruction::impl {
public:

   /**
    * Constructor
    */
   impl(Instruction & instruction) :
      instruction{instruction},
      recipe{} {
      return;
   }

   /**
    * Destructor
    */
   ~impl() = default;

   std::shared_ptr<Recipe> getRecipe() {
      // If we already know which recipe we're in, we just return that...
      if (this->recipe) {
         return this->recipe;
      }

      // ...otherwise we have to ask the recipe object store to find our recipe
      auto result = DbNamedEntityRecords<Recipe>::getInstance().findFirstMatching(
         [this](std::shared_ptr<Recipe> rec) {return rec->uses(instruction);}
      );

      if (!result.has_value()) {
         qCritical() << Q_FUNC_INFO << "Unable to find Recipe for Instruction #" << this->instruction.key();
         return nullptr;
      }

      this->recipe = result.value();

      return result.value();
   }

private:
   Instruction & instruction;
   std::shared_ptr<Recipe> recipe;
};

bool Instruction::isEqualTo(NamedEntity const & other) const {
   // Base class (NamedEntity) will have ensured this cast is valid
   Instruction const & rhs = static_cast<Instruction const &>(other);
   // Base class will already have ensured names are equal
   return (
      this->m_directions == rhs.m_directions &&
      this->m_hasTimer   == rhs.m_hasTimer   &&
      this->m_timerValue == rhs.m_timerValue
   );
}

DbRecords & Instruction::getDbNamedEntityRecordsInstance() const {
   return DbNamedEntityRecords<Instruction>::getInstance();
}


QString Instruction::classNameStr()
{
   static const QString name("Instruction");
   return name;
}

Instruction::Instruction(DatabaseConstants::DbTableId table, int key)
   : NamedEntity(table, key, QString(), true),
   pimpl{new impl{*this}},
     m_directions(QString()),
     m_hasTimer  (false),
     m_timerValue(QString()),
     m_completed (false),
     m_interval  (0.0),
     m_cacheOnly(false) {
   return;
}

Instruction::Instruction(Instruction const & other) :
     NamedEntity(other),
   pimpl{new impl{*this}},
     m_directions{other.m_directions},
     m_hasTimer  {other.m_hasTimer  },
     m_timerValue{other.m_timerValue},
     m_completed {other.m_completed },
     m_interval  {other.m_interval  },
     m_cacheOnly {other.m_cacheOnly } {
   return;
}

Instruction::Instruction(QString name, bool cache)
   : NamedEntity(DatabaseConstants::INSTRUCTIONTABLE, -1, name, true),
   pimpl{new impl{*this}},
     m_directions(QString()),
     m_hasTimer  (false),
     m_timerValue(QString()),
     m_completed (false),
     m_interval  (0.0),
     m_cacheOnly(cache) {
   return;
}

Instruction::Instruction(NamedParameterBundle & namedParameterBundle) :
   NamedEntity{namedParameterBundle, DatabaseConstants::INSTRUCTIONTABLE},
   pimpl{new impl{*this}},
   m_directions{namedParameterBundle(PropertyNames::Instruction::directions).toString()},
   m_hasTimer  {namedParameterBundle(PropertyNames::Instruction::hasTimer  ).toBool()},
   m_timerValue{namedParameterBundle(PropertyNames::Instruction::timerValue).toString()},
   m_completed {namedParameterBundle(PropertyNames::Instruction::completed ).toBool()},
   m_interval  {namedParameterBundle(PropertyNames::Instruction::interval  ).toDouble()},
   m_cacheOnly {false} {
   return;
}

Instruction::Instruction(DatabaseConstants::DbTableId table, int key, QSqlRecord rec)
   : NamedEntity(table, key, rec.value(kcolName).toString(), rec.value(kcolDisplay).toBool() ),
   pimpl{new impl{*this}},
     m_directions(rec.value(kcolInstructionDirections).toString()),
     m_hasTimer  (rec.value(kcolInstructionHasTimer).toBool()),
     m_timerValue(rec.value(kcolInstructionTimerValue).toString()),
     m_completed (rec.value(kcolInstructionCompleted).toBool()),
     m_interval  (rec.value(kcolInstructionInterval).toDouble()),
     m_cacheOnly(false) {
   return;
}

// See https://herbsutter.com/gotw/_100/ for why we need to explicitly define the destructor here (and not in the
// header file)
Instruction::~Instruction() = default;

// Setters ====================================================================
void Instruction::setDirections(const QString& dir)
{
   m_directions = dir;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Instruction::directions,  dir);
   }
}

void Instruction::setHasTimer(bool has)
{
   m_hasTimer = has;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Instruction::hasTimer,  has);
   }
}

void Instruction::setTimerValue(const QString& timerVal)
{
   m_timerValue = timerVal;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Instruction::timerValue,  timerVal);
   }
}

void Instruction::setCompleted(bool comp)
{
   m_completed = comp;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Instruction::completed,  comp);
   }
}

// TODO: figure out.
/*
void Instruction::setReagent(const QString& reagent)
{
   reagents.push_back(QString(reagent));
}
*/

void Instruction::setInterval(double time)
{
   m_interval = time;
   if ( ! m_cacheOnly ) {
      setEasy(PropertyNames::Instruction::interval,  time);
   }
}

void Instruction::addReagent(const QString& reagent)
{
   m_reagents.append(reagent);
}

void Instruction::setCacheOnly(bool cache) { m_cacheOnly = cache; }
// Accessors ==================================================================
QString Instruction::directions() { return m_directions; }

bool Instruction::hasTimer() { return m_hasTimer; }

QString Instruction::timerValue() { return m_timerValue; }

bool Instruction::completed() { return m_completed; }

QList<QString> Instruction::reagents() { return m_reagents; }

double Instruction::interval() { return m_interval; }

int Instruction::instructionNumber() const {
   return this->pimpl->getRecipe()->instructionNumber(*this);
}

bool Instruction::cacheOnly() { return m_cacheOnly; }
