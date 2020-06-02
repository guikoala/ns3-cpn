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
 * This class slighly differ from the default simulator implementation.
 * The purpose of this class is to provide to the simulator the capanility to manage different timing references
 * within the nodes of the network.
 * This class provide the posibility to schedule events with different notion of time and to reschedule events
 * when clock update happens during the simulation.
 * For that a new functionality is provide to the Schedule function. Note that ScheduleWithContext function remain the same a in the defaultsimulatorImpl.
 * This is because in the mayority of the times a calle to ScheduleWithContext happen, is due to a packet transmission within the channel. The delay that 
 * is introduced in that call is normally the channel transmission time (As in CsmaNetDevice), which doesn't depend on the node clock. Because of that, 
 * this function remain unchanged. 
 * 
 * Schedule function allows to retrieve from the conext the node that is scheduling the events. After retrieving the node, the Localclock object 
 * is accessed trhough the aggregation system. This new functionality, is just possible if a LocalClock object has been aggregated before to the node.
 * It permits to attach to each in the simulation different time notions.
 * Context with number 4294967295 that are set after application stop events are skiped.
 * 
 *  This class also provide the posibilty to the nodes to cancel events due to a clock update event. Imagine that a node receive a clock update 
 * message. All the events that has been scheduled by that node need to be reschedule at the proper time. This class maintains a list of cancel events 
 * by the nodes. Node don't cancell the events directly because if Simulator::Cancel () or Simulator::Destory() there is no way to recover the event 
 * implementation. This has been done in this way becuase the base code of EventImpl doesn't provide a copy contructor. 
 * So, ones the eventImpl is cancell there is no way that the event can be reschedule. 
 * To over come from that situation, the simulator maintains a list with all the cancelled events by the nodes. When procesing an event, the simulator checks
 * first if the event has been cancelled. If yes it discard the event, if not it execute.
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
    virtual bool IsExpired (const EventId &id) const;
    virtual void Run (void);
    virtual Time Now (void) const;
    virtual Time GetDelayLeft (const EventId &id) const;
    virtual Time GetMaximumSimulationTime (void) const;
    virtual void SetScheduler (ObjectFactory schedulerFactory);
    virtual uint32_t GetSystemId (void) const;
    virtual uint32_t GetContext (void) const;
    virtual uint64_t GetEventCount (void) const;

    virtual void DoDispose (void);

    


  /**
   *  \brief This function provides a mechanism to translate the delay which is based on the node local time. 
   * The node implementation is obtained through the node list and the current context.
   * 
   * \param delay Delay in time measured in localtime
   * \param event Event implementation
   */
  virtual EventId Schedule (const Time &delay, EventImpl *event);
  /**
  * \brief Every time a there is a cancel call ensure if the cancel comes from a rescheduling event 
  * due to a clock change. If the cancel call is due to a rescehduling, we insert those events 
  * in m_eventCancelation but we dont set the flag m_cancel on eventImpl so we can reschedule afterards.
  * 
  * \param 
  */
  virtual void Cancel (const EventId &id);

  /**
   * \brief This function is only call when the node is when a node whats to reschedule a function. Instead of calling to cancel() it shoudl call to
   * CancelRescheduling. Cancel() method will invalidate the event implementation and would be impossible to reschedule the event with anoter time. 
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
  /** Container type for the events that has been cancelled due to rescheduling */
  typedef std::list<EventId> CancelEvents;
  /** List of events cancelled due to rescheduling */
  CancelEvents m_eventCancelation;

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

