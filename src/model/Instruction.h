/*======================================================================================================================
 * model/Instruction.h is part of Brewken, and is copyright the following authors 2009-2023:
 *   • Brian Rower <brian.rower@gmail.com>
 *   • Jeff Bailey <skydvr38@verizon.net>
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
 =====================================================================================================================*/
#ifndef MODEL_INSTRUCTION_H
#define MODEL_INSTRUCTION_H
#pragma once

#include <memory> // For PImpl

#include <QDomNode>
#include <QString>
#include <QVector>

#include "model/NamedEntity.h"

//======================================================================================================================
//========================================== Start of property name constants ==========================================
// See comment in model/NamedEntity.h
#define AddPropertyName(property) namespace PropertyNames::Instruction { BtStringConst const property{#property}; }
AddPropertyName(completed )
AddPropertyName(directions)
AddPropertyName(hasTimer  )
AddPropertyName(interval  )
AddPropertyName(timerValue)
#undef AddPropertyName
//=========================================== End of property name constants ===========================================
//======================================================================================================================


/*!
 * \class Instruction
 *
 * \brief Model class for an instruction record in the database.
 *
 *        This class is completely outside the BeerXML spec.
 */
class Instruction : public NamedEntity {
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

   Instruction(QString name = "");
   Instruction(NamedParameterBundle const & namedParameterBundle);
   Instruction(Instruction const & other);

   virtual ~Instruction();

   Q_PROPERTY( QString directions READ directions WRITE setDirections /*NOTIFY changed*/ /*changedDirections*/ )
   Q_PROPERTY( bool hasTimer READ hasTimer WRITE setHasTimer /*NOTIFY changed*/ /*changedHasTimer*/ )
   Q_PROPERTY( QString timerValue READ timerValue WRITE setTimerValue /*NOTIFY changed*/ /*changedTimerValue*/ )
   Q_PROPERTY( bool completed READ completed WRITE setCompleted /*NOTIFY changed*/ /*changedCompleted*/ )
   Q_PROPERTY( double interval READ interval WRITE setInterval /*NOTIFY changed*/ /*changedInterval*/ )
   Q_PROPERTY( QList<QString> reagents READ reagents /*WRITE*/ /*NOTIFY changed*/ /*changedReagents*/ )

   Q_PROPERTY( int instructionNumber READ instructionNumber /*WRITE*/ /*NOTIFY changed*/ STORED false )

   // "set" methods.
   void setDirections(const QString& dir);
   void setHasTimer(bool has);
   void setTimerValue(const QString& timerVal);
   void setCompleted(bool comp);
   void setInterval(double interval);
   void addReagent(const QString& reagent);

   // "get" methods.
   QString directions();
   bool hasTimer();
   QString timerValue();
   bool completed();
   //! This is a non-stored temporary in-memory set.
   QList<QString> reagents();
   double interval();

   int instructionNumber() const;

   virtual Recipe * getOwningRecipe() const;

signals:

protected:
   virtual bool isEqualTo(NamedEntity const & other) const;
   virtual ObjectStore & getObjectStoreTypedInstance() const;

private:
   // Private implementation details - see https://herbsutter.com/gotw/_100/
   class impl;
   std::unique_ptr<impl> pimpl;

   QString m_directions;
   bool    m_hasTimer;
   QString m_timerValue;
   bool    m_completed;
   double  m_interval;

   QList<QString> m_reagents;
};

Q_DECLARE_METATYPE(QList<                Instruction *>)
Q_DECLARE_METATYPE(QList<std::shared_ptr<Instruction> >)

//! \brief Compares Instruction pointers by Instruction::instructionNumber().
inline bool insPtrLtByNumber( Instruction* lhs, Instruction* rhs)
{
   return lhs->instructionNumber() < rhs->instructionNumber();
}

#endif
