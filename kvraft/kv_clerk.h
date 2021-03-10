#pragma once

#include "src/args.pb.h"
#include "src/polished_rpc_client.h"

#include "reyao/scheduler.h"

namespace raftcpp {

class KvClerk : public reyao::NoCopyable {
public:
	KvClerk(int64_t cid, const std::vector<PolishedRpcClient::SPtr>& peers);
	bool get(const std::string& key, std::string& value);
	void put(const std::string& key, const std::string& value);
	void append(const std::string& key, const std::string& value);
	void del(const std::string& key);
private:
	std::shared_ptr<KvCommandReply> sendCommand(const std::string& operation,
									const std::string& key,
									const std::string& value = "");

	int64_t cid_;
	uint32_t seq_;
	int latest_leader_id_;
	std::vector<PolishedRpcClient::SPtr> peers_;
};

} // namepsace raftcpp