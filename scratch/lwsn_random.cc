#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mac48-address.h>
#include <ns3/packet.h>
#include <ns3/log.h>
#include <ns3/lwsn-header.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/mobility-module.h>
#include <ns3/net-device.h>
#include <ns3/callback.h>
#include <ns3/object.h>
#include <ns3/lwsn-header.h>

#include <string>
#include <iostream>
#include <math.h>

using namespace ns3;


int main(void)
{
    LogComponentEnableAll(LOG_PREFIX_TIME);
    LogComponentEnableAll(LOG_PREFIX_FUNC);
    Packet::EnablePrinting();
    LogComponentEnable("SimpleNetDevice", LOG_LEVEL_FUNCTION);
    //LogComponentEnable("SimpleNetDevice", LOG_LEVEL_DEBUG);
    //LogComponentEnable("SimpleChannel", LOG_LEVEL_ALL);
    //LogComponentEnable("Packet", LOG_LEVEL_ALL);

    //Config::SetDefault("ns3::SimpleChannel::Delay",TimeValue(Seconds(0.5)));
    // Node Configuration --begin 
    NodeContainer c;
    uint16_t numGateway=2;
    uint16_t numSensor=6;
    uint16_t numNode = numGateway + numSensor;
    c.Create(numNode);
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();    
    Ptr<SimpleChannel> channel = CreateObject<SimpleChannel> ();
    Ptr<SimpleNetDevice> dev[numNode];

    std::string baseAddress = "00:00:00:00:00:";
    
    for(uint16_t i = 0; i < numNode ;i++)
    {
    	dev[i] = CreateObject<SimpleNetDevice> ();
    	c.Get(i)->AddDevice(dev[i]);
    	dev[i]->SetNode(c.Get(i));
    	dev[i]->SetChannel(channel);
    	//position->Add(Vector(i*10.0 ,0.0 ,0.0));
    	dev[i]->SetGid(0);
    	dev[i]->SetSid(i);
    	//dev[i]->SetPosition(i*10);
    	std::stringstream ss;
    	if(i<9){
    		ss << baseAddress << 0 << i+1;
    	}else{
    		ss << baseAddress << i+1;
    	}
    	dev[i] ->SetAddress(Mac48Address(ss.str().c_str()));
    	//std::cout << dev[i]->GetAddress() << std::endl;
    }

    for(uint16_t i = 1; i<numNode-1; i++){
    	dev[i] -> SetSideAddress(dev[i-1]->GetAddress(),dev[i+1]->GetAddress());
    }
	
	// mobility.SetPositionAllocator(position);
 //    mobility.Install(c);
	// mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    
    dev[0]->SetGid(1);			dev[0]->SetSid(0);
    dev[numNode-1]->SetGid(2);  dev[numNode-1]->SetSid(0);

    // dev[0]->SetLowPositionInfo(0,0,0,Mac48Address("00:00:00:00:00:00"));
    // dev[0]->SetHighPositionInfo(dev[1],dev[numNode-1]->GetGid(),dev[1]->GetSid(),
    // 							Mac48Address::ConvertFrom (dev[1]->GetAddress()));
    
    
    // dev[numNode-1]->SetLowPositionInfo(dev[numNode-2],dev[0]->GetGid(),dev[numNode-2]->GetSid(),
    // 							Mac48Address::ConvertFrom(dev[numNode-2]->GetAddress()));
    // dev[numNode-1]->SetHighPositionInfo(0, 0,0,Mac48Address("00:00:00:00:00:00"));
    
    // for(uint16_t i = 1; i<numNode-1;i++)
    // {
    // 	dev[i]->SetLowPositionInfo(dev[i-1],dev[0]->GetGid(),dev[i-1]->GetSid(),
    // 								Mac48Address::ConvertFrom(dev[i-1]->GetAddress()));
    // 	dev[i]->SetHighPositionInfo(dev[i+1],dev[numNode-1]->GetGid(), dev[i+1]->GetSid(), 
    // 								Mac48Address::ConvertFrom(dev[i+1]->GetAddress()));
    // }

	double min = 1.0;
	double max = 6.0;
    RngSeedManager::SetSeed(10);
    // RngSeedManager::SetRun(7);
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
    x->SetAttribute ("Min", DoubleValue (min));
	x->SetAttribute ("Max", DoubleValue (max));

	double minTime = 0.0;
	double maxTime = 10.0;
	RngSeedManager::SetSeed(10);

	Ptr<UniformRandomVariable> t = CreateObject<UniformRandomVariable> ();
	t->SetAttribute ("Min", DoubleValue (minTime));
	t->SetAttribute ("Max", DoubleValue (maxTime));

	int number = 10;
	int randomTime[50]={0,};
	int randomSid[50]={0,};
	for(int i = 0; i<number ; i++){
	  randomSid[i] = x->GetInteger();
	}

	for(int i = 0; i<number ; i++){
	  randomTime[i] =t->GetInteger();
	}

	// int random[50]={0};
	// for(int i=0;i<50;i++){
	//  	random[i]=x->GetInteger();
	//  }
	
	 Ptr<Packet> packet = Create<Packet> (100);
	 for(int i=0;i<number;i++){
	 	std::cout<<randomTime[i]<<","<<randomSid[i]<<std::endl;
	  	Simulator::Schedule(Seconds(randomTime[i]),&SimpleNetDevice::OriginalTransmission,dev[randomSid[i]],packet,false);
	}

	// for(int i = 1; i<numNode-1;i++){
	// 	Simulator::Schedule(Seconds(300.0),&SimpleNetDevice::Print,dev[i]);
	// }


    // Node Configuration -- end
	RngSeedManager::SetSeed(10);

    Simulator::Run ();
  
	Simulator::Destroy ();

	// uint16_t systemSendCount =0;
	// uint16_t systemRetransferCount = 0;

	// for(uint16_t i = 0; i<numNode;i++){
	// 	systemSendCount += dev[i]->sendCount;
	// 	systemRetransferCount += dev[i]->retransferCount;
	// }

	
	// std::cout << "total send count of the system " << systemSendCount<< std::endl;    
	// std::cout << "total retransfer count of the system " << systemRetransferCount<< std::endl;    

	return 0;

}