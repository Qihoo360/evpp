namespace evldd {
void PacketReader::OnRead(const evpp::TCPConnPtr& conn,
            evpp::Buffer* buf,
            evpp::Timestamp ts) {
    if (read_stat_ == kHeader) {
        const void* data = buf->data();
        if (buf->size() >= kHeaderLen) {
            packet_->ReadHeader(data);
            read_stat_ = kBody;
        }
    }
    if (read_stat_ == kBody) {
        if (buf->size() >= kHeaderLen + packet_->BodySize()) {
            packet_->ReadBody(data);
            read_stat_ = kHeader;
            if (packet_->IsOk()) {
                Done();
            }
            buf->Retrieve(kHeaderLen + packet_->BodySize());
        }
    }
    if (!packet_->IsOk()) {
        Error();
    }
}

}
