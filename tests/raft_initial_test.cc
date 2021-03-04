#include "config.h"

#include <gtest/gtest.h>

namespace raftcpp {

TEST(raft_election_test, TestInitialElection) {
    reyao::Scheduler sche;
    
    sche.addTask([&]() {
        Config cfg(&sche, 3);
        cfg.checkOneLeader();

        usleep(50 * 1000);
        auto term1 = cfg.checkTerm();

        usleep(2 * 1000 * 1000);
        uint32_t term2 = cfg.checkTerm();
        EXPECT_EQ(term1, term2);

        cfg.checkOneLeader();

        reyao::Worker::GetWorker()->getScheduler()->stop();
    });

    sche.start();
}

} // namespace raftcpp

int main(int argc, char** argv) {
    g_logger->setFormatter("[%p %f:%l]%T%m%n");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}