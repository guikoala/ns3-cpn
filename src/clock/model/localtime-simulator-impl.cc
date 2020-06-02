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


#include "localtime-simulator-impl.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/assert.h"
#include "ns3/node-list.h"
#include "ns3/node.h"


   

/**
 * \file
 * \ingroup simulator
 * ns3::LocalTimeSimulatorImpl implementation
 */

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("LocalTimeSimulatorImpl");

NS_OBJECT_ENSURE_REGISTERED (LocalTimeSimulatorImpl);

TypeId
LocalTimeSimulatorImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LocalTimeSimulatorImpl")
    .SetParent<DefaultSimulatorImpl> ()
    .SetGroupName ("Core")
    .AddConstructor<LocalTimeSimulatorImpl> ()
  ;
  return tid;
}

LocalTimeSimulatorImpl::LocalTimeSimulatorImpl()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Local Time Simulator implementation created");
  m_stop = false;
  // uids are allocated from 4.
  // uid 0 is "invalid" events
  // uid 1 is "now" events
  // uid 2 is "destroy" events
  m_uid = 4;
  // before ::Run is entered, the m_currentUid will be zero
  m_currentUid = 0;
  m_currentTs = 0;
  m_currentContext = Simulator::NO_CONTEXT;
  m_unscheduledEvents = 0;
  m_eventCount = 0;
  m_eventsWithContextEmpty = true;
  m_main = SystemThread::Self();
}

LocalTimeSimulatorImpl::~LocalTimeSimulatorImpl ()
{
  NS_LOG_FUNCTION (this);
}


void
LocalTimeSimulatorImpl::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  ProcessEventsWithContext ();

  while (!m_events->IsEmpty ())
    {
      Scheduler::Event next = m_events->RemoveNext ();
      next.impl->Unref ();
    }
  m_events = 0;
  SimulatorImpl::DoDispose ();
}
void
LocalTimeSimulatorImpl::Destroy ()
{
  NS_LOG_FUNCTION (this);
  while (!m_destroyEvents.empty ()) 
    {
      Ptr<EventImpl> ev = m_destroyEvents.front ().PeekEventImpl ();
      m_destroyEvents.pop_front ();
      NS_LOG_LOGIC ("handle destroy " << ev);
      if (!ev->IsCancelled ())
        {
          ev->Invoke ();
        }
    }
}

void
LocalTimeSimulatorImpl::SetScheduler (ObjectFactory schedulerFactory)
{
  NS_LOG_FUNCTION (this << schedulerFactory);
  Ptr<Scheduler> scheduler = schedulerFactory.Create<Scheduler> ();

  if (m_events != 0)
    {
      while (!m_events->IsEmpty ())
        {
          Scheduler::Event next = m_events->RemoveNext ();
          scheduler->Insert (next);
        }
    }
  m_events = scheduler;
}

// System ID for non-distributed simulation is always zero
uint32_t 
LocalTimeSimulatorImpl::GetSystemId (void) const
{
  return 0;
}

void
LocalTimeSimulatorImpl::ProcessOneEvent (void)
{
  NS_LOG_FUNCTION (this); 

  Scheduler::Event next = m_events->RemoveNext ();

  //Do not process events that have been cancelled by a node due to clock update
  CancelEventsMap::iterator it = m_cancelEventMap.begin();
  while(it != m_cancelEventMap.end ())
  {
    if (it ->first == next.key.m_uid)
    {
      m_unscheduledEvents--;
      m_cancelEventMap.erase (it);
      return;
    }
    it++;
  }

  NS_ASSERT (next.key.m_ts >= m_currentTs);
  m_unscheduledEvents--;
  m_eventCount++;

  NS_LOG_LOGIC ("handle " << next.key.m_ts);
  m_currentTs = next.key.m_ts;

  //avoid 4294967295 context
  if ( next.key.m_context == uint32_t(4294967295))
  {
    NS_LOG_DEBUG ("The context doens't correspond to a node -> Application stop event");
  }
  else
  {
    m_currentContext = next.key.m_context;
  }
  
  m_currentUid = next.key.m_uid;
  NS_LOG_DEBUG ("EXECUTING EVENT ID " << m_currentUid);
  next.impl->Invoke ();
  next.impl->Unref ();
  ProcessEventsWithContext ();
}

bool 
LocalTimeSimulatorImpl::IsFinished (void) const
{
  return m_events->IsEmpty () || m_stop;
}

