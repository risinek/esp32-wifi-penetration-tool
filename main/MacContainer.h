#ifndef MAC_CONTAINER_H
#define MAC_CONTAINER_H

#include <array>
#include <vector>

// This class is used to simplify adding of new MACs before passing them to C (not C++) function for performing deauth
// attack
class MacContainer {
 public:
  MacContainer() = default;
  explicit MacContainer(const std::vector<std::array<uint8_t, 6>>& macs);

  void add(const std::array<uint8_t, 6>& mac);
  void add(const std::vector<std::array<uint8_t, 6>>& macs);
  const std::vector<uint8_t>& getStorage() const;

 private:
  uint16_t mSize{0};
  std::vector<uint8_t> mStorage;
};

#endif  // MAC_CONTAINER_H
