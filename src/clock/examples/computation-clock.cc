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
 */


#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/config-store.h"
#include "ns3/clock-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Computation");


void aggregateClock (double freq, Ptr<Node> node)
{
  Ptr<PerfectClockModelImpl> clockImpl = CreateObject<PerfectClockModelImpl> ();
  clockImpl -> SetAttribute ("Frequency", DoubleValue (freq));
  Ptr<LocalClock> clock0 = CreateObject<LocalClock> ();
  clock0 -> SetAttribute ("ClockModelImpl", PointerValue (clockImpl));
  node -> AggregateObject (clock0);
}

void SetClock (Ptr<LocalClock> clock, Ptr<ClockModelImpl> clockImpl, double freq)
{
  NS_LOG_DEBUG ("Calling function set clock");
  clockImpl -> SetAttribute ("Frequency", DoubleValue (freq));
  clock -> SetClock (clockImpl);
}

int 
main (int argc, char *argv[])
{
  //
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  //

  LogComponentEnable ("Computation", LOG_LEVEL_INFO);
  //Set LocalTime Simulator Impl
  GlobalValue::Bind ("SimulatorImplementationType", 
                     StringValue ("ns3::LocalTimeSimulatorImpl"));

  int m_nodes = 200;
  std::string AppPacketRate ("40Kbps");
  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("1000"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (AppPacketRate));
  int port = 10;
  double freq = 1.01;
  CommandLine cmd;
  cmd.Parse (argc, argv);


  NS_LOG_INFO ("Number of nodes for the simulation " << m_nodes);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer terminals;
  terminals.Create (m_nodes);

  NodeContainer csmaSwitch;
  csmaSwitch.Create (1);

  NS_LOG_INFO ("Build Topology");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (50000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0)));

  // Create the csma links, from each terminal to the switch
   // Add internet stack to the terminals
  InternetStackHelper internet;
 
  internet.Install (terminals);
  NetDeviceContainer switchDevices;
  NetDeviceContainer terminalDevices;

  Ipv4AddressHelper ipv4;

  ipv4.SetBase ("10.0.0.0", "255.255.0.0");

  for (int i = 0; i < m_nodes; i++)
    {
      aggregateClock (freq, terminals.Get (i));
      NetDeviceContainer link = csma.Install (NodeContainer (terminals.Get (i), csmaSwitch));
      terminalDevices.Add (link.Get (0));
      switchDevices.Add (link.Get (1));
      ipv4.Assign (terminalDevices);
      ipv4.NewNetwork ();
    }

  //Add clock to switch
  aggregateClock (freq, csmaSwitch.Get (0));

  // Create the bridge netdevice, which will do the packet switching
  Ptr<Node> switchNode = csmaSwitch.Get (0);
  BridgeHelper bridge;
  bridge.Install (switchNode, switchDevices);

  int half = m_nodes / 2;

  for (int i = 0; i<m_nodes/2; i++)
  {
    Ptr<Node> n = terminals.Get (half);
    Ptr<Ipv4> ipv4 = n->GetObject<Ipv4> ();
    Ipv4InterfaceAddress ipv4_int_addr = ipv4->GetAddress (1, 0);
    Ipv4Address ip_addr = ipv4_int_addr.GetLocal ();
    OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (ip_addr, port)); // traffic flows from node[i] to node[j]
    onoff.SetConstantRate (DataRate (AppPacketRate));
    ApplicationContainer apps = onoff.Install (terminals.Get (i));  // traffic sources are installed on all nodes
    apps.Start (Seconds (2));
    apps.Stop (Seconds (12));
    half++;
  }
  
  for (int i = half; i<m_nodes; i++)
  {
    PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
    ApplicationContainer app = sink.Install (terminals.Get (1));
    app.Start (Seconds (0.0));
  }
  
  //Schedule for clock update
  NS_LOG_INFO ("Schedule Updates");
  Ptr<LocalClock> clock;
  Ptr<Node> node;
  freq =1 ;
  /*for (int i =0; i<m_nodes/2; i++)
  {
    Ptr<PerfectClockModelImpl> clockImpl = CreateObject<PerfectClockModelImpl> ();
    node = terminals.Get (i);
    clock = node -> GetObject <LocalClock> ();
    Simulator::ScheduleWithContext (i, Seconds (7), &SetClock, clock, clockImpl, freq);
  }*/

  NS_LOG_INFO ("Configure Tracing.");

  //
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
  // Trace output will be sent to the file "csma-bridge.tr"
  //
  //AsciiTraceHelper ascii;
 //csma.EnableAsciiAll (ascii.CreateFileStream ("csma-bridge.tr"));

  //
  // Also configure some tcpdump traces; each interface will be traced.
  // The output files will be named:
  //     csma-bridge-<nodeId>-<interfaceId>.pcap
  // and can be read by the "tcpdump -r" command (use "-tt" option to
  // display timestamps correctly)
  //
  //csma.EnablePcapAll ("csma-bridge", false);

  //
  // Now, do the actual simulation.
  //

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
