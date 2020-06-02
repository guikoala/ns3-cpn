/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/test.h"
#include "ns3/localtime-simulator-impl.h"
#include "ns3/local-clock.h"
#include "ns3/perfect-clock-model-impl.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/core-module.h"

using namespace ns3;


class EventSchedulTestCase : public TestCase
{
public:
  EventSchedulTestCase (std::string descr, ObjectFactory schedulerFactory);

  void EventA (Time localTime, Time globalTime);
  void EventB (Time globalTime);
  void EventC ();

  void CreateNode ();
  void Send (Time t, Time checkTime);
  void ScheduleCheck (Time globalTime);
  void Eventfoo0 (void);
  void NewFrequency (double freq, Time offset);
  void destroy (void);
  void Expired (EventId id);
  void NotExpired (EventId id);
  bool m_b;
  bool m_a;
  bool m_destroy;
  EventId m_destroyId;
  
  virtual ~EventSchedulTestCase ();
  virtual void DoRun (void);
  virtual void DoSetup ();
 
  Ptr<Node> m_node;
  Ptr<ClockModelImpl> m_clock;
  Ptr<LocalClock> clock0;
  ObjectFactory m_schedulerFactory;

};

/**
* This test aim to check that events are scheduled according to a global time shifted 
* from a local time.  
* This test work just with the perfect clock impl, however is not the purpose of this test to evaluate the perfomance of the 
* clock implementation itself. The purpose is to validate that the node is able to schedule event with it's own notion of time. 
* This test schedule 2 events, it checks that the first event is scheduled according to
* node clock freqeuncy. Second event is rescheduled due to a clock change. The test checks 
* that the event has been rescheduled in the proper time. 
*/

EventSchedulTestCase::EventSchedulTestCase (
  std::string descr,
  ObjectFactory schedulerFactory
)
: TestCase ("Check that basic event handling is working with " + 
              schedulerFactory.GetTypeId ().GetName ()),
    m_schedulerFactory (schedulerFactory)
{
}
EventSchedulTestCase::~EventSchedulTestCase ()
{
}

void 
EventSchedulTestCase::EventA (Time localTime, Time globalTime)
{
  m_a = true;
}

void 
EventSchedulTestCase::EventB (Time globalTime)
{
  ScheduleCheck (globalTime);
  m_b = true;
}

void
EventSchedulTestCase::EventC ()
{
  std::cout << "EXECUTE EVENT C AT TIME : " << Simulator::Now () << std::endl;
}

void
EventSchedulTestCase::Eventfoo0 (void)
{
  EventId id = Simulator::Schedule (Seconds (3), &EventSchedulTestCase::EventC, this);
  //Check that the scheduled events that have been reschedule are still active even if their time has elapsed.
  Simulator::ScheduleWithContext (0, Time (id.GetTs ())-Simulator::Now (), &EventSchedulTestCase::NotExpired, this, id);
}

void 
EventSchedulTestCase::Send (Time t, Time checkTime)
{
  Simulator::Schedule (t, &EventSchedulTestCase::EventB, this, checkTime);
}

void
EventSchedulTestCase::Expired (EventId id)
{
  NS_TEST_ASSERT_MSG_EQ (id.IsExpired (), true, "Should have expired");
}

void
EventSchedulTestCase::NotExpired (EventId id)
{
  NS_TEST_ASSERT_MSG_EQ (!id.IsExpired (), true, "Should have not expired");
}

void
EventSchedulTestCase::CreateNode ()
{
  m_node = CreateObject<Node> ();
  clock0 = CreateObject<LocalClock> ();
  m_clock = CreateObject<PerfectClockModelImpl> ();
  m_clock -> SetAttribute ("Frequency", DoubleValue (2));
  m_clock -> SetAttribute ("Offset", TimeValue (Seconds (0)));
  clock0 -> SetAttribute ("ClockModelImpl", PointerValue (m_clock));
  m_node -> AggregateObject (clock0);
}

void 
EventSchedulTestCase::ScheduleCheck (Time globalTime)
{
  std::cout << " Expected at "  << globalTime << "(sim/node)" << std::endl;
  NS_TEST_ASSERT_MSG_EQ (globalTime, Simulator::Now (), "Wrong global time");
}

void
EventSchedulTestCase::NewFrequency (double freq, Time offset)
{
  std::cout << "Event New freq at " << Simulator::Now () << std::endl;
  Ptr<ClockModelImpl> newClock = CreateObject<PerfectClockModelImpl> ();
  newClock -> SetAttribute ("Frequency", DoubleValue (freq));
  newClock -> SetAttribute ("Offset", TimeValue (offset));
  clock0 -> SetClock (newClock);
}

