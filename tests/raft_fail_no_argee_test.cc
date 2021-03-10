#include "config.h"

#include <gtest/gtest.h>
#include "reyao/log.h"

using namespace reyao;

namespace raftcpp {

int min_raft_election_timeout = 1000;

TEST(raft_argee_test, TestFailNoArgee) {
    reyao::g_logger->setLevel(LogLevel::INFO);
    uint32_t servers = 5;
    Scheduler sche;
    sche.startAsync();
    Config cfg(&sche, servers);
    
    cfg.one("10", servers, false);

    int leader = cfg.checkOneLeader();
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
        LOG_FMT_ERROR("expected indexx 2, got %d", index);
    }
    usleep(min_raft_election_timeout * 1000);

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

} // namespace raftcpp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}