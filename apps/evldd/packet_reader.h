#pragma once

namespace evldd {

class Packet;

class PacketReader {
    enum ReadStat {
        kHeader;
        kBody;
    }
public:
    PacketReader();
    virtual ~PacketReader();

    void OnRead(const evpp::TCPConnPtr& conn,
                evpp::Buffer* buf,
                evpp::Timestamp ts);
public:
    const Packet* packet() const {
        return packet_;
    }

protected:
    virtual void Read()     = 0;
    virtual void Done()     = 0;
    virtual void Error()    = 0;

protected:
    ReadStat                read_stat_;
    Packet*                 packet_;
}


}
