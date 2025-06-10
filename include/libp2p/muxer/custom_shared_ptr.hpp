//
// Created by taisei on 09.06.2025.
//

#ifndef CUSTOM_SHARED_PTR_HPP
#define CUSTOM_SHARED_PTR_HPP

namespace libp2p::connection {

  // Forward declarations are necessary because shared_ptr, weak_ptr,
  // and enable_shared_from_this have circular dependencies.
  template <typename T>
  class shared_ptr;
  template <typename T>
  class weak_ptr;
  template <typename T>
  class enable_shared_from_this;
  template <typename T, typename... Args>
  shared_ptr<T> make_shared(Args &&...args);

  namespace detail {
    // The ControlBlock is the core of the reference counting mechanism.
    // It is a non-templated base class to allow for type-erased deletion of the
    // managed object.
    class ControlBlock {
     public:
      // 'shared_count' tracks how many shared_ptrs own the managed object.
      // The object is destroyed when this count reaches zero.
      std::atomic<long> shared_count;

      // 'weak_count' tracks how many weak_ptrs (and shared_ptrs) observe the
      // object. The ControlBlock itself is destroyed when this count reaches
      // zero.
      std::atomic<long> weak_count;

      // Pure virtual function for disposing of the managed object.
      // This allows the non-templated base to call the correct derived-class
      // deleter.
      virtual void dispose_object() = 0;

      // Virtual destructor to ensure proper cleanup of derived
      // ControlBlockImpl.
      virtual ~ControlBlock() = default;

      // Initial counts are 1 because a ControlBlock is only created
      // when the first shared_ptr is created.
      ControlBlock() : shared_count(1), weak_count(1) {}

      void increment_shared() {
        shared_count.fetch_add(1, std::memory_order_relaxed);
      }

      void decrement_shared() {
        // If this was the last shared_ptr...
        if (shared_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
          dispose_object();  // ...destroy the managed object...
          decrement_weak();  // ...and also decrement the weak count.
        }
      }

      void increment_weak() {
        weak_count.fetch_add(1, std::memory_order_relaxed);
      }

      void decrement_weak() {
        // If this was the last pointer of any kind (shared or weak)...
        if (weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
          delete this;  // ...destroy the control block itself.
        }
      }
    };

    // This templated implementation of the ControlBlock is used when shared_ptr
    // is constructed from a raw pointer (new T). It manages a pointer to the
    // object.
    template <typename T>
    class ControlBlockImpl : public ControlBlock {
     public:
      T *ptr;  // Raw pointer to the managed object

      explicit ControlBlockImpl(T *p) : ptr(p) {}

      // The override that actually deletes the object.
      void dispose_object() override {
        delete ptr;
        ptr = nullptr;
      }
    };

    // This ControlBlock is used by make_shared. It allocates space for the
    // object and the control block in a single memory chunk for efficiency.
    template <typename T>
    class ControlBlockMakeShared : public ControlBlock {
     public:
      // Constructor forwards arguments to T's constructor using placement new.
      template <typename... Args>
      explicit ControlBlockMakeShared(Args &&...args) {
        new (&storage_) T(std::forward<Args>(args)...);
      }

      // The override that calls the object's destructor directly.
      // Memory is freed when the control block itself is deleted.
      void dispose_object() override {
        get_ptr()->~T();
      }

      // Returns a pointer to the in-place constructed object.
      T *get_ptr() noexcept {
        return reinterpret_cast<T *>(&storage_);
      }

     private:
      // Aligned storage for the T object itself.
      std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
    };

  }  // namespace detail

  //==============================================================================
  // enable_shared_from_this
  //
  // Inheriting from this class allows an object that is currently managed by a
  // shared_ptr to safely create new shared_ptr instances that share ownership.
  //==============================================================================
  template <typename T>
  class enable_shared_from_this {
   protected:
    // Only derived classes can be constructed or destroyed.
    enable_shared_from_this() = default;
    enable_shared_from_this(const enable_shared_from_this &) = default;
    enable_shared_from_this &operator=(const enable_shared_from_this &) =
        default;
    ~enable_shared_from_this() = default;

   public:
    // Creates a new shared_ptr that shares ownership of this object.
    shared_ptr<T> shared_from_this() {
      // Attempt to lock the internal weak pointer.
      // If it fails, it means the object is not managed by a shared_ptr.
      return shared_ptr<T>(_internal_weak_this);
    }