void
LocalTimeSimulatorImpl::ProcessEventsWithContext (void)
{
  if (m_eventsWithContextEmpty)
    {
      return;
    }

  // swap queues
  EventsWithContext eventsWithContext;
  {
    CriticalSection cs (m_eventsWithContextMutex);
    m_eventsWithContext.swap(eventsWithContext);
    m_eventsWithContextEmpty = true;
  }
  while (!eventsWithContext.empty ())
    {
       EventWithContext event = eventsWithContext.front ();
       eventsWithContext.pop_front ();
       Scheduler::Event ev;
       ev.impl = event.event;
       ev.key.m_ts = m_currentTs + event.timestamp;
       ev.key.m_context = event.context;
       ev.key.m_uid = m_uid;
       m_uid++;
       m_unscheduledEvents++;
       m_events->Insert (ev);
    }
}

void
LocalTimeSimulatorImpl::Run (void)
{
  NS_LOG_FUNCTION (this);
  // Set the current threadId as the main threadId
  m_main = SystemThread::Self();
  ProcessEventsWithContext ();
  m_stop = false;

  while (!m_events->IsEmpty () && !m_stop) 
    {
      ProcessOneEvent ();
    }
  // If the simulator stopped naturally by lack of events, make a
  // consistency test to check that we didn't lose any events along the way.
  NS_ASSERT (!m_events->IsEmpty () || m_unscheduledEvents == 0);
}

void 
LocalTimeSimulatorImpl::Stop (void)
{
  NS_LOG_FUNCTION (this);
  m_stop = true;
}

void 
LocalTimeSimulatorImpl::Stop (Time const &delay)
{
  NS_LOG_FUNCTION (this << delay.GetTimeStep ());
  Simulator::Schedule (delay, &Simulator::Stop);
}

EventId
LocalTimeSimulatorImpl::Schedule (Time const &localDelay, EventImpl *event)
{
  NS_LOG_INFO (this << localDelay.GetTimeStep () << event);
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::Schedule Thread-unsafe invocation!");
  Time tAbsolute;

  //Avoid stop application events with 4294967295 context
  if ( m_currentContext == uint32_t(4294967295))
  {
    tAbsolute = CalculateAbsoluteTime (localDelay);
  }
  else
  {
    // Obtain nodes clock from the context
    Ptr <Node>  n = NodeList::GetNode (m_currentContext);
    Ptr <LocalClock> clock = n -> GetObject <LocalClock> ();
    Time globalTimeDelay = clock -> LocalToGlobalAbs (localDelay);
    tAbsolute = CalculateAbsoluteTime (globalTimeDelay);
    //Insert eventId in the list of scheduled events by the node.
    EventId eventId = EventId(event, tAbsolute.GetTimeStep (), GetContext (), m_uid);
    Time localTimeStamp = clock -> GetLocalTime () + localDelay;
    clock -> InsertEvent (eventId);
  }

  Scheduler::Event ev = InsertScheduler (event,tAbsolute);
  EventId eventId = EventId (event, ev.key.m_ts, ev.key.m_context, ev.key.m_uid);
  NS_LOG_DEBUG ("SCHEDULING EVENT" << eventId.GetUid () << "AT TIME " << ev.key.m_ts);
  return eventId;
}

Scheduler::Event 
LocalTimeSimulatorImpl::InsertScheduler (EventImpl *event, Time tAbsolute)
{
  Scheduler::Event ev;
  ev.impl = event;
  ev.key.m_ts = (uint64_t) tAbsolute.GetTimeStep ();
  ev.key.m_context = GetContext ();
  ev.key.m_uid = m_uid;
  m_uid++;
  m_unscheduledEvents++;
  m_events->Insert (ev);
  return ev;
}

Time
LocalTimeSimulatorImpl::CalculateAbsoluteTime (Time delay)
{ 
  NS_ASSERT_MSG (delay.IsPositive (), "DefaultSimulatorImpl::Schedule(): Negative delay");
  Time tAbsolute = delay + TimeStep (m_currentTs);
  return tAbsolute;
}
void
LocalTimeSimulatorImpl::ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event)
{
  NS_LOG_FUNCTION (this << context << delay.GetTimeStep () << event);
    
  Time time = Simulator::Now() + delay ;

  if (SystemThread::Equals (m_main))
    {
      Time tAbsolute = delay + TimeStep (m_currentTs);
      Scheduler::Event ev;
      ev.impl = event;
      ev.key.m_ts = (uint64_t) tAbsolute.GetTimeStep ();
      ev.key.m_context = context;
      ev.key.m_uid = m_uid;
      m_uid++;
      m_unscheduledEvents++;
      m_events->Insert (ev);
    }
  else
    {
      EventWithContext ev;
      ev.context = context;
      // Current time added in ProcessEventsWithContext()
      ev.timestamp = delay.GetTimeStep ();
      ev.event = event;
      {
        CriticalSection cs (m_eventsWithContextMutex);
        m_eventsWithContext.push_back(ev);
        m_eventsWithContextEmpty = false;
      }
    }
}

