#include "config.h"

#include <gtest/gtest.h>
#include "reyao/log.h"

using namespace reyao;

namespace raftcpp {

int milli_raft_election_timeout = 1000;

void printLog(uint32_t servers, Config& cfg) {
    for (uint32_t i = 0; i < servers; i++) {
        const auto& logs = cfg.getLog(i);
        std::stringstream ss;
        ss << "server " << i << " ";
        for (auto& log : logs) {
            ss << log.command() << " ";
        }
        LOG_INFO << ss.str();
    }
}

TEST(raft_agree_test, TestRejoin) {
	g_logger->setLevel(LogLevel::INFO);

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
} // namespace raftcpp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}