/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Guillermo Aguirre 
 */


#include "ns3/local-clock.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/localtime-simulator-impl.h"

/**
 * \file clock
 * ns3::LocalClock implementation 
 */

namespace ns3{ 

NS_LOG_COMPONENT_DEFINE ("LocalClock");

NS_OBJECT_ENSURE_REGISTERED (LocalClock);

TypeId
LocalClock::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LocalClock")
    .SetParent<Object> ()
    .SetGroupName ("Clock")
    .AddConstructor<LocalClock> ()
    .AddAttribute ("ClockModelImpl",
                  "The clock model implementation used to simulate local clock",
                  PointerValue (),
                  MakePointerAccessor (&LocalClock::m_clock),
                  MakePointerChecker<ClockModelImpl> ())
  ;
  return tid;
}

LocalClock::LocalClock ()
{
  NS_LOG_FUNCTION (this);
}

LocalClock::LocalClock (Ptr<ClockModelImpl> clock)
{
  NS_LOG_FUNCTION (this);
  m_clock = clock;
}

LocalClock::~LocalClock()
{
  NS_LOG_FUNCTION (this);
}

Time 
LocalClock::GetLocalTime ()
{
  NS_LOG_FUNCTION (this);
  return m_clock->GetLocalTime();
}

void 
LocalClock::SetClock (Ptr<ClockModelImpl> newClock)
{
    NS_LOG_FUNCTION (this << newClock);
    Ptr<ClockModelImpl> oldClock = m_clock;
    m_clock = newClock;

    //First run over the list to remove expire events. 

    for (std::list<EventId>::iterator iter = m_events.begin (); iter != m_events.end ();)
    { 
      if ((*iter).IsExpired ())
      {
        iter = m_events.erase (iter);    
      }
      else
      {
        iter++;
      }
    }

    std::list<EventId> eventListAux (m_events);
    m_events.clear ();

    for (std::list<EventId>::iterator iter = eventListAux.begin (); iter != eventListAux.end ();++iter)
  {
      Time eventTimeStamp = TimeStep((*iter).GetTs ()); 
      Ptr<SimulatorImpl> simImpl = Simulator::GetImplementation ();
      Ptr<LocalTimeSimulatorImpl> mysimImpl= DynamicCast<LocalTimeSimulatorImpl> (simImpl);

      if(mysimImpl == nullptr)
      {
        NS_LOG_WARN ("NOT USING THE CORRECT SIMULATOR IMPLEMENTATION");
      }
      EventId newID = ReSchedule (eventTimeStamp, (*iter).PeekEventImpl(), oldClock);
      mysimImpl -> CancelRescheduling (*iter, newID);
    }
}

Time 
LocalClock::GlobalToLocalTime (Time globalTime)
{
  NS_LOG_FUNCTION (this << globalTime);
  return m_clock->GlobalToLocalTime (globalTime);
}

Time 
LocalClock::LocalToGlobalTime (Time localTime)
{
  NS_LOG_FUNCTION (this << localTime);
  return m_clock->LocalToGlobalTime (localTime);
}

Time 
LocalClock::GlobalToLocalAbs (Time globalDelay)
{
  NS_LOG_FUNCTION (this << globalDelay);
  return m_clock->GlobalToLocalAbs (globalDelay);
}

Time 
LocalClock::LocalToGlobalAbs (Time localDelay)
{
  NS_LOG_FUNCTION (this << localDelay);
  return m_clock->LocalToGlobalAbs (localDelay);
}

void 
LocalClock::InsertEvent (EventId event)
{
  for (EventList::iterator i = m_events.begin (); i != m_events.end ();)
  {
    if (i->IsExpired ())
    {
      i = m_events.erase (i);
    }
    else
    {
      ++i;
    }

  }
  m_events.push_back (event);
}

EventId 
LocalClock::ReSchedule (Time globalTimeStamp, EventImpl *impl, Ptr<ClockModelImpl> oldclock)
{
  NS_LOG_FUNCTION (this << globalTimeStamp << impl);
  Time globalOldDurationRemain = globalTimeStamp - Simulator::Now ();
  Time localOldDurationRemain = oldclock->GlobalToLocalAbs (globalOldDurationRemain);
  return Simulator::Schedule (localOldDurationRemain, impl);
}
}//namespace ns3