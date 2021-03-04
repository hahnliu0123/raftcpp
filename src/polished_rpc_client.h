#pragma once

#include <reyao/rpc/rpc_client.h>


namespace raftcpp {

class PolishedRpcClient {
public:
    typedef std::shared_ptr<PolishedRpcClient> SPtr;
    PolishedRpcClient(reyao::Scheduler* sche, reyao::IPv4Address::SPtr addr)
        : connect_(true), client_(sche, addr) {}
    
    bool isConnected() const { return connect_; }
    void setConnect(bool v) { connect_ = v; }

    template<typename T>
    bool Call(reyao::rpc::MessageSPtr req, typename reyao::rpc::TypeTraits<T>::ResponseHandler handler) {
        if (connect_) {
            return client_.Call<T>(req, handler);
        } else {
            return false;
        }
    }

private:
    bool connect_;
    reyao::rpc::RpcClient client_;
};

} //namespace raftcpp
