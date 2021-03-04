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
      next_index_(0),
      match_index_(0),
      last_broadcast_time_(0),
      role_(Role::Follower),
      last_active_time_(reyao::GetCurrentTime()),
      sche_(sche),
      // FIXME: server's addr field should be a value
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

    // TODO: read persist()
    sche_->addTask(std::bind(&Raft::onElection, this));

}

Raft::~Raft() {
    stop();
    LOG_INFO << "~Raft()";
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
        LOG_DEBUG << "leader:" << me_ << " term:" << term <<" append a cmd["
                  << cmd << "]";
    }
    return is_leader;
}

void Raft::stop() {
    role_ = Role::Dead;
}

bool Raft::isKilled() {
    return role_ == Dead;
}

void Raft::onElection() {
    while (true) {
        int64_t start_time = reyao::GetCurrentTime();
        int64_t expierd_ms = 200 + (rand() % 100);
        usleep(expierd_ms * 1000);
        // FIXME: 退出时 Raft 先析构会导致访问已析构内存，未定义行为
        if (role_ == Role::Dead) {
            LOG_INFO <<  me_ << " quit onElection";
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
    convertToCandidate();
    std::shared_ptr<RequestVoteArgs> args(new RequestVoteArgs);
    args->set_term(current_term_);
    args->set_candidate_id(me_);
    args->set_last_log_index(getLastLogIndex());
    args->set_last_log_term(getLastLogTerm());
    for (size_t i = 0; i < peers_.size(); i++) {
        if (i == me_) {
            continue;
        }
        sche_->addTask([=]() {
            if (!sendRequestVote(i, args)) {
                LOG_INFO << "sendRequestVote " << me_ << " -> " << i << " failed";
            }
        });
    }
}

void Raft::convertToLeader() {
    if (role_ != Candidate) {
        return;
    }
    LOG_DEBUG << me_ << " convert to leader. term: " << current_term_;
    role_ = Leader;
    next_index_ = std::vector<uint32_t>(peers_.size()); 
    match_index_ = std::vector<uint32_t>(peers_.size());
    for (size_t i = 0; i < peers_.size(); i++) {
        next_index_[i] = getLastLogIndex() + 1;
    }
    // TODO: persist()
}

void Raft::convertToCandidate() {
    LOG_DEBUG << me_ << " convert to candidate. term: " << current_term_;
    role_ = Candidate;
    ++current_term_;
    vote_for_ = me_;
    num_of_vote_ = 1;
    // TODO: persist()
}


void Raft::convertToFollower(uint32_t term) {
    LOG_DEBUG << me_ << " convert to follower. term: " << current_term_;
    role_ = Follower;
    current_term_ = term;
    vote_for_ = -1;
    num_of_vote_ = 0;
    // TODO: persist()
}

void Raft::sendHeartBeat() {
    while (!isKilled()) {
        usleep(100 * 1000);
        // FIXME: 退出时 Raft 先析构会导致访问已析构内存，未定义行为
        if (role_ != Leader || role_ == Dead) {
            LOG_INFO << me_ << " quit sendHeartBeat";
            return;
        }

        int64_t now = reyao::GetCurrentTime();
        if (now < last_broadcast_time_ + 100) {
            return; //NOTE:
        }

        last_broadcast_time_ = now;

        for (size_t i = 0; i < peers_.size(); i++) {
            if (i == me_) {
                continue;
            }
            // TODO: how to send entries?
            uint32_t next_index = next_index_[i];
            std::shared_ptr<AppendEntriesArgs> args(new AppendEntriesArgs);
            args->set_term(current_term_);
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
    // match_index_记录着所有server已经获得的log
	// 取其中位数（多数server）的值作为raft集群可以向上层提交的log
    std::vector<uint32_t> match_index(match_index_);
    match_index[me_] = logs_.size() - 1;
    std::sort(match_index.begin(), match_index.end());
    uint32_t N = match_index[match_index.size() / 2];
    LOG_DEBUG << me_ << "advanceCommitIndex, N:" << N << " commit_index:"
              << commit_index_ << " logs_[N].term:" << logs_[N].term()
              << " current_term:" << current_term_;
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

// @args index--candidate lastLogIndex
//       term--candidate lastLogTerm
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
    if (!reply->vote_granted()) {
        if (reply->term() > current_term_) {
            convertToFollower(reply->term());
        }
    } 
    ++num_of_vote_;
    if (num_of_vote_ > peers_.size() / 2) {
        convertToLeader();
        sche_->addTask(std::bind(&Raft::sendHeartBeat, this));
    }

}
    
void Raft::onAppendEntriesReply(uint32_t peer,
                                std::shared_ptr<AppendEntriesArgs> args, 
                                std::shared_ptr<AppendEntriesReply> reply) {
    if (args->term() != current_term_) {
        // 任期不同，有别的server成为leader并更改了me这个旧leader的任期
        return;
    }
    if (reply->term() > current_term_) {
        // 有term更新的leader
        convertToFollower(reply->term());
        return;
    }
    if (reply->success() == true) {
        LOG_DEBUG << me_ << " -> " << peer 
                  << " sendHeartBeat success. leader_commit:" << args->leader_commit()
                  << " match_index:" << args->prev_log_index() << " -> "
                  << args->prev_log_index() + args->entries().size();

        match_index_[peer] = args->prev_log_index() + args->entries().size();
        next_index_[peer] = match_index_[peer] + 1;
        // 看看是否大部分server都更新的log index，如果有更新，说明可以往上层应用提交
        advanceCommitIndex();
    } else {
        if (reply->conflict_term() != 0) {
            // server的PrevLog的term冲突
			// 找到leader的conflictTerm出现的最后位置
			// 如果没有再找conflictIndex
            uint32_t last_index_of_conflict_term = 0;
            for (size_t i = args->prev_log_index(); i > 0; i--) {
                if (logs_[i].term() == reply->conflict_term()) {
                    last_index_of_conflict_term = i;
                    break;
                }
            }
            if (last_index_of_conflict_term == 0) {
                // leader的log中没有reply.ConflictTerm这个任期的日志
			    // 那么从follower首次出现ConflictTerm的位置开始同步
                next_index_[peer] = reply->conflict_index();
            } else {
                // 从leader的log中最后一个reply.ConflictTerm的下一个log开始同步
                next_index_[peer] = last_index_of_conflict_term + 1;
            }
        } else {
            // follower记录的日志缺失
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

    reply->set_term(current_term_);
    // TODO: see paper 5.4
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
    // 用conflictIndex记录第一个不同步的log
	// 目前对于不匹配的log，会不断回滚
    bool success = false;
    uint32_t conflict_index = 0;
    uint32_t conflict_term = 0;

    if (args->term() < current_term_) {
        // 收到旧leader的消息
        handleReply(reply, success, conflict_index, conflict_term);
        return reply;
    }

    if (args->term() > current_term_) {
        // 收到leader的请求且发现自己没有更新任期
        convertToFollower(args->term());
    }

    if (logs_.size() - 1 < args->prev_log_index()) {
        // server的最后一条索引小于leader记录的最后一条索引
		// 说明server丢失了logs_.size()到leader.nextIndex[server]之间的log
        conflict_index = logs_.size();
        conflict_term = 0;
        handleReply(reply, success, conflict_index, conflict_term);
        return reply;
    }

    if (args->prev_log_index() > 0 && 
        logs_[args->prev_log_index()].term() != args->prev_log_term()) {
        // leader记录的follower最后一条log的任期与follower上的log不同步
		// follower应该告知leader第一个不同步的term的位置
		// 如果leader找不到这个不同步的任期，则从这个位置开始重新同步
		// 如果leader能找到这个任期的log，则从leader的log上最后一条这个任期的log往后同步
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
    uint32_t index = args->prev_log_index();
    const auto& entries = args->entries();
    for (int i = 0; i < entries.size(); i++) {
        ++index;
        if (index > logs_.size() - 1) {
            logs_.insert(logs_.end(), entries.begin(), entries.end());
        } else {
            if (logs_[index].term() != entries[index].term()) {
                // 删除follower与leader不同步的log
                logs_.erase(logs_.begin() + index, logs_.end());
                logs_.insert(logs_.end(), entries[index]);
            }
            // term一样什么也不做，跳过
        }
    }

    // TODO: persist()

    // leader告知follower可以是否可以向上层提交log
    if (args->leader_commit() > commit_index_) {
        commit_index_ = args->leader_commit();
        if (getLastLogIndex() < commit_index_) {
            commit_index_ = getLastLogIndex();
        }
    }

    success = true;
    handleReply(reply, success, conflict_index, conflict_term);
    return reply;
}

} //namespace raftcpp
