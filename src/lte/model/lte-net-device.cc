/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include "lte-net-device.h"

#include "ns3/callback.h"
#include "ns3/channel.h"
#include "ns3/enum.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/llc-snap-header.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet-burst.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteNetDevice");

NS_OBJECT_ENSURE_REGISTERED(LteNetDevice);

////////////////////////////////
// LteNetDevice
////////////////////////////////

TypeId
LteNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LteNetDevice")
            .SetParent<NetDevice>()

            .AddAttribute("Mtu",
                          "The MAC-level Maximum Transmission Unit",
                          UintegerValue(30000),
                          MakeUintegerAccessor(&LteNetDevice::SetMtu, &LteNetDevice::GetMtu),
                          MakeUintegerChecker<uint16_t>());
    return tid;
}

LteNetDevice::LteNetDevice()
{
    NS_LOG_FUNCTION(this);
}

LteNetDevice::~LteNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
LteNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_node = nullptr;
    NetDevice::DoDispose();
}

Ptr<Channel>
LteNetDevice::GetChannel() const
{
    NS_LOG_FUNCTION(this);
    // we can't return a meaningful channel here, because LTE devices using FDD have actually two
    // channels.
    return nullptr;
}

void
LteNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_address = Mac64Address::ConvertFrom(address);
}

Address
LteNetDevice::GetAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_address;
}

void
LteNetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
}

Ptr<Node>
LteNetDevice::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

void
LteNetDevice::SetReceiveCallback(ReceiveCallback cb)
{
    NS_LOG_FUNCTION(this);
    m_rxCallback = cb;
}

bool
LteNetDevice::SendFrom(Ptr<Packet> packet,
                       const Address& source,
                       const Address& dest,
                       uint16_t protocolNumber)
{
    NS_FATAL_ERROR("SendFrom () not supported");
    return false;
}

bool
LteNetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
LteNetDevice::SetMtu(const uint16_t mtu)
{
    NS_LOG_FUNCTION(this << mtu);
    m_mtu = mtu;
    return true;
}

uint16_t
LteNetDevice::GetMtu() const
{
    NS_LOG_FUNCTION(this);
    return m_mtu;
}

void
LteNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    m_ifIndex = index;
}

uint32_t
LteNetDevice::GetIfIndex() const
{
    NS_LOG_FUNCTION(this);
    return m_ifIndex;
}

bool
LteNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION(this);
    return m_linkUp;
}

bool
LteNetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

Address
LteNetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return Mac48Address::GetBroadcast();
}

bool
LteNetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
LteNetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
LteNetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
LteNetDevice::IsBridge() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

Address
LteNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_LOG_FUNCTION(this << multicastGroup);

    Mac48Address ad = Mac48Address::GetMulticast(multicastGroup);

    //
    // Implicit conversion (operator Address ()) is defined for Mac48Address, so
    // use it by just returning the EUI-48 address which is automatically converted
    // to an Address.
    //
    NS_LOG_LOGIC("multicast address is " << ad);

    return ad;
}

Address
LteNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);
    Mac48Address ad = Mac48Address::GetMulticast(addr);

    NS_LOG_LOGIC("MAC IPv6 multicast address is " << ad);
    return ad;
}

void
LteNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    NS_LOG_FUNCTION(this);
    m_linkChangeCallbacks.ConnectWithoutContext(callback);
}

void
LteNetDevice::SetPromiscReceiveCallback(PromiscReceiveCallback cb)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_WARN("Promisc mode not supported");
}

void
LteNetDevice::Receive(Ptr<Packet> p)
{
    NS_LOG_FUNCTION(this << p);

    Ipv4Header ipv4Header;
    Ipv6Header ipv6Header;

    if (p->PeekHeader(ipv4Header) != 0)
    {
        NS_LOG_LOGIC("IPv4 stack...");
        m_rxCallback(this, p, Ipv4L3Protocol::PROT_NUMBER, Address());
    }
    else if (p->PeekHeader(ipv6Header) != 0)
    {
        NS_LOG_LOGIC("IPv6 stack...");
        m_rxCallback(this, p, Ipv6L3Protocol::PROT_NUMBER, Address());
    }
    else
    {
        NS_ABORT_MSG("LteNetDevice::Receive - Unknown IP type...");
    }
}
} // namespace ns3
