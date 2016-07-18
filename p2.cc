//                              Network topology
//
// 
//       UDP   l0 ---|                                      |----- r0   UDP
//                   |                                      |
//       UDP   l1 ---|                                      |----- r1   UDP
//                   |                                      |
//                   |                                      |        
//         SOURCES   c0 --------- c1 --------- c2 -------- c3       SINKS
//                   |                                      |
//       TCP   l3 ---|                                      |---- r3    TCP
//                   |                                      |
//       TCP   l4 ---|                                      |---- r4    TCP
//                   |                                      |      
//       TCP   l5 ---|                                      |---- r5    TCP
//


#include "ns3/netanim-module.h"
#include <string>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/queue.h"
#include "ns3/packet-sink.h"
#include "ns3/random-variable-stream.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");

int
main (int argc, char *argv[])
{

  bool tracing = false;
  uint32_t maxBytes = 100000000;
  uint32_t winSize_Bytes = 2000;
  uint32_t segSize_Bytes = 128;
//  uint32_t nFlows = 1;
//  uint32_t tcpType = 0;

  uint32_t qlen = 2000;
  double minTh = 500.;
  double maxTh = 1500.;
  double Wq = 1./128.;
  double maxP = 1./10.;
  std::string queueType;
  std::string animFile= "p2-anim.xml"; 
  double load = 0.9;


  LogComponentEnable("TcpBulkSendExample", LOG_LEVEL_ALL);


// Allow the user to override any of the defaults at
// run-time, via command-line arguments

  RngSeedManager::SetSeed (11223344);

  Ptr<UniformRandomVariable> randNumber = CreateObject<UniformRandomVariable> ();
  randNumber->SetAttribute ("Stream", IntegerValue (6110));
  randNumber->SetAttribute ("Min", DoubleValue (0.0));
  randNumber->SetAttribute ("Max", DoubleValue (0.1));

    CommandLine cmd;
  cmd.AddValue ("tracing" ,  "Flag to enable/disable tracing"               , tracing);
  cmd.AddValue ("maxBytes", "Total number of bytes for application to send" , maxBytes);
  cmd.AddValue ("windowSize"  , "RX Window Size", winSize_Bytes);
  
  cmd.AddValue ("load", "load"   , load);
  cmd.AddValue ("queue", "Queue Type"   , queueType);
  cmd.AddValue ("MinTh", "MinTh"   , minTh);
  cmd.AddValue ("MaxTh", "MaxTh"   , maxTh);
  cmd.AddValue ("Wq", "Wq"   , Wq);
  cmd.AddValue ("qlen", "Queue Length"   , qlen);
  cmd.AddValue ("maxP", "max drop prob"   , maxP);
  //cmd.AddValue ("segSize"  , "TCP Segment (Packet) Size "                   , segSize_Bytes);
  //cmd.AddValue ("tcpType"  , "TCP Flavour : Use 0 for Tahoe and 1 for Reno "         , tcpType);
  //cmd.AddValue ("nFlows"   , "Number of Simultaneous Flows "                , nFlows);
  
  cmd.Parse (argc, argv);

//
// Set the parameters as per the command line inputs
//
   Config::SetDefault ("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (winSize_Bytes));
   Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
   Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue(segSize_Bytes));

  if (queueType == "RED")
      queueType = "ns3::RedQueue";
  else
      queueType = "ns3::DropTailQueue";

//
// Set the Queue mode and parameters
//


  Config::SetDefault ("ns3::DropTailQueue::Mode", EnumValue (DropTailQueue::QUEUE_MODE_BYTES));
  Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue(qlen));

  Config::SetDefault ("ns3::RedQueue::Mode", EnumValue (RedQueue::QUEUE_MODE_BYTES));
  Config::SetDefault ("ns3::RedQueue::MinTh", DoubleValue (minTh));
  Config::SetDefault ("ns3::RedQueue::MaxTh", DoubleValue (maxTh));
  Config::SetDefault ("ns3::RedQueue::QW", DoubleValue (Wq));
  Config::SetDefault ("ns3::RedQueue::QueueLimit", UintegerValue (qlen));
  Config::SetDefault ("ns3::RedQueue::LInterm", DoubleValue (maxP));

