/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "tag-buffer.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <cstring>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TagBuffer");

#ifndef TAG_BUFFER_USE_INLINE

void
TagBuffer::WriteU8(uint8_t v)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(v));
    NS_ASSERT(m_current + 1 <= m_end);
    *m_current = v;
    m_current++;
}

void
TagBuffer::WriteU16(uint16_t data)
{
    NS_LOG_FUNCTION(this << data);
    WriteU8((data >> 0) & 0xff);
    WriteU8((data >> 8) & 0xff);
}

void
TagBuffer::WriteU32(uint32_t data)
{
    NS_LOG_FUNCTION(this << data);
    WriteU8((data >> 0) & 0xff);
    WriteU8((data >> 8) & 0xff);
    WriteU8((data >> 16) & 0xff);
    WriteU8((data >> 24) & 0xff);
}

uint8_t
TagBuffer::ReadU8()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_current + 1 <= m_end);
    uint8_t v;
    v = *m_current;
    m_current++;
    return v;
}

uint16_t
TagBuffer::ReadU16()
{
    NS_LOG_FUNCTION(this);
    uint8_t byte0 = ReadU8();
    uint8_t byte1 = ReadU8();
    uint16_t data = byte1;
    data <<= 8;
    data |= byte0;
    return data;
}

uint32_t
TagBuffer::ReadU32()
{
    NS_LOG_FUNCTION(this);
    uint8_t byte0 = ReadU8();
    uint8_t byte1 = ReadU8();
    uint8_t byte2 = ReadU8();
    uint8_t byte3 = ReadU8();
    uint32_t data = byte3;
    data <<= 8;
    data |= byte2;
    data <<= 8;
    data |= byte1;
    data <<= 8;
    data |= byte0;
    return data;
}

#endif /* TAG_BUFFER_USE_INLINE */

void
TagBuffer::WriteU64(uint64_t data)
{
    NS_LOG_FUNCTION(this << data);
    WriteU8((data >> 0) & 0xff);
    WriteU8((data >> 8) & 0xff);
    WriteU8((data >> 16) & 0xff);
    WriteU8((data >> 24) & 0xff);
    WriteU8((data >> 32) & 0xff);
    WriteU8((data >> 40) & 0xff);
    WriteU8((data >> 48) & 0xff);
    WriteU8((data >> 56) & 0xff);
}

void
TagBuffer::WriteDouble(double v)
{
    NS_LOG_FUNCTION(this << v);
    auto buf = (uint8_t*)&v;
    for (uint32_t i = 0; i < sizeof(double); ++i, ++buf)
    {
        WriteU8(*buf);
    }
}

void
TagBuffer::Write(const uint8_t* buffer, uint32_t size)
{
    NS_LOG_FUNCTION(this << &buffer << size);
    for (uint32_t i = 0; i < size; ++i, ++buffer)
    {
        WriteU8(*buffer);
    }
}

uint64_t
TagBuffer::ReadU64()
{
    NS_LOG_FUNCTION(this);
    uint8_t byte0 = ReadU8();
    uint8_t byte1 = ReadU8();
    uint8_t byte2 = ReadU8();
    uint8_t byte3 = ReadU8();
    uint8_t byte4 = ReadU8();
    uint8_t byte5 = ReadU8();
    uint8_t byte6 = ReadU8();
    uint8_t byte7 = ReadU8();
    uint64_t data = byte7;
    data <<= 8;
    data |= byte6;
    data <<= 8;
    data |= byte5;
    data <<= 8;
    data |= byte4;
    data <<= 8;
    data |= byte3;
    data <<= 8;
    data |= byte2;
    data <<= 8;
    data |= byte1;
    data <<= 8;
    data |= byte0;

    return data;
}

double
TagBuffer::ReadDouble()
{
    NS_LOG_FUNCTION(this);
    double v;
    auto buf = (uint8_t*)&v;
    for (uint32_t i = 0; i < sizeof(double); ++i, ++buf)
    {
        *buf = ReadU8();
    }
    return v;
}

void
TagBuffer::Read(uint8_t* buffer, uint32_t size)
{
    NS_LOG_FUNCTION(this << &buffer << size);
    std::memcpy(buffer, m_current, size);
    m_current += size;
    NS_ASSERT(m_current <= m_end);
}

TagBuffer::TagBuffer(uint8_t* start, uint8_t* end)
    : m_current(start),
      m_end(end)
{
    NS_LOG_FUNCTION(this << &start << &end);
}

void
TagBuffer::TrimAtEnd(uint32_t trim)
{
    NS_LOG_FUNCTION(this << trim);
    NS_ASSERT(m_current <= (m_end - trim));
    m_end -= trim;
}

void
TagBuffer::CopyFrom(TagBuffer o)
{
    NS_LOG_FUNCTION(this << &o);
    NS_ASSERT(o.m_end >= o.m_current);
    NS_ASSERT(m_end >= m_current);
    uintptr_t size = o.m_end - o.m_current;
    NS_ASSERT(size <= (uintptr_t)(m_end - m_current));
    std::memcpy(m_current, o.m_current, size);
    m_current += size;
}

} // namespace ns3