    shared_ptr<const T> shared_from_this() const {
      return shared_ptr<const T>(_internal_weak_this);
    }

    // Creates a new weak_ptr that observes this object.
    weak_ptr<T> weak_from_this() noexcept {
      return _internal_weak_this;
    }

    weak_ptr<const T> weak_from_this() const noexcept {
      return _internal_weak_this;
    }

   private:
    // This weak_ptr holds a non-owning reference back to the object.
    // It is initialized by the shared_ptr's constructor.
    // 'mutable' allows it to be modified in const contexts, which is
    // a common pattern for this mechanism.
    mutable weak_ptr<T> _internal_weak_this;

    // The shared_ptr constructor needs to be a friend to set the weak_ptr.
    template <typename U>
    friend class shared_ptr;
  };

  //==============================================================================
  // weak_ptr
  //
  // A non-owning "observer" of an object managed by shared_ptr. It can be used
  // to break circular references.
  //==============================================================================
  template <typename T>
  class weak_ptr {
   public:
    // should be private
    weak_ptr(T *p) noexcept
        : cb_(p ? new detail::ControlBlockImpl<T>(p) : nullptr) {
      if (cb_) {
        cb_->increment_weak();
      }
    }
    // Default constructor: creates an empty weak_ptr.
    weak_ptr() noexcept : cb_(nullptr) {}

    // Construct from a shared_ptr.
    weak_ptr(const shared_ptr<T> &sp) noexcept : cb_(sp.cb_) {
      if (cb_) {
        cb_->increment_weak();
      }
    }

    // Copy constructor.
    weak_ptr(const weak_ptr &other) noexcept : cb_(other.cb_) {
      if (cb_) {
        cb_->increment_weak();
      }
    }

    // Templated copy constructor for conversions (e.g., from non-const to
    // const).
    template <typename U,
              typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    weak_ptr(const weak_ptr<U> &other) noexcept : cb_(other.cb_) {
      if (cb_) {
        cb_->increment_weak();
      }
    }

    // Move constructor.
    weak_ptr(weak_ptr &&other) noexcept : cb_(other.cb_) {
      other.cb_ =
          nullptr;  // Leave the moved-from object in a valid empty state.
    }

    // Destructor.
    ~weak_ptr() {
      if (cb_) {
        cb_->decrement_weak();
      }
    }

    // Copy assignment.
    weak_ptr &operator=(const weak_ptr &other) noexcept {
      if (this != &other) {
        if (cb_) {
          cb_->decrement_weak();
        }
        cb_ = other.cb_;
        if (cb_) {
          cb_->increment_weak();
        }
      }
      return *this;
    }

    // Move assignment.
    weak_ptr &operator=(weak_ptr &&other) noexcept {
      if (this != &other) {
        if (cb_) {
          cb_->decrement_weak();
        }
        cb_ = other.cb_;
        other.cb_ = nullptr;
      }
      return *this;
    }

    // Assign from a shared_ptr.
    weak_ptr &operator=(const shared_ptr<T> &sp) noexcept {
      if (cb_) {
        cb_->decrement_weak();
      }
      cb_ = sp.cb_;
      if (cb_) {
        cb_->increment_weak();
      }
      return *this;
    }

    // Returns the number of shared_ptrs that own the object.
    long use_count() const noexcept {
      return cb_ ? cb_->shared_count.load() : 0;
    }

    // Checks if the managed object has been deleted.
    bool expired() const noexcept {
      return use_count() == 0;
    }

    // Attempts to create a shared_ptr from this weak_ptr.
    // Returns an empty shared_ptr if the object is expired.
    shared_ptr<T> lock() const noexcept {
      if (expired()) {
        return shared_ptr<T>();
      }
      // Promotion: creates a new shared_ptr that shares ownership.
      return shared_ptr<T>(*this);
    }

    // Resets the weak_ptr to be empty.
    void reset() noexcept {
      if (cb_) {
        cb_->decrement_weak();
      }
      cb_ = nullptr;
    }

    void swap(weak_ptr &other) noexcept {
      std::swap(cb_, other.cb_);
    }

   private:
    detail::ControlBlock *cb_;  // Pointer to the shared control block.

    // Allow access between different template instantiations.
    template <typename U>
    friend class weak_ptr;
    // Allow shared_ptr to construct from a weak_ptr (for lock and
    // enable_shared_from_this).
    friend class shared_ptr<T>;
  };

