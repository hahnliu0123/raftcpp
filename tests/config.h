#pragma once

#include "../src/raft.h"

#include <vector>
#include <map>

namespace raftcpp {

class Raft;

class Config {
public:
    Config(reyao::Scheduler* sche, uint32_t num, 
           const std::string& ip_addr = "0.0.0.0", 
           int base_port = 9000);

    // void start();

    void setConnect(uint32_t server, bool v);
    std::shared_ptr<Raft> getRaft(uint32_t server) { return rafts_[server]; }
    int checkOneLeader();
    void checkNoLeader();
    uint32_t checkTerm();
    int nCommitted(uint32_t index);
    int one(const std::string& cmd, int32_t expected_server, bool retry);

    void applyFunc(uint32_t server, LogEntry entry);

    std::vector<LogEntry>& getLog(int server) { return logs_[server]; }
    void printLogs();

private:
    std::shared_ptr<Raft> makeRaft(uint32_t server, uint32_t num, reyao::Scheduler* sche);

private:
    reyao::Scheduler* sche_ = nullptr;
    uint32_t num_;
    const std::string ip_addr_;
    int base_port_;

    std::vector<std::shared_ptr<Raft>> rafts_;
    std::vector<bool> connect_state_;
    std::map<int, std::vector<LogEntry>> logs_;
    reyao::Mutex mutex_;
};


} //namespace raftcpp