#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

#include "ns3/core-module.h"
#include "ns3/aodv-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/stats-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
int j=0;


using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("Teamx-ECE6110-p3");

#define IEEE_80211_BANDWIDTH 20000000

//Function to send traffic
static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
    if (pktCount > 0)
    {
        // std::cout << "RandomFunction received event at "<<j++<<"\n";
            // << Simulator::Now ().GetSeconds () << "s" << std::endl;
        socket->Send (Create<Packet> (pktSize));
        Simulator::Schedule (pktInterval, &GenerateTraffic,
                             socket, pktSize,pktCount-1, pktInterval);
    }
    else
    {
        socket->Close ();
    }
}

class AdHocExperiment
{
public:
    
    AdHocExperiment ();
    AdHocExperiment (double nodeDensity, double txp, uint32_t protocol, double intensity, double onTime, double offTime);
    Gnuplot2dDataset Run (const WifiHelper &wifi, const YansWifiPhyHelper &wifiPhy,
                          const NqosWifiMacHelper &wifiMac, const YansWifiChannelHelper &wifiChannel, const MobilityHelper &mobility);
    
    bool CommandSetup (int argc, char **argv);
    double CheckEfficiency();
    
private:
    void ApplicationSetup (Ptr<Node> client, Ptr<Node> server, double start, double stop, uint64_t dataRate);
    void SelectSrcDest (NodeContainer c, uint64_t dataRate);
    void ReceivePacket (Ptr<Socket> socket);
    void SendPacket (Ptr<Socket> socket, uint32_t dataSent);


    void CheckThroughput ();
    
    Gnuplot2dDataset m_output;
    
    double m_totalTime;
    double m_intensity;
    double m_onTime;
    double m_offTime;
    double m_txp;

    uint32_t m_SentBytesTotal;
    uint32_t m_RecvBytesTotal;
    uint32_t m_packetSize;
    uint32_t m_gridSize;
    uint32_t m_nodeDistance;
    uint32_t m_port;
    double m_nodeDensity;
    uint32_t m_protocol;
    
};

AdHocExperiment::AdHocExperiment ()
{
}

AdHocExperiment::AdHocExperiment (double nodeDensity, double txp, uint32_t protocol, double intensity, double onTime, double offTime) :
m_output("P3Experiment"),
m_totalTime (33),
m_intensity(intensity),
m_onTime(onTime),
m_offTime(offTime),
m_txp(txp),
m_SentBytesTotal (0),
m_RecvBytesTotal (0),
m_packetSize (2000),
m_gridSize (1000), //1000x1000 grid  for a total of 1000000 nodes
m_nodeDistance (30),
m_port (5000),
m_nodeDensity(nodeDensity),
m_protocol(protocol)
{
    m_output.SetStyle (Gnuplot2dDataset::LINES);

}


//Used to make sure we get the accurate bytesTotal at the end
void AdHocExperiment::ReceivePacket (Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    while ((packet = socket->Recv ()))
    {
        m_RecvBytesTotal += packet->GetSize ();
    }
}

//Used to make sure we get the accurate bytesTotal at the end
void AdHocExperiment::SendPacket (Ptr<Socket> socket, uint32_t dataSent)
{
    m_SentBytesTotal += dataSent ;

}

//Used to check throughput
void AdHocExperiment::CheckThroughput ()
{
    double mbs = ((m_RecvBytesTotal * 8.0) /1000000);
    //m_RecvBytesTotal = 0;
    m_output.Add ((Simulator::Now ()).GetSeconds (), mbs);
    
    //check throughput every 1/10 of a second
    Simulator::Schedule (Seconds (0.1), &AdHocExperiment::CheckThroughput, this);
}

//Used to check efficiency
double AdHocExperiment::CheckEfficiency ()
{
    if((m_SentBytesTotal == 0) || (m_RecvBytesTotal == 0)) {
        return 0;
    } else {
        double networkEfficiency = ((m_RecvBytesTotal) / (double)m_SentBytesTotal);
        return networkEfficiency;
    }
}

/**
 * Sources and destinations are randomly selected such that a node
 * may be the source for multiple destinations and a node maybe a destination
 * for multiple sources.
 */
