#include "raft.h"

#include <reyao/util.h>

#include <stdlib.h>

namespace raftcpp {


Raft::Raft(std::vector<PolishedRpcClient::SPtr> peers, uint32_t me, 
           reyao::IPv4Address::SPtr addr, reyao::Scheduler* sche)
    : peers_(peers),
      me_(me),
      current_term_(1),
      vote_for_(-1),
      num_of_vote_(0),
      commit_index_(0),
      last_applied_(0),
      next_index_(),
      match_index_(),
      last_broadcast_time_(0),
      apply_func_(std::bind(&Raft::defaultApplyFunc, this, std::placeholders::_1, std::placeholders::_2)),
      role_(Role::Follower),
      last_active_time_(reyao::GetCurrentTime()),
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

    next_index_.insert(next_index_.begin(), peers_.size(), logs_.size());
    match_index_.insert(match_index_.begin(), peers_.size(), 0);

    sche_->addTask(std::bind(&Raft::onElection, this));
}

Raft::~Raft() {
    stop();
    LOG_DEBUG << "~Raft()";
}

bool Raft::append(const std::string& cmd, uint32_t& index, uint32_t& term) {
    bool is_leader = isLeader();
    term = current_term_;
    if (is_leader) {
        index = getLastLogIndex() + 1;
        LogEntry entry;
        entry.set_term(term);
        entry.set_index(index);
        entry.set_command(cmd);
        logs_.push_back(entry);
    }
    return is_leader;
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
        int64_t start_time = reyao::GetCurrentTime();
        int64_t expierd_ms = 200 + (rand() % 100);
        usleep(expierd_ms * 1000);
        // FIXME: 退出时 Raft 先析构会导致访问已析构内存，未定义行为
        if (role_ == Role::Dead) {
            LOG_DEBUG <<  me_ << " quit onElection";
            return;
        }
        if (last_active_time_ < start_time) {
            if (role_ != Role::Leader) {
                sche_->addTask(std::bind(&Raft::kickoffElection, this));
            }
        }
    }
}

void Raft::kickoffElection() {
    last_active_time_ = reyao::GetCurrentTime();
    convertToPreCandidate();
    std::shared_ptr<RequestVoteArgs> args(new RequestVoteArgs);
    args->set_term(current_term_ + 1);
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
    last_active_time_ = reyao::GetCurrentTime();
    convertToPreCandidate();
    std::shared_ptr<RequestVoteArgs> args(new RequestVoteArgs);
    args->set_term(current_term_ + 1);
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
    LOG_INFO << me_ << " convert to leader. term: " << current_term_;
    role_ = Leader;
    next_index_ = std::vector<uint32_t>(peers_.size()); 
    match_index_ = std::vector<uint32_t>(peers_.size());
    for (size_t i = 0; i < peers_.size(); i++) {
        next_index_[i] = getLastLogIndex() + 1;
    }
}

void Raft::convertToPreCandidate() {
    LOG_INFO << me_ << " convert to pre_candidate. term: " << current_term_;
    role_ = PreCandidate;
    vote_for_ = me_;
    num_of_vote_ = 1;
}

void Raft::convertToCandidate() {
    LOG_INFO << me_ << " convert to candidate. term: " << current_term_;
    role_ = Candidate;
    ++current_term_;
    vote_for_ = me_;
    num_of_vote_ = 1;
}


void Raft::convertToFollower(uint32_t term) {
    LOG_DEBUG << me_ << " convert to follower. term: " << current_term_;
    role_ = Follower;
    current_term_ = term;
    vote_for_ = -1;
    num_of_vote_ = 0;
}

void Raft::sendHeartBeat() {
    while (!isKilled()) {
        usleep(100 * 1000); // send heartbeat per 100 ms
        // FIXME: 退出时 Raft 先析构会导致访问已析构内存，未定义行为
        if (role_ != Leader || role_ == Dead) {
            LOG_INFO << me_ << " quit sendHeartBeat";
            return;
        }

        int64_t now = reyao::GetCurrentTime();
        if (now < last_broadcast_time_ + 100) {
            return;
        }

        last_broadcast_time_ = now;

        int term = current_term_; //NOTE: 先保存 term，防止心跳包发到一半发现新主节点从而修改到最新 term，这样就会有两个相同 term 的主节点发送心跳包
        for (size_t i = 0; i < peers_.size(); i++) {
            if (i == me_) {
                continue;
            }
            uint32_t next_index = next_index_[i];
            std::shared_ptr<AppendEntriesArgs> args(new AppendEntriesArgs);
            args->set_term(term);
            args->set_leader_id(me_);
            args->set_prev_log_index(getPrevLogIndex(i));
            args->set_prev_log_term(getPrevLogTerm(i));
            args->set_leader_commit(commit_index_);
            addLogEntries(args, next_index);

            sendAppendEntries(i, args);
        }
    }
}

