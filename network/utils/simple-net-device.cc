/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "simple-net-device.h"
#include "simple-channel.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/tag.h"
#include "ns3/simulator.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/network-module.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SimpleNetDevice");

/**
 * \brief SimpleNetDevice tag to store source, destination and protocol of each packet.
 */
class SimpleTag : public Tag {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);

  /**
   * Set the source address
   * \param src source address
   */
  void SetSrc (Mac48Address src);
  /**
   * Get the source address
   * \return the source address
   */
  Mac48Address GetSrc (void) const;

  /**
   * Set the destination address
   * \param dst destination address
   */
  void SetDst (Mac48Address dst);
  /**
   * Get the destination address
   * \return the destination address
   */
  Mac48Address GetDst (void) const;

  /**
   * Set the protocol number
   * \param proto protocol number
   */
  void SetProto (uint16_t proto);
  /**
   * Get the protocol number
   * \return the protocol number
   */
  uint16_t GetProto (void) const;

  void Print (std::ostream &os) const;

private:
  Mac48Address m_src; //!< source address
  Mac48Address m_dst; //!< destination address
  uint16_t m_protocolNumber; //!< protocol number
};


NS_OBJECT_ENSURE_REGISTERED (SimpleTag);

TypeId
SimpleTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleTag")
    .SetParent<Tag> ()
    .SetGroupName("Network")
    .AddConstructor<SimpleTag> ()
  ;
  return tid;
}
TypeId
SimpleTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
SimpleTag::GetSerializedSize (void) const
{
  return 8+8+2;
}
void
SimpleTag::Serialize (TagBuffer i) const
{
  uint8_t mac[6];
  m_src.CopyTo (mac);
  i.Write (mac, 6);
  m_dst.CopyTo (mac);
  i.Write (mac, 6);
  i.WriteU16 (m_protocolNumber);
}
void
SimpleTag::Deserialize (TagBuffer i)
{
  uint8_t mac[6];
  i.Read (mac, 6);
  m_src.CopyFrom (mac);
  i.Read (mac, 6);
  m_dst.CopyFrom (mac);
  m_protocolNumber = i.ReadU16 ();
}

void
SimpleTag::SetSrc (Mac48Address src)
{
  m_src = src;
}

Mac48Address
SimpleTag::GetSrc (void) const
{
  return m_src;
}

void
SimpleTag::SetDst (Mac48Address dst)
{
  m_dst = dst;
}

Mac48Address
SimpleTag::GetDst (void) const
{
  return m_dst;
}

void
SimpleTag::SetProto (uint16_t proto)
{
  m_protocolNumber = proto;
}

uint16_t
SimpleTag::GetProto (void) const
{
  return m_protocolNumber;
}

void
SimpleTag::Print (std::ostream &os) const
{
  os << "src=" << m_src << " dst=" << m_dst << " proto=" << m_protocolNumber;
}



NS_OBJECT_ENSURE_REGISTERED (SimpleNetDevice);

TypeId 
SimpleNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SimpleNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("Network") 
    .AddConstructor<SimpleNetDevice> ()
    .AddAttribute ("ReceiveErrorModel",
                   "The receiver error model used to simulate packet loss",
                   PointerValue (),
                   MakePointerAccessor (&SimpleNetDevice::m_receiveErrorModel),
                   MakePointerChecker<ErrorModel> ())
    .AddAttribute ("PointToPointMode",
                   "The device is configured in Point to Point mode",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SimpleNetDevice::m_pointToPointMode),
                   MakeBooleanChecker ())
    .AddAttribute ("TxQueue",
                   "A queue to use as the transmit queue in the device.",
                   StringValue ("ns3::DropTailQueue"),
                   MakePointerAccessor (&SimpleNetDevice::m_queue),
                   MakePointerChecker<Queue> ())
    .AddAttribute ("DataRate",
                   "The default data rate for point to point links. Zero means infinite",
                   DataRateValue (DataRate ("0b/s")),
                   MakeDataRateAccessor (&SimpleNetDevice::m_bps),
                   MakeDataRateChecker ())
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been dropped "
                     "by the device during reception",
                     MakeTraceSourceAccessor (&SimpleNetDevice::m_phyRxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("ReadyQueue",
                   "A queue to use as the transmit queue in the device.",
                   StringValue ("ns3::DropTailQueue"),
                   MakePointerAccessor (&SimpleNetDevice::r_queue),
                   MakePointerChecker<Queue> ())
  ;
  return tid;
}

