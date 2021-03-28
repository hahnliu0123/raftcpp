#include "config.h"
#include "../src/polished_rpc_client.h"

#include <gtest/gtest.h>
#include "reyao/util.h"

namespace raftcpp {

Config::Config(reyao::Scheduler* sche, uint32_t num, 
               const std::string& ipAddr, 
               int basePort) 
    : sche_(sche),
      num_(num), 
      ipAddr_(ipAddr),
      basePort_(basePort),
      connectState_(num, true) {
    
    for (uint32_t i = 0; i < num; i++) {
        rafts_.push_back(makeRaft(i, num, sche));
    }
    
    LogEntry dummy;
    dummy.set_term(0);
    dummy.set_index(0);
    for (uint32_t i = 0; i < num; i++) {
        logs_[i].push_back(dummy);
    }
}

void Config::setConnect(uint32_t server, bool v) {
    connectState_[server] = v;
    std::vector<PolishedRpcClient::SPtr>& peers = rafts_[server]->getPeers();
    // server1 and server2 can only commicate when their connect state is true  

    // set connect server -> all peers
    for (size_t i = 0; i < peers.size(); i++) {
        auto peer = peers[i];
        peer->setConnect(v & connectState_[i]);
    }

    // set connect all peers -> server
    for (uint32_t i = 0; i < num_; i++) {
        auto& peers = rafts_[i]->getPeers();
        peers[server]->setConnect(v && connectState_[i]);
    }

    LOG_INFO << "server:" << server << (v ? " connected " : " disconnect");
}

int Config::checkOneLeader() {
    for (int i = 0; i < 10; i++) {
        int duration = 450 + rand() % 100;
        usleep(duration * 1000);

        std::map<uint32_t, std::vector<uint32_t>> term2LeaderId;
        for (uint32_t server = 0; server < num_; server++) {
            if (connectState_[server]) {
                bool leader = rafts_[server]->isLeader();
                uint32_t term = rafts_[server]->getTerm();
                if (leader) {
                    term2LeaderId[term].push_back(server);
                }
            }
        }
        uint32_t lastTerm = 0;
        for (const auto& pair : term2LeaderId) {
            EXPECT_EQ(1, static_cast<int>(pair.second.size()));
            if (pair.first > lastTerm) {
                lastTerm = pair.first;
            }
        }
        if (term2LeaderId.size() != 0) {
            return static_cast<int>(term2LeaderId[lastTerm][0]);
        }
    }
    EXPECT_TRUE(false);
    return -1;
}

void Config::checkNoLeader() {
    for (uint32_t server = 0; server < num_; server++) {
        if (connectState_[server]) {
            bool leader = rafts_[server]->isLeader();
            EXPECT_TRUE(!leader);
        }
    }
}

uint32_t Config::checkTerm() {
    uint32_t term = 0;
    for (uint32_t server = 0; server < num_; server++) {
        if (connectState_[server]) {
            uint32_t t = rafts_[server]->getTerm();
            if (term == 0) {
                term = t;
            } else {
                EXPECT_EQ(term, t);
            }
        }
    }

    return term;
}

// 检查每个 raft 对象 index 处的 log 是否处于一致，返回处于一致的 raft 对象个数
int Config::nCommitted(uint32_t index) {
    int count = 0;
    std::string cmd;
    {
        reyao::MutexGuard lock(mutex_);
        for (uint32_t server = 0; server < num_; server++) {
            if (logs_[server].size() > index) {
                const std::string cmd1 = logs_[server][index].command();
                if (count > 0) {
                    EXPECT_EQ(cmd, cmd1);
                }
                count++;
                cmd = cmd1;
            }
        }
    }
    return count;
}

// 向 leader 发送一个命令，等待日志同步，检查是否所有 raft 对象达成一致
// 返回 index 一致的节点数量
int Config::one(const std::string& cmd, int32_t expected_server, bool retry) {
    int64_t start = reyao::GetCurrentMs();
    int64_t now;
    uint32_t server = 0;    
    do {
        uint32_t index = 0;
        // find leader and append a log, then return log index
        // LOG_WARN << "one";
        for (uint32_t i = 0; i < num_; i++) {
            server = (server + 1) % num_;
            if (connectState_[server]) {
                uint32_t index1, term;
                bool leader = rafts_[server]->append(cmd, index1, term);
                if (leader) {
                   // LOG_WARN << "leader append index1" << index1;
                    index = index1;
                    break;
                }
            }
        }

        // if append a log to leader successfully, then wait a while 
        // to see if raft complete commit log
        int64_t start1 = reyao::GetCurrentMs();
        do {
            int nc = nCommitted(index);
            if (nc > 0 && nc >= expected_server) {
                return index;
            }
            usleep(20 * 1000);
            now = reyao::GetCurrentMs();
        } while (now < start1 + 2 * 1000); // 每 20 ms 重试一次，总时长为 2 s

        if (!retry) {
            LOG_FMT_ERROR("cmd[%s] failed to reach agreement", cmd.c_str());
        } else {
            usleep(50 * 1000); // 如果 leader 添加 log 失败，等待 50 ms 后重新 append log
        }

        now = reyao::GetCurrentMs();
    } while (now  < start + 3 * 1000); // total expired time 3 s.
    LOG_FMT_ERROR("failed to reach agreement");
    return -1;
}

void Config::applyFunc(uint32_t server, LogEntry entry) {
    uint32_t index = entry.index();
    {
        reyao::MutexGuard lock(mutex_);
        for (uint32_t serv = 0; serv < num_; serv++) {
            if (logs_[serv].size() > index) {
                const LogEntry& old = logs_[serv][index];
                EXPECT_EQ(old.command(), entry.command());
            }
        }
        logs_[server].push_back(entry);
    }
}

std::shared_ptr<Raft> Config::makeRaft(uint32_t server, uint32_t num, 
                                       reyao::Scheduler* sche) {
    std::vector<PolishedRpcClient::SPtr> peers;
    for (uint32_t i = 0; i < num; i++) {
        // gene peers
        auto serv_addr = reyao::IPv4Address::CreateAddress(ipAddr_.c_str(), basePort_ + i);
        peers.push_back(std::make_shared<PolishedRpcClient>(sche, serv_addr));
    }

    // init raft
    auto addr = reyao::IPv4Address::CreateAddress(ipAddr_.c_str(), basePort_ + server);
    std::shared_ptr<Raft> raft(new Raft(peers, server, addr, sche));
    raft->setApplyLogFunc(std::bind(&Config::applyFunc, this, std::placeholders::_1,
                                    std::placeholders::_2));
    return raft;
}

void Config::printLogs() {
    for (uint32_t i = 0; i < num_; i++) {
        const auto& logs = getLog(i);
        std::stringstream ss;
        ss << "server " << i << " ";
        for (auto& log : logs) {
            ss << log.command() << " ";
        }
        LOG_INFO << ss.str();
    }
}

} // namespace raftcpp
