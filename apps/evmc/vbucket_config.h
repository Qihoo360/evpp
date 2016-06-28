#pragma once

#include <string>
#include <vector>
#include <memory>

namespace evmc {

extern const uint16_t BAD_SERVER_ID;

class VbucketConfig {
public:
    uint16_t GetVbucketByKey(const char* key, size_t nkey) const;
    uint16_t SelectServerId(uint16_t vbucket, uint16_t last_id) const;
    std::string GetServerAddrById(uint16_t server_id) const;

    void OnVbucketResult(uint16_t vbucket, bool success);

    bool Load(const char * json_file);
private:
    int replicas_;
    std::string algorithm_;
    std::vector<std::string> server_list_;
    mutable std::vector<int> server_health_; // value为健康值,越高越健康，命中率越高

    std::vector<std::vector<int> > vbucket_map_;
};

typedef std::shared_ptr<VbucketConfig> VbucketConfigPtr;

}

