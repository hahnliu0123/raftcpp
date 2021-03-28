#include "config.h"

#include <gtest/gtest.h>
using namespace reyao;
using namespace raftcpp;

static const int64_t kMinRaftElectionTimeout = 1000; // 1000 ms

// 测试3个节点能否选举成功
TEST(TestElection, TestInitialElection) {
    Scheduler sche;
    Config cfg(&sche, 3);
    sche.startAsync();
    cfg.checkOneLeader();

    usleep(50 * 1000);
    auto term1 = cfg.checkTerm();

    usleep(2 * 1000 * 1000);
    uint32_t term2 = cfg.checkTerm();
    EXPECT_EQ(term1, term2);

    cfg.checkOneLeader();
}

// 测试主节点掉线能否成功选主；旧主节点重连后能否降级为从节点；集群有一半以上节点掉线能否选举成功
TEST(TestElection, TestReElection) {

    Scheduler sche;
    Config cfg(&sche, 3);
    sche.startAsync();

    int leader1 = cfg.checkOneLeader();
    LOG_INFO << "first check one leader passed, leader is " << leader1;

    // leader disconnect, a new leader should be elected
    cfg.setConnect(leader1, false);
    int leader2 = cfg.checkOneLeader();
    LOG_INFO << "second check one leader passed, leader is " << leader2;

    // old leader reconnect, it should convert to follower
    cfg.setConnect(leader1, true);
    int leader3 = cfg.checkOneLeader();
    LOG_INFO << "third check one leader passed, leader is " << leader3;

    // there's no quorum, no leader should be elected
    cfg.setConnect(leader3, false);
    cfg.setConnect((leader3 + 1) % 3, false);
    usleep(2 * kMinRaftElectionTimeout * 1000);
    cfg.checkNoLeader();
    LOG_INFO << "fourth check no leader passed";

    // if a quorum arises, it should elect a leader
    cfg.setConnect((leader3 + 1) % 3, true);
    int leader4 = cfg.checkOneLeader();
    LOG_INFO << "fifth check one leader passed, leader is " << leader4;

    // last node reconnect and convert to follower
    cfg.setConnect(leader3, true);
    int leader5 = cfg.checkOneLeader();
    LOG_INFO << "fifth check one leader passed, leader is " << leader5;
}

// 测试正常连接下往 leader 发三个请求并检查所有 Raft 是否同步
TEST(TestReplication, TestBaseArgee) {
    Scheduler sche;
    Config cfg(&sche, 5);
    sche.startAsync();

    // sleep to wait a leader 
    cfg.checkOneLeader();
    for (int idx = 1; idx < 4; ++idx) {
        int nd = cfg.nCommitted(idx);
        if (nd > 0) {
            LOG_FATAL << "some have committed before start";
        }

        int xidx = cfg.one(std::to_string(idx * 100), 5, false);
        if (xidx != idx) {
            LOG_FATAL << "got index " << xidx << " but expected" << idx;
        }
    }
}

// 测试少数节点掉线下往 leader 发送请求剩余节点能否同步
TEST(TestReplication, TestFailArgee) {
    Scheduler sche;
    int servers = 3;
    Config cfg(&sche, servers, "0.0.0.0", 10000);
    sche.startAsync();

    cfg.one("101", servers, false);

    int leader = cfg.checkOneLeader();
    cfg.setConnect((leader + 1) % servers, false);

	cfg.one("102", servers - 1, false);
	cfg.one("103", servers - 1, false);
	usleep(kMinRaftElectionTimeout * 1000);
	cfg.one("104", servers - 1, false);
	cfg.one("105", servers - 1, false);

	// re-connect
	cfg.setConnect((leader + 1) % servers, true);

	usleep(kMinRaftElectionTimeout * 1000);
	cfg.one("106", servers, false);
	usleep(kMinRaftElectionTimeout * 1000);
	cfg.one("107", servers, false);
}

// 测试多数节点掉线时 leader 能否 commit；网络恢复后发送命令所有节点能否同步
TEST(TestReplication, TestFailNoArgee) {
    uint32_t servers = 5;
    Scheduler sche;
    sche.startAsync();
    Config cfg(&sche, servers);
    
    auto leader = cfg.checkOneLeader();
    cfg.one("10", servers, false);
    cfg.setConnect((leader + 1) % servers, false);
    cfg.setConnect((leader + 2) % servers, false);
    cfg.setConnect((leader + 3) % servers, false);

    uint32_t term;
    uint32_t index;
    bool is_leader = cfg.getRaft(leader)->append("20", index, term);
    if (!is_leader) {
        LOG_FMT_ERROR("leader rejected append log");
    }
    if (index != 2) {
        LOG_FMT_ERROR("expected index 2, got %d", index);
    }
    usleep(kMinRaftElectionTimeout * 1000);

    int n = cfg.nCommitted(index);
    if (n > 0) {
        LOG_FMT_ERROR("%d committed but no majority", n);
    }

    // repair
    cfg.setConnect((leader + 1) % servers, true);
    cfg.setConnect((leader + 2) % servers, true);
    cfg.setConnect((leader + 3) % servers, true);

    // the disconnected majority may have chosen a leader
    // from their own ranks, forgetting index 2
    int leader2 = cfg.checkOneLeader();
    uint32_t index2;
    uint32_t term2;
    is_leader = cfg.getRaft(leader2)->append("30", index2, term2);
    if (!is_leader) {
        LOG_FMT_ERROR("leader2 rejected append log");
    }
    if (index2 < 2 || index > 3) {
        LOG_FMT_ERROR("unexpected index", index2);
    }

    cfg.one("1000", servers, true);
}

