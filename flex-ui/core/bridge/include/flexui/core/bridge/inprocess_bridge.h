// inprocess_bridge.h
#pragma once
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "flexui/core/bridge/ibridge.h"

namespace flexui::core::bridge {

class InProcessBridge : public IBridge {
 public:
  InProcessBridge(std::shared_ptr<flexui::common::TaskRunner> js_runner,
                  std::shared_ptr<flexui::common::TaskRunner> ui_runner);
  ~InProcessBridge() override;

  void Post(const Endpoint& dest, EncodedFrame frame) override;
  uint64_t Subscribe(const Endpoint& endpoint, MessageHandler handler) override;
  bool Unsubscribe(uint64_t token) override;

 private:
  struct Sub {
    Endpoint endpoint;
    MessageHandler handler;
  };
  std::shared_ptr<flexui::common::TaskRunner> js_runner_;
  std::shared_ptr<flexui::common::TaskRunner> ui_runner_;
  std::mutex mu_;
  std::unordered_map<uint64_t, Sub> subs_;
  std::atomic<uint64_t> next_token_{1};
};

}  // namespace flexui::core::bridge
