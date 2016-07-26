#include "packet_reader.h"
#include "packet.h"

namespace evldd {

const static size_t kHeaderLen = 16;

void PacketReader::OnRead(const evpp::TCPConnPtr& conn,
            evpp::Buffer* buf,
            evpp::Timestamp ts) {
    const void* data = buf->data();
    if (read_stat_ == kHeader) {
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
