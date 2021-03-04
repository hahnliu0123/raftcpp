#include "config.h"
#include "../src/polished_rpc_client.h"

#include <gtest/gtest.h>
#include <reyao/util.h>

namespace raftcpp {

Config::Config(reyao::Scheduler* sche, uint32_t num, 
               const std::string& ip_addr, 
               int base_port) 
    : sche_(sche),
      num_(num), 
      ip_addr_(ip_addr),
      base_port_(base_port),
      connect_state_(num, true) {
    
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
    connect_state_[server] = v;
    std::vector<PolishedRpcClient::SPtr>& peers = rafts_[server]->getPeers();
    // server1 and server2 can only commicate when their connect state is true  

    // set connect server -> all peers
    for (size_t i = 0; i < peers.size(); i++) {
        auto peer = peers[i];
        peer->setConnect(v & connect_state_[i]);
    }

    // set connect all peers -> server
    for (uint32_t i = 0; i < num_; i++) {
        auto& peers = rafts_[i]->getPeers();
        peers[server]->setConnect(v && connect_state_[i]);
    }

    LOG_INFO << "server:" << server << (v ? " connected " : " disconnect");
}

int Config::checkOneLeader() {
    for (int i = 0; i < 10; i++) {
        int duration = 450 + rand() % 100;
        usleep(duration * 1000);

        std::map<uint32_t, std::vector<uint32_t>> term_2_leader_id;
        for (uint32_t server = 0; server < num_; server++) {
            if (connect_state_[server]) {
                bool is_leader = rafts_[server]->isLeader();
                uint32_t term = rafts_[server]->getTerm();
                if (is_leader) {
                    term_2_leader_id[term].push_back(server);
                }
            }
        }
        uint32_t last_term = 0;
        for (const auto& pair : term_2_leader_id) {
            EXPECT_EQ(1, static_cast<int>(pair.second.size()));
            if (pair.first > last_term) {
                last_term = pair.first;
            }
        }
        if (term_2_leader_id.size() != 0) {
            return static_cast<int>(term_2_leader_id[last_term][0]);
        }
    }
    EXPECT_TRUE(false);
    return -1;
}

void Config::checkNoLeader() {
    for (uint32_t server = 0; server < num_; server++) {
        if (connect_state_[server]) {
            bool is_leader = rafts_[server]->isLeader();
            EXPECT_TRUE(!is_leader);
        }
    }
}

uint32_t Config::checkTerm() {
    uint32_t term = 0;
    for (uint32_t server = 0; server < num_; server++) {
        if (connect_state_[server]) {
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

// 检查每个raft对象index处的log是否处于一致，返回处于一致的raft对象个数
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

// 向leader发送一个命令，等待日志同步，检查是否所有raft对象达成一致
int Config::one(const std::string& cmd, int32_t expected_server, bool retry) {
    int64_t start = reyao::GetCurrentTime();
    int64_t now;
    uint32_t server = 0;    
    do {
        uint32_t index = 0;
        // find leader and append a log, then return log index
        for (uint32_t i = 0; i < num_; i++) {
            server = (server + 1) % num_;
            if (connect_state_[server]) {
                uint32_t index1, term;
                bool is_leader = rafts_[server]->append(cmd, index1, term);
                if (is_leader) {
                    index = index1;
                    break;
                }
            }
        }

        // if append a log to leader successfully, then wait a while 
        // to see if raft complete commit log
        int64_t start1 = reyao::GetCurrentTime();
        do {
            int nc = nCommitted(index);
            if (nc > 0 && nc >= expected_server) {
                LOG_INFO << "pass one(), index:" << index;
                return index;
            }
            usleep(20 * 1000);
            now = reyao::GetCurrentTime();
        } while (now < start1 + 2 * 1000 * 1000); // 每20ms重试一次，总时长为2s

        if (!retry) {
            LOG_ERROR << "cmd[" << cmd << "] failed to reach agreement";
        } else {
            usleep(50 * 1000); // 如果leader添加log失败，等待50ms后重新append log
        }

        now = reyao::GetCurrentTime();
    } while (now  < start + 10 * 1000 * 1000); // total expired time 10s.
    LOG_ERROR << "failed to reach agreement";
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
        LOG_DEBUG << server << " apply Log index:" << index << " cmd:" << entry.command();
    }
}

std::shared_ptr<Raft> Config::makeRaft(uint32_t server, uint32_t num, 
                                       reyao::Scheduler* sche) {
    std::vector<PolishedRpcClient::SPtr> peers;
    for (uint32_t i = 0; i < num; i++) {
        // gene peers
        auto serv_addr = reyao::IPv4Address::CreateAddress(ip_addr_.c_str(), base_port_ + i);
        peers.push_back(std::make_shared<PolishedRpcClient>(sche, serv_addr));
    }

    // init raft
    auto addr = reyao::IPv4Address::CreateAddress(ip_addr_.c_str(), base_port_ + server);
    std::shared_ptr<Raft> raft(new Raft(peers, server, addr, sche));
    raft->setApplyLogFunc(std::bind(&Config::applyFunc, this, std::placeholders::_1,
                                    std::placeholders::_2));
    return raft;
}

} //namespace raftcpp
