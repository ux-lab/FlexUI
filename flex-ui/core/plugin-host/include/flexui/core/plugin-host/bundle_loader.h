#pragma once

#include <functional>
#include <future>
#include <string>
#include <vector>

namespace flexui::core::plugin_host {

struct BundleSource {
  std::string uri;
  std::string text;                    // for JSON-ish bundles
  std::vector<uint8_t> bytes;          // for binary bundles
};

struct BundleLoader {
  std::string scheme;                  // "file" / "https" / "rcs"
  std::function<std::future<BundleSource>(const std::string& uri)> load;
};

}  // namespace flexui::core::plugin_host
