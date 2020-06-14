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


#include "periodic-clock-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("PeriodicClock");

NS_OBJECT_ENSURE_REGISTERED (PeriodicClock);  

TypeId
PeriodicClock::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PeriodicClock")
    .SetParent<ClockModel> ()
    .SetGroupName ("Clock")
    .AddConstructor<PeriodicClock> ()
    .AddAttribute ("Delta", "",
                  TimeValue(MicroSeconds(1)),
                  MakeTimeAccessor (&PeriodicClock::m_delta),
                  MakeTimeChecker ())
    .AddAttribute ("Period", "",
                  TimeValue(MicroSeconds(1)),
                  MakeTimeAccessor (&PeriodicClock::m_period),
                  MakeTimeChecker ())
    .AddAttribute ("Rho", "",
                  TimeValue(MicroSeconds(1)),
                  MakeTimeAccessor (&PeriodicClock::m_rho),
                  MakeTimeChecker ())        
    .AddAttribute ("Slope", "",
                  DoubleValue (1.5),
                  MakeDoubleAccessor (&PeriodicClock::m_slope),
                  MakeDoubleChecker <double> ())
    .AddAttribute ("Interval", "",
                  TimeValue(MicroSeconds(1)),
                  MakeTimeAccessor (&PeriodicClock::m_interval),
                  MakeTimeChecker ())
    .AddAttribute ("xvalueGlobal", "",
                  TimeValue(MicroSeconds(1)),
                  MakeTimeAccessor (&PeriodicClock::m_xjglobal),
                  MakeTimeChecker ())                            
  ;
  return tid;
}

PeriodicClock::PeriodicClock ()
{
  NS_LOG_FUNCTION (this);
  
}
PeriodicClock::~PeriodicClock ()
{
  NS_LOG_FUNCTION (this);
}

Time
PeriodicClock::GetLocalTime ()
{
  NS_LOG_FUNCTION (this);
  Time ret;
  ret = GlobalToLocalTime (Simulator::Now ());
  return ret;
}

Time 
PeriodicClock::GlobalToLocalTime (Time globalTime)
{
  NS_LOG_FUNCTION (this << globalTime);

  NS_LOG_DEBUG ("Global Time - xjGlobal : " << globalTime);

  uint64_t cycles =  (globalTime - m_xjglobal).GetInteger () / (m_period).GetInteger ();

  NS_LOG_DEBUG ("Number of cycles global time: " << cycles);
  
  if (globalTime >  m_xjglobal + cycles)
  {
    globalTime = globalTime -  cycles * m_period;
    NS_LOG_DEBUG ("Adjust globaltime: " << globalTime);
  }

  Time ret;
  ret = CalculateRelativeTimeGlobalToLocal (globalTime);
  ret += cycles * m_period; 
  NS_LOG_DEBUG ("Time + cycles = LocalTime " << ret);

  return  ret;
}

Time 
PeriodicClock::LocalToGlobalTime (Time localtime)
{
  NS_LOG_FUNCTION (this << localtime);

  m_xjlocal = GlobalToLocalTime (m_xjglobal);

  NS_LOG_DEBUG ("Local Time - xjLocal : " << localtime);

  uint64_t cycles =  (localtime - m_xjlocal).GetInteger () / (m_period).GetInteger ();
  NS_LOG_DEBUG ("local int; " << localtime.GetInteger () << "period int" << (m_period).GetInteger ());
  NS_LOG_DEBUG ("Number of cycles: " << cycles);
  
  if (localtime >  m_xjlocal + cycles)
  {
    localtime = localtime -  cycles * m_period;
    NS_LOG_DEBUG ("Adjust localtime: " << localtime);
  }
  Time ret;
  ret = CalculateRelativeTimeLocalToGlobal (localtime);
  ret += cycles * m_period; 
  NS_LOG_DEBUG ("Time + cycles " << ret);
  return  ret;
}

