#pragma once

#include <evpp/tcp_conn.h>

namespace evldd {

class IncomingPacket;

class PacketReader {
    enum ReadStat {
        kHeader,
        kBody,
    };
public:
    PacketReader();
    virtual ~PacketReader();

    void OnRead(const evpp::TCPConnPtr& conn,
                evpp::Buffer* buf,
                evpp::Timestamp ts);
public:
    const IncomingPacket* packet() const {
        return packet_;
    }

protected:
    virtual void Read()     = 0;
    virtual void Done()     = 0;
    virtual void Error()    = 0;

protected:
    ReadStat         read_stat_;
    IncomingPacket*  packet_;
};


}
