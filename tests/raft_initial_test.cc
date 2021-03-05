#include "config.h"

#include <gtest/gtest.h>

using namespace reyao;

namespace raftcpp {

TEST(raft_election_test, TestInitialElection) {
    g_logger->setLevel(LogLevel::INFO);
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

} // namespace raftcpp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}