Time
PeriodicClock::GlobalToLocalDelay (Time globaldDelay)
{
  NS_LOG_FUNCTION (this << globaldDelay);

  Time ret = GlobalToLocalTime (Simulator::Now () + globaldDelay);
  Time localtime = GlobalToLocalTime (Simulator::Now ());
  ret -=localtime;

  NS_LOG_DEBUG ("Local Absolute time: " << ret);

  return ret;
}

Time
PeriodicClock::LocalToGlobalDelay (Time localdelay)
{
  NS_LOG_FUNCTION (this << localdelay);
  Time localTime = GlobalToLocalTime (Simulator::Now ());
  Time ret = LocalToGlobalTime (localTime + localdelay);
  ret -=Simulator::Now (); 

  NS_LOG_DEBUG ("Returned timed to simulator " << ret); 

  return ret;
}

Time
PeriodicClock::CalculateRelativeTimeGlobalToLocal (Time time)
{
  NS_LOG_FUNCTION (this << time);

  Time ret;
  if ((time <  m_xjglobal) || (time == m_xjglobal))
  {
      ret = time - (m_delta/2);
      NS_LOG_DEBUG ("Time 1: " << ret);
  }

  if (((m_xjglobal < time) && (time < m_xjglobal + Time ( m_interval.GetDouble ()/m_slope))) || (time == m_xjglobal + Time ( m_interval.GetDouble ()/m_slope)))
  {
    ret = Time (m_slope * (time - m_xjglobal).GetDouble ()) + m_xjglobal - m_delta / 2; 
    NS_LOG_DEBUG ("Time 2: " << ret);
  }

  if (((m_xjglobal + Time ( m_interval.GetDouble ()/m_slope) < time) && (time < m_xjglobal + Time ( m_interval.GetDouble ()/m_slope) + m_interval)) || 
  (time == m_xjglobal + Time ( m_interval.GetDouble ()/m_slope) + m_interval))
  {
    ret = Time ((1 / m_slope) * (time - m_xjglobal - m_interval.GetDouble () / m_slope) + m_interval + m_xjglobal - m_delta/2);
    NS_LOG_DEBUG ("Time 3: " << ret);
  }

  if (((m_xjglobal + Time ( m_interval.GetDouble ()/m_slope) + m_interval < time) && (time < m_xjglobal + m_period)) || 
  (time == m_xjglobal + m_period))
  {
    ret = time - m_delta/2;
    NS_LOG_DEBUG ("Time 4: " << ret);
  }

  return ret;
}

Time
PeriodicClock::CalculateRelativeTimeLocalToGlobal (Time time)
{

  Time ret;
  if ((time <  m_xjlocal) || (time == m_xjlocal))
  {
    ret = time + (m_delta/2);
    NS_LOG_DEBUG ("Time 1: " << ret);
  }

  if (((m_xjlocal < time) && (time < m_xjlocal + m_interval)) || (time == m_xjlocal + m_interval))
  {
    ret = Time ((1 / m_slope) * (time - m_xjlocal).GetDouble ()) + m_xjlocal + m_delta / 2; 
    NS_LOG_DEBUG ("Time 2: " << ret);
  }

  if (((m_xjlocal + m_interval < time) && (time < m_xjlocal + Time ( m_interval.GetDouble ()/m_slope) + m_interval)) || 
  (time == m_xjlocal + Time ( m_interval.GetDouble ()/m_slope) + m_interval))
  {
    ret = Time ( m_slope * (time.GetDouble () - m_xjlocal.GetDouble () - m_interval.GetDouble () / m_slope) + m_interval + m_xjlocal + m_delta/2);
    NS_LOG_DEBUG ("Time 3: " << ret);
  }

  if (((m_xjlocal + Time ( m_interval.GetDouble ()/m_slope) + m_interval < time) && (time < m_xjlocal + m_period)) || 
  (time == m_xjlocal + m_period))
  {
    ret = time + m_delta/2;
    NS_LOG_DEBUG ("Time 4: " << ret);
  }
  return ret;  
}

}