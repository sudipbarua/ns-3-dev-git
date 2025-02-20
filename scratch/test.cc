#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/lora-helper.h"
#include "ns3/lorawan-module.h"
#include "ns3/basic-energy-source-helper.h"
#include <fstream>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE ("LoRaWANRSSILogger");

// Function to print RSSI and SNR when a packet is received
void PrintRssiAndSnr(double rssi, double snr) {

    // Print the RSSI and SNR
    std::cout << "Packet received with RSSI: " << rssi << " dBm, SNR: " << snr << " dB" << std::endl;
}


int main(int argc, char *argv[])
{
    LogComponentEnable ("LoRaWANRSSILogger", LOG_LEVEL_INFO);
    // LogComponentEnable ("LoraPacketTracker", LOG_LEVEL_ALL);
    // LogComponentEnable ("NetworkServer", LOG_LEVEL_ALL);
    // LogComponentEnable ("NetworkController", LOG_LEVEL_ALL);
    // LogComponentEnable ("NetworkScheduler", LOG_LEVEL_ALL);
    // LogComponentEnable ("NetworkStatus", LOG_LEVEL_ALL);
    // LogComponentEnable ("EndDeviceStatus", LOG_LEVEL_ALL);
    // LogComponentEnable("AdrComponent", LOG_LEVEL_ALL);
    // LogComponentEnable("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("MacCommand", LOG_LEVEL_ALL);
    // LogComponentEnable ("AdrExploraSf", LOG_LEVEL_ALL);
    // LogComponentEnable ("AdrExploraAt", LOG_LEVEL_ALL);
    LogComponentEnable ("EndDeviceLorawanMac", LOG_LEVEL_ALL);

    LogComponentEnableAll(LOG_PREFIX_FUNC);
    LogComponentEnableAll(LOG_PREFIX_NODE);
    LogComponentEnableAll(LOG_PREFIX_TIME);


    // Create nodes (End Devices, Gateways)
    NodeContainer endDevices, gateways;
    endDevices.Create(1); // 10 End Devices
    gateways.Create(2);    // 3 Gateways

    // Creating modbilty model
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(endDevices);
    mobility.Install(gateways);

    /*************************************************************************************************************
    ************************************* Create the wireless channel *******************************+++++++++++++
    *************************************************************************************************************/

    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel>();
    loss->SetPathLossExponent(3.76);
    loss->SetReference(1, 7.7);

    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
    x->SetAttribute("Min", DoubleValue(0.0));
    x->SetAttribute("Max", DoubleValue(10));

    Ptr<RandomPropagationLossModel> randomLoss = CreateObject<RandomPropagationLossModel>();
    randomLoss->SetAttribute("Variable", PointerValue(x));

    loss->SetNext(randomLoss);

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();

    Ptr<LoraChannel> channel = CreateObject<LoraChannel>(loss, delay);

        /*********************************************************************************************************
    ***************************************** Continue configuring EDs and GWs *******************************
    *********************************************************************************************************/
    // Create the LoraPhyHelper
    LoraPhyHelper phyHelper = LoraPhyHelper();
    phyHelper.SetChannel(channel);

    // Create the LorawanMacHelper
    LorawanMacHelper macHelper = LorawanMacHelper();

    // Create the LoraHelper
    LoraHelper helper = LoraHelper();
    helper.EnablePacketTracking();

    // Create the LoraNetDevices of the gateways
    phyHelper.SetDeviceType(LoraPhyHelper::GW);
    macHelper.SetDeviceType(LorawanMacHelper::GW);
    helper.Install(phyHelper, macHelper, gateways);


    // Create a LoraDeviceAddressGenerator
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator>(nwkId, nwkAddr);

    // Create the LoraNetDevices of the end devices
    phyHelper.SetDeviceType(LoraPhyHelper::ED);
    macHelper.SetDeviceType(LorawanMacHelper::ED_A);
    macHelper.SetAddressGenerator(addrGen);
    macHelper.SetRegion(LorawanMacHelper::EU);
    NetDeviceContainer endDevicesNetDevices = helper.Install(phyHelper, macHelper, endDevices);

    // Install applications in end devices
    int appPeriodSeconds = 1200; // One packet every 20 minutes
    PeriodicSenderHelper appHelper = PeriodicSenderHelper();
    appHelper.SetPeriod(Seconds(appPeriodSeconds));
    ApplicationContainer appContainer = appHelper.Install(endDevices);

    /******************************************************************************************************
    ************************************* Create network server *******************************************
    ******************************************************************************************************/
    Ptr<Node> networkServer = CreateObject<Node>();

    // PointToPoint links between gateways and server
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    // Store network server app registration details for later
    P2PGwRegistration_t gwRegistration;
    for (auto gw = gateways.Begin(); gw != gateways.End(); ++gw)
    {
        auto container = p2p.Install(networkServer, *gw);
        auto serverP2PNetDev = DynamicCast<PointToPointNetDevice>(container.Get(0));
        gwRegistration.emplace_back(serverP2PNetDev, *gw);
    }

    // Install the NetworkServer application on the network server
    NetworkServerHelper networkServerHelper;
    // networkServerHelper.EnableAdr(adrEnabled);
    // networkServerHelper.SetAdr(adrType);
    networkServerHelper.SetGatewaysP2P(gwRegistration);
    networkServerHelper.SetEndDevices(endDevices);
    networkServerHelper.Install(networkServer);

    // Install the Forwarder application on the gateways
    ForwarderHelper forwarderHelper;
    forwarderHelper.Install(gateways);

    /**********************************************************************************************************
    *************************************** Install Energy Model *********************************************
    **********************************************************************************************************/
    BasicEnergySourceHelper basicSourceHelper; // we can also use LiIonEnergySourceHelper or BasicEnergyHarvester
    LoraRadioEnergyModelHelper radioEnergyHelper;

    // configure energy source
    basicSourceHelper.Set("BasicEnergySourceInitialEnergyJ", DoubleValue(10000)); // Energy in J
    basicSourceHelper.Set("BasicEnergySupplyVoltageV", DoubleValue(3.3));

    // add a logic so that the current or voltage or energy consumption changes according to the tx power or duration
    radioEnergyHelper.Set("StandbyCurrentA", DoubleValue(0.0014));
    radioEnergyHelper.Set("TxCurrentA", DoubleValue(0.028));
    radioEnergyHelper.Set("SleepCurrentA", DoubleValue(0.0000015));
    radioEnergyHelper.Set("RxCurrentA", DoubleValue(0.0112));

    radioEnergyHelper.SetTxCurrentModel("ns3::ConstantLoraTxCurrentModel",
                                        "TxCurrent",
                                        DoubleValue(0.028));

    // install source on end devices' nodes
    EnergySourceContainer ergSources = basicSourceHelper.Install(endDevices);
    Names::Add("/Names/EnergySource", ergSources.Get(0));

    // install device model
    DeviceEnergyModelContainer deviceModels =
        radioEnergyHelper.Install(endDevicesNetDevices, ergSources);

    // Get/save output: the remaining energy of the nodes


    //++++++++++++++++++++++++++++++++++++End of energy module configuration+++++++++++++++++++++++++++++++++++++//

    ////////////////////////////////////////// Running Simulation ////////////////////////////////////
    // Run simulation
    Simulator::Stop(Seconds(4*1200));  // Run for 100 seconds
    Simulator::Run();
    for (auto it = endDevices.Begin(); it != endDevices.End(); ++it)
    {
        Ptr<Node> Ed = *it;
        // Adding a packet trace callback for receiving packets
        Ptr<LoraNetDevice> device = Ed->GetDevice(0)->GetObject<LoraNetDevice>();
        if (!device)
        {
            std::cerr << "Error: Could not retrieve LoraNetDevice for node " << (*it)->GetId() << std::endl;
            continue;
        }
        Ptr<EndDeviceLorawanMac> mac = device->GetMac()->GetObject<EndDeviceLorawanMac>();
        if (!mac) {
            std::cerr << "Error: Could not retrieve EndDeviceLorawanMac for node " << (*it)->GetId() << std::endl;
            continue;
        }

        Ptr<EndDeviceLoraPhy> phy = device->GetPhy()->GetObject<EndDeviceLoraPhy>();
        if (!phy)
        {
            std::cerr << "Error: Could not retrieve EndDeviceLoraPhy for node " << (*it)->GetId() << std::endl;
            continue;
        }
        uint8_t dr = mac->GetDataRate();
        uint8_t tp = mac->GetTransmissionPower();
        LoraDeviceAddress edAddr = mac->GetDeviceAddress();
        uint8_t sf = phy->GetSpreadingFactor();
        std::cout << edAddr << " Dr: " << dr << " tp: "<< tp << " sf:" << sf << std::endl;

    }

    Simulator::Destroy();

    return 0;
}
