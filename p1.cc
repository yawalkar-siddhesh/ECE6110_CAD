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

// Network topology
//
//       n0 ----------- n1
//            500 Kbps
//             5 ms
//
// - Flow from n0 to n1 using BulkSendApplication.
// - Tracing of queues and packet receptions to file "tcp-bulk-send.tr"
//   and pcap tracing available when tracing is turned on.

//#include "ns3/netanim-module.h"
#include <string>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink-helper.h"
//#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/queue.h"
#include "ns3/packet-sink.h"
#include "ns3/random-variable-stream.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");

int
main (int argc, char *argv[])
{

  bool tracing = false;
  uint32_t maxBytes = 100000000;
  uint32_t winSize_Bytes = 2000;
  uint32_t queueSize_Bytes = 2000;
  uint32_t segSize_Bytes = 128;
  uint32_t nFlows = 1;
  uint32_t tcpType = 0;

 LogComponentEnable("TcpBulkSendExample", LOG_LEVEL_ALL);


// Allow the user to override any of the defaults at
// run-time, via command-line arguments
//

RngSeedManager::SetSeed (11223344);

Ptr<UniformRandomVariable> randNumber = CreateObject<UniformRandomVariable> ();
  randNumber->SetAttribute ("Stream", IntegerValue (6110));
   randNumber->SetAttribute ("Min", DoubleValue (0.0));
   randNumber->SetAttribute ("Max", DoubleValue (0.1));

    CommandLine cmd;
  cmd.AddValue ("tracing" ,  "Flag to enable/disable tracing"               , tracing);
  
  cmd.AddValue ("maxBytes", "Total number of bytes for application to send" , maxBytes);
  
  cmd.AddValue ("windowSize"  , "Total number of bytes for application to send", winSize_Bytes);
  cmd.AddValue ("queueSize", "Queue Limit at the Bottlenect node (Bytes)"   , queueSize_Bytes);
  cmd.AddValue ("segSize"  , "TCP Segment (Packet) Size "                   , segSize_Bytes);
  
  cmd.AddValue ("tcpType"  , "TCP Flavour : Use 0 for Tahoe and 1 for Reno "         , tcpType);
  
  cmd.AddValue ("nFlows"   , "Number of Simultaneous Flows "                , nFlows);
  
  cmd.Parse (argc, argv);

//
// Set the parameters as per the command line inputs
//

  //Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue(winSize_Bytes));

    if (tcpType == 0) {
        NS_LOG_INFO ("Setting TCP Flavour to Tahoe");
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpTahoe"));
        } 
    else {
        NS_LOG_INFO ("Setting TCP Flavour to Reno");
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpReno"));
        } 

   Config::SetDefault ("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (winSize_Bytes));
   Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
   Config::SetDefault ("ns3::DropTailQueue::Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES));
   Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue(queueSize_Bytes));
   Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(segSize_Bytes));
//
// Explicitly create the nodes required by the topology (shown above).
//
//

   PointToPointHelper p2pLeaf, p2pRouters;
   p2pLeaf.SetDeviceAttribute     ("DataRate", StringValue ("5Mbps"));
   p2pLeaf.SetChannelAttribute    ("Delay",    StringValue ("10ms"));
   p2pRouters.SetDeviceAttribute  ("DataRate", StringValue ("1Mbps"));
   p2pRouters.SetChannelAttribute ("Delay",    StringValue ("20ms"));
   PointToPointDumbbellHelper dumbbell (nFlows, p2pLeaf, nFlows, p2pLeaf, p2pRouters);

  InternetStackHelper stack;
  dumbbell.InstallStack (stack);

  Ipv4AddressHelper ltIps     = Ipv4AddressHelper ("10.1.1.0", "255.255.255.0");
     Ipv4AddressHelper rtIps     = Ipv4AddressHelper ("10.2.1.0", "255.255.255.0");
     Ipv4AddressHelper routerIps = Ipv4AddressHelper ("10.3.1.0", "255.255.255.0");
     dumbbell.AssignIpv4Addresses(ltIps, rtIps, routerIps);



//
//

  NS_LOG_INFO ("Create Applications.");

//
// Create a BulkSendApplication and install it on node 0
//
    ApplicationContainer sourceApps[10];
    ApplicationContainer sinkApps[10];


  uint16_t port = 9;  // well-known echo port number

   double start_time[10];

    for(uint16_t i=0; i<nFlows; i++){
  
    BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (dumbbell.GetRightIpv4Address (i), port));
    // Set the amount of data to send in bytes.  Zero is unlimited.
    source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
    sourceApps[i] = source.Install (dumbbell.GetLeft (i));

     double randfloat = randNumber->GetValue ();
     start_time[i] = randfloat;

    sourceApps[i].Start (Seconds (randfloat));
    sourceApps[i].Stop (Seconds (10.0));
  
//
// Create a PacketSinkApplication and install it on node 1
//
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
    sinkApps[i] = sink.Install (dumbbell.GetRight (i));
    sinkApps[i].Start (Seconds (0.0));
    sinkApps[i].Stop (Seconds (10.0));

    }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  std::cout << "+++++++++++++++++++++++++++++++++" <<std::endl; 

  
  for(uint16_t ii = 0; ii < nFlows ; ii++){
     //Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (*it);
     Ptr<PacketSink> sink2 = DynamicCast<PacketSink> (sinkApps[ii].Get(0));
     //float bytesRcvd = sink1->GetTotalRx ();
     //goodputs.push_back(bytesRcvd / 10);
     std::cout << "tcp," << tcpType;
     std::cout << ",flow," << ii;
     std::cout << ",windowSize,"<< winSize_Bytes; 
     std::cout << ",queueSize,"<< queueSize_Bytes; 
     std::cout << ",segSize,"<< segSize_Bytes; 
     //std::cout << ", goodput: " << goodputs.back() << " Bytes/seconds" << std::endl;
     //std::cout << ",goodput," <<(bytesRcvd / 10-start_time[i]) << std::endl;
     std::cout << ",goodput," <<((sink2->GetTotalRx ())/( 10 - start_time[ii]))<< std::endl;
   }
}
