// inprocess_bridge.cc
#include "flexui/core/bridge/inprocess_bridge.h"

#include "flexui/common/log_tag.h"

namespace flexui::core::bridge {

InProcessBridge::InProcessBridge(
    std::shared_ptr<flexui::common::TaskRunner> js,
    std::shared_ptr<flexui::common::TaskRunner> ui)
    : js_runner_(std::move(js)), ui_runner_(std::move(ui)) {}

InProcessBridge::~InProcessBridge() = default;

void InProcessBridge::Post(const Endpoint& dest, EncodedFrame frame) {
  FLEXUI_TLOG(Bridge, Post, DEBUG)
      << "channel=" << dest.channel << " scope=" << dest.scope_id
      << " bytes=" << frame.bytes.size();

  // Route by channel name: js2* -> ui_runner, ui2* -> js_runner, else js.
  auto target = (dest.channel.rfind("js2", 0) == 0) ? ui_runner_ : js_runner_;
  if (!target) {
    FLEXUI_TLOG(Bridge, Post, ERROR) << "no target runner for channel="
                                     << dest.channel;
    return;
  }

  auto frame_copy = std::make_shared<EncodedFrame>(std::move(frame));
  Endpoint dest_copy = dest;
  std::vector<MessageHandler> matched;
  {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& kv : subs_) {
      if (kv.second.endpoint == dest_copy) {
        matched.push_back(kv.second.handler);
      }
    }
  }
  for (auto& h : matched) {
    target->PostTask([h, dest_copy, frame_copy] {
      h(dest_copy, *frame_copy);
    });
  }
}

uint64_t InProcessBridge::Subscribe(const Endpoint& endpoint,
                                    MessageHandler handler) {
  uint64_t token = next_token_.fetch_add(1);
  std::lock_guard<std::mutex> lock(mu_);
  subs_[token] = {endpoint, std::move(handler)};
  FLEXUI_TLOG(Bridge, Subscribe, DEBUG)
      << "channel=" << endpoint.channel << " token=" << token;
  return token;
}

bool InProcessBridge::Unsubscribe(uint64_t token) {
  std::lock_guard<std::mutex> lock(mu_);
  return subs_.erase(token) > 0;
}

}  // namespace flexui::core::bridge