  //==============================================================================
  // shared_ptr
  //
  // An owning smart pointer that manages the lifetime of an object through
  // reference counting.
  //==============================================================================
  template <typename T>
  class shared_ptr {
   public:
    // Default constructor: creates an empty shared_ptr.
    shared_ptr() noexcept
        : ptr_(nullptr), cb_(nullptr), memory_block_(nullptr) {}

    // Constructor for nullptr.
    shared_ptr(std::nullptr_t) noexcept
        : ptr_(nullptr), cb_(nullptr), memory_block_(nullptr) {}

    // Main constructor from a raw pointer. Takes ownership of the pointer.
    explicit shared_ptr(T *p) : ptr_(p), cb_(nullptr), memory_block_(nullptr) {
      // Allocate the special 8KB block for this shared_ptr object.
      allocate_special_buffer();

      if (ptr_) {
        try {
          // Create the control block that will manage the object's lifetime.
          cb_ = new detail::ControlBlockImpl<T>(p);

          // If the managed object inherits from enable_shared_from_this,
          // we must initialize its internal weak_ptr.
          init_enable_shared(p);
        } catch (...) {
          // If control block allocation fails, clean up.
          delete p;
          deallocate_special_buffer();
          throw;
        }
      }
    }

    // Constructor from a raw pointer of derived type. Takes ownership of the
    // pointer.
    template <typename U,
              typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    explicit shared_ptr(U *p) : ptr_(p), cb_(nullptr), memory_block_(nullptr) {
      // Allocate the special 8KB block for this shared_ptr object.
      allocate_special_buffer();

      if (ptr_) {
        try {
          // Create the control block that will manage the object's lifetime.
          cb_ = new detail::ControlBlockImpl<U>(p);

          // If the managed object inherits from enable_shared_from_this,
          // we must initialize its internal weak_ptr.
          init_enable_shared(p);
        } catch (...) {
          // If control block allocation fails, clean up.
          delete p;
          deallocate_special_buffer();
          throw;
        }
      }
    }

    // Copy constructor: shares ownership with 'other'.
    shared_ptr(const shared_ptr &other) noexcept
        : ptr_(other.ptr_), cb_(other.cb_), memory_block_(nullptr) {
      allocate_special_buffer();  // Each shared_ptr object gets its own 8KB
                                  // block.
      if (cb_) {
        cb_->increment_shared();
      }
    }

    // Move constructor: transfers ownership from 'other'.
    shared_ptr(shared_ptr &&other) noexcept
        : ptr_(other.ptr_), cb_(other.cb_), memory_block_(other.memory_block_) {
      // Leave the moved-from object in a valid, empty state.
      other.ptr_ = nullptr;
      other.cb_ = nullptr;
      other.memory_block_ =
          nullptr;  // Ownership of the 8KB block is transferred.
    }

    // Constructor from weak_ptr (used by weak_ptr::lock and
    // enable_shared_from_this).
    explicit shared_ptr(const weak_ptr<T> &wp) : memory_block_(nullptr) {
      cb_ = wp.cb_;
      allocate_special_buffer();
      if (cb_ && !wp.expired()) {
        cb_->increment_shared();
        // This is a simplification. A full implementation would need to handle
        // different ControlBlock types (e.g., from make_shared vs. new).
        // For now, we assume ControlBlockImpl for simplicity when locking.
        // A more robust solution involves a virtual get_ptr() in ControlBlock.
        ptr_ = static_cast<detail::ControlBlockImpl<T> *>(cb_)->ptr;
      } else {
        // If lock() fails, this results in an empty shared_ptr.
        ptr_ = nullptr;
        cb_ = nullptr;
      }
    }

    // Destructor: decrements the reference count.
    ~shared_ptr() {
      cleanup();
    }

    // Copy assignment using the copy-and-swap idiom for exception safety.
    shared_ptr &operator=(const shared_ptr &other) noexcept {
      shared_ptr(other).swap(*this);
      return *this;
    }

    // Move assignment.
    shared_ptr &operator=(shared_ptr &&other) noexcept {
      if (this != &other) {
        cleanup();  // Clean up current state before moving.

        ptr_ = other.ptr_;
        cb_ = other.cb_;
        memory_block_ = other.memory_block_;

        other.ptr_ = nullptr;
        other.cb_ = nullptr;
        other.memory_block_ = nullptr;
      }
      return *this;
    }

