/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: guillermoaguirre10@gmail.com
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/local-clock.h"
#include "ns3/perfect-clock-model-impl.h"
#include "ns3/gnuplot-helper.h"

/**
 * This example shows the basic stepts that are needed to configure a clock in a node. 
 * First, a LocalClock object must be aggregated to the node. The local simulator will use 
 * this aggregated clock to access to the clock. This LocalClock object has as a main attributte the ClockModelImpl.
 * The ClockModelImpl is the object that is in charge of translating the time between local domain and global domain. 
 * In this case, a PerfectClockImpl.
 * Once the attribute is been set and the object is been aggregated to the node no further installations must be done. 
 * 
 * In this example a basic network configuration is set up. Two nodes which are connected throught a Point to Point device and channel.
 * The main point of this example is that each node runs a different clock configurations. 
 * 
 * 
 *      Simulation Time = Global Time
 *              P2P
 *   n1-----------------------n2
 * clock1                   clock2
 * 
 * As the application, the UdpEchoClientApplication and UdpEchoServerApplication is used. This allows as to configure the interval time between packets.
 * This interval time configure by the attribute system represent the interval time measure by the local time. 
 * As it can be observed in the traces generated, the local time is adjusted depending on the frequency and ofsset set on the node. 
 * For the clock model implementation, PerfectClockModelImpl has been used. This class allows to recreate a linear function with an slope and 
 * offset to shape clock difference. In order to give a more realistic approach and to simulate a clock variation (e.g due to a recepetion on 
 * the synchronization message) the clock get update every ten seconds with a new offset a a new frequency. If we map this shape in a 2D plane 
 * with the x axis measure in global time and y axis measure in local time, we would get a function compose with affine functions with continuity 
 * in the extremes. This for 0 to x1 value we have one affine function with slope and offset. From x1 to x2 the affine function change but always
 * with continuity at the verge. x1,x2------n reprenset the clock updates. This, could be also done by implementing a clockmodel with 
 * if statements without need of the clock update. 
 * 
 * |LocalTime
 * |           /
 * |          /
 * |    _____/
 * |   /
 * |  /
 * |_/_____________GlobalTime
 *     x1    x2 ......n
 **/


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

void setClock (Ptr<LocalClock> clock, double freq, Time offset)
{
  NS_LOG_DEBUG ("Calling function set clock");
  Ptr<PerfectClockModelImpl> clockImpl = CreateObject<PerfectClockModelImpl> ();
  clockImpl -> SetAttribute ("Frequency", DoubleValue (freq));
  clockImpl -> SetAttribute ("Offset", TimeValue (offset));
  clock -> SetClock (clockImpl);
}

int
main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.Parse (argc, argv);

  //Set LocalTime Simulator Impl
  GlobalValue::Bind ("SimulatorImplementationType", 
                     StringValue ("ns3::LocalTimeSimulatorImpl"));
  
  Time::SetResolution (Time::NS);
  
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);


  NodeContainer nodes;
  nodes.Create (2);

  //Aggregate clock 
  //Create both clocks, however
  Time init_offset = Seconds (0);
  double freq=1;

  Ptr<PerfectClockModelImpl> clockImpl0 = CreateObject <PerfectClockModelImpl> ();
  Ptr<PerfectClockModelImpl> clockImpl1 = CreateObject <PerfectClockModelImpl> ();

  clockImpl0 -> SetAttribute ("Frequency", DoubleValue (1));
  clockImpl1 -> SetAttribute ("Frequency", DoubleValue (1));
  clockImpl0 -> SetAttribute ("Offset", TimeValue (init_offset));
  clockImpl1 -> SetAttribute ("Offset", TimeValue (init_offset));

  Ptr<LocalClock> clock0 = CreateObject<LocalClock> ();
  Ptr<LocalClock> clock1 = CreateObject<LocalClock> ();

  clock0 -> SetAttribute ("ClockModelImpl", PointerValue (clockImpl0));
  clock1 -> SetAttribute ("ClockModelImpl", PointerValue (clockImpl1));

  Ptr<Node> n1 = nodes.Get (0);
  Ptr<Node> n2 = nodes.Get (1); 

  n1 -> AggregateObject (clock0);
  n2 -> AggregateObject (clock1);

  //Create Point to point channel and install on devices
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  UdpEchoServerHelper echoServer (9);

  int maxTime = 100;
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (0));
  serverApps.Stop (Seconds (maxTime));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (100));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (3.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (0));
  clientApps.Stop (Seconds (maxTime));


  //Clock update. Here clock update are scheduled beforehand

  int j=0; //variable to swap between clocks every 20 seconds
 
  Ptr<ClockModelImpl> clockImpl;
  
  for (int i=20;i<maxTime;i+=10)
  {
    if (j==0)
    {
      freq=0.9; //With this freq node1 runs faster than global clock
      init_offset += Seconds (i); //Adjust offset
      j=1; 
    }
    else
    {     
      freq=2; //With this freq node1 runs slower than global clock
      init_offset += Seconds (i); //Adjust offset
      j=0;
    }
    Simulator::ScheduleWithContext (0, Seconds (i), &setClock, clock0, freq, init_offset);
  }

  GnuplotHelper plotHelper;
  plotHelper.ConfigurePlot ("ReceiveNode2",
                            "Pack Count vs. Time",
                            "Time (Seconds)",
                            "Emitter Count",
                            "png");

  plotHelper.PlotProbe ("ns3::PacketProbe",
                        "/NodeList/1/ApplicationList/0/Rx",
                        "OutputBytes",
                        "Instant where packet is received",
                        GnuplotAggregator::KEY_INSIDE);

  plotHelper.ConfigurePlot ("SendNode1",
                            "Pack Count vs. Time",
                            "Time (Seconds)",
                            "Emitter Count",
                            "png");

  plotHelper.PlotProbe ("ns3::PacketProbe",
                        "/NodeList/0/ApplicationList/0/Tx",
                        "OutputBytes",
                        "Instant where packet is sent",
                        GnuplotAggregator::KEY_INSIDE);

  Simulator::Run ();
  Simulator::Destroy ();
}

