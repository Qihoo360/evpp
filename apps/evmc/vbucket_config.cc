#include "vbucket_config.h"

#include <map>
#include <cassert>

#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <libhashkit/hashkit.h>

#include "extract_vbucket_conf.h"
#include "likely.h"


namespace evmc {

const uint16_t BAD_SERVER_ID = 65535;

VbucketConfig::VbucketConfig() {
}

VbucketConfig::~VbucketConfig() {
}

enum {
    INC_WEIGHT = 1,
    MAX_WEIGHT = 50,
    MIN_WEIGHT = 2,
    CLUSTER_MODE = 1,
    STAND_ALONE_MODE = 2,
};

void VbucketConfig::OnVbucketResult(uint16_t vbucket, bool success) {
    // 捎带更新健康值，不专门更新. 这样该函数就是多余的

    // 健康值/权重更新策略:
    // 1. 健康值快速(指数)衰减，慢速(线性)恢复
    // 2. N个replica，目前是选不同端口重试两次. 是否需要全部重试一遍？
    // 3. 更新健康值时，兼顾线程安全和性能
    return;
}

uint16_t VbucketConfig::SelectServerFirstId(uint16_t vbucket) const {
    uint16_t vb = vbucket % vbucket_map_.size();
    const std::vector<int>& server_ids = vbucket_map_[vb];
    return server_ids[0];
}

uint16_t VbucketConfig::SelectServerId(uint16_t vbucket, std::vector<uint16_t>& last_ids) {
    uint16_t vb = vbucket % vbucket_map_.size();
    uint16_t server_id = BAD_SERVER_ID;

    const std::vector<int>& server_ids = vbucket_map_[vb];
    if (last_ids.size() >= server_ids.size()) {
        return server_id;
    }
    thread_local std::vector<int32_t> server_health;
    if (server_health.empty()) {
        server_health.resize(server_list().size(), MAX_WEIGHT);
    }


    {
        // 按健康权重选定server id
        std::map<int64_t, uint16_t> weighted_items;
        int64_t total_weight = 0;

        auto begin = last_ids.begin();
        for (size_t i = 0 ; i < server_ids.size(); ++i) {
            for (begin = last_ids.begin(); begin != last_ids.end(); ++begin) {
                if (server_ids[i] == *begin) {
                    break;
                }
            }
            if (begin != last_ids.end()) {
                continue;
            }

            total_weight += server_health[server_ids[i]];
            // total_weight += 1000; // for test only
            weighted_items[total_weight] = server_ids[i];
        }

        if (total_weight > 0) {
            server_id = weighted_items.upper_bound(rand() % total_weight)->second;
        } else {
            return BAD_SERVER_ID;
        }
    }

    // 捎带更新健康值，不专门更新
    server_health[server_id] += INC_WEIGHT;

    if (server_health[server_id] > MAX_WEIGHT) {
        server_health[server_id] = MAX_WEIGHT;
    }

    uint16_t last_id = BAD_SERVER_ID;
    if (last_ids.size() > 0) {
        last_id = *last_ids.rbegin();
    }
    LOG_DEBUG << "SelectServerId selected_server_id=" << server_id << " last_id=" << *last_ids.rbegin();

    if (last_id < server_health.size()) {
        server_health[last_id] /= 2;

        if (server_health[last_id] <= MIN_WEIGHT) {
            server_health[last_id] = MIN_WEIGHT;
        }
    }

    return server_id;
}


static hashkit_hash_algorithm_t algorithm(const std::string& alg) {
    if (alg == "MD5") {
        return HASHKIT_HASH_MD5;
    }
    return HASHKIT_HASH_MAX;
}

uint16_t VbucketConfig::GetVbucketByKey(const char* key, size_t nkey) const {
    uint32_t digest = libhashkit_digest(key, nkey, algorithm(algorithm_));
    return digest % vbucket_map_.size();
}

bool VbucketConfig::Load(const char* json_info) {
    rapidjson::Document d;

    d.Parse(json_info);

    replicas_ = d["numReplicas"].GetInt();
    algorithm_ = d["hashAlgorithm"].GetString();

    rapidjson::Value& servers = d["serverList"];
    LOG_DEBUG << "server count = " << servers.Size();

    for (rapidjson::SizeType i = 0; i < servers.Size(); i++) {
        server_list_.emplace_back(servers[i].GetString());
    }

    rapidjson::Value& vbuckets = d["vBucketMap"];

    for (rapidjson::SizeType i = 0; i < vbuckets.Size(); i++) {
        rapidjson::Value& ids = vbuckets[i];
        vbucket_map_.emplace_back(std::vector<int>());

        for (rapidjson::SizeType j = 0; j < ids.Size(); j++) {
            vbucket_map_.back().emplace_back(ids[j].GetInt());
        }

    }

    return true;
}

bool MultiModeVbucketConfig::IsStandAlone(const char* serv) {
    bool has_semicolon = false;
    int i = 0;
    char c = *(serv + i);
    while (c != '\0') {
        if (c == ':') {
            has_semicolon = true;
        }
        if (c == '/') {
            break;
        }
        ++i;
        c = *(serv + i);
    }
    if (c == '\0' && has_semicolon) {
        return true;
    }
    return false;
}

bool MultiModeVbucketConfig::Load(const char* json_file) {
    if (IsStandAlone(json_file)) {
        mode_ = STAND_ALONE_MODE;
        single_server_.emplace_back(json_file);
        return true;
    } else {
        mode_ = CLUSTER_MODE;
        std::string context;
        int ret = GetVbucketConf::GetVbucketConfContext(json_file, context);
        if (ret == 0) {
            return VbucketConfig::Load(context.c_str());
        }
        return false;
    }
}


uint16_t MultiModeVbucketConfig::GetVbucketByKey(const char* key, size_t nkey) const {
    if (mode_ != STAND_ALONE_MODE) {
        return VbucketConfig::GetVbucketByKey(key, nkey);
    }
    return 0;
}

uint16_t MultiModeVbucketConfig::SelectServerFirstId(uint16_t vbucket) const {
    if (mode_ != STAND_ALONE_MODE) {
        return VbucketConfig::SelectServerFirstId(vbucket);
    }
    return 0;
}

uint16_t MultiModeVbucketConfig::SelectServerId(uint16_t vbucket, std::vector<uint16_t>& last_id) {
    if (mode_ != STAND_ALONE_MODE) {
        return VbucketConfig::SelectServerId(vbucket, last_id);
    }
    return 0;
}

const std::string& MultiModeVbucketConfig::GetServerAddrById(uint16_t server_id) const {
    if (mode_ != STAND_ALONE_MODE) {
        return VbucketConfig::GetServerAddrById(server_id);
    }
    assert(single_server_.size() == 1);
    return single_server_[0];
}

const std::vector<std::string>& MultiModeVbucketConfig::server_list() const {
    if (mode_ != STAND_ALONE_MODE) {
        return VbucketConfig::server_list();
    }
    return single_server_;
}

}