    // Resets the shared_ptr to be empty.
    void reset() noexcept {
      cleanup();
      ptr_ = nullptr;
      cb_ = nullptr;
      // The 8KB block is deallocated by cleanup(). A new one will be allocated
      // if this shared_ptr object is later assigned a new pointer.
    }

    // Resets the shared_ptr to manage a new pointer.
    template <typename U>
    void reset(U *p) {
      shared_ptr<T>(p).swap(*this);
    }

    // Swaps the contents of this shared_ptr with another.
    void swap(shared_ptr &other) noexcept {
      std::swap(ptr_, other.ptr_);
      std::swap(cb_, other.cb_);
      std::swap(memory_block_, other.memory_block_);
    }

    // Accessors and operators
    T *get() const noexcept {
      return ptr_;
    }
    T &operator*() const noexcept {
      return *ptr_;
    }
    T *operator->() const noexcept {
      return ptr_;
    }
    long use_count() const noexcept {
      return cb_ ? cb_->shared_count.load() : 0;
    }
    explicit operator bool() const noexcept {
      return ptr_ != nullptr;
    }

    // Equality comparison operators (required for containers like
    // unordered_set)
    template <typename U>
    bool operator==(const shared_ptr<U> &other) const noexcept {
      return ptr_ == other.get();
    }

    template <typename U>
    bool operator!=(const shared_ptr<U> &other) const noexcept {
      return ptr_ != other.get();
    }

    // Comparison with nullptr
    bool operator==(std::nullptr_t) const noexcept {
      return ptr_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const noexcept {
      return ptr_ != nullptr;
    }

   private:
    // Private constructor for make_shared
    shared_ptr(detail::ControlBlockMakeShared<T> *cb_ms)
        : ptr_(cb_ms->get_ptr()), cb_(cb_ms), memory_block_(nullptr) {
      allocate_special_buffer();
      init_enable_shared(ptr_);
    }

    T *ptr_;                    // Raw pointer to the managed object.
    detail::ControlBlock *cb_;  // Pointer to the shared control block.
    char *memory_block_;        // The required 8KB special memory block.

    // Allocates the 8KB buffer associated with this shared_ptr object.
    void allocate_special_buffer() {
      try {
        memory_block_ = new char[8192];  // 8 KB
      } catch (...) {
        // If this allocation fails, we can't create the shared_ptr object.
        memory_block_ = nullptr;
        throw;
      }
    }

    // Deallocates the 8KB buffer.
    void deallocate_special_buffer() {
      delete[] memory_block_;
      memory_block_ = nullptr;
    }

    // Central cleanup function called by destructor and assignments.
    void cleanup() {
      if (cb_ != NULL) {
        cb_->decrement_shared();
      }
      if (memory_block_) {
        deallocate_special_buffer();
      }
    }

    // Helper to initialize enable_shared_from_this, using SFINAE.
    // This version is called if T derives from enable_shared_from_this<T>.
    template <typename U>
    void init_enable_shared(
        U *p,
        typename std::enable_if<
            std::is_convertible<U *, enable_shared_from_this<U> *>::value>::type
            * = 0) {
      p->_internal_weak_this = weak_ptr<U>(static_cast<U *>(ptr_));
    }

    // This overload is chosen if T does NOT derive from
    // enable_shared_from_this. It does nothing.
    void init_enable_shared(...) {}

    // Give weak_ptr access to private members.
    friend class weak_ptr<T>;
    // Give make_shared access to the private constructor.
    template <typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args &&...args);
  };

  //==============================================================================
  // make_shared
  //
  // Factory function for creating shared_ptrs in a more efficient and
  // exception-safe manner.
  //==============================================================================
  template <typename T, typename... Args>
  shared_ptr<T> make_shared(Args &&...args) {
    auto *cb_ms =
        new detail::ControlBlockMakeShared<T>(std::forward<Args>(args)...);
    return shared_ptr<T>(cb_ms);
  }

}  // namespace libp2p::connection

// Specializations in the std namespace for standard library compatibility
namespace std {
  // Specialization of std::hash for libp2p::connection::shared_ptr<T>
  // This enables the usage of shared_ptr in std::unordered_set
  template <typename T>
  struct hash<libp2p::connection::shared_ptr<T>> {
    std::size_t operator()(
        const libp2p::connection::shared_ptr<T> &sp) const noexcept {
      // Hash the pointer value itself
      return std::hash<T *>{}(sp.get());
    }
  };
}  // namespace std

#endif  // CUSTOM_SHARED_PTR_HPP
