#include <evpp/tcp_conn.h>
#include "codec.h"
#include "channel.h"
#include "packet_reader.h"
#include <evpp/any.h>

namespace evldd {

void Codec::OnCodecMessage(const evpp::TCPConnPtr& conn,
                    evpp::Buffer* buf,
                    evpp::Timestamp ts) {
    ChannelPtr channel = evpp::any_cast<ChannelPtr>(conn->context());
    if (!channel) {
        channel.reset(new IncomingChannel(conn.get(),owner_));
        conn->set_context(evpp::Any(channel));
    }
    channel->reader()->OnRead(conn,buf,ts);

}

}
