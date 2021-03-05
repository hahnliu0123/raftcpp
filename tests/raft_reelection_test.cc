#include "config.h"
#include "gtest/gtest.h"

#include "reyao/log.h"

using namespace reyao;

namespace raftcpp {


static int min_raft_election_timeout = 1000;

TEST(raft_election_test, TestReElection) {
    g_logger->setLevel(LogLevel::INFO);

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
    usleep(2 * min_raft_election_timeout * 1000);
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

} // namespace raftcpp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}