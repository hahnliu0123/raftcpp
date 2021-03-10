#include "config.h"

#include <gtest/gtest.h>
#include "reyao/log.h"

using namespace reyao;

namespace raftcpp {

int milli_raft_election_timeout = 1000;


TEST(raft_agree_test, TestBackup) {
	reyao::g_logger->setLevel(LogLevel::INFO);

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

	// put leader and one follower in a partition
	int leader1 = cfg.checkOneLeader();
	cfg.setConnect((leader1 + 2) % servers, false);
	cfg.setConnect((leader1 + 3) % servers, false);
	cfg.setConnect((leader1 + 4) % servers, false);
	LOG_INFO << "put leader and one follower in a partition";

	// submit lots of commands that won't commit
    LOG_INFO << "append log to old leader which in partition with two node, the log should not be commit";
	uint32_t index;
	uint32_t term;
	for (int i = 0; i < 50; ++i) {
		cfg.getRaft(leader1)->append(std::to_string(count++), index, term);
	}
    cfg.printLogs();

	usleep(milli_raft_election_timeout * 1000 / 2);

	cfg.setConnect((leader1 + 0) % servers, false);
	cfg.setConnect((leader1 + 1) % servers, false);

	// allow other partition to recover
	cfg.setConnect((leader1 + 2) % servers, true);
	cfg.setConnect((leader1 + 3) % servers, true);
	cfg.setConnect((leader1 + 4) % servers, true);
	LOG_INFO << "allow other partition to recover";
	usleep(1 * 1000 * 1000);
	cfg.printLogs();

    LOG_INFO << "majority server recover, election and choose new leader and append 50 logs";
	// lots of successful commands to new group.
	for (int i = 0; i < 50; ++i) {
		cfg.one(std::to_string(count++), 3, true);
	}
    cfg.printLogs();
	LOG_INFO << "pass 2";

	// 两个分区各只有一个主节点和从节点
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

	usleep(milli_raft_election_timeout * 1000 / 2);

	for (uint32_t i = 0; i < servers; ++i) {
		cfg.setConnect(i, false);
	}

	// 第一个分区和第二个分区的其中一个从节点恢复连接
	cfg.setConnect(leader1 + 0, true);
	cfg.setConnect(leader1 + 1, true);
	cfg.setConnect(other, true);
	LOG_INFO << "bring original leader back to life";
	usleep(1 * 1000 * 1000);
	LOG_INFO << "leader is " << cfg.checkOneLeader();
	// lots of successful commands to new group.
	for (int i = 0; i < 50; ++i) {
		cfg.one(std::to_string(count++), 3, true);
	}
	LOG_INFO << "pass 3";
	cfg.printLogs();

	// now everyone
	for (uint32_t i = 0; i < servers; ++i) {
		cfg.setConnect(i, true);
	}
	LOG_INFO << "bring all back to life";
	usleep(1 * 1000 * 1000);

	cfg.one(std::to_string(count++), servers, true);
    cfg.printLogs();
	LOG_INFO << "pass 4";
}

} // namespace raftcpp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}