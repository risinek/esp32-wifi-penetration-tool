#include "MacContainer.h"

namespace {
constexpr uint8_t kMacSize{6};
}  // namespace

MacContainer::MacContainer(const std::vector<std::array<uint8_t, 6>>& macs) { add(macs); }

void MacContainer::add(const std::array<uint8_t, 6>& mac) {
  ++mSize;
  mStorage.reserve(mSize * kMacSize);
  for (auto byte : mac) {
    mStorage.push_back(byte);
  }
}

void MacContainer::add(const std::vector<std::array<uint8_t, 6>>& macs) {
  mSize += macs.size();
  mStorage.reserve(mSize * kMacSize);

  for (const auto& mac : macs) {
    for (auto byte : mac) {
      mStorage.push_back(byte);
    }
  }
}

const std::vector<uint8_t>& MacContainer::getStorage() const { return mStorage; }
