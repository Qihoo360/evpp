#pragma once

#include "evmc/config.h"
#include "evpp/buffer.h"
#include "folly/futures/Future.h"
#include "folly/Expected.h"
#include "memcached/protocol_binary.h"

#include "mctypes.h"
#include "likely.h"
#include <type_traits>

namespace evmc {

class Command {
public:
    virtual std::vector<std::string>& send_data() = 0;
    virtual std::vector<int>& vbucketids() = 0;
    virtual std::vector<uint16_t>& server_ids() = 0;
    virtual void  set_server_id(const uint16_t id) = 0;
    virtual uint32_t recv_id() = 0;
    virtual void set_recv_id(const uint32_t r_id) = 0;
    virtual void construct_command(evpp::Buffer* buf) = 0;
    virtual void error(const EvmcCode err) = 0;
};

typedef std::shared_ptr<Command> CommandPtr;

enum CmdTag {
    RemoveTag = 0,
    SetTag,
    GetTag,
    MultiGetTag,
    PrefixGetTag,
};

#define construct_multikey_cmd(proto_kq, proto_k) \
    assert(send_data_.size() > 0);\
    assert(send_data_.size() == vbucketids_.size());\
    std::size_t size = 0; \
    protocol_binary_request_header req;\
    memset((void*)&req, 0, sizeof(req));\
    auto keys = send_data_; \
    const std::size_t keys_num = keys.size();\
    int total_size = 0;\
    for (std::size_t i = 0; i < keys_num; ++i) {\
        total_size += keys[i].size() + sizeof(protocol_binary_request_header);\
    }\
    buf->EnsureWritableBytes(total_size);\
    for (size_t i = 0; i < keys_num; ++i) {\
        size = keys[i].size();\
        req.request.magic    = PROTOCOL_BINARY_REQ;\
        if (i < keys_num - 1) {\
            req.request.opcode   = proto_kq;\
        } else {\
            req.request.opcode   = proto_k;\
        }\
        req.request.keylen = htons(uint16_t(size));\
        req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;\
        req.request.vbucket  = htons(vbucketids_[i]);\
        req.request.opaque   = recv_id_;\
        req.request.extlen = 0;\
        req.request.bodylen  = htonl(size);\
        buf->Write((const void *)&req, sizeof(protocol_binary_request_header));\
        buf->Write((const void *)keys[i].data(), size);\
    }\


#define construct_onekey_cmd(proto) \
    assert(send_data_.size() == 1);\
    assert(vbucketids_.size() == 1);\
    std::string& key = send_data_[0];\
    const int size = sizeof(protocol_binary_request_header) + key.size();\
    buf->EnsureWritableBytes(size);\
    protocol_binary_request_header req;\
    memset((void*)&req, 0, sizeof(req));\
    req.request.magic  = PROTOCOL_BINARY_REQ;\
    req.request.opcode = proto ;\
    req.request.keylen = htons(uint16_t(key.size()));\
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;\
    req.request.extlen = 0;\
    req.request.vbucket  = htons(vbucketids_[0]);\
    req.request.opaque   = recv_id_;\
    req.request.bodylen = htonl(key.size());\
    buf->Write((const void *)&req, sizeof(protocol_binary_request_header));\
    buf->Write((const void *)&key[0], key.size());\


#define construct_keyvalue_cmd(proto) \
    assert(send_data_.size() == 2);\
    assert(vbucketids_.size() == 1);\
    std::string& key = send_data_[0];\
    std::string& value = send_data_[1];\
    uint32_t flag = htonl(0); \
    uint32_t expire = htonl(0);\
    const int size = sizeof(protocol_binary_request_header) + key.size() + value.size() + sizeof(flag) + sizeof(expire);\
    buf->EnsureWritableBytes(size);\
    protocol_binary_request_header req;\
    memset((void*)&req, 0, sizeof(req));\
    req.request.magic  = PROTOCOL_BINARY_REQ;\
    req.request.opcode = proto ;\
    req.request.keylen = htons(uint16_t(key.size()));\
    req.request.datatype = PROTOCOL_BINARY_RAW_BYTES;\
    req.request.extlen = sizeof(flag) + sizeof(expire);\
    req.request.vbucket  = htons(vbucketids_[0]);\
    req.request.opaque   = recv_id_;\
    req.request.bodylen = htonl((uint32_t)(key.size() + value.size() + sizeof(flag) + sizeof(expire)));\
    buf->Write((const void *)&req, sizeof(protocol_binary_request_header));\
    buf->Write((const void *)&flag, sizeof(flag));\
    buf->Write((const void *)&expire, sizeof(expire));\
    buf->Write((const void *)key.data(), key.size());\
    buf->Write((const void *)value.data(), value.size());\

