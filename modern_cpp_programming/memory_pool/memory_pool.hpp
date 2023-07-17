#include <array>

template <typename T>
inline constexpr size_t memory_chunk_size = 64;

template <typename T>
class memory_chunk {
 public:
  union node {
    T data{};
    node* next;
  };
  memory_chunk(memory_chunk* next_chunk);
  node* get_free_nodes() { return storage_.data(); }
  memory_chunk* get_next() const { return next_chunk_; }

 private:
  memory_chunk* next_chunk_{};
  std::array<node, memory_chunk_size<T>> storage_;
};

template <typename T>
memory_chunk<T>::memory_chunk(memory_chunk* next_chunk) : next_chunk_(next_chunk) {
  for (size_t i = 0; i < storage_.size() - 1; ++i) {
    storage_[i].next = &storage_[i + 1];
  }
  storage_[storage_.size() - 1].next = nullptr;
}

template <typename T>
class memory_pool {
 public:
  using node = typename memory_chunk<T>::node;
  memory_pool() = default;
  memory_pool(const memory_pool&) = delete;
  memory_pool& operator=(const memory_pool&) = delete;
  ~memory_pool();
  T* allocate();
  void deallocate(T* ptr);

 private:
  node* free_list_{};
  memory_chunk<T>* chunk_list_{};
};

template <typename T>
memory_pool<T>::~memory_pool() {
  while (chunk_list_) {
    memory_chunk<T>* chunk = chunk_list_;
    chunk_list_ = chunk_list_->get_next();
    delete chunk;
  }
}

template <typename T>
T* memory_pool<T>::allocate() {
  if (free_list_ == nullptr) {
    chunk_list_ = new memory_chunk<T>(chunk_list_);
    free_list_ = chunk_list_->get_free_nodes();
  }
  T* result = &free_list_->data;
  free_list_ = free_list_->next;
  return result;
}

template <typename T>
void memory_pool<T>::deallocate(T* ptr) {
  auto free_item = reinterpret_cast<node*>(ptr);
  free_item->next = free_list_;
  free_list_ = free_item;
}

template <typename T>
memory_pool<T>& get_memory_pool() {
  thread_local memory_pool<T> pool;
  return pool;
}

template <typename T, typename Base = std::allocator<T>>
struct pooled_allocator : private Base {
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using value_type = T;
  using is_always_equal = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;

  template <typename U>
  struct rebind {
    using other = pooled_allocator<U>;
  };

  pooled_allocator() = default;

  template <typename U>
  pooled_allocator(const pooled_allocator<U, typename Base::template rebind<U>::other>&) noexcept {
    get_memory_pool<T>();
  }

  T* allocate(size_t n) {
    if (n == 1) {
      return get_memory_pool<T>().allocate();
    } else {
      return Base::allocate(n);
    }
  }

  void deallocate(T* p, size_t n) {
    if (n == 1) {
      return get_memory_pool<T>().deallocate(p);
    } else {
      return Base::deallocate(p, n);
    }
  }
};