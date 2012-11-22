/* This file is part of Clementine.
   Copyright 2011, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "closure.h"

#include <QTimer>

#include "core/timeconstants.h"

namespace _detail {

ClosureBase::ClosureBase(ObjectHelper* helper)
    : helper_(helper) {
}

ClosureBase::~ClosureBase() {
}

ObjectHelper* ClosureBase::helper() const {
  return helper_;
}

ObjectHelper::ObjectHelper(
    QObject* sender,
    const char* signal,
    ClosureBase* closure)
  : closure_(closure) {
  connect(sender, signal, SLOT(Invoked()));
  connect(sender, SIGNAL(destroyed()), SLOT(deleteLater()));
}

void ObjectHelper::Invoked() {
  closure_->Invoke();
  deleteLater();
}

template<>
void Arg<boost::tuples::tuple<>>(
    const boost::tuples::tuple<>&,
    QList<QGenericArgument>* list) {}

template<>
void Arg<boost::tuples::null_type>(
    const boost::tuples::null_type&,
    QList<QGenericArgument>* list) {}

}  // namespace _detail

void DoAfter(QObject* receiver, const char* slot, int msec) {
  QTimer::singleShot(msec, receiver, slot);
}

void DoInAMinuteOrSo(QObject* receiver, const char* slot) {
  int msec = (60 + (qrand() % 60)) * kMsecPerSec;
  DoAfter(receiver, slot, msec);
}
