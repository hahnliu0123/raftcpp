#include "config.h"
#include "gtest/gtest.h"

#include "reyao/log.h"

using namespace reyao;

namespace raftcpp {

TEST(raft_election_test, TestReElection) {
    g_logger->setLevel(LogLevel::INFO);

    Scheduler sche;
    Config cfg(&sche, 10);
    sche.startAsync();

    usleep(10 * 1000);
    int iters = 3;
    for (int idx = 1; idx < iters + 1; ++idx) {
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

} // namespace raftcpp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}