#include "tests/flex-ui/unit/support/fake_js_engine.h"

#include <memory>

namespace flexui::core::js_engine::test {

// FakeJsValue
std::shared_ptr<IJsValue> FakeJsValue::GetProperty(const std::string&) const {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsValue::GetArrayItem(uint32_t) const {
  return std::make_shared<FakeJsValue>();
}

// FakeJsContext
std::shared_ptr<IJsValue> FakeJsContext::Eval(const std::string& /*source*/,
                                               const std::string& /*origin*/,
                                               flexui::common::Error* err) {
  ++eval_call_count;
  if (err) *err = flexui::common::Error::Ok();
  return std::make_shared<FakeJsValue>();
}

std::shared_ptr<IJsValue> FakeJsContext::NewUndefined() {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsContext::NewNull() {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsContext::NewBoolean(bool) {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsContext::NewNumber(double) {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsContext::NewString(const std::string&) {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsContext::NewObject() {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsContext::NewArray(uint32_t) {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsContext::FromFlexUIValue(
    const flexui::common::FlexUIValue&) {
  return std::make_shared<FakeJsValue>();
}
std::shared_ptr<IJsValue> FakeJsContext::Call(
    std::shared_ptr<IJsValue>,
    std::shared_ptr<IJsValue>,
    const std::vector<std::shared_ptr<IJsValue>>&,
    flexui::common::Error* err) {
  if (err) *err = flexui::common::Error::Ok();
  return std::make_shared<FakeJsValue>();
}

// FakeJsEngine
std::shared_ptr<IJsContext> FakeJsEngine::CreateContext() {
  return std::make_shared<FakeJsContext>();
}

}  // namespace flexui::core::js_engine::test