void AdHocExperiment::SelectSrcDest (NodeContainer c, uint64_t dataRate)
{
    uint32_t totalNodes = c.GetN ();
    Ptr<UniformRandomVariable> uvDest = CreateObject<UniformRandomVariable> ();
    uvDest->SetAttribute ("Min", DoubleValue (0));
    uvDest->SetAttribute ("Max", DoubleValue (totalNodes - 1));
    
    //Every node picks a random DST
    for (uint32_t i=0; i < (uint32_t) totalNodes; i++)
    {
        double a = i;
        double b = uvDest->GetInteger ();
        while (b == a) {
            b = uvDest->GetInteger ();
        }
        //std::cout << "A: " << a;
        //std::cout << " B, " <<b << std::endl;
        ApplicationSetup (c.Get (a), c.Get (b),  0, m_totalTime, dataRate);
    }
}

void AdHocExperiment::ApplicationSetup (Ptr<Node> client, Ptr<Node> server, double start, double stop, uint64_t dataRate)
{
    Ptr<Ipv4> ipv4Server = server->GetObject<Ipv4> ();
    
    Ipv4InterfaceAddress iaddrServer = ipv4Server->GetAddress (1,0);
    Ipv4Address ipv4AddrServer = iaddrServer.GetLocal ();
    
    // Setting up sink and source
    //Sink
    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket (server, tid);
    InetSocketAddress local = InetSocketAddress (ipv4AddrServer, m_port);
    recvSink->Bind (local);
    recvSink->SetRecvCallback (MakeCallback (&AdHocExperiment::ReceivePacket, this));
    
    //Source
    Ptr<Socket> source = Socket::CreateSocket (client, tid);
    InetSocketAddress remote = InetSocketAddress (ipv4AddrServer, m_port);
    source->SetDataSentCallback (MakeCallback (&AdHocExperiment::SendPacket, this));
    source->Connect (remote);
    
    
    //TODO make dependent on m_intensity
    
    uint32_t numPackets = dataRate /(double) m_packetSize ;  //Number of packets based on intensity and networkCapacity
    double interPacketInterval = (double) 9/numPackets; //Periodic interval to send packets
    Time interPacketInterval2(interPacketInterval);
    // std::cout<<"packet count="<<numPackets<<"time="<<interPacketInterval<<"\n";
    //std::cout<< numPackets << std::endl;
    
    
    // Give time to converge-- 30 seconds perhaps
    Simulator::Schedule (Seconds (1.0), &GenerateTraffic,
                         source, m_packetSize, numPackets, interPacketInterval2);
    
    
    
    
}

