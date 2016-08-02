#pragma once

#include <string>
#include <vector>
#include <memory>

namespace evmc {

class Random;
extern const uint16_t BAD_SERVER_ID;

class VbucketConfig {
public:
    VbucketConfig();
    virtual ~VbucketConfig();
    uint16_t GetVbucketByKey(const char* key, size_t nkey) const;
    uint16_t SelectServerId(uint16_t vbucket, uint16_t last_id) const;
    std::string GetServerAddrById(uint16_t server_id) const;

    void OnVbucketResult(uint16_t vbucket, bool success);

    bool Load(const char* json_file);
    const std::vector<std::string>& server_list() const {
        return server_list_;
    };
private:
    int replicas_;
    std::string algorithm_;
    std::vector<std::string> server_list_;
    mutable std::vector<int> server_health_; // value为健康值，越高越好
    Random* rand_;

    std::vector<std::vector<int> > vbucket_map_;
};

typedef std::shared_ptr<VbucketConfig> VbucketConfigPtr;

class MultiModeVbucketConfig : public VbucketConfig {
	public:
		uint16_t GetVbucketByKey(const char* key, size_t nkey) const;
		uint16_t SelectServerId(uint16_t vbucket, uint16_t last_id) const;
		std::string GetServerAddrById(uint16_t server_id) const;
		const std::vector<std::string>& server_list() const;
		bool Load(const char* json_file);
	private:
		bool IsStandAlone(const char *serv);
	private:
		int mode_;
		std::vector<std::string> single_server_;
};
typedef std::shared_ptr<MultiModeVbucketConfig> MultiModeVbucketConfigPtr;

}

