#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <cstdint>
#include <vector>

// Thisclass implements ring buffer which overwrites its old data if newly inserted data doesn't fit it
class RingBuffer {
  using ElementType = char;

 public:
  RingBuffer(uint32_t size = 0);
  // Returns true if all inserted elements fit the buffer, false otherwise
  template <class ItType>
  bool insert(ItType first, ItType last);
  bool insert(const std::vector<ElementType>& vec);
  bool insert(std::vector<ElementType>&& vec);

  std::vector<ElementType> extract(uint32_t size);
  void clear();
  // NOTE! Clears buffer!
  void resize(uint32_t size);

  // Return total number of elements, which this buffer can store
  // NOTE! Because of one empty element, real capacity sometimes can be 1 element less
  uint32_t capacity() const;
  // Return number of elements inside buffer
  uint32_t size() const;
  uint32_t getFreeElements() const;
  bool empty() const;

 private:
  template <class ItType>
  bool do_insert(ItType first, ItType last);

  std::vector<ElementType> mData;
  decltype(mData)::iterator mItFirst;
  decltype(mData)::iterator mItLast;
};

template <class ItType>
bool RingBuffer::insert(ItType first, ItType last) {
  uint32_t numOfNewElements = std::distance(first, last);
  if (numOfNewElements >= mData.size()) {
    // Internal buffer will be entirely overwritten.
    // So, just take last elements of input data and place them inside buffer
    auto prevSize = size();
    mItFirst = mItLast = mData.begin();
    uint32_t newFirstElementIndex = numOfNewElements - mData.size();
    std::advance(first, newFirstElementIndex);
    do_insert(first, last);
    return (prevSize == 0);
  }

  if (numOfNewElements <= getFreeElements()) {
    // New elements will entirely fit buffer
    return do_insert(first, last);
  }

  // Part of elements will be overwritten. So we just extract them before insertion
  uint32_t numOfOverwrittenElements = numOfNewElements - getFreeElements();
  if (mItFirst == mData.begin()) {
    // In case 1st stored element is placed at the very beggining of internal storrage,
    // then need to free it as well, because in that case internal buffer doesn't have empty element,
    // which is normally required
    ++numOfOverwrittenElements;
  }

  extract(numOfOverwrittenElements);
  return do_insert(first, last);
}

template <class ItType>
bool RingBuffer::do_insert(ItType first, ItType last) {
  uint32_t numOfNewElements = std::distance(first, last);
  if (mItFirst <= mItLast) {
    // Number of elements, that we can copy to the right from existing interval
    uint32_t elementsToCopyR = std::min(numOfNewElements, (uint32_t)std::distance(mItLast, mData.end()));
    if (elementsToCopyR != 0) {
      // Copy first part of elements
      std::copy(first, first + elementsToCopyR, mItLast);
      std::advance(first, elementsToCopyR);
      std::advance(mItLast, elementsToCopyR);
      numOfNewElements -= elementsToCopyR;
      if (numOfNewElements == 0) {
        return true;
      }
    }

    // Number of elements, that we can copy to the left from existing interval
    uint32_t elementsToCopyL{0};
    if (mItFirst != mData.begin()) {
      elementsToCopyL = std::min(numOfNewElements, (uint32_t)std::distance(mData.begin(), (mItFirst - 1)));
    }

    if (elementsToCopyL != 0) {
      // Copy second part of elements
      std::copy(first, first + elementsToCopyL, mData.begin());
      mItLast = mData.begin();
      std::advance(mItLast, elementsToCopyL);
      numOfNewElements -= elementsToCopyL;
    }

    return (numOfNewElements == 0);
  } else {
    uint32_t elementsToCopy = std::min(numOfNewElements, (uint32_t)std::distance(mItLast, mItFirst) - 1);
    std::copy(first, first + elementsToCopy, mItLast);
    std::advance(mItLast, elementsToCopy);
    return (numOfNewElements == 0);
  }
}

#endif  // RING_BUFFER_H