SimpleNetDevice::SimpleNetDevice ()
  : m_channel (0),
    m_node (0),
    m_mtu (0xffff),
    m_ifIndex (0),
    m_linkUp (false)
{
  NS_LOG_FUNCTION (this);
  flag=0;
  send_flag = false;
  ndid = 1;
}
//

void 
SimpleNetDevice::SetGid(uint16_t gid)
{
  m_gid=gid;
}
uint16_t
SimpleNetDevice::GetGid()
{
  return m_gid;
}
void
SimpleNetDevice::SetSid(uint16_t sid)
{
  m_sid=sid;
}
uint16_t
SimpleNetDevice::GetSid()
{ 
  return m_sid;
}
void
SimpleNetDevice::ReceiveStart(Ptr<Packet> packet, uint16_t protocol,
                          Mac48Address to, Mac48Address from)
{
  Simulator::Schedule(Seconds(0.9),&SimpleNetDevice::Receive,this,packet,protocol,to,from);
}                          

void
SimpleNetDevice::Receive (Ptr<Packet> packet, uint16_t protocol,
                          Mac48Address to, Mac48Address from)
{
 // NS_LOG_FUNCTION ("Sid"<<this->GetSid() << packet << protocol << to << from);
  NetDevice::PacketType packetType;


  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet) )
    {
      m_phyRxDropTrace (packet);
      return;
    }

  if (to == m_address)
    {
      packetType = NetDevice::PACKET_HOST;

      LwsnHeader tempHeader;
      packet->PeekHeader(tempHeader);

      if(m_gid>0)
      {
        uint16_t time=Simulator::Now().GetSeconds()-tempHeader.GetStartTime();
        NS_LOG_UNCOND("------------------------");
        NS_LOG_UNCOND("Gate receive : "<<m_gid );
        NS_LOG_UNCOND("Osid : "<<tempHeader.GetOsid() <<" Did : "<< tempHeader.GetDid()<< " Total Time : "<<time);
        NS_LOG_UNCOND("------------------------");

        return;
      }


      if(tempHeader.GetType()==LwsnHeader::ORIGINAL_TRANSMISSION)
      {
        NS_LOG_FUNCTION("1Sid"<<this->GetSid()<<"Receive" << "Osid : "<<tempHeader.GetOsid() << "Did : "<<tempHeader.GetDid());
        Forwarding(packet);
      }
      else if(tempHeader.GetType()==LwsnHeader::FORWARDING){
        if(from == r_address && m_sid < tempHeader.GetOsid()){
          NS_LOG_FUNCTION("2Sid"<<this->GetSid()<<"Receive" << "Osid : "<<tempHeader.GetOsid() << "Did : "<<tempHeader.GetDid());
          Forwarding(packet);
        }
        else if(from == l_address && m_sid > tempHeader.GetOsid()){
          NS_LOG_FUNCTION("3Sid"<<this->GetSid()<<"Receive" << "Osid : "<<tempHeader.GetOsid() << "Did : "<<tempHeader.GetDid());
          Forwarding(packet);
        }
      }
      
    }
  else if (to.IsBroadcast ())
    {
      packetType = NetDevice::PACKET_BROADCAST;
    }
  else if (to.IsGroup ())
    {
      packetType = NetDevice::PACKET_MULTICAST;
    }
  else 
    {
      packetType = NetDevice::PACKET_OTHERHOST;
    }

  if (packetType != NetDevice::PACKET_OTHERHOST)
    {
      m_rxCallback (this, packet, protocol, from);
    }

  if (!m_promiscCallback.IsNull ())
    {
      m_promiscCallback (this, packet, protocol, from, to, packetType);
    }
  
  
}


void 
SimpleNetDevice::SetChannel (Ptr<SimpleChannel> channel)
{
  NS_LOG_FUNCTION (this << channel);
  m_channel = channel;
  m_channel->Add (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

Ptr<Queue>
SimpleNetDevice::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
SimpleNetDevice::SetQueue (Ptr<Queue> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
SimpleNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void 
SimpleNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}
uint32_t 
SimpleNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}
Ptr<Channel> 
SimpleNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}
void
SimpleNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}
//
void
SimpleNetDevice::SetSideAddress(Address laddress, Address raddress){
  l_address=Mac48Address::ConvertFrom (laddress);
  r_address=Mac48Address::ConvertFrom (raddress);
}

Mac48Address
SimpleNetDevice::GetLeftAddress(){
  return l_address;
}

