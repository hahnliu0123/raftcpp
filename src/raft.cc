#include "raft.h"

#include <reyao/util.h>

#include <stdlib.h>

namespace raftcpp {


Raft::Raft(std::vector<PolishedRpcClient::SPtr> peers, uint32_t me, 
           reyao::IPv4Address::SPtr addr, reyao::Scheduler* sche)
    : peers_(peers),
      me_(me),
      currentTerm_(1),
      voteFor_(-1),
      numOfVote_(0),
      commitIndex_(0),
      lastApplied_(0),
      nextIndex_(),
      matchIndex_(),
      lastBroadcastTime_(0),
      applyFunc_(std::bind(&Raft::defaultApplyFunc, this, std::placeholders::_1, std::placeholders::_2)),
      role_(Role::Follower),
      lastActiveTime_(reyao::GetCurrentMs()),
      sche_(sche),
      server_(sche, addr) {
    
    // register Rpc
    server_.registerRpcHandler<RequestVoteArgs>(std::bind(&Raft::RequestVote, this, std::placeholders::_1));
    server_.registerRpcHandler<AppendEntriesArgs>(std::bind(&Raft::AppendEntries, this, std::placeholders::_1));
    server_.start();

    // log start with index 1, so push a dummy node in the begin
    LogEntry dummy;
    dummy.set_term(0);
    dummy.set_index(0);
    logs_.push_back(dummy);

    nextIndex_.insert(nextIndex_.begin(), peers_.size(), logs_.size());
    matchIndex_.insert(matchIndex_.begin(), peers_.size(), 0);

    sche_->addTask(std::bind(&Raft::onElection, this));
}

Raft::~Raft() {
    stop();
    LOG_DEBUG << "~Raft()";
}

bool Raft::append(const std::string& cmd, uint32_t& index, uint32_t& term) {
    bool leader = isLeader();
    term = currentTerm_;
    if (leader) {
        index = getLastLogIndex() + 1;
        LogEntry entry;
        entry.set_term(term);
        entry.set_index(index);
        entry.set_command(cmd);
        logs_.push_back(entry);
    }
    return leader;
}

void Raft::stop() {
    role_ = Role::Dead;
}

bool Raft::isKilled() {
    return role_ == Dead;
}

// 从节点没收到心跳包，开启定时选举
void Raft::onElection() {
    while (true) {
        int64_t startTime = reyao::GetCurrentMs();
        int64_t expierdMs = 200 + (rand() % 100);
        usleep(expierdMs * 1000); // 200 ~ 300 ms
        // FIXME: 退出时 Raft 先析构会导致访问已析构内存，未定义行为
        if (role_ == Role::Dead) {
            LOG_DEBUG <<  me_ << " quit onElection";
            return;
        }
        if (lastActiveTime_ < startTime) {
            if (role_ != Role::Leader) {
                LOG_INFO << "expire";
                sche_->addTask(std::bind(&Raft::kickoffElection, this));
            }
        } else {
            // LOG_INFO << "already recv heartbeat";
        }
    }
}

void Raft::kickoffElection() {
    lastActiveTime_ = reyao::GetCurrentMs();
    convertToPreCandidate();
    std::shared_ptr<RequestVoteArgs> args(new RequestVoteArgs);
    args->set_term(currentTerm_ + 1);
    args->set_candidate_id(me_);
    args->set_last_log_index(getLastLogIndex());
    args->set_last_log_term(getLastLogTerm());
    for (size_t i = 0; i < peers_.size(); i++) {
        if (i == me_) {
            continue;
        }
        sche_->addTask([=]() {
            if (!sendRequestVote(i, args)) {
                LOG_DEBUG << "sendRequestVote " << me_ << " -> " << i << " failed";
            }
        });
    }
}

void Raft::PreElection() {
    lastActiveTime_ = reyao::GetCurrentMs();
    convertToPreCandidate();
    std::shared_ptr<RequestVoteArgs> args(new RequestVoteArgs);
    args->set_term(currentTerm_ + 1);
    args->set_candidate_id(me_);
    args->set_last_log_index(getLastLogIndex());
    args->set_last_log_term(getLastLogTerm());
    for (size_t i = 0; i < peers_.size(); i++) {
        if (i == me_) {
            continue;
        }
        sche_->addTask([=]() {
            if (!sendRequestVote(i, args)) {
                LOG_DEBUG << "sendRequestVote " << me_ << " -> " << i << " failed";
            }
        });
    }
}

void Raft::convertToLeader() {
    if (role_ != Candidate) {
        return;
    }
    LOG_INFO << me_ << " convert to leader. term: " << currentTerm_;
    role_ = Leader;
    nextIndex_ = std::vector<uint32_t>(peers_.size()); 
    matchIndex_ = std::vector<uint32_t>(peers_.size());
    for (size_t i = 0; i < peers_.size(); i++) {
        nextIndex_[i] = getLastLogIndex() + 1;
    }
}

void Raft::convertToPreCandidate() {
    LOG_INFO << me_ << " convert to pre_candidate. term: " << currentTerm_;
    role_ = PreCandidate;
    voteFor_ = me_;
    numOfVote_ = 1;
}

void Raft::convertToCandidate() {
    LOG_INFO << me_ << " convert to candidate. term: " << currentTerm_;
    role_ = Candidate;
    ++currentTerm_;
    voteFor_ = me_;
    numOfVote_ = 1;
}


void Raft::convertToFollower(uint32_t term) {
    LOG_DEBUG << me_ << " convert to follower. term: " << currentTerm_;
    role_ = Follower;
    currentTerm_ = term;
    voteFor_ = -1;
    numOfVote_ = 0;
}

void Raft::sendHeartBeat() {
    while (!isKilled()) {
        usleep(100 * 1000); // send heartbeat per 100 ms
        // FIXME: 退出时 Raft 先析构会导致访问已析构内存，未定义行为
        if (role_ != Leader || role_ == Dead) {
            LOG_INFO << me_ << " quit sendHeartBeat";
            return;
        }

        int64_t now = reyao::GetCurrentMs();
        if (now < lastBroadcastTime_ + 100) {
            return;
        }

        lastBroadcastTime_ = now;
        // LOG_INFO << "now send heart beat";
        int term = currentTerm_; // NOTE: 先保存 term，防止心跳包发到一半发现新主节点从而修改到最新 term，这样就会有两个相同 term 的主节点发送心跳包
        for (size_t i = 0; i < peers_.size(); i++) {
            if (i == me_) {
                continue;
            }
            uint32_t nextIndex = nextIndex_[i];
            std::shared_ptr<AppendEntriesArgs> args(new AppendEntriesArgs);
            args->set_term(term);
            args->set_leader_id(me_);
            args->set_prev_log_index(getPrevLogIndex(i));
            args->set_prev_log_term(getPrevLogTerm(i));
            args->set_leader_commit(commitIndex_);
            addLogEntries(args, nextIndex);

            sendAppendEntries(i, args);
        }
    }
}

void Raft::addLogEntries(std::shared_ptr<AppendEntriesArgs> args, uint32_t nextIndex) {
    for (size_t i = nextIndex; i < logs_.size(); i++) {
        const LogEntry& srcEntry = logs_[i];
        LogEntry* entry = args->add_entries();
        entry->set_term(srcEntry.term());
        entry->set_index(srcEntry.index());
        entry->set_command(srcEntry.command());
    }
}

uint32_t Raft::getLastLogIndex() const {
    return logs_.size() - 1;
}

uint32_t Raft::getLastLogTerm() const {
    uint32_t lastLogIndex = getLastLogIndex();
    if (lastLogIndex == 0) {
        return 0;
    } else {
        return logs_[lastLogIndex].term();
    }
}

uint32_t Raft::getPrevLogIndex(uint32_t peer) const {
    return nextIndex_[peer] - 1;
} 

uint32_t Raft::getPrevLogTerm(uint32_t peer) const {
    uint32_t prevLogIndex = getPrevLogIndex(peer);
    if (prevLogIndex == 0) {
        return 0;
    } else {
        return logs_[prevLogIndex].term();
    }
}

void Raft::advanceCommitIndex() {
    // 主节点更新commit_index
    // 需要找到过半从节点都收到的日志
    std::vector<uint32_t> matchIndex(matchIndex_);
    matchIndex[me_] = logs_.size() - 1;
    std::sort(matchIndex.begin(), matchIndex.end());
    uint32_t N = matchIndex[matchIndex.size() / 2];
    if (role_ == Leader && N > commitIndex_ &&
        logs_[N].term() == currentTerm_) {
        // TODO: see paper 5.4
        commitIndex_ = N;
        applyLog();
    }
}

void Raft::handleReply(std::shared_ptr<AppendEntriesReply> reply, 
                       bool success, uint32_t conflictIndex, 
                       uint32_t conflictTerm) {
    applyLog();
    reply->set_success(success);
    reply->set_term(currentTerm_);
    reply->set_conflict_index(conflictIndex);
    reply->set_conflict_term(conflictTerm);
}

void Raft::applyLog() {
    while (lastApplied_ < commitIndex_) {
        ++lastApplied_;
        assert(lastApplied_ < logs_.size());
        applyFunc_(me_, logs_[lastApplied_]);
    }
}

bool Raft::isLogMoreUpToDate(uint32_t index, uint32_t term) {
    return term > getLastLogTerm() ||
           (term == getLastLogTerm() && index >= getLastLogIndex());
}

bool Raft::sendRequestVote(uint32_t peer, std::shared_ptr<RequestVoteArgs> args) {
    assert(peer < peers_.size());
    // LOG_DEBUG << me_ << " -> " << peer << " snedRequestVote";
    std::function<void(std::shared_ptr<RequestVoteReply>)> func = 
        std::bind(&Raft::onRequestVoteReply, this, args, std::placeholders::_1);
    return peers_[peer]->Call<RequestVoteReply>(args, func);
}

bool Raft::sendAppendEntries(uint32_t peer, std::shared_ptr<AppendEntriesArgs> args) {
    assert(peer < peers_.size());
    // LOG_DEBUG << me_ << " -> " << peer << " snedRequestVote";
    std::function<void(std::shared_ptr<AppendEntriesReply>)> func = 
        std::bind(&Raft::onAppendEntriesReply, this, peer, args, std::placeholders::_1);
    return peers_[peer]->Call<AppendEntriesReply>(args, func);
}

void Raft::onRequestVoteReply(std::shared_ptr<RequestVoteArgs> args, 
                              std::shared_ptr<RequestVoteReply> reply) {
    if (role_ == PreCandidate) {
        if (!reply->vote_granted()) {
            if (reply->term() > currentTerm_) {
                convertToFollower(reply->term());
            }
        }
        ++numOfVote_;
        if (numOfVote_ > peers_.size() / 2 && role_ == PreCandidate) {
            convertToCandidate();
            for (uint32_t i = 0; i < peers_.size(); i++) {
                if (i == me_)   continue;
                sche_->addTask([=]() {
                    if (!sendRequestVote(i, args)) {
                        LOG_DEBUG << "sendRequestVote " << me_ << " -> " << i << " failed";
                    }
                });
            }
        }
    } else if (role_ == Candidate) {
        if (!reply->vote_granted()) {
            if (reply->term() > currentTerm_) {
                convertToFollower(reply->term());
            }
        }
        ++numOfVote_;
        if (numOfVote_ > peers_.size() / 2 && role_ == Candidate) {
            convertToLeader();
            sche_->addTask(std::bind(&Raft::sendHeartBeat, this));
        }
    }
}
    
void Raft::onAppendEntriesReply(uint32_t peer,
                                std::shared_ptr<AppendEntriesArgs> args, 
                                std::shared_ptr<AppendEntriesReply> reply) {
    if (args->term() != currentTerm_) {
        // 该主节点已经变成从节点
        return;
    }
    if (reply->term() > currentTerm_) {
        // 发现更新任期
        // LOG_FMT_INFO("server:%d:%d find a reply fome term:%d", me_, current_term_, reply->term());
        convertToFollower(reply->term());
        return;
    }
    if (reply->success() == true) {
        // LOG_DEBUG << me_ << " -> " << peer 
        //           << " sendHeartBeat success. leader_commit:" << args->leader_commit()
        //           << " match_index:" << args->prev_log_index() << " -> "
        //           << args->prev_log_index() + args->entries().size();

        matchIndex_[peer] = args->prev_log_index() + args->entries_size();
        nextIndex_[peer] = matchIndex_[peer] + 1;
        advanceCommitIndex();
    } else {
        if (reply->conflict_term() != 0) {
            uint32_t lastIndexOfConflictTerm = 0;
            for (size_t i = args->prev_log_index(); i > 0; i--) {
                if (logs_[i].term() == reply->conflict_term()) {
                    lastIndexOfConflictTerm = i;
                    break;
                }
            }
            if (lastIndexOfConflictTerm == 0) {
                nextIndex_[peer] = reply->conflict_index();
            } else {
                nextIndex_[peer] = lastIndexOfConflictTerm + 1;
            }
        } else {
            nextIndex_[peer] = reply->conflict_index();
        }
    }
}

MessageSPtr Raft::RequestVote(std::shared_ptr<RequestVoteArgs> args) {

    lastActiveTime_ = reyao::GetCurrentMs();
    LOG_DEBUG << args->candidate_id() << " -> " << me_ << " RequestVote."
              << " candidate[term:" << args->term() << " last_log_index:"
              << args->last_log_index() << " last_log_term:" << args->last_log_term()
              << " ] local[term:" << currentTerm_ << " last_log_index:"
              << getLastLogIndex() << " last_log_term:" << getLastLogTerm() << "]";
    
    std::shared_ptr<RequestVoteReply> reply(new RequestVoteReply);

    if (args->term() < currentTerm_) {
        // 有更新的任期
        reply->set_term(currentTerm_);
        reply->set_vote_granted(false);
        return reply;
    }

    if (args->term() > currentTerm_) {
        // 候选人有资格成为leader，local降级为follower
        convertToFollower(args->term());
    }

    // vote_for_ == args->candidate_id() 表明真正的选举
    reply->set_term(currentTerm_);
    if ((voteFor_ == -1 || voteFor_ == args->candidate_id()) &&
        isLogMoreUpToDate(args->last_log_index(), args->last_log_term())) {
        voteFor_ = args->candidate_id();
        reply->set_vote_granted(true);
    } else {
        reply->set_vote_granted(false);
    }

    return reply;
}

MessageSPtr Raft::AppendEntries(std::shared_ptr<AppendEntriesArgs> args) {
    // TODO: persist() when leaving

    // LOG_DEBUG << "AppendEntries peer:" << args->leader_id() << "[term:" << args->term()
    //           << " prev_log_index:" << args->prev_log_index() << " prev_log_term:"
    //           << args->prev_log_term() << " lead_commit:" << args->leader_commit() 
    //           << "] " << "local:" << me_ << "[term:" << currentTerm_
    //           << " prev_log_index:" << getLastLogIndex() << " prev_log_term:"
    //           << getLastLogTerm() << " commit_index:" << commitIndex_ << "]";
    LOG_DEBUG << "ticket";
    
    lastActiveTime_ = reyao::GetCurrentMs();

    std::shared_ptr<AppendEntriesReply> reply(new AppendEntriesReply);
    // 用 conflictIndex 记录第一个不同步的log
	// 目前对于不匹配的 log，会不断回滚
    bool success = false;
    uint32_t conflictIndex = 0;
    uint32_t conflictTerm = 0;
    // LOG_FMT_INFO("server:%d:%d recv entries from leader:%d:%d", me_, current_term_, args->leader_id(), args->term());
    if (args->term() < currentTerm_) {
        // 旧的主节点发来心跳包
        handleReply(reply, success, conflictIndex, conflictTerm);
        return reply;
    }

    if (args->term() > currentTerm_) {
        // 收到新的主节点的心跳包并发现当前节点没有更新任期
        convertToFollower(args->term());
    }

    if (logs_.size() - 1 < args->prev_log_index()) {
        // 从节点缺失主节点的部分记录
        conflictIndex = logs_.size();
        conflictTerm = 0;
        handleReply(reply, success, conflictIndex, conflictTerm);
        return reply;
    }

    if (args->prev_log_index() > 0 && 
        logs_[args->prev_log_index()].term() != args->prev_log_term()) {
        conflictTerm = logs_[args->prev_log_index()].term();
        for (size_t i = 1; i < logs_.size(); i++) {
            // 找到第一个冲突term的位置
            if (logs_[i].term() == conflictTerm) {
                conflictIndex = i;
                break;
            }
        }
        handleReply(reply, success, conflictIndex, conflictTerm);
        return reply;
    } 
    
    // leader 发送的 prevLogIndex 之前的 log 与 follower 同步了
	// follower 可以接收 leader 的 log，把其添加到 logs
    std::vector<LogEntry> logs;
    for (int i = 0; i < args->entries_size(); i++) {
        const LogEntry& entry = args->entries(i);
        logs.push_back(entry);
    }

    // logs_.erase(logs_.begin() + args->prev_log_index() + 1, logs_.end());
    // const LogEntry& last_entry = args->entries(args->entries_size() - 1);
    // uint32_t index = getLastLogIndex() + args->prev_log_index() + 1;

    for (uint32_t i = 0; i < logs.size(); i++) {
        uint32_t index = args->prev_log_index() + 1 + i;
        if (index > getLastLogIndex()) {
            logs_.insert(logs_.end(), logs[i]);
        } else {
            if (logs_[index].term() != logs[i].term()) {
                // 删除 follower 与 leader 不同步的 log
                LOG_FMT_INFO("find me:%d log conflict with leader:%d.start_index:%d, end_index:%d", me_, args->leader_id(), index, getLastLogIndex());
                logs_.erase(logs_.begin() + index, logs_.end());
                logs_.insert(logs_.end(), logs[i]);
            }
            // term一样什么也不做，跳过
        }
    }
    // uint32_t prev_log_index = args->prev_log_index();
    // if (prev_log_index < logs_.size() - 1) {
    //     // 出现了在 follower 和 leader 不同步的 log
    //     logs_.pop_back();
    // }
    // logs_.insert(logs_.end(), logs.begin(), logs.end());


    // TODO: persist()

    // leader告知follower可以是否可以向上层提交log
    if (args->leader_commit() > commitIndex_) {
        commitIndex_ = args->leader_commit();
        if (getLastLogIndex() < commitIndex_) {
            commitIndex_ = getLastLogIndex();
        }
        applyLog();
    }

    success = true;
    handleReply(reply, success, conflictIndex, conflictTerm);
    return reply;
}

void Raft::defaultApplyFunc(uint32_t server, LogEntry entry) {
	(void) server;
	LOG_DEBUG << toString() << " :apply msg index=" << entry.index();
}

} // namespace raftcpp
