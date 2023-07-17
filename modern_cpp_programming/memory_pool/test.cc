#include <assert.h>
#include <stdint.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <random>
#include <unordered_set>

#include "memory_pool.hpp"

class Obj {
 public:
  void* operator new(size_t size);
  void operator delete(void* ptr) noexcept;
  Obj() : data_(0) {}
  Obj(size_t data) : data_(data) {}
  bool operator==(const Obj& other) const { return data_ == other.data_; }

  size_t data_;
};

struct ObjHash {
  std::size_t operator()(const Obj& obj) const { return std::hash<size_t>()(obj.data_); }
};

memory_pool<Obj> obj_pool;

void* Obj::operator new(size_t size) {
  assert(size == sizeof(Obj));
  return obj_pool.allocate();
}

void Obj::operator delete(void* ptr) noexcept { obj_pool.deallocate(static_cast<Obj*>(ptr)); }

int main(int argc, char** argv) {
  std::chrono::_V2::steady_clock::time_point start;
  size_t min = 0;
  size_t max = 100000;
  std::random_device seed;
  std::ranlux48 engine(seed());
  std::uniform_int_distribution<size_t> distrib(min, max);

  std::unordered_set<Obj, ObjHash> datas;
  const size_t count = 1000000000;
  start = std::chrono::steady_clock::now();
  for (size_t index = 0; index < count; ++index) {
    Obj obj(index);
    datas.insert(obj);
  }
  std::cout << "Obj insert cost : "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count() /
                   count
            << "ns" << std::endl;

  using TestType = std::unordered_set<int, std::hash<int>, std::equal_to<int>, pooled_allocator<int>>;
  TestType test_datas;
  start = std::chrono::steady_clock::now();
  for (size_t index = 0; index < count; ++index) {
    test_datas.insert(index);
  }
  std::cout << "test data insert cost : "
            << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count() /
                   count
            << "ns" << std::endl;
}