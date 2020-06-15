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
 * Authors: Guillermo Aguirre <guillermo.aguirrerodrigo@epfl.ch> Ludovic Thomas <ludovic.thomas@epfl.ch>
 */
#ifndef CLOCK_MODEL_H
#define CLOCK_MODEL_H

#include "ns3/object.h"
#include "ns3/object-factory.h"
#include "ns3/nstime.h"

/**
 * \file
 * \ingroup clock
 * ns3:ClockModel interface declaration
 */

namespace ns3{
/**
* \ingroup clock
* Clock model 
*/ 
class ClockModel : public Object
{
public:
  /**
   * Register this type
   * \return The object TypeId
   */
  static TypeId GetTypeId (void);

  /**  \copydoc ClockModel::GetLocalTime  */
  virtual Time GetLocalTime () = 0;
  /**  \copydoc ClockModel::GlobalToLocalAbs  */
  virtual Time GlobalToLocalTime (Time globalTime) = 0;
  /**  \copydoc ClockModel::LocalToGlobalAbs  */
  virtual Time LocalToGlobalTime (Time localtime) = 0;
  /**  \copydoc ClockModel::GlobalToLocalTime  */
  virtual Time GlobalToLocalDelay (Time globaldDelay) = 0;
  /**  \copydoc ClockModel::GlobalToLocalAbs  */
  virtual Time LocalToGlobalDelay (Time localdelay) = 0;
  
};
}// namespace ns3

#endif /* CLOCK_MODEL_H */