#include "ring_buffer.h"

#include <algorithm>
#include <iterator>

RingBuffer::RingBuffer(uint32_t size) : mData(size), mItFirst{mData.begin()}, mItLast{mData.begin()} {}

std::vector<RingBuffer::ElementType> RingBuffer::extract(uint32_t size) {
  std::vector<RingBuffer::ElementType> result;
  if (mItFirst == mItLast) {
    return {};
  } else if (mItFirst < mItLast) {
    uint32_t elementsToExtract = std::min(size, (uint32_t)std::distance(mItFirst, mItLast));
    result.insert(result.end(), mItFirst, mItFirst + elementsToExtract);
    std::advance(mItFirst, elementsToExtract);
    if (mItFirst == mItLast) {
      mItFirst = mItLast = mData.begin();
    }
    return result;
  } else {
    // Number of elements, that we can extract from the right part of existing interval
    uint32_t elementsToExtractR = std::min(size, (uint32_t)std::distance(mItFirst, mData.end()));
    result.insert(result.end(), mItFirst, mItFirst + elementsToExtractR);
    std::advance(mItFirst, elementsToExtractR);
    if (mItFirst == mData.end()) {
      mItFirst = mData.begin();
    }
    size -= elementsToExtractR;
    if (size == 0) {
      return result;
    }

    // Number of elements, that we can extract from the left part of existing interval
    uint32_t elementsToExtractL = std::min(size, (uint32_t)std::distance(mData.begin(), mItLast));
    result.insert(result.end(), mItFirst, mItFirst + elementsToExtractL);
    std::advance(mItFirst, elementsToExtractL);
    if (mItFirst == mItLast) {
      mItFirst = mItLast = mData.begin();
    }
    return result;
  }
}

bool RingBuffer::insert(const std::vector<RingBuffer::ElementType>& vec) { return insert(vec.begin(), vec.end()); }

bool RingBuffer::insert(std::vector<RingBuffer::ElementType>&& vec) { return insert(vec.begin(), vec.end()); }

void RingBuffer::clear() {
  mData.clear();
  mItFirst = mItLast = mData.begin();
}

void RingBuffer::resize(uint32_t size) {
  clear();
  mData.resize(size);
  mData.shrink_to_fit();
  mItFirst = mItLast = mData.begin();
}

uint32_t RingBuffer::capacity() const {
  uint32_t result{mData.size()};
  if ((mItFirst != mItLast) && (mItFirst != mData.begin())) {
    // Reserve space for empty element
    --result;
  }
  return result;
}

uint32_t RingBuffer::size() const {
  if (mItFirst == mItLast) {
    return 0;
  } else if (mItFirst < mItLast) {
    return (uint32_t)std::distance(mItFirst, mItLast);
  } else {
    decltype(mData)::const_iterator constItFirst{mItFirst};
    decltype(mData)::const_iterator constItLast{mItLast};
    return (uint32_t)(std::distance(constItFirst, mData.cend()) + std::distance(mData.cbegin(), constItLast));
  }
}

uint32_t RingBuffer::getFreeElements() const { return (capacity() - size()); }

bool RingBuffer::empty() const { return mItFirst == mItLast; }
