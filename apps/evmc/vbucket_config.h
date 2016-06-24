#pragma once

#include <string>
#include <vector>

namespace evmc {

class VbucketConfig {
public:
    std::string GetServerAddr(const char* key, size_t nkey) const;
    std::string GetServerAddr(uint16_t vbucket) const;
    uint16_t GetVbucketByKey(const char* key, size_t nkey) const;

    bool Load(const char * json_file);
private:
    int replicas_;
    std::string algorithm_;
    std::vector<std::string> server_list_;
    std::vector<std::vector<int> > vbucket_map_;
};

}