void
EventSchedulTestCase::destroy (void)
{
  if (m_destroyId.IsExpired ())
    {
      m_destroy = true; 
    }
}
void
EventSchedulTestCase::DoSetup ()
{  
  GlobalValue::Bind ("SimulatorImplementationType", 
                     StringValue ("ns3::LocalTimeSimulatorImpl"));
  CreateNode ();
  uint32_t id = 0;
  m_a = false;
  m_b = false;
  double freq = 4;
  Time offset = Seconds (0);

  Simulator::SetScheduler (m_schedulerFactory);
 //These events are scheduled without the clock freq, because are scheduled before run

  Simulator::ScheduleWithContext (id, Seconds (1), &EventSchedulTestCase::EventA, this, Seconds (1), Seconds (2));
  EventId a = Simulator::Schedule (Seconds (2), &EventSchedulTestCase::Send, this, Seconds (1), Seconds (4));
  EventId b = Simulator::Schedule (Seconds (2), &EventSchedulTestCase::Send, this, Seconds (2), Seconds (6));
  EventId c = Simulator::Schedule (Seconds (3), &EventSchedulTestCase::Send, this, Seconds (3), Seconds (11));

  EventId d1 = Simulator::Schedule (Seconds (4), &EventSchedulTestCase::Eventfoo0, this);//Schedule at 13
  EventId d2 = Simulator::Schedule (Seconds (5), &EventSchedulTestCase::Eventfoo0, this);//Schedule at 14
  EventId d3 = Simulator::Schedule (Seconds (6), &EventSchedulTestCase::Eventfoo0, this);//Schedule at 15

  //schedule new clock with new offset
  Simulator::Schedule (Seconds (7), &EventSchedulTestCase::NewFrequency, this, freq, offset);
  //When schedule
  //d1 is moved to 11
  //d1 is moved to 13
  //d1 is moved to 15
  //d2 is moved to 17

  NS_TEST_EXPECT_MSG_EQ (!a.IsExpired (), true, "Event a expired when it shouldn't");
  NS_TEST_EXPECT_MSG_EQ (!b.IsExpired (), true, "Event a expired when it shouldn't");
  NS_TEST_EXPECT_MSG_EQ (!c.IsExpired (), true, "Event c should not have expired");
  Simulator::Cancel (a);
  NS_TEST_EXPECT_MSG_EQ (a.IsExpired (), true, "Event a didn't expire");
  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (m_a, true, "Event A did not run ?");
  NS_TEST_EXPECT_MSG_EQ (m_b, true, "Event B did not run ?");

  EventId anId = Simulator::ScheduleNow (&EventSchedulTestCase::Eventfoo0, this);
  EventId anotherId = anId;
  NS_TEST_EXPECT_MSG_EQ (!(anId.IsExpired () || anotherId.IsExpired ()), true, "Event should not have expired yet.");

  Simulator::Remove (anId);
  NS_TEST_EXPECT_MSG_EQ (anId.IsExpired (), true, "Event was removed: it is now expired");
  NS_TEST_EXPECT_MSG_EQ (anotherId.IsExpired (), true, "Event was removed: it is now expired");

  m_destroy = false;
  m_destroyId = Simulator::ScheduleDestroy (&EventSchedulTestCase::destroy, this);
  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
  m_destroyId.Cancel ();
  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event was canceled: should have expired now");

  m_destroyId = Simulator::ScheduleDestroy (&EventSchedulTestCase::destroy, this);
  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
  Simulator::Remove (m_destroyId);
  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event was canceled: should have expired now");

  m_destroyId = Simulator::ScheduleDestroy (&EventSchedulTestCase::destroy, this);
  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");

  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (!m_destroyId.IsExpired (), true, "Event should not have expired yet");
  NS_TEST_EXPECT_MSG_EQ (!m_destroy, true, "Event should not have run");

  Simulator::Destroy ();
  NS_TEST_EXPECT_MSG_EQ (m_destroyId.IsExpired (), true, "Event should have expired now");
  NS_TEST_EXPECT_MSG_EQ (m_destroy, true, "Event should have run");
  Simulator::Run ();
  



}

void
EventSchedulTestCase::DoRun (void)
{
  Simulator::Run ();
  NS_TEST_EXPECT_MSG_EQ (m_a, true, "Event A did not run ?");
  NS_TEST_EXPECT_MSG_EQ (m_b, true, "Event B did not run ?");
  Simulator::Destroy ();
}

class LocalSimulatorTestSuite : public TestSuite
{
public:
  LocalSimulatorTestSuite ()
  :TestSuite ("node-scheduling", UNIT)
  {
    ObjectFactory factory;
    factory.SetTypeId (ListScheduler::GetTypeId ());

    AddTestCase (new EventSchedulTestCase ("Check basic event handling is working", factory), TestCase::QUICK);
  }
}g_localSimulatorTestSuite;


