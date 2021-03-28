#pragma once

#include "src/raft.h"
#include "src/args.pb.h"

#include "reyao/scheduler.h"
#include <unordered_map>

namespace raftcpp {

// 一个 KvServer 对应一个 Raft 节点
class KvServer : public reyao::NoCopyable {
public:
    KvServer(reyao::Scheduler* sche, uint32_t me, 
             reyao::IPv4Address::SPtr addr,
             const std::vector<PolishedRpcClient::SPtr>& peers);

    void start() {}

private:
    MessageSPtr onCommand(std::shared_ptr<KvCommand> args);
    void applyFunc(uint32_t server, LogEntry log);
private:
    reyao::Scheduler* sche_;
    uint32_t me_;
    Raft raft_;

    std::unordered_map<std::string, std::string> db_;
    std::unordered_map<int64_t, uint32_t> latestAppliedSeqPerClient_; // cid -> lastSeq
	std::unordered_map<uint32_t, reyao::CoroutineCondition> notify_;

};

} // namespace raftcpp