// 测试主节点出现网络分区后分别往旧主节点和新主节点发送请求能否 commit
// 测试旧主节点恢复分区后能否日志同步
TEST(TestReplication, TestRejoinTest) {
    uint32_t servers = 3;
	Scheduler sche;
	sche.startAsync();
	Config cfg(&sche, servers);

	cfg.one("101", servers, true);
	// 主节点1掉线
	int leader1 = cfg.checkOneLeader();
	cfg.setConnect(leader1, false);

	// 向掉线的主节点1添加日志
	uint32_t index;
	uint32_t term;
	cfg.getRaft(leader1)->append("102", index, term);
	cfg.getRaft(leader1)->append("103", index, term);
	cfg.getRaft(leader1)->append("104", index, term);

	// 向新的主节点2添加日志
	cfg.one("103", 2, true);

	// 主节点2掉线
	int leader2 = cfg.checkOneLeader();
	cfg.setConnect(leader2, false);

	// 主节点1重连，应该转为从节点
	cfg.setConnect(leader1, true);
    
	cfg.one("104", 2, true);

	// 所有节点重连
	cfg.setConnect(leader2, true);

	cfg.one("105", servers, true);
}

// 出现掉线和网络分区时能否正常选主和 commit；最终日志能否一致
TEST(TestReplication, TestBackup) {
    uint32_t servers = 5;
	Scheduler sche;
	sche.startAsync();
	Config cfg(&sche, servers);
	usleep(1 * 1000 * 1000);
	int count = 1;

    LOG_INFO << "add a log to cfg while all servers connect";
	cfg.one(std::to_string(count++), servers, true);
    usleep(100 * 1000);
    cfg.printLogs();
	LOG_INFO << "pass 1";

	// 主节点和另一个从节点出现网络分区
	int leader1 = cfg.checkOneLeader();
	cfg.setConnect((leader1 + 2) % servers, false);
	cfg.setConnect((leader1 + 3) % servers, false);
	cfg.setConnect((leader1 + 4) % servers, false);
	LOG_INFO << "put leader and one follower in a partition";

	// 少数分区不能 commit
    LOG_INFO << "append log to old leader which in partition with two node, the log should not be commit";
	uint32_t index;
	uint32_t term;
	for (int i = 0; i < 50; ++i) {
		cfg.getRaft(leader1)->append(std::to_string(count++), index, term);
	}
    cfg.printLogs();

	usleep(kMinRaftElectionTimeout * 1000 / 2);
    // 少数分区掉线
	cfg.setConnect((leader1 + 0) % servers, false);
	cfg.setConnect((leader1 + 1) % servers, false);

	// 多数分区恢复网络
	cfg.setConnect((leader1 + 2) % servers, true);
	cfg.setConnect((leader1 + 3) % servers, true);
	cfg.setConnect((leader1 + 4) % servers, true);
	LOG_INFO << "allow other partition to recover";
	usleep(1 * 1000 * 1000);
	cfg.printLogs();

    LOG_INFO << "majority server recover, election and choose new leader and append 50 logs";
	// 新主节点接收50条日志并 commit
	for (int i = 0; i < 50; ++i) {
		cfg.one(std::to_string(count++), 3, true);
	}
    cfg.printLogs();
	LOG_INFO << "pass 2";

	// 让其中一个节点掉线，提交50条日志，此时节点数不过半
	int leader2 = cfg.checkOneLeader();
	int other = (leader1 + 2) % servers;
	if (leader2 == other) {
		other = (leader2 + 1) % servers;
	}
	cfg.setConnect(other, false);
	usleep(1 * 1000 * 1000);

	// 所有日志都不该被提交
	for (uint32_t i = 0; i < 50; ++i) {
		cfg.getRaft(leader2)->append(std::to_string(count++), index, term);
	}
    cfg.printLogs();

	usleep(kMinRaftElectionTimeout * 1000 / 2);

	for (uint32_t i = 0; i < servers; ++i) {
		cfg.setConnect(i, false);
	}

	// 第一个分区和第二个分区的其中一个从节点恢复连接
    // 第一个分区的节点由于日志的 index 和 term 较旧，不能成为 leader
    // 应该降级为 follower。另一个节点成为 leader，并将日志同步给其他节点
	cfg.setConnect(leader1 + 0, true);
	cfg.setConnect(leader1 + 1, true);
	cfg.setConnect(other, true);
	LOG_INFO << "bring original leader back to life";
	usleep(1 * 1000 * 1000);
	LOG_INFO << "leader is " << cfg.checkOneLeader();
	// 添加50条日志
	for (int i = 0; i < 50; ++i) {
		cfg.one(std::to_string(count++), 3, true);
	}
	LOG_INFO << "pass 3";
	cfg.printLogs();

	// 全部节点恢复网络，检查最终是否一致
	for (uint32_t i = 0; i < servers; ++i) {
		cfg.setConnect(i, true);
	}
	LOG_INFO << "bring all back to life";
	usleep(1 * 1000 * 1000);

	cfg.one(std::to_string(count++), servers, true);
    cfg.printLogs();
	LOG_INFO << "pass 4";
}

int main(int argc, char** argv) {
    g_logger->setLevel(LogLevel::WARN);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
