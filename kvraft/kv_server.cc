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
    std::shared_ptr<KvCommandReply> reply(new KvCommandReply);
    bool is_leader = raft_.isLeader();
    if (!is_leader) {
        reply->set_leader(false);
    } else {
		reply->set_leader(true);
		auto it = latest_applied_seq_per_client_.find(args->cid());
		if (it != latest_applied_seq_per_client_.end() && args->seq() <= it->second) {
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
			std::string cmd;
			args->SerializeToString(&cmd);
			uint32_t term;
			uint32_t index;
			raft_.append(cmd, index, term);

			// 等待，直到日志达成一致
			reyao::CoroutineCondition cond;
			notify_[index] = cond;
			// TODO:处理超时的情况
			notify_[index].wait();

			if (args->operation() == operation::GET) {
				auto key_it = db_.find(args->key());
				if (key_it == db_.end()) {
					reply->set_error(operation::ERROR_NO_KEY);
				} else {
					reply->set_value(key_it->second);
					reply->set_error(operation::ERROR_OK);
				}
			}
		}
    }

    return reply;
}

void KvServer::applyFunc(uint32_t server, LogEntry log) {
    LOG_INFO << "server " << server << " commit log at " << log.index();
	KvCommand cmd;
	cmd.ParseFromString(log.command());
	latest_applied_seq_per_client_[cmd.cid()] = cmd.seq();
	if (cmd.operation() == operation::GET) {
		LOG_INFO << "server " << me_ << ", GET: key " << cmd.key();
		//do nothing
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

	g_logger->setLevel(LogLevel::INFO);


	if (args < 4 + n) {
		printf("Usage: %s n me base_port peer_ips\n", argv[0]);
		return 0;
	}

	Scheduler scheduler;
    scheduler.startAsync();

	std::vector<PolishedRpcClient::SPtr> peers;
	for (int i = 0; i < n; ++i) {
		IPv4Address::SPtr peer_addr = IPv4Address::CreateAddress(argv[4 + i], base_port + i);
		peers.push_back(std::make_shared<PolishedRpcClient>(peer_addr, &scheduler));
	}

	IPv4Address::SPtr serv_addr = IPv4Address::CreateAddress("0.0.0.0", base_port + me);
	KvServer kvserver(&scheduler, me, serv_addr, peers);
	sleep(1);

	kvserver.start();
	printf("server %d start\n", me);
	
    scheduler.wait();
	return 0;
}