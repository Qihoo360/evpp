#include "vbucket_config.h"

#include <map>

#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <libhashkit/hashkit.h>

#include "evpp/exp.h"


namespace evmc {

const uint16_t BAD_SERVER_ID = 65535;

const int INIT_WEIGHT = 1000;
const int MAX_WEIGHT  = 1000000;
const int MIN_WEIGHT  = 100;

/*
static uint32_t libhashkit_digest(const char *key, size_t key_length, hashkit_hash_algorithm_t hash_algorithm);
static uint32_t hashkit_md5(const char *key, size_t key_length, void *context __attribute__((unused)));
*/

void VbucketConfig::OnVbucketResult(uint16_t vbucket, bool success) {
    // 捎带更新健康值，不专门更新. 这样该函数就是多余的

    // 详细策略:
    // 1. 健康值
    // 2. N个replica，全部重试一遍，还是只重试一次？
    // 3. 更新健康值时，要兼顾线程安全和性能
    return;
}

uint16_t VbucketConfig::SelectServerId(uint16_t vbucket, uint16_t last_id) const {
    // TODO : 排除failed_id对应的server
    uint16_t vb = vbucket % vbucket_map_.size();

    const std::vector<int>& server_ids = vbucket_map_[vb];

    // int server_id = ids[rand() % vbucket_map_[vb].size()];
    uint16_t server_id = BAD_SERVER_ID;
    {
        // 按健康权重选定server id
        std::map<int64_t, uint16_t> weighted_items;
        int64_t total_weight = 0;
        for(size_t i = 0 ; i < server_ids.size(); ++i) {
            if (server_ids[i] == last_id) {
                continue;
            }
            total_weight += server_health_[server_ids[i]];
            weighted_items[total_weight] = server_ids[i];
        }
        if (total_weight > 0) {
            server_id = weighted_items.upper_bound(rand() % total_weight)->second;
            LOG_DEBUG << "SelectServerId selected_server_id=" << server_id << " last_id=" << last_id;
        } else {
            return BAD_SERVER_ID;
        }
    }

    // 捎带更新健康值，不专门更新
    server_health_[server_id] += 1000;
    if (server_health_[server_id] > MAX_WEIGHT) {
        server_health_[server_id] = MAX_WEIGHT;
    }
    if (last_id < server_health_.size()) {
        server_health_[last_id] /= 2;
        if (server_health_[last_id] <= MIN_WEIGHT) {
            server_health_[last_id] = 100;
        }
    }

    return server_id;
}

std::string VbucketConfig::GetServerAddrById(uint16_t server_id) const {
    return server_list_[server_id];
}

static hashkit_hash_algorithm_t algorithm(const std::string& alg) {
    if (alg == "MD5") {
        return HASHKIT_HASH_MD5;
    }
    return HASHKIT_HASH_MAX;
}

uint16_t VbucketConfig::GetVbucketByKey(const char* key, size_t nkey) const {
    uint32_t digest = libhashkit_digest(key, nkey, algorithm(algorithm_));
    LOG_DEBUG << "GetVbucketByKey key=" << key << " hash=" << digest << " bucket=" << digest % vbucket_map_.size();
    return digest % vbucket_map_.size();
}

// TODO : double buffering
bool VbucketConfig::Load(const char * json_file) {
    rapidjson::Document d;
    {
        // FILE* fp = fopen("./kill_storage_cluster.json", "r");
        FILE* fp = fopen(json_file, "r");
        if (!fp) {
            return false;
        }
        char buf[2048];
        rapidjson::FileReadStream is(fp, buf, sizeof(buf));
        d.ParseStream(is);
    }

    replicas_ = d["numReplicas"].GetInt();
    algorithm_ = d["hashAlgorithm"].GetString();

    rapidjson::Value& servers = d["serverList"];
    LOG_DEBUG << "server count = " << servers.Size();
    for (rapidjson::SizeType i = 0; i < servers.Size(); i++) {
        server_list_.push_back(servers[i].GetString());
        server_health_.push_back(INIT_WEIGHT);
    }


    rapidjson::Value& vbuckets = d["vBucketMap"];
    for (rapidjson::SizeType i = 0; i < vbuckets.Size(); i++) {
        rapidjson::Value& ids = vbuckets[i];
        vbucket_map_.push_back(std::vector<int>());
        for (rapidjson::SizeType j = 0; j < ids.Size(); j++) {
            vbucket_map_.back().push_back(ids[j].GetInt());
        }
        LOG_DEBUG << "vbuckets[" << i << "].size= " << vbucket_map_.back().size();
    }

    return true;
}

/*
uint32_t libhashkit_digest(const char *key, size_t key_length, hashkit_hash_algorithm_t hash_algorithm) {
    switch (hash_algorithm)
    {
        case HASHKIT_HASH_DEFAULT:
            return libhashkit_one_at_a_time(key, key_length);
        case HASHKIT_HASH_MD5:
            return libhashkit_md5(key, key_length);
        case HASHKIT_HASH_CRC:
            return libhashkit_crc32(key, key_length);
        case HASHKIT_HASH_FNV1_64:
            return libhashkit_fnv1_64(key, key_length);
        case HASHKIT_HASH_FNV1A_64:
            return libhashkit_fnv1a_64(key, key_length);
        case HASHKIT_HASH_FNV1_32:
            return libhashkit_fnv1_32(key, key_length);
        case HASHKIT_HASH_FNV1A_32:
            return libhashkit_fnv1a_32(key, key_length);
        case HASHKIT_HASH_HSIEH:
#ifdef HAVE_HSIEH_HASH
            return libhashkit_hsieh(key, key_length);
#else
            return 1;
#endif
        case HASHKIT_HASH_MURMUR:
            return libhashkit_murmur(key, key_length);
        case HASHKIT_HASH_JENKINS:
            return libhashkit_jenkins(key, key_length);
        case HASHKIT_HASH_CUSTOM:
        case HASHKIT_HASH_MAX:
        default:
#ifdef HAVE_DEBUG
            fprintf(stderr, "hashkit_hash_t was extended but libhashkit_generate_value was not updated\n");
            fflush(stderr);
            assert(0);
#endif
            break;
    }

    return 1;
}

uint32_t hashkit_md5_2(const char *key, size_t key_length, void *context __attribute__((unused))) {
    unsigned char results[16];

    libhashkit_md5_signature((const unsigned char*)key, (unsigned int)key_length, results); // 这一步计算MD5

    return ((uint32_t) (results[3] & 0xFF) << 24)
        | ((uint32_t) (results[2] & 0xFF) << 16)
        | ((uint32_t) (results[1] & 0xFF) << 8)
        | (results[0] & 0xFF);
}
*/

}