void Raft::addLogEntries(std::shared_ptr<AppendEntriesArgs> args, uint32_t next_index) {
    for (size_t i = next_index; i < logs_.size(); i++) {
        const LogEntry& src_entry = logs_[i];
        LogEntry* entry = args->add_entries();
        entry->set_term(src_entry.term());
        entry->set_index(src_entry.index());
        entry->set_command(src_entry.command());
    }
}

uint32_t Raft::getLastLogIndex() const {
    return logs_.size() - 1;
}

uint32_t Raft::getLastLogTerm() const {
    uint32_t last_log_index = getLastLogIndex();
    if (last_log_index == 0) {
        return 0;
    } else {
        return logs_[last_log_index].term();
    }
}

uint32_t Raft::getPrevLogIndex(uint32_t peer) const {
    return next_index_[peer] - 1;
} 

uint32_t Raft::getPrevLogTerm(uint32_t peer) const {
    uint32_t prev_log_index = getPrevLogIndex(peer);
    if (prev_log_index == 0) {
        return 0;
    } else {
        return logs_[prev_log_index].term();
    }
}

void Raft::advanceCommitIndex() {
    // 主节点更新commit_index
    // 需要找到过半从节点都收到的日志
    std::vector<uint32_t> match_index(match_index_);
    match_index[me_] = logs_.size() - 1;
    std::sort(match_index.begin(), match_index.end());
    uint32_t N = match_index[match_index.size() / 2];
    if (role_ == Leader && N > commit_index_ &&
        logs_[N].term() == current_term_) {
        // TODO: see paper 5.4
        commit_index_ = N;
        applyLog();
    }
}

void Raft::handleReply(std::shared_ptr<AppendEntriesReply> reply, 
                       bool success, uint32_t conflict_index, 
                       uint32_t conflict_term) {
    applyLog();
    reply->set_success(success);
    reply->set_term(current_term_);
    reply->set_conflict_index(conflict_index);
    reply->set_conflict_term(conflict_term);
}

void Raft::applyLog() {
    while (last_applied_ < commit_index_) {
        ++last_applied_;
        assert(last_applied_ < logs_.size());
        apply_func_(me_, logs_[last_applied_]);
    }
}

bool Raft::isLogMoreUpToDate(uint32_t index, uint32_t term) {
    return term > getLastLogTerm() ||
           (term == getLastLogTerm() && index >= getLastLogIndex());
}

bool Raft::sendRequestVote(uint32_t peer, std::shared_ptr<RequestVoteArgs> args) {
    assert(peer < peers_.size());
    LOG_DEBUG << me_ << " -> " << peer << " snedRequestVote";
    return peers_[peer]->Call<RequestVoteReply>(args, 
                         std::bind(&Raft::onRequestVoteReply, this, args, std::placeholders::_1));
}

bool Raft::sendAppendEntries(uint32_t peer, std::shared_ptr<AppendEntriesArgs> args) {
    assert(peer < peers_.size());
    LOG_DEBUG << me_ << " -> " << peer << " snedRequestVote";
    return peers_[peer]->Call<AppendEntriesReply>(args, 
                         std::bind(&Raft::onAppendEntriesReply, this, peer, args, std::placeholders::_1));
}

void Raft::onRequestVoteReply(std::shared_ptr<RequestVoteArgs> args, 
                              std::shared_ptr<RequestVoteReply> reply) {
    if (role_ == PreCandidate) {
        if (!reply->vote_granted()) {
            if (reply->term() > current_term_) {
                convertToFollower(reply->term());
            }
        }
        ++num_of_vote_;
        if (num_of_vote_ > peers_.size() / 2 && role_ == PreCandidate) {
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
            if (reply->term() > current_term_) {
                convertToFollower(reply->term());
            }
        }
        ++num_of_vote_;
        if (num_of_vote_ > peers_.size() / 2 && role_ == Candidate) {
            convertToLeader();
            sche_->addTask(std::bind(&Raft::sendHeartBeat, this));
        }
    }
}
    
void Raft::onAppendEntriesReply(uint32_t peer,
                                std::shared_ptr<AppendEntriesArgs> args, 
                                std::shared_ptr<AppendEntriesReply> reply) {
    if (args->term() != current_term_) {
        // 该主节点已经变成从节点
        return;
    }
    if (reply->term() > current_term_) {
        // 发现更新任期
        // LOG_FMT_INFO("server:%d:%d find a reply fome term:%d", me_, current_term_, reply->term());
        convertToFollower(reply->term());
        return;
    }
    if (reply->success() == true) {
        LOG_DEBUG << me_ << " -> " << peer 
                  << " sendHeartBeat success. leader_commit:" << args->leader_commit()
                  << " match_index:" << args->prev_log_index() << " -> "
                  << args->prev_log_index() + args->entries().size();

        match_index_[peer] = args->prev_log_index() + args->entries_size();
        next_index_[peer] = match_index_[peer] + 1;
        advanceCommitIndex();
    } else {
        if (reply->conflict_term() != 0) {
            uint32_t last_index_of_conflict_term = 0;
            for (size_t i = args->prev_log_index(); i > 0; i--) {
                if (logs_[i].term() == reply->conflict_term()) {
                    last_index_of_conflict_term = i;
                    break;
                }
            }
            if (last_index_of_conflict_term == 0) {
                next_index_[peer] = reply->conflict_index();
            } else {
                next_index_[peer] = last_index_of_conflict_term + 1;
            }
        } else {
            next_index_[peer] = reply->conflict_index();
        }
    }
}