    template<class T, CmdTag tag>
    class CommandImpl final: public Command {
    public:
        CommandImpl(int32_t server_id, std::vector<std::string>& datas, std::vector<int>& vbucket_ids
                    , folly::Promise<folly::Expected<T, EvmcCode>>& result): server_id_(1, server_id)
            , send_data_(std::move(datas)), vbucketids_(std::move(vbucket_ids)), result_(std::move(result)) {
        }

        CommandImpl(const CommandImpl& cmd) = delete;
        CommandImpl& operator=(const CommandImpl& cmd) = delete;
        CommandImpl(CommandImpl&& cmd) : recv_id_(cmd.recv_id_), server_id_(std::move(cmd.server_id_)), send_data_(std::move(cmd.send_data_))
            , vbucketids_(std::move(cmd.vbucketids_)), result_(std::move(cmd.result_)) {
        }

        CommandImpl& operator= (CommandImpl&& cmd) {
            recv_id_ = cmd.recv_id_;
            server_id_.swap(cmd.server_id_);
            vbucketids_ = std::move(cmd.vbucketids_);
            send_data_ = std::move(cmd.send_data_);
            result_ = std::move(cmd.result);
            return *this;
        }

        std::vector<std::string>& send_data() override {
            return send_data_;
        }

        std::vector<int>& vbucketids() override {
            return vbucketids_;
        }

        std::vector<uint16_t>& server_ids() override {
            return server_id_;
        }

        void set_server_id(const uint16_t id) override {
            server_id_.push_back(id);
        }

        uint32_t recv_id() override {
            return recv_id_;
        }

        void set_recv_id(const uint32_t r_id) override {
            recv_id_ = r_id;
        }

        void construct_command(evpp::Buffer* buf) override {
            switch (tag) {
            case MultiGetTag: {
                construct_multikey_cmd(PROTOCOL_BINARY_CMD_GETKQ, PROTOCOL_BINARY_CMD_GETK);
            }
            break;
            case RemoveTag: {
                construct_onekey_cmd(PROTOCOL_BINARY_CMD_DELETE);
            }
            break;
            case PrefixGetTag: {
                construct_multikey_cmd(PROTOCOL_BINARY_CMD_PGETKQ, PROTOCOL_BINARY_CMD_PGETK);
            }
            break;
            case GetTag: {
                construct_onekey_cmd(PROTOCOL_BINARY_CMD_GET);
            }
            break;
            case SetTag: {
                construct_keyvalue_cmd(PROTOCOL_BINARY_CMD_SET);
            }
            break;
            }
        }

        void error(const EvmcCode err) override {
            auto ret = folly::makeUnexpected((EvmcCode)err);
            folly::Expected<T, EvmcCode> result(std::move(ret));
            set_result(result);
        }

        void set_result(folly::Expected<T, EvmcCode>& r) {
            result_.setValue(std::move(r));
        }
    private:
        uint32_t recv_id_{0};
        std::vector<uint16_t> server_id_;
        std::vector<std::string> send_data_;
        std::vector<int> vbucketids_;
        folly::Promise<folly::Expected<T, EvmcCode>> result_;
    };

    typedef folly::Expected<std::map<std::string, std::string>, EvmcCode> str2strExpected;
    typedef folly::Promise<str2strExpected> str2strPromise;
    typedef folly::Future<str2strExpected> str2strFuture;

    typedef folly::Expected<int, EvmcCode> intExpected;
    typedef folly::Promise<intExpected> intPromise;
    typedef folly::Future<intExpected> intFuture;

    typedef folly::Expected<std::string, EvmcCode> stringExpected;
    typedef folly::Promise<stringExpected> stringPromise;
    typedef folly::Future<stringExpected> stringFuture;

    typedef std::map<std::string, std::map<std::string, std::string>> str2map;
    typedef folly::Expected<str2map, EvmcCode> str2mapExpected;
    typedef folly::Promise<str2mapExpected> str2mapPromise;
    typedef folly::Future<str2mapExpected> str2mapFuture;

    typedef CommandImpl<std::map<std::string, std::string>, MultiGetTag> MultiGetCommand;
    typedef CommandImpl<int, RemoveTag> RemoveCommand;
    typedef CommandImpl<int, SetTag> SetCommand;
    typedef CommandImpl<std::string, GetTag> GetCommand;
    typedef CommandImpl<str2map, PrefixGetTag> PrefixGetCommand;
    }

