namespace evldd {

void OnCodecMessage(const evpp::TCPConnPtr& conn,
                    evpp::Buffer* buf,
                    evpp::Timestamp ts) {
    ChannelPtr channel = any_cast<ChannelPtr>(conn->context());
    if (!channel) {
        channel = ChannelPtr.reset(new IncomingChannel(conn.get(),owner_));
        conn->set_context(channel);
    }
    channel->reader()->OnRead(conn,buf,ts);

}

}
