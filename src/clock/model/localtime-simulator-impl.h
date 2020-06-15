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
 * Author: Guillermo Aguirre <guillermoaguirre10@gmail.com> 
 */
#ifndef LOCALTIME_SIMULATOR_IMPL_H
#define LOCALTIME_SIMULATOR_IMPL_H

#include "ns3/default-simulator-impl.h"
#include "ns3/local-clock.h"
#include <map>




/**
 * \file
 * \ingroup simulator
 * ns3::LocalTimetSimulatorImpl declaration.
 */

namespace ns3{

/**
 *  \ingroup simulator
 * 
 * @brief Implementation of single process simulator based on node local time.
 * The main differences between LocalTimeSimulatorImpl and DefaultSimulatorImpl are: 

    *  When an event is scheduled using Simulator::Schedule() function, the delay is understood as being a local-time delay. 
    The delay is then translated into a global-time delay before being inserted into the scheduler (the scheduler only operates in the global-time domain.
    * When a clock model is updated, LocalTimeSimulatorImpl keeps track of the events that have been rescheduled, and will not execute the old events.

  When Simulator::Schedule() is called, the Node object  is retrieved from the NodeList using the current context of the simulator. 
  When the context do not correspond to any node, delays are considered to be in global time.

  When Simulator::ScheduleWithContext (uint32_t context, const Time &delay, EventImpl *event) no further actions are taken. the delay is considered 
  to be in global time. This is because when a call to ScheduleWithContext happen, is due to a packet transmission within the channel. 
  The delay in that call is normally the channel transmission time (As in CsmaNetDevice), which does not depend on the nodes clock. Because of that, 
  this function remain unchanged in comparison with the DefaultSimulatorImpl function.

  When Simulator::ScheduleWithContext() is called, the same actions as in DefaultSimulatorImpl are taken. 
  The delay is understood as being already a global-time delay.

  When Simulator::ScheduleNow() is called, a call to Schedule() function with local-time delay 0 is done.

  One of the things to take into account is that Simulator::Now()  returns the current global time and not the local time.
  Another particularity of this implementation resides in the CancelEventsMap map located in LocalTimeSimulatorImpl. When events are 
  rescheduled, old events id (as key) with their corresponding new events (as value) are inserted in this map. As explained in the LocalClock 
  section, we cannot use  Simulator::Cancel() to remove old events or their implementation would be lost.
  In order not to execute events that should not be invoked (because the execution time attach to the event does not correspond to the new clock of
  the node), ProcessOneEvent() function is slightly modifyied and checks the CancelEventsMap. 
  If the EventId that is going to be executed is found as the map key, the event is skipped.

  Other problem arises from the fact that the original event is never again valid, when rescheduling events. Any process (i.e Applications) that 
  schedule events will never realize about the change of EventId due to the rescheduling. Therefore, there is a need to map between the original 
  events and the reschedule events.
 * 
 */

class LocalTimeSimulatorImpl : public SimulatorImpl
{
public:
  /**
   *  Register this type.
   *  \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Contructor. */
  LocalTimeSimulatorImpl ();
  /** Destructor. */
  ~LocalTimeSimulatorImpl();

  // Inherited
    virtual void Destroy ();
    virtual bool IsFinished (void) const;
    virtual void Stop (void);
    virtual void Stop (const Time &delay);
    virtual void ScheduleWithContext (uint32_t context, const Time &delay, EventImpl *event);
    virtual EventId ScheduleNow (EventImpl *event);
    virtual EventId ScheduleDestroy (EventImpl *event);
    virtual void Remove (const EventId &id);
    virtual void Run (void);
    virtual Time Now (void) const;
    virtual Time GetDelayLeft (const EventId &id) const;
    virtual Time GetMaximumSimulationTime (void) const;
    virtual void SetScheduler (ObjectFactory schedulerFactory);
    virtual uint32_t GetSystemId (void) const;
    virtual uint32_t GetContext (void) const;
    virtual uint64_t GetEventCount (void) const;
    virtual void Cancel (const EventId &id);

    virtual void DoDispose (void);

    
  /**
   *  \brief This function apart from what it provides in DefaultSimulatorImpl, provides also a way to chech if events that have been reschedule due
   * to a clock update are expired.
   *  
   * \param id Event id to check
   */
  virtual bool IsExpired (const EventId &id) const;
  /**
   *  \brief This function provides a mechanism to translate the delay which is based on the node local time. 
   * The node is obtained through the node list and the current context.
   * 
   * \param delay Delay in time measured in localtime
   * \param event Event implementation
   */
  virtual EventId Schedule (const Time &delay, EventImpl *event);

  /**
   * \brief This function is only call when the node  reschedule an event. Instead of calling to cancel() it shoudl call to
   * CancelRescheduling. Cancel() method will invalidate the event implementation and would be impossible to reschedule the event with another time. 
   * 
   * \param id Event Id that want to be removed.
  */
  void CancelRescheduling (const EventId &id, const EventId &newId);

private:

  /** \brief Process the next event. Check if the event to invoke is one of the events that is been 
   * canceled by the clock update function. We  don't invoke those events. This is done in order to maintain the event implementation. 
   */
  void ProcessOneEvent (void);
  /** Move events from a different context into the main event queue. */
  void ProcessEventsWithContext (void);
  /** Function that insert and event in the scheduler */
  Scheduler::Event InsertScheduler (EventImpl *impl, Time tAbsolute);
  /** Calculate absoulte time*/
  Time CalculateAbsoluteTime (Time delay);
 
  /** Wrap an event with its execution context. */
  struct EventWithContext {
    /** The event context. */
    uint32_t context;
    /** Event timestamp. */
    uint64_t timestamp;
    /** The event implementation. */
    EventImpl *event;
  };
  /** Container type for the events from a different context. */
  typedef std::list<struct EventWithContext> EventsWithContext;
  /** The container of events from a different context. */
  EventsWithContext m_eventsWithContext;
  /**
   * Flag \c true if all events with context have been moved to the
   * primary event queue.
   */
  bool m_eventsWithContextEmpty;
  /** Mutex to control access to the list of events with context. */
  SystemMutex m_eventsWithContextMutex;

  /** Container type for the events to run at Simulator::Destroy() */
  typedef std::list<EventId> DestroyEvents;
  /** The container of events to run at Destroy. */
  DestroyEvents m_destroyEvents;
  /** Flag calling for the end of the simulation. */
  bool m_stop;
  /** The event priority queue. */
  Ptr<Scheduler> m_events;

  /** Next event unique id. */
  uint32_t m_uid;
  /** Unique id of the current event. */
  uint32_t m_currentUid;
  /** Timestamp of the current event. */
  uint64_t m_currentTs;
  /** Execution context of the current event. */
  uint32_t m_currentContext;
  /** The event count. */
  uint64_t m_eventCount;
  /** Container type for the events that has been cancelled due to rescheduling. Map between the old event id and the new event that has been 
   * reschedule
  */
  typedef std::map<uint32_t, EventId> CancelEventsMap;

  CancelEventsMap m_cancelEventMap;

  /**
   * Number of events that have been inserted but not yet scheduled,
   *  not counting the Destroy events; this is used for validation
   */
  int m_unscheduledEvents;

  /** Main execution thread. */
  SystemThread::ThreadId m_main;
};

}// namespace ns3

#endif /* LOCALTIME_SIMULATOR_IMPL_H */

