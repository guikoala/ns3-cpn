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
#ifndef PERFECT_CLOCK_MODEL_IMPL_H
#define PERFECT_CLOCK_MODEL_IMPL_H

#include "ns3/clock-model-impl.h"
#include "ns3/object.h"

namespace ns3 {
/**
 * \file Clock
 * ns3::PerfectClockModelImpl declaration
 * 
 * @brief This class represents a perfect clock modelling. 
 * The mapping between the local time and global time is set by perfect linear function.
 *  The slope of the function is determined by the frequency value differece. So if for example the frequency is set to 2.
 * Local clock will be two times slower that the global time. When local time says 2 global time will be saying 4.
 * Also a initial offest is possible to set up. So LT = f*GT + offset 
 */

class PerfectClockModelImpl : public ClockModelImpl
{
public:
  static TypeId GetTypeId (void);

  PerfectClockModelImpl ();
  ~PerfectClockModelImpl ();

  Time GetLocalTime ();
  Time GlobalToLocalTime (Time globalTime);
  Time LocalToGlobalTime (Time localtime);
  Time GlobalToLocalAbs (Time globaldDelay);
  Time LocalToGlobalAbs (Time localdelay);

private:
//Frequency of the clock
  double m_frequency;
  Time m_offset;
};


}//namespace ns3
#endif /* PERFECT_CLOCK_MODEL_IMPL_H */
