#include "kv_server.h"
#include "common.h"

#include <unistd.h>

namespace raftcpp {

KvServer::KvServer(reyao::Scheduler* sche, uint32_t me, 
                   reyao::IPv4Address::SPtr addr,
                   const std::vector<PolishedRpcClient::SPtr>& peers)
    : sche_(sche),
      me_(me),
      raft_(peers, me_, addr, sche_) {
    reyao::rpc::RpcServer& server = raft_.getRpcServer();
    raft_.setApplyLogFunc(std::bind(&KvServer::applyFunc, this, std::placeholders::_1, std::placeholders::_2));
    server.registerRpcHandler<KvCommand>(std::bind(&KvServer::onCommand, this, std::placeholders::_1));
}

MessageSPtr KvServer::onCommand(std::shared_ptr<KvCommand> args) {
	LOG_INFO <<  me_ << " recv setCommand";
    std::shared_ptr<KvCommandReply> reply(new KvCommandReply);
    bool is_leader = raft_.isLeader();
	// 检查节点是否为主节点，只有主节点才能完成操作
    if (!is_leader) {
        reply->set_leader(false);
    } else {
		reply->set_leader(true);
		auto it = latestAppliedSeqPerClient_.find(args->cid());
		if (it != latestAppliedSeqPerClient_.end() && args->seq() <= it->second) {
			if (args->operation() == operation::GET) {
				auto key_it = db_.find(args->key());
				if (key_it == db_.end()) {
					reply->set_error(operation::ERROR_NO_KEY);
				} else {
					reply->set_value(key_it->second);
					reply->set_error(operation::ERROR_OK);
				}
			}
		} else {
			// 将 KvCommand 先序列化交给 Raft 完成日志复制
			std::string cmd;
			args->SerializeToString(&cmd);
			uint32_t term;
			uint32_t index;
			raft_.append(cmd, index, term);

			// 主节点必须等待日志同步
			reyao::CoroutineCondition cond;
			notify_[index] = cond;
			LOG_INFO << "leader " << me_ << " wait to sync";

			notify_[index].wait();
			LOG_INFO << "leader " << me_ << " await from sync";

			if (args->operation() == operation::GET) {
				auto key_it = db_.find(args->key());
				if (key_it == db_.end()) {
					reply->set_error(operation::ERROR_NO_KEY);
				} else {
					reply->set_value(key_it->second);
					reply->set_error(operation::ERROR_OK);
				}
			} else {
				reply->set_error(operation::ERROR_OK);
			}
		}
    }
	LOG_INFO << "reply error:" << reply->error();
    return reply;
}

void KvServer::applyFunc(uint32_t server, LogEntry log) {
	// Raft 完成日志同步后，提交到 db 中，此时先将 log 反序列化为 KvCommand，完成对应的 db 操作
    LOG_INFO << "server " << server << " commit log at " << log.index();
	KvCommand cmd;
	cmd.ParseFromString(log.command());
	latestAppliedSeqPerClient_[cmd.cid()] = cmd.seq();
	if (cmd.operation() == operation::GET) {
		LOG_INFO << "server " << me_ << ", GET: key " << cmd.key();
	} else if (cmd.operation() == operation::PUT) {
		db_[cmd.key()] = cmd.value();
		LOG_INFO << "server " << me_ << ", PUT: key " << cmd.key() << ", value " << cmd.value();
	} else if (cmd.operation() == operation::APPEND) {
		db_[cmd.key()] += cmd.value();
		LOG_INFO << "server " << me_ << ", APPEND: key " << cmd.key() << ", value " << cmd.value();
	} else if (cmd.operation() == operation::DELETE) {
		db_.erase(cmd.key());
		LOG_INFO << "server " << me_ << ", DELETE: key " << cmd.key();
	} else {
		LOG_ERROR << "invalid command operation";
	}

	auto it = notify_.find(log.index());
	if (it != notify_.end()) {
		it->second.notify();
		notify_.erase(it);
	}
}


} // namespace raftcpp

int main(int args, char* argv[]) {
    using namespace raftcpp;
    using namespace reyao;
    
	if (args < 3) {
		printf("Usage: %s n me base_port peer_ips\n", argv[0]);
		return 0;
	}

	int n = std::atoi(argv[1]);
	uint32_t me = std::atoi(argv[2]);
	int base_port = std::atoi(argv[3]);

	// g_logger->setLevel(LogLevel::INFO);

	if (args < 4 + n) {
		printf("Usage: %s n me base_port peer_ips\n", argv[0]);
		return 0;
	}

	Scheduler sche;
    sche.startAsync();

	std::vector<PolishedRpcClient::SPtr> peers;
	for (int i = 0; i < n; ++i) {
		IPv4Address::SPtr peer_addr = IPv4Address::CreateAddress(argv[4 + i], base_port + i);
		peers.push_back(std::make_shared<PolishedRpcClient>(&sche, peer_addr));
	}

	IPv4Address::SPtr serv_addr = IPv4Address::CreateAddress("0.0.0.0", base_port + me);
	KvServer kvserver(&sche, me, serv_addr, peers);
	sleep(1);

	kvserver.start();
	printf("server %d start\n", me);
	
    sche.wait();
	return 0;
}