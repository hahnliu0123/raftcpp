#include "config.h"
#include "gtest/gtest.h"

#include "reyao/log.h"

using namespace reyao;

namespace raftcpp {

int milli_raft_election_timeout = 1000;

TEST(raft_agree_test, TestFailAgree) {
    g_logger->setLevel(LogLevel::INFO);

    Scheduler sche;
    int servers = 3;
    Config cfg(&sche, servers, "0.0.0.0", 10000);
    sche.startAsync();

    cfg.one("101", servers, false);

    int leader = cfg.checkOneLeader();
    cfg.setConnect((leader + 1) % servers, false);

	// agree despite one disconnected server?
	cfg.one("102", servers - 1, false);
	cfg.one("103", servers - 1, false);
	usleep(milli_raft_election_timeout * 1000);
	cfg.one("104", servers - 1, false);
	cfg.one("105", servers - 1, false);

	// re-connect
	cfg.setConnect((leader + 1) % servers, true);

	// agree with full set of servers?
	usleep(milli_raft_election_timeout * 1000);
	cfg.one("106", servers, false);
	usleep(milli_raft_election_timeout * 1000);
	cfg.one("107", servers, false);

}

} // namespace raftcpp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}