//
// Explicitly create the nodes required by the topology (shown above).
//

   NodeContainer n;
   int numNodes = 5 + 4 + 5;
   n.Create(numNodes);
 
 
   //------------------------
   //      CREATE LINKS
   //------------------------
   // Router links
   NodeContainer c0c1 = NodeContainer(n.Get(0), n.Get(1));
   NodeContainer c1c2 = NodeContainer(n.Get(1), n.Get(2));
   NodeContainer c2c3 = NodeContainer(n.Get(2), n.Get(3));
 
   // Left links
   NodeContainer c0l1 = NodeContainer(n.Get(0), n.Get(4));
   NodeContainer c0l2 = NodeContainer(n.Get(0), n.Get(5));
   NodeContainer c0l3 = NodeContainer(n.Get(0), n.Get(6));
   NodeContainer c0l4 = NodeContainer(n.Get(0), n.Get(7));
   NodeContainer c0l5 = NodeContainer(n.Get(0), n.Get(8));
 
   // Right links
   NodeContainer c3r1 = NodeContainer(n.Get(3), n.Get(9));
   NodeContainer c3r2 = NodeContainer(n.Get(3), n.Get(10));
   NodeContainer c3r3 = NodeContainer(n.Get(3), n.Get(11));
   NodeContainer c3r4 = NodeContainer(n.Get(3), n.Get(12));
   NodeContainer c3r5 = NodeContainer(n.Get(3), n.Get(13));

   // Vector of TCP links
   NodeContainer routerNodes;
   routerNodes.Add(n.Get(0));
   routerNodes.Add(n.Get(1));
   routerNodes.Add(n.Get(2));
   routerNodes.Add(n.Get(3));
 
   NodeContainer leftNodes;
   leftNodes.Add(n.Get(4));
   leftNodes.Add(n.Get(5));
   leftNodes.Add(n.Get(6));
   leftNodes.Add(n.Get(7));
   leftNodes.Add(n.Get(8));
 
   // Vector UDF links
   NodeContainer rightNodes;
   rightNodes.Add(n.Get(9));
   rightNodes.Add(n.Get(10));
   rightNodes.Add(n.Get(11));
   rightNodes.Add(n.Get(12));
   rightNodes.Add(n.Get(13));



   //----------------------------------
   //     PLACE NODES FOR ANIMATION
   //----------------------------------
   // NetAnim positions are only positive
   // Add router node locations
   for(uint32_t i=0; i<routerNodes.GetN(); ++i) {
     Ptr<Node> node = routerNodes.Get(i);
     Ptr<ConstantPositionMobilityModel> loc = node->GetObject<ConstantPositionMobilityModel>();
     loc = CreateObject<ConstantPositionMobilityModel>();
     node->AggregateObject(loc);
     Vector locVec(i, 2, 0);
     loc->SetPosition(locVec);
   }
   // Add left node locations
   for(uint32_t i=0; i<leftNodes.GetN(); ++i) {
     Ptr<Node> node = leftNodes.Get(i);
     Ptr<ConstantPositionMobilityModel> loc = node->GetObject<ConstantPositionMobilityModel>();
     loc = CreateObject<ConstantPositionMobilityModel>();
     node->AggregateObject(loc);
     Vector locVec(-1, 3 - i, 0);
     loc->SetPosition(locVec);
   }
   // Add right node locations
   for(uint32_t i=0; i<rightNodes.GetN(); ++i) {
     Ptr<Node> node = rightNodes.Get(i);
     Ptr<ConstantPositionMobilityModel> loc = node->GetObject<ConstantPositionMobilityModel>();
     loc = CreateObject<ConstantPositionMobilityModel>();
     node->AggregateObject(loc);
     Vector locVec(4, 3 - i, 0);
     loc->SetPosition(locVec);
   }



   //------------------------
   //     CREATE DEVICES
   //------------------------
   // Bottleneck 1
   PointToPointHelper bottleneck1;
   bottleneck1.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
   bottleneck1.SetChannelAttribute ("Delay", StringValue ("10ms"));
   bottleneck1.SetQueue (queueType);
   NetDeviceContainer dc0c1 = bottleneck1.Install(c0c1);
 
   // Bottleneck 2
   PointToPointHelper bottleneck2;
   bottleneck2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
   bottleneck2.SetChannelAttribute ("Delay", StringValue ("100ms"));
   bottleneck2.SetQueue (queueType);
   NetDeviceContainer dc1c2 = bottleneck2.Install(c1c2);
 
   // Center routers
   PointToPointHelper center;
   center.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
   center.SetChannelAttribute ("Delay", StringValue ("10ms"));
   NetDeviceContainer dc2c3 = center.Install(c2c3);
 
   // Left leaves
   PointToPointHelper p2pLeft;
   p2pLeft.SetDeviceAttribute ("DataRate", StringValue ("15Mbps"));
   p2pLeft.SetChannelAttribute ("Delay", StringValue ("10ms"));
   NetDeviceContainer dc0l1 = p2pLeft.Install(c0l1);
   NetDeviceContainer dc0l2 = p2pLeft.Install(c0l2);
   NetDeviceContainer dc0l3 = p2pLeft.Install(c0l3);
   NetDeviceContainer dc0l4 = p2pLeft.Install(c0l4);
   NetDeviceContainer dc0l5 = p2pLeft.Install(c0l5);
 
   // Right leaves
   PointToPointHelper p2pRight;
   p2pRight.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
   p2pRight.SetChannelAttribute ("Delay", StringValue ("10ms"));
   NetDeviceContainer dc3r1 = p2pRight.Install(c3r1);
   NetDeviceContainer dc3r2 = p2pRight.Install(c3r2);
   NetDeviceContainer dc3r3 = p2pRight.Install(c3r3);
   NetDeviceContainer dc3r4 = p2pRight.Install(c3r4);
   NetDeviceContainer dc3r5 = p2pRight.Install(c3r5);
  
   // Create vector of NetDeviceContainer to loop through when assigning ip addresses
   NetDeviceContainer devsArray[] = {dc0c1, dc1c2, dc2c3, dc0l1, dc0l2, dc0l3, dc0l4, dc0l5, dc3r1, dc3r2, dc3r3, dc3r4, dc3r5};
   std::vector<NetDeviceContainer> devices(devsArray, devsArray + sizeof(devsArray) / sizeof(NetDeviceContainer));
 


   //std::cerr << "Installing internet stack" << std::endl;
   InternetStackHelper stack;
   stack.Install (n);

  Ipv4AddressHelper ipv4;
   std::vector<Ipv4InterfaceContainer> ifaceLinks(numNodes-1);
   for(uint32_t i=0; i<devices.size(); ++i) {
     std::ostringstream subnet;
     subnet << "10.1." << i+1 << ".0";
     ipv4.SetBase(subnet.str().c_str(), "255.255.255.0");
     ifaceLinks[i] = ipv4.Assign(devices[i]);
   }


   double bottleneckBW = 1000000; // bits per second
   double numSources = 3;         // number of source apps to divey load between
   double dutyCycle = 0.5;        // duty cycle
   uint64_t rate = (uint64_t)(load*bottleneckBW / numSources / dutyCycle); // rate per source onoff app
 
   // Set port
   uint16_t port = 9;
   // UDP On/Off 1
   OnOffHelper onOffUdp1("ns3::UdpSocketFactory", Address(InetSocketAddress(ifaceLinks[8].GetAddress(1), port)));
   onOffUdp1.SetConstantRate(DataRate(rate));
   onOffUdp1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   onOffUdp1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   // UDP On/Off 2
   OnOffHelper onOffUdp2("ns3::UdpSocketFactory", Address(InetSocketAddress(ifaceLinks[9].GetAddress(1), port)));
   onOffUdp2.SetConstantRate(DataRate(rate));
   onOffUdp2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   onOffUdp2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   // // TCP On/Off 1
   OnOffHelper onOffTcp1("ns3::TcpSocketFactory", Address(InetSocketAddress(ifaceLinks[10].GetAddress(1), port)));
   onOffTcp1.SetConstantRate(DataRate(rate));
   onOffTcp1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   onOffTcp1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   // TCP On/Off 2
   OnOffHelper onOffTcp2("ns3::TcpSocketFactory", Address(InetSocketAddress(ifaceLinks[11].GetAddress(1), port)));
   onOffTcp2.SetConstantRate(DataRate(rate));
   onOffTcp2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   onOffTcp2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   // TCP On/Off 3
   OnOffHelper onOffTcp3("ns3::TcpSocketFactory", Address(InetSocketAddress(ifaceLinks[12].GetAddress(1), port)));
   onOffTcp3.SetConstantRate(DataRate(rate));
   onOffTcp3.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   onOffTcp3.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=0.5]"));
   // Add source apps
   ApplicationContainer sourceApps;
   sourceApps.Add(onOffUdp1.Install(c0l1.Get(1)));
   sourceApps.Add(onOffUdp2.Install(c0l2.Get(1)));
   sourceApps.Add(onOffTcp1.Install(c0l3.Get(1)));
   sourceApps.Add(onOffTcp2.Install(c0l4.Get(1)));
   sourceApps.Add(onOffTcp3.Install(c0l5.Get(1)));
   // Set up run time of source apps
   sourceApps.Start(Seconds(0.0));
   sourceApps.Stop(Seconds(10.0));



   //---------------------------
   //    INSTALL SINK APPS
   //---------------------------
   // Create Udp sink app and install it on node r7
   PacketSinkHelper sinkUdp1("ns3::UdpSocketFactory",
                             Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
   PacketSinkHelper sinkUdp2("ns3::UdpSocketFactory",
                             Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
   // Create Tcp sink apps and install them on nodes r8 & r9
   PacketSinkHelper sinkTcp1("ns3::TcpSocketFactory",
                             Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
   PacketSinkHelper sinkTcp2("ns3::TcpSocketFactory",
                             Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
   PacketSinkHelper sinkTcp3("ns3::TcpSocketFactory",
                             Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
   // Add sink apps
   ApplicationContainer sinkApps;
   sinkApps.Add(sinkUdp1.Install(c3r1.Get(1)));
   sinkApps.Add(sinkUdp2.Install(c3r2.Get(1)));
   sinkApps.Add(sinkTcp1.Install(c3r3.Get(1)));
   sinkApps.Add(sinkTcp1.Install(c3r4.Get(1)));
   sinkApps.Add(sinkTcp3.Install(c3r5.Get(1)));
   // Set up runtime of sink apps
   sinkApps.Start(Seconds(0.0));
   sinkApps.Stop(Seconds(10.0));
 
   AnimationInterface animInterface(animFile);
   animInterface.EnablePacketMetadata(true);
   //std::cerr << "\nSaving animation file: " << animFile << std::endl;
 



//------------------------------
   //    POPULATE ROUTING TABLES
   //------------------------------
   // Uses shortest path search from every node to every possible destination
   // to tell nodes how to route packets. A routing table is the next top route
   // to the possible routing destinations.
   Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
 
 
   //---------------------
   //    RUN SIMULATION
   //---------------------
   //std::cout << "\nRuning simulation..." << std::endl;
   Simulator::Stop (Seconds (10.0));
   Simulator::Run ();
   Simulator::Destroy ();
   std::cout << "\nSimulation finished!" << std::endl;
 
   std::cout << "queueType = " << queueType << "\n"
             << "load = " << load << "\n"
             << "TCP RX winSize = " << winSize_Bytes << "\n"
             << "maxBytes = " << maxBytes << "\n"
             << "\tMinTh = " << minTh << "\n"
             << "\tMaxTh = " << maxTh << "\n"
             << "\tmaxP  = " << maxP  << "\n"
             << "\tWq    = " << Wq    << "\n"
             << "\tqLen  = " << qlen  << "\n"; 
 
   //------------------------
   //    COMPUTE GOODPUT
   //------------------------
   // Print out Goodput of the network communication from source to sink
   // Goodput: Amount of useful information (bytes) per unit time (seconds)
   std::vector<uint32_t> goodputs;
   uint32_t total_bytes = 0;
   uint32_t i = 0;
   for(ApplicationContainer::Iterator it = sinkApps.Begin(); it != sinkApps.End(); ++it) {
     Ptr<PacketSink> sink = DynamicCast<PacketSink> (*it);
     uint32_t bytesRcvd = sink->GetTotalRx ();
     goodputs.push_back(bytesRcvd / 10.0);
     std::cout << "\nFlow " << i << ":\t";
     std::cout << "\tGoodput: " << bytesRcvd/10 << std::endl;
     total_bytes += bytesRcvd;
     //std::cout << "\t\tGoodput: " << goodputs.back() << " Bytes/seconds" << std::endl;
     ++i;
   }
    std::cout <<"\n Overall goodput = " << float(total_bytes/10) << std::endl;
   return 0;
}
