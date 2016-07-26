#pragma once

namespace evpp {
    class TCPConn;
}

namespace evldd {

class PacketReader;
class PacketWriter;

class ChannelBase : public std::enable_shared_from_this<ChannelBase> {
public:
    explicit ChannelBase(const evpp::TCPConn* conn);
    virtual ~ChannelBase();

    evpp::TCPConn* conn();

    virtual PacketReader* reader() = 0;
    virtual PacketWriter* writer() = 0;
    
    virtual void OnPacketDone(PacketReader* reader) = 0;
    virtual void OnPacketError(PacketReader* reader) = 0;

    virtual void Close() = 0;

protected:
    evpp::TCPConn*  conn_;
}

class IncomingChannel : public ChannelBase {

public:
    IncomingChannel(const evpp::TCPConn* conn, Server* owner);
    virtual ~IncomingChannel();

    virtual PacketReader* reader() ;
    virtual PacketWriter* writer() ;
    
    virtual void OnPacketDone(PacketReader* reader) ;
    virtual void OnPacketError(PacketReader* reader);

private:
    Server*                         owner_;
    std::shared_ptr<PacketReader>   reader_;
    std::shared_ptr<PacketWriter>   writer_;
}

}