Mac48Address
SimpleNetDevice::GetRightAddress(){
  return r_address;
}

Address 
SimpleNetDevice::GetAddress (void) const
{
  //
  // Implicit conversion from Mac48Address to Address
  //
  NS_LOG_FUNCTION (this);
  return m_address;
}
bool 
SimpleNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}
uint16_t 
SimpleNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}
bool 
SimpleNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}
void 
SimpleNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
 NS_LOG_FUNCTION (this << &callback);
 m_linkChangeCallbacks.ConnectWithoutContext (callback);
}
bool 
SimpleNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
Address
SimpleNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}
bool 
SimpleNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
Address 
SimpleNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address SimpleNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool 
SimpleNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return true;
    }
  return false;
}

bool 
SimpleNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}
void 
SimpleNetDevice::ChannelSend(Ptr<Packet> p, uint16_t protocol){
        NS_LOG_FUNCTION("Sid"<<m_sid);
        m_channel->Send(p, protocol, r_address, m_address, this);
        m_channel->Send(p, protocol, l_address, m_address, this);
        Simulator::Schedule(Seconds(0.9),&SimpleNetDevice::SetSleep,this);
}
void
SimpleNetDevice::SetSleep(){
  //NS_LOG_FUNCTION("Sid"<<m_sid);
  send_flag = false;
}

// void
// SimpleNetDevice::WaitSend(){
//   if(m_queue->GetNPackets()>0){

//     send_flag = true;

//     Ptr<Packet> packet = m_queue->Dequeue()->GetPacket();
//     LwsnHeader temp;
//     packet -> PeekHeader(temp);

//     if(temp.GetType()==LwsnHeader::FORWARDING){
//       Forwarding(packet);
//     }
//     else{
//       OriginalTransmission(packet,true);
//     }
//   }
//   return;
// }
void
SimpleNetDevice::Forwarding(Ptr<Packet> p){
    NS_LOG_FUNCTION("Sid : "<<m_sid);
    Ptr<Packet> packet = p->Copy();

    LwsnHeader tempHeader;
    packet->RemoveHeader(tempHeader);

    LwsnHeader forwardingheader;
    forwardingheader.SetOsid(tempHeader.GetOsid());
    forwardingheader.SetDid(tempHeader.GetDid());
    forwardingheader.SetType(LwsnHeader::FORWARDING);
    forwardingheader.SetStartTime(tempHeader.GetStartTime());

    packet->AddHeader(forwardingheader);
    OriginalTransmission(packet,true);
/*
  //if(!send_flag){
    if (m_queue->Enqueue (Create<QueueItem> (packet)))
      {
        if(m_queue->GetNPackets()==1 && !TransmitCompleteEvent.IsRunning ()){
  //  send_flag = true;
          packet = m_queue->Dequeue()->GetPacket ();

          int delay=Simulator::Now().GetSeconds();
          int slot=0;
          slot=delay%3;
          int protocolNumber = 0;

          if(m_sid%3==1){
                       
            if(slot==0){
              Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,packet,protocolNumber);
            }
            else if(slot==1){
              Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
            }
            else{
              Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
            }
         
          }
          else if(m_sid%3==2){
                        
            if(slot==1){
              Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,packet,protocolNumber);
            }
            else if(slot==2){
              Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
            }
            else{
              Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
            }
          }
          else{
                        
            if(slot==2){
              Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,packet,protocolNumber);
            }
            else if(slot==0){
              Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
            }
            else{
              Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
            }
          }
        TransmitCompleteEvent = Simulator::Schedule (Seconds(0+1.0), &SimpleNetDevice::TransmitComplete, this);

        }
      }
  // else{
  //   m_queue->Enqueue(Create<QueueItem>(packet));
  // }*/

}

