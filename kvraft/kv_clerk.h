#pragma once

#include "src/args.pb.h"
#include "src/polished_rpc_client.h"

#include "reyao/scheduler.h"

namespace raftcpp {

// 客户端通过 KvClerk 的方法向 KvServer 发送请求
// 定义了 get put append del 四种操作
// 内部通过 sendCommand 向 KvServer 发送 rpc 请求
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
	int latestLeaderId_;
	std::vector<PolishedRpcClient::SPtr> peers_;
};

} // namepsace raftcpp