Gnuplot2dDataset AdHocExperiment::Run (const WifiHelper &wifi, const YansWifiPhyHelper &wifiPhy,
                 const NqosWifiMacHelper &wifiMac, const YansWifiChannelHelper &wifiChannel, const MobilityHelper &mobility)
{
    
    
    uint32_t mapSize = m_gridSize * m_gridSize;            //Total map size
    uint32_t numOfNodes = (uint32_t)(m_nodeDensity * (double)mapSize);   //Number of total nodes on the map
    NodeContainer c;
    c.Create (numOfNodes);      //Create the nodes
    
    YansWifiPhyHelper phy = wifiPhy;
    phy.SetChannel (wifiChannel.Create ());
    
    NqosWifiMacHelper mac = wifiMac;
    mac.SetType ("ns3::AdhocWifiMac");
    
    phy.Set ("TxPowerStart",DoubleValue (m_txp));   //Set transmission power
    phy.Set ("TxPowerEnd", DoubleValue (m_txp));    //Set transmission power
    NetDeviceContainer devices = wifi.Install (phy, mac, c);
    

    AodvHelper aodv;
    OlsrHelper olsr;

    Ipv4StaticRoutingHelper staticRouting;
                 
    Ipv4ListRoutingHelper list;
    list.Add (staticRouting, 0);
    
    InternetStackHelper internet;
    if (m_protocol == 1)
        list.Add (aodv, 10);
    else
        list.Add (olsr, 10);
    
    internet.SetRoutingHelper (list); // has effect on the next Install ()
    internet.Install (c);
    
    
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.0.0");
    
    Ipv4InterfaceContainer ipInterfaces;
    ipInterfaces = address.Assign (devices);
    
    MobilityHelper mobil= mobility;

    
    
    mobil.SetPositionAllocator ("ns3::GridPositionAllocator",
                                "MinX", DoubleValue (0.0),
                                "MinY", DoubleValue (0.0),
                                "DeltaX", DoubleValue (m_nodeDistance),
                                "DeltaY", DoubleValue (m_nodeDistance),
                                "GridWidth", UintegerValue (m_gridSize),
                                "LayoutType", StringValue ("RowFirst"));
    
    
    mobil.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobil.Install (c);
    
    // Determine the data rate to be used based on the intensity and network capacity.
    Ptr<WifiNetDevice> wifiDevice = DynamicCast<WifiNetDevice> (devices.Get(0));
    uint64_t networkCap = IEEE_80211_BANDWIDTH *
                          log (1 + wifiDevice->GetPhy()->CalculateSnr (wifiDevice->GetPhy()->GetMode(0), 0.1)) / log (2);
    
     
    uint64_t dataRate  = (uint64_t)(networkCap * m_intensity) / (double)(numOfNodes);


    
    SelectSrcDest (c, dataRate);  //Setup applications
    CheckThroughput ();

    
    Ptr<FlowMonitor> flowmon;
    FlowMonitorHelper flowmonHelper;
    

    flowmon = flowmonHelper.InstallAll ();

    
    Simulator::Stop (Seconds (10.0));
    Simulator::Run ();
    
    /*if (enableFlowMon)
    {
        flowmon->SerializeToXmlFile ((GetOutputFileName () + ".flomon"), false, false);
    }*/
    
    std::cout << "nodeDensity,"   << m_nodeDensity;
    std::cout << ",protocol,"     << m_protocol;
    std::cout << ",txp(dBm/n),"   << m_txp;
    std::cout << ",intensity,"    << m_intensity;
    std::cout << ",Efficiency, "  << CheckEfficiency(); //Print out efficiency
    std::cout << ",totalRxBytes," << m_RecvBytesTotal;
    std::cout << ",totalTxBytes," << m_SentBytesTotal;
    std::cout << ",Network Capacity(Mbs)," << networkCap/1000000 << std::endl;
    Simulator::Destroy ();
    
    return m_output;
}

int main (int argc, char* argv[]) {
    
    RngSeedManager::SetSeed (11223344); //Change Seed to the one the instructor provided
    
    
    double nodeDensity = 0.00002; //0.00002 * 10^6 = 20 nodes
    uint32_t protocol = 0;        //OSLR default
    double txp= 500.0;            //Default transmission power
    double intensity = 0.1;       //Default traffic intensity

    double onTime = 0.1; //onoff application onTime
    double offTime = 0.1; //onoff application offTime
    
    //Get Command line values
    CommandLine cmd;
    cmd.AddValue("nodeDensity", "Number of Nodes per unit aream (1000m by 1000m)", nodeDensity);
    cmd.AddValue("protocol", "0 = OLSR, 1 = AODV", protocol);
    cmd.AddValue("txp", "Transmission Power", txp);
    cmd.AddValue("intensity", "Traffic Intensity", intensity);
    
    cmd.Parse(argc, argv);
    //Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue (psize));
    //Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));
    //Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
    //Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    // Fix non-unicast data rate to be the same as that of unicast
    //Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue ("DsssRate1Mbps"));
    std::ofstream outfile (("P3Experiment.plt"));

    
    if(protocol == 1) {
        protocol = 1;
    } else {
        protocol = 0;
    }
    
    txp = 10*(log10(txp));
    //std::cout << "txp is "<< txp <<"in dBm\n";
    AdHocExperiment experiment;
    experiment = AdHocExperiment (nodeDensity, txp, protocol, intensity, onTime, offTime);
    
    
    
    MobilityHelper mobility;
    //Gnuplot gnuplot;
    Gnuplot2dDataset dataset;
    
    WifiHelper wifi = WifiHelper::Default ();
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    Ssid ssid = Ssid ("Testbed");
    
    wifiMac.SetType ("ns3::AdhocWifiMac",
                     "Ssid", SsidValue (ssid));
    wifi.SetStandard (WIFI_PHY_STANDARD_holland);
    wifi.SetRemoteStationManager ("ns3::MinstrelWifiManager");
    
    
    dataset = experiment.Run (wifi, wifiPhy, wifiMac, wifiChannel, mobility);
    
    //gnuplot.AddDataset (dataset);
    //gnuplot.GenerateOutput (outfile);

    
    return 0;

}