void
SimpleNetDevice::OriginalTransmission(Ptr<Packet> p,bool header){
  //NS_LOG_FUNCTION("Sid : "<<m_sid);
  Ptr<Packet> packet = p->Copy ();
  LwsnHeader sendheader;
  LwsnHeader tempHeader;

  packet ->RemoveHeader(tempHeader);

  if(header){
    sendheader.SetOsid(tempHeader.GetOsid());
    sendheader.SetDid(tempHeader.GetDid());
    sendheader.SetType(tempHeader.GetType());
    sendheader.SetStartTime(tempHeader.GetStartTime());
  }
  else{
    sendheader.SetOsid(m_sid);
    sendheader.SetDid(ndid++);
    sendheader.SetType(LwsnHeader::ORIGINAL_TRANSMISSION);
    sendheader.SetStartTime(Simulator::Now().GetSeconds());
  }
  int protocolNumber = 0;
  packet->AddHeader(sendheader);

  if(send_flag){
    NS_LOG_FUNCTION("-------------Sid : "<<m_sid<<"   sending~~");
    if(TransmitCompleteEvent.IsRunning()){
      m_queue->Enqueue (Create<QueueItem> (packet));
    }
    else{
     Simulator::Schedule(Seconds(1.0),&SimpleNetDevice::OriginalTransmission,this,packet,true); 
    }
    return;
  }
    if (m_queue->Enqueue (Create<QueueItem> (packet)))
      {
          if(m_queue->GetNPackets()==1 && !TransmitCompleteEvent.IsRunning ()){
            send_flag = true;

            p = m_queue->Dequeue()->GetPacket ();
            Time txTime = Time (0);
            if (m_bps > DataRate (0))
              {
                //txTime=Seconds(1.0);
                txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
              }
              int delay=Simulator::Now().GetSeconds()+0.5;
              int slot=0;
              slot=delay%3;
              NS_LOG_UNCOND(m_sid<<" now time : "<<Simulator::Now() <<" slot :"<<slot << " delay : "<<delay);
              if(m_sid%3==1){                  
                    if(slot==0){
                      delay = 0;
                      NS_LOG_UNCOND("00");
                            Simulator::Schedule(Seconds(0.1),&SimpleNetDevice::ChannelSend,this,p,protocolNumber);
                    }
                    else if(slot==1){
                      NS_LOG_UNCOND("22");
                      delay = 2;
                            Simulator::Schedule(Seconds(2.1), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                    }
                    else{
                      NS_LOG_UNCOND("11");
                      delay = 1;
                            Simulator::Schedule(Seconds(1.1), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                    }
                  
              }
              else if(m_sid%3==2){
                    
                    if(slot==1){
                      delay = 0;

                            Simulator::Schedule(Seconds(0.1),&SimpleNetDevice::ChannelSend,this,p,protocolNumber);
                    }
                    else if(slot==2){
                      delay = 2;
                            Simulator::Schedule(Seconds(2.1), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                    }
                    else{
                      delay = 1;
                            Simulator::Schedule(Seconds(1.1), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                    }
               }
              else{
                    
                    if(slot==2){
                      delay = 0;
                            Simulator::Schedule(Seconds(0.1),&SimpleNetDevice::ChannelSend,this,p,protocolNumber);
                    }
                    else if(slot==0){
                      delay = 2;
                            Simulator::Schedule(Seconds(2.1), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                   }
                    else{
                      delay = 1;
                            Simulator::Schedule(Seconds(1.1), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                    }
            }
          TransmitCompleteEvent = Simulator::Schedule (Seconds(delay+3.0+0.1), &SimpleNetDevice::TransmitComplete, this);

          }
          else{
            NS_LOG_FUNCTION("Sid "<<m_sid << "~~~~~~~~~~~~~~~~");
          }
        return ;
      }

  return ;
   
}

bool 
SimpleNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);

  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
SimpleNetDevice::SendFrom (Ptr<Packet> p, const Address& source, const Address& dest, uint16_t protocolNumber)
{

  Ptr<Packet> packet = p->Copy ();
  LwsnHeader sendheader;
  LwsnHeader tempHeader;

  packet ->RemoveHeader(tempHeader);

  if(tempHeader.GetType()==LwsnHeader::FORWARDING || tempHeader.GetType()==LwsnHeader::ORIGINAL_TRANSMISSION){
    sendheader.SetOsid(tempHeader.GetOsid());
    sendheader.SetDid(tempHeader.GetDid());
    sendheader.SetType(tempHeader.GetType());
    sendheader.SetStartTime(tempHeader.GetStartTime());
  }
  else{
    sendheader.SetOsid(m_sid);
    sendheader.SetDid(ndid++);
    sendheader.SetType(LwsnHeader::ORIGINAL_TRANSMISSION);
    sendheader.SetStartTime(Simulator::Now().GetSeconds());
  }

  packet->AddHeader(sendheader);

    if (m_queue->Enqueue (Create<QueueItem> (packet)))
      {
        if(m_queue->GetNPackets()==1 && !TransmitCompleteEvent.IsRunning ()){
          send_flag = true;

          p = m_queue->Dequeue()->GetPacket ();
          Time txTime = Time (0);
          if (m_bps > DataRate (0))
            {
              //txTime=Seconds(1.0);
              txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
            }
            int delay=Simulator::Now().GetSeconds();
            int slot=0;
            slot=delay%3;
            if(m_sid%3==1){                  
                  if(slot==0){
                    delay = 0;
                          Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,protocolNumber);
                  }
                  else if(slot==1){
                    delay = 2;
                          Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                  }
                  else{
                    delay = 1;
                          Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                  }
                
            }
            else if(m_sid%3==2){
                  
                  if(slot==1){
                    delay = 0;

                          Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,protocolNumber);
                  }
                  else if(slot==2){
                    delay = 2;
                          Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                  }
                  else{
                    delay = 1;
                          Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                  }
             }
            else{
                  
                  if(slot==2){
                    delay = 0;
                          Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,p,protocolNumber);
                  }
                  else if(slot==0){
                    delay = 2;
                          Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                 }
                  else{
                    delay = 1;
                          Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, p, protocolNumber);
                  }
          }
        TransmitCompleteEvent = Simulator::Schedule (Seconds(delay+3.0), &SimpleNetDevice::TransmitComplete, this);

        }
        else{
          TransmitCompleteEvent = Simulator::Schedule (Seconds(3.0), &SimpleNetDevice::TransmitComplete, this);
        }
      return true;
    }

  return true ;
   
}



void
SimpleNetDevice::TransmitComplete ()
{
  if (m_queue->GetNPackets () == 0)
    {
      return;
    }
  NS_LOG_FUNCTION ("Sid"<<this->GetSid() <<"TransmitComplete");
  send_flag = true;
  Ptr<Packet> packet = m_queue->Dequeue ()->GetPacket ();

  int delay=Simulator::Now().GetSeconds();
  NS_LOG_FUNCTION(" time now : "<<Simulator::Now().GetSeconds() << "Delay : "<<delay );
  int slot=0;
  int protocolNumber = 0;
  slot=delay%3;


  if(m_sid%3==1){
                  
    if(slot==0){
      delay = 0;
      Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,packet,protocolNumber);
      }
    else if(slot==1){
      delay = 2;
      Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
    }
    else{
      delay = 1;
      Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
    }                
  }
  else if(m_sid%3==2){
                  
    if(slot==1){
      delay = 0;
      Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,packet,protocolNumber);
    }
    else if(slot==2){
      delay = 2;
      Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
    }
    else{
      delay = 1;
      Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
    }
  }
  else{
                  
    if(slot==2){
      delay = 0;
      Simulator::ScheduleNow(&SimpleNetDevice::ChannelSend,this,packet,protocolNumber);
    }
    else if(slot==0){
      delay = 2;
      Simulator::Schedule(Seconds(2.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
    }
    else{
      delay = 1;
      Simulator::Schedule(Seconds(1.0), &SimpleNetDevice::ChannelSend, this, packet, protocolNumber);
    }
  }
   if (m_queue->GetNPackets ())
     {
      TransmitCompleteEvent = Simulator::Schedule (Seconds(delay+3.0), &SimpleNetDevice::TransmitComplete, this);
    }

  // if (m_queue->GetNPackets ())
  //    {
  //   //   Time txTime = Time (0);
  //   //   if (m_bps > DataRate (0))
  //   //     {
  //   //       //txTime=Seconds(1.0);
  //   //       txTime = m_bps.CalculateBytesTxTime (packet->GetSize ());
  //   //     }
  //     TransmitCompleteEvent = Simulator::Schedule (Seconds(1.0), &SimpleNetDevice::TransmitComplete, this);
  //   }
  //   else{
  //     TransmitCompleteEvent = Simulator::Schedule (Seconds(1.0), &SimpleNetDevice::TransmitComplete, this);

  //   }

  return;
}

Ptr<Node> 
SimpleNetDevice::GetNode (void) const
{
  //NS_LOG_FUNCTION (this);
  return m_node;
}
void 
SimpleNetDevice::SetNode (Ptr<Node> node)
{
  //NS_LOG_FUNCTION (this << node);
  m_node = node;
}
bool 
SimpleNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_pointToPointMode)
    {
      return false;
    }
  return true;
}
void 
SimpleNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void
SimpleNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_node = 0;
  m_receiveErrorModel = 0;
  m_queue->DequeueAll ();
  if (TransmitCompleteEvent.IsRunning ())
    {
      TransmitCompleteEvent.Cancel ();
    }
  NetDevice::DoDispose ();
}


void
SimpleNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_promiscCallback = cb;
}

bool
SimpleNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

} // namespace ns3
