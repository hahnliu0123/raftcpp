#include "kv_clerk.h"
#include "common.h"

#include <sstream>

namespace raftcpp {

KvClerk::KvClerk(int64_t cid, 
				 const std::vector<PolishedRpcClient::SPtr>& peers) 
	: cid_(cid),
	  seq_(0),
      latestLeaderId_(0),
	  peers_(peers) {

}
	
bool KvClerk::get(const std::string& key, std::string& value) {
	std::shared_ptr<KvCommandReply> reply = sendCommand(operation::GET, key);
	if (reply->error() == operation::ERROR_OK) {
		value = reply->value();
		return true;
	}
	return false;
}
	
void KvClerk::put(const std::string& key, const std::string& value) {
	sendCommand(operation::PUT, key, value);
	printf("PUT: key %s, value %s\n", key.c_str(), value.c_str());
}
	
void KvClerk::append(const std::string& key, const std::string& value) {
	sendCommand(operation::APPEND, key, value);
	printf("APPEND: key %s, value %s\n", key.c_str(), value.c_str());
}
	
void KvClerk::del(const std::string& key) {
	sendCommand(operation::DELETE, key);
	printf("DELETE: key %s\n", key.c_str());
}

std::shared_ptr<KvCommandReply> KvClerk::sendCommand(const std::string& operation, 
											const std::string& key, 
											const std::string& value) {
	std::shared_ptr<KvCommand> cmd = std::make_shared<KvCommand>();
	cmd->set_operation(operation);
	cmd->set_cid(cid_);
	cmd->set_seq(seq_);
	cmd->set_key(key);
	if (operation == operation::PUT || operation == operation::APPEND) {
		cmd->set_value(value);
	}

	while (true) {
		std::shared_ptr<KvCommandReply> reply = nullptr;

		reyao::Mutex mutex;
		reyao::Condition cond(mutex);
		// Clerk 持有所有节点的地址并记录着最近一次访问的 leadId
		// 其首先先主节点请求，通过条件变量等待响应
		// 如果失败，则遍历所有节点，直到得到成功的响应
		peers_[latestLeaderId_]->Call<KvCommandReply>(cmd, 
											std::bind([&reply, &cond](std::shared_ptr<KvCommandReply> response) {
														reply = response;
														LOG_INFO << "notify";
														cond.notifyAll();
													 }, std::placeholders::_1));
		bool is_timeout = cond.waitForSeconds(1);
		if (is_timeout || !reply || !reply->leader()) { 
			++latestLeaderId_;
			latestLeaderId_ %= peers_.size();
			LOG_INFO << "try send to " << latestLeaderId_ << "error"
			         << "timout:" << is_timeout;
			continue;
		} else {
			seq_++;
			return reply;
		}
	}
}

} // namespace raftcpp

void usage(std::string operation) {
	if (operation == "get") {
		printf("Usage:get key\n");
	} else if (operation == "put") {
		printf("Usage:put key value\n");
	} else if (operation == "append") {
		printf("Usage:append key value\n");
	} else if (operation == "delete") {
		printf("Usage:delete key\n");
	} else {
		printf("unknown operation\n");
	}
}

int main(int argc, char* argv[]) {
	using namespace raftcpp;
	using namespace reyao;
	if (argc < 3) {
		printf("Usage: %s n base_port server_ips\n", argv[0]);
		return 0;
	}
	int n = std::atoi(argv[1]);
	int base_port = std::atoi(argv[2]);
	if (argc < 3 + n) {
		printf("Usage: %s n base_port server_ips\n", argv[0]);
		return 0;
	}
	// g_logger->setLevel(LogLevel::INFO);
	// g_logger->setFileAppender();
	Scheduler sche;
	sche.startAsync();
	// 初始化所有节点信息
	std::vector<raftcpp::PolishedRpcClient::SPtr> peers;
	for (int i = 0; i < n; ++i) {
		IPv4Address::SPtr peer_addr = IPv4Address::CreateAddress(argv[3 + i], base_port + i);
		peers.push_back(std::make_shared<raftcpp::PolishedRpcClient>(&sche, peer_addr));
	}

	// TODO: 不同的 clerk 进程应该有唯一的 cid
	KvClerk clerk(std::rand(), peers);
	std::string line, word;
	std::vector<std::string> words;
	while (getline(std::cin, line)) {
		words.clear();
		std::istringstream cmd(line);
		// 接收 cmd
		while (cmd >> word) {
			words.push_back(word);
		}
		if (words.size() == 0) {
			continue;
		}
		if (words[0] == "get") {
			if (words.size() < 2) {
				usage(words[0]);
				continue;
			}
			std::string value;
			bool exist = clerk.get(words[1], value);
			if (exist) {
				printf("value:%s\n", value.c_str());
			} else {
				printf("no such key\n");
			}
		} else if (words[0] == "put") {
			if (words.size() < 3) {
				usage(words[0]);
				continue;
			}
			clerk.put(words[1], words[2]);
		} else if (words[0] == "append") {
			if (words.size() < 3) {
				usage(words[0]);
				continue;
			}
			clerk.append(words[1], words[2]);
		} else if (words[0] == "delete") {
			if (words.size() < 2) {
				usage(words[0]);
				continue;
			}
			clerk.del(words[1]);
		} else if (words[0] == "quit") {
			printf("bye\n");
			break;
		} else {
			usage(words[0]);
			continue;
		}			
	}

	return 0;
}