MessageSPtr Raft::RequestVote(std::shared_ptr<RequestVoteArgs> args) {

    last_active_time_ = reyao::GetCurrentTime();
    LOG_DEBUG << args->candidate_id() << " -> " << me_ << " RequestVote."
              << " candidate[term:" << args->term() << " last_log_index:"
              << args->last_log_index() << " last_log_term:" << args->last_log_term()
              << " ] local[term:" << current_term_ << " last_log_index:"
              << getLastLogIndex() << " last_log_term:" << getLastLogTerm() << "]";
    
    std::shared_ptr<RequestVoteReply> reply(new RequestVoteReply);

    if (args->term() < current_term_) {
        // 有更新的任期
        reply->set_term(current_term_);
        reply->set_vote_granted(false);
        return reply;
    }

    if (args->term() > current_term_) {
        // 候选人有资格成为leader，local降级为follower
        convertToFollower(args->term());
    }

    // vote_for_ == args->candidate_id() 表明真正的选举
    reply->set_term(current_term_);
    if ((vote_for_ == -1 || vote_for_ == args->candidate_id()) &&
        isLogMoreUpToDate(args->last_log_index(), args->last_log_term())) {
        vote_for_ = args->candidate_id();
        reply->set_vote_granted(true);
    } else {
        reply->set_vote_granted(false);
    }

    return reply;
}

MessageSPtr Raft::AppendEntries(std::shared_ptr<AppendEntriesArgs> args) {
    // TODO: persist() when leaving

    LOG_DEBUG << "AppendEntries peer:" << args->leader_id() << "[term:" << args->term()
              << " prev_log_index:" << args->prev_log_index() << " prev_log_term:"
              << args->prev_log_term() << " lead_commit:" << args->leader_commit() 
              << "] " << "local:" << me_ << "[term:" << current_term_
              << " prev_log_index:" << getLastLogIndex() << " prev_log_term:"
              << getLastLogTerm() << " commit_index:" << commit_index_ << "]";
    
    last_active_time_ = reyao::GetCurrentTime();

    std::shared_ptr<AppendEntriesReply> reply(new AppendEntriesReply);
    // 用 conflictIndex 记录第一个不同步的log
	// 目前对于不匹配的 log，会不断回滚
    bool success = false;
    uint32_t conflict_index = 0;
    uint32_t conflict_term = 0;
    // LOG_FMT_INFO("server:%d:%d recv entries from leader:%d:%d", me_, current_term_, args->leader_id(), args->term());
    if (args->term() < current_term_) {
        // 旧的主节点发来心跳包
        handleReply(reply, success, conflict_index, conflict_term);
        return reply;
    }

    if (args->term() > current_term_) {
        // 收到新的主节点的心跳包并发现当前节点没有更新任期
        convertToFollower(args->term());
    }

    if (logs_.size() - 1 < args->prev_log_index()) {
        // 从节点缺失主节点的部分记录
        conflict_index = logs_.size();
        conflict_term = 0;
        handleReply(reply, success, conflict_index, conflict_term);
        return reply;
    }

    if (args->prev_log_index() > 0 && 
        logs_[args->prev_log_index()].term() != args->prev_log_term()) {
        conflict_term = logs_[args->prev_log_index()].term();
        for (size_t i = 1; i < logs_.size(); i++) {
            // 找到第一个冲突term的位置
            if (logs_[i].term() == conflict_term) {
                conflict_index = i;
                break;
            }
        }
        handleReply(reply, success, conflict_index, conflict_term);
        return reply;
    } 
    
    // leader发送的prevLogIndex之前的log与follower同步了
	// follower可以接收leader的log，把其添加到log中并持久化
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
                // 删除follower与leader不同步的log
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
    if (args->leader_commit() > commit_index_) {
        commit_index_ = args->leader_commit();
        if (getLastLogIndex() < commit_index_) {
            commit_index_ = getLastLogIndex();
        }
        applyLog();
    }

    success = true;
    handleReply(reply, success, conflict_index, conflict_term);
    return reply;
}

void Raft::defaultApplyFunc(uint32_t server, LogEntry entry) {
	(void) server;
	LOG_DEBUG << toString() << " :apply msg index=" << entry.index();
}

} //namespace raftcpp
