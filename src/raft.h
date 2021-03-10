#pragma once

#include "args.pb.h"
#include "polished_rpc_client.h"

#include <google/protobuf/message.h>
#include "reyao/mutex.h"
#include "reyao/scheduler.h"
#include "reyao/rpc/rpc_server.h"
#include "reyao/address.h"

#include <vector>
#include <memory>

namespace raftcpp {

typedef std::shared_ptr<::google::protobuf::Message> MessageSPtr;

class Raft {
public:
    enum Role {
        Follower = 0,
        PreCandidate,
        Candidate,
        Leader,
        Dead,
    };

    typedef std::function<void(uint32_t, LogEntry)> ApplyLogFunc;

    Raft(std::vector<PolishedRpcClient::SPtr> peers, uint32_t me, 
         reyao::IPv4Address::SPtr addr, reyao::Scheduler* sche);
    ~Raft();

    bool append(const std::string& cmd, uint32_t& index, uint32_t& term);
    void stop();
    bool isKilled();
    bool isLeader() { 
        return role_ == Leader; 
    }
    uint32_t getTerm() { 
        return current_term_; 
    }
    void setApplyLogFunc(ApplyLogFunc func) { apply_func_ = func; }
    void defaultApplyFunc(uint32_t server, LogEntry);
    std::string toString()
    {
        std::ostringstream os;
        os << "server(id=" << me_ << ", term=" << current_term_ << ", log=[";
        for (const LogEntry &entry : logs_)
        {
            os << "<index=" << entry.index() << ", term=" << entry.term() << ">";
        }
        os << "], peers(";
        for (const PolishedRpcClient::SPtr &peer : peers_)
        {
            os << (peer->isConnected() ? "1" : "0") << ",";
        }
        os << "))";
        return os.str();
    }
    std::vector<PolishedRpcClient::SPtr>& getPeers() { return peers_; }
    
    // server election, run in a coroutine to see if server should kick off election
    void onElection();
    // state monitor, run in a coroutine to monitor server's state
    // void stateMonitor();
    // kick off an election when server doesn't receive heartbeat from leader in a duration
    void kickoffElection();
    void PreElection();
    
    // state change
    void convertToLeader();
    void convertToPreCandidate();
    void convertToCandidate();
    void convertToFollower(uint32_t term);

    // for server, send heartbeat to all followers
    void sendHeartBeat();

    void addLogEntries(std::shared_ptr<AppendEntriesArgs> args, uint32_t next_index);

    uint32_t getLastLogIndex() const;
    uint32_t getLastLogTerm() const;
    uint32_t getPrevLogIndex(uint32_t peer) const;
    uint32_t getPrevLogTerm(uint32_t peer) const;
    reyao::rpc::RpcServer& getRpcServer() { return server_; }

    void advanceCommitIndex();
    void handleReply(std::shared_ptr<AppendEntriesReply> reply, bool success, uint32_t conflict_index, uint32_t conflict_term);

    // commit log to app
    void applyLog();

    bool isLogMoreUpToDate(uint32_t index, uint32_t term);

    // for rpc call
    bool sendRequestVote(uint32_t peer, std::shared_ptr<RequestVoteArgs> args);
    bool sendAppendEntries(uint32_t peer, std::shared_ptr<AppendEntriesArgs> args);
    
    // handle rpc response
    void onRequestVoteReply(std::shared_ptr<RequestVoteArgs> args, std::shared_ptr<RequestVoteReply> reply);
    void onAppendEntriesReply(uint32_t peer, std::shared_ptr<AppendEntriesArgs> args, std::shared_ptr<AppendEntriesReply> reply);

    // rpc register
    MessageSPtr RequestVote(std::shared_ptr<RequestVoteArgs> args);
    MessageSPtr AppendEntries(std::shared_ptr<AppendEntriesArgs> args);

private:
    std::vector<PolishedRpcClient::SPtr> peers_;
    uint32_t me_;

    // persist state
    uint32_t current_term_;
    int32_t vote_for_;
    std::vector<LogEntry> logs_;


    uint32_t num_of_vote_;
    uint32_t commit_index_;
    uint32_t last_applied_;

    std::vector<uint32_t> next_index_;
    std::vector<uint32_t> match_index_;

    int64_t last_broadcast_time_;

    ApplyLogFunc apply_func_;

    Role role_;
    int64_t last_active_time_;

    reyao::Scheduler* sche_;
    reyao::rpc::RpcServer server_;


};

} //namespace raftcpp

