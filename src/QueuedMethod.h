/**
 * QueuedMethod.h is part of Brewken, and is copyright the following authors 2009-2014:
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
#ifndef QUEUEDMETHOD_H
#define QUEUEDMETHOD_H
#pragma once

#include <QString>
#include <QGenericArgument>
#include <QGenericReturnArgument>
#include <QList>
#include <QThread>
#include <QSharedPointer>

/*!
 * \class QueuedMethod
 *
 * .:TODO:. AFAICT this is not used anywhere in the code and duplicates functionality in Qt, Boost, etc.  So we should delete.
 *
 * \brief Runs long methods in the background.
 *
 * This class allows you to queue any \em invokable function call that would
 * normally block so that it executes in the background. Have I duplicated the
 * functionality of QtConcurrent::run()?
 */
class QueuedMethod : public QThread
{
   Q_OBJECT
public:
   /*!
    * Note: may add more available arguments in future.
    *
    * \param startImmediately true if you want to immediately execute.
    *        Otherwise, call \b start() manually to begin.
    * \param arg0 is the first argument to the method.
    */
   QueuedMethod(QObject* obj, QString const& methodName,
                //QGenericReturnArgument ret,
                bool startImmediately = true,
                QGenericArgument arg0 = QGenericArgument(0) );
   virtual ~QueuedMethod();

   /*!
    * Chain the method call with \b other. I.e. when \b this finishes,
    * \b other will be started.
    * \returns \b other so you can do a->chainWith(b)->chainWith(c) which
    * executes a, then b, then c.
    */
   QSharedPointer<QueuedMethod> chainWith( QSharedPointer<QueuedMethod> other );

   /*!
    * Push a method onto the queue. When \b qm->done() is emitted, \b qm
    * will be destructed and dequeued. Only use this when qm is allocated
    * via the \b new operator. Maybe it's a bad name, but please note that
    * the order of enqueuing is not necessarily the order of execution. For
    * order control, see \b chainWith().
    */
   static void enqueue( QSharedPointer<QueuedMethod> qm );

protected:
   //! Reimplemented from QThread.
   void run();

signals:
   /*!
    * Emitted when the encapsulated function has completed.
    * \param success is return value of QMetaObject::invokeMethod().
    */
   void done(bool success);

public slots:

private slots:
   void executeFunction();
   void dequeueMyself();
   void startChained();

private:
   QSharedPointer<QueuedMethod> _chainedMethod;
   QObject* _obj;
   QString _methodName;
   //const char* _retName;
   //void* _retData;
   const char* _arg0Name;
   void* _arg0Data;
   bool success;

   static QList< QSharedPointer<QueuedMethod> > _queue;
};

#endif
