#include "vbucket_config.h"

#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <libhashkit/hashkit.h>

#include "evpp/exp.h"


namespace evmc {

/*
static uint32_t libhashkit_digest2(const char *key, size_t key_length, hashkit_hash_algorithm_t hash_algorithm);
static uint32_t hashkit_md5(const char *key, size_t key_length, void *context __attribute__((unused)));
*/

std::string VbucketConfig::GetServerAddr(const char* key, size_t nkey) const {
    uint16_t vb = GetVbucketByKey(key, nkey);
    int index = vbucket_map_[vb][0]; // TOTO : 多个互备主机，如何选定?
    return server_list_[index];
}

static hashkit_hash_algorithm_t algorithm(const std::string& alg) {
    if (alg == "MD5") {
        return HASHKIT_HASH_MD5;
    }
    return HASHKIT_HASH_MD5;
}


uint16_t VbucketConfig::GetVbucketByKey(const char* key, size_t nkey) const {
    uint32_t digest = libhashkit_digest(key, nkey, algorithm(algorithm_));
    return digest % vbucket_map_.size();
}

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
uint32_t libhashkit_digest2(const char *key, size_t key_length, hashkit_hash_algorithm_t hash_algorithm) {
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



uint32_t hashkit_md5(const char *key, size_t key_length, void *context __attribute__((unused))) {
    unsigned char results[16];

    libhashkit_md5_signature((const unsigned char*)key, (unsigned int)key_length, results); // 这一步计算MD5

    return ((uint32_t) (results[3] & 0xFF) << 24)
        | ((uint32_t) (results[2] & 0xFF) << 16)
        | ((uint32_t) (results[1] & 0xFF) << 8)
        | (results[0] & 0xFF);
}

*/

}