EventId
LocalTimeSimulatorImpl::ScheduleNow (EventImpl *event)
{
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::ScheduleNow Thread-unsafe invocation!");

  Ptr <Node>  n = NodeList::GetNode (m_currentContext);
  Ptr <LocalClock> clock = n -> GetObject <LocalClock> ();
  Time globalTime = clock->LocalToGlobalTime (Now ());
  Scheduler::Event ev;
  ev.impl = event;
  ev.key.m_ts = globalTime.GetTimeStep ();
  ev.key.m_context = GetContext ();
  ev.key.m_uid = m_uid;
  m_uid++;
  m_unscheduledEvents++;
  m_events->Insert (ev);
  EventId eventId = EventId (event, ev.key.m_ts, ev.key.m_context, ev.key.m_uid);
  clock -> InsertEvent (eventId);
  return eventId;
}

EventId
LocalTimeSimulatorImpl::ScheduleDestroy (EventImpl *event)
{
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::ScheduleDestroy Thread-unsafe invocation!");

  EventId id (Ptr<EventImpl> (event, false), m_currentTs, 0xffffffff, 2);
  m_destroyEvents.push_back (id);
  m_uid++;
  return id;
}
Time
LocalTimeSimulatorImpl::Now (void) const
{
  // Do not add function logging here, to avoid stack overflow
  return TimeStep (m_currentTs);
}

Time 
LocalTimeSimulatorImpl::GetDelayLeft (const EventId &id) const
{
  if (IsExpired (id))
    {
      return TimeStep (0);
    }
  else
    {
      return TimeStep (id.GetTs () - m_currentTs);
    }
}

void
LocalTimeSimulatorImpl::Remove (const EventId &id)
{
  if (id.GetUid () == 2)
    {
      // destroy events.
      for (DestroyEvents::iterator i = m_destroyEvents.begin (); i != m_destroyEvents.end (); i++)
        {
          if (*i == id)
            {
              m_destroyEvents.erase (i);
              break;
            }
        }
      return;
    }
  if (IsExpired (id))
    {
      return;
    }
  Scheduler::Event event;
  event.impl = id.PeekEventImpl ();
  event.key.m_ts = id.GetTs ();
  event.key.m_context = id.GetContext ();
  event.key.m_uid = id.GetUid ();
  m_events->Remove (event);
  event.impl->Cancel ();
  // whenever we remove an event from the event list, we have to unref it.
  event.impl->Unref ();
  m_unscheduledEvents--;
}

void 
LocalTimeSimulatorImpl::Cancel (const EventId &id)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("CANCELING EVENT ID " << id.GetUid ());
  if (!IsExpired (id))
    {
      id.PeekEventImpl ()->Cancel ();
    }
}

void
LocalTimeSimulatorImpl::CancelRescheduling (const EventId &id, const EventId &newId)
{
  NS_LOG_FUNCTION (this);
  if (!IsExpired (id))
    {
      m_eventCancelation.push_back (id);    
      m_cancelEventMap.insert (std::make_pair (id.GetUid (), newId));
      NS_LOG_DEBUG ("EVENT CANCEL BECAUSE RESCHEDULING " << id.GetUid ());
    }
}

bool
LocalTimeSimulatorImpl::IsExpired (const EventId &id) const
{
  if (id.GetUid () == 2)
    {
      if (id.PeekEventImpl () == 0 ||
          id.PeekEventImpl ()->IsCancelled ())
        {
          return true;
        }
      // destroy events.
      for (DestroyEvents::const_iterator i = m_destroyEvents.begin (); i != m_destroyEvents.end (); i++)
        {
          if (*i == id)
            {
              return false;
            }
        }
      return true;
    }
  CancelEventsMap::const_iterator it = m_cancelEventMap.begin();
  uint64_t realExecTime;
  while(it != m_cancelEventMap.end ())
  {
    if (it ->first == id.GetUid ())
    {
      realExecTime = it->second.GetTs ();
      if (m_currentTs < realExecTime)
      {
        return false;
      }
      else if (m_currentTs == realExecTime)
      {
        return true;
      }
      else if (m_currentTs > realExecTime)
      {
        return true;
      }
    }
    it++;
  }
  
  if (id.PeekEventImpl () == 0 ||
      id.GetTs () < m_currentTs ||
      (id.GetTs () == m_currentTs &&
       id.GetUid () <= m_currentUid) ||
      id.PeekEventImpl ()->IsCancelled ()) 
    {
      return true;
    }
  else
    {
      return false;
    }
}

Time 
LocalTimeSimulatorImpl::GetMaximumSimulationTime (void) const
{
  return TimeStep (0x7fffffffffffffffLL);
}

uint32_t
LocalTimeSimulatorImpl::GetContext (void) const
{
  return m_currentContext;
}

uint64_t
LocalTimeSimulatorImpl::GetEventCount (void) const
{
  return m_eventCount;
}


}// namespace ns