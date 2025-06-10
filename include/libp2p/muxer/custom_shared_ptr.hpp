//
// Created by taisei on 09.06.2025.
//

#ifndef CUSTOM_SHARED_PTR_HPP
#define CUSTOM_SHARED_PTR_HPP

namespace libp2p::connection {

  template <typename T>
  class shared_ptr;
  template <typename T>
  class weak_ptr;
  template <typename T>
  class enable_shared_from_this;
  template <typename T, typename... Args>
  shared_ptr<T> make_shared(Args &&...args);

  //==============================================================================
  // enable_shared_from_this
  //
  // A simple wrapper around std::enable_shared_from_this that works with our
  // custom shared_ptr.
  //==============================================================================
  template <typename T>
  class enable_shared_from_this : public std::enable_shared_from_this<T> {
   protected:
    // Only derived classes can be constructed or destroyed.
    enable_shared_from_this() = default;
    enable_shared_from_this(const enable_shared_from_this &) = default;
    enable_shared_from_this &operator=(const enable_shared_from_this &) =
        default;
    ~enable_shared_from_this() = default;

   public:
    // Creates a new shared_ptr that shares ownership of this object.
    shared_ptr<T> shared_from_this();
    shared_ptr<const T> shared_from_this() const;

    // Creates a new weak_ptr that observes this object.
    weak_ptr<T> weak_from_this() noexcept;
    weak_ptr<const T> weak_from_this() const noexcept;
  };

  //==============================================================================
  // weak_ptr
  //
  // A wrapper around std::weak_ptr with the same interface as our custom
  // weak_ptr.
  //==============================================================================
  template <typename T>
  class weak_ptr {
   public:
    // Default constructor: creates an empty weak_ptr.
    weak_ptr() noexcept = default;

    // Construct from a shared_ptr.
    weak_ptr(const shared_ptr<T> &sp) noexcept;

    // Copy and move constructors/assignments use compiler defaults
    weak_ptr(const weak_ptr &) = default;
    weak_ptr(weak_ptr &&) noexcept = default;
    weak_ptr &operator=(const weak_ptr &) = default;
    weak_ptr &operator=(weak_ptr &&) noexcept = default;

    // Copy constructor for convertible types
    template <typename U,
              typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    weak_ptr(const weak_ptr<U> &other) noexcept;

    // Assign from a shared_ptr.
    weak_ptr &operator=(const shared_ptr<T> &sp) noexcept;

    // Standard weak_ptr operations
    long use_count() const noexcept;
    bool expired() const noexcept;
    shared_ptr<T> lock() const noexcept;
    void reset() noexcept;
    void swap(weak_ptr &other) noexcept;

   private:
    std::weak_ptr<T> wp_;

    template <typename U>
    friend class weak_ptr;

    template <typename U>
    friend class shared_ptr;

    // Add friendship for enable_shared_from_this to access wp_
    template <typename U>
    friend class enable_shared_from_this;
  };

  //==============================================================================
  // shared_ptr
  //
  // A wrapper around std::shared_ptr that maintains an 8KB buffer.
  //==============================================================================
  template <typename T>
  class shared_ptr {
   private:
    // Helper for enable_shared_from_this
    template <typename U>
    void enable_weak_this(const U *ptr) {
      if (ptr != nullptr && sp_) {
        if (auto esft =
                dynamic_cast<const std::enable_shared_from_this<U> *>(ptr)) {
          // Get a shared_ptr of the correct type before converting to weak_ptr
          auto shared_ptr_u = std::static_pointer_cast<U>(sp_);
          const_cast<std::enable_shared_from_this<U> *>(esft)
              ->weak_from_this() = shared_ptr_u;
        }
      }
    }

   public:
    // Default constructor: creates an empty shared_ptr.
    shared_ptr() noexcept : sp_(), memory_block_(nullptr) {}

    // Constructor for nullptr.
    shared_ptr(std::nullptr_t) noexcept : sp_(), memory_block_(nullptr) {}

    // Constructor from raw pointer
    explicit shared_ptr(T *p) : sp_(p), memory_block_(nullptr) {
      allocate_special_buffer();
    }

    // Constructor from a raw pointer of derived type
    template <typename U,
              typename = std::enable_if_t<std::is_convertible_v<U *, T *>>>
    explicit shared_ptr(U *p) : sp_(p), memory_block_(nullptr) {
      allocate_special_buffer();
      enable_weak_this<U>(p);
    }

    // Copy constructor
    shared_ptr(const shared_ptr &other) noexcept
        : sp_(other.sp_), memory_block_(nullptr) {
      allocate_special_buffer();  // Each shared_ptr object gets its own 8KB
                                  // block
    }

    // Move constructor
    shared_ptr(shared_ptr &&other) noexcept
        : sp_(std::move(other.sp_)), memory_block_(other.memory_block_) {
      other.memory_block_ =
          nullptr;  // Ownership of the 8KB block is transferred
    }

    // Constructor from weak_ptr
    explicit shared_ptr(const weak_ptr<T> &wp) : memory_block_(nullptr) {
      sp_ = wp.wp_.lock();
      if (sp_) {
        allocate_special_buffer();
      }
    }

    // Destructor
    ~shared_ptr() {
      cleanup();
    }

    // Copy assignment
    shared_ptr &operator=(const shared_ptr &other) noexcept {
      if (this != &other) {
        cleanup();
        sp_ = other.sp_;
        allocate_special_buffer();
      }
      return *this;
    }

    // Move assignment
    shared_ptr &operator=(shared_ptr &&other) noexcept {
      if (this != &other) {
        cleanup();
        sp_ = std::move(other.sp_);
        memory_block_ = other.memory_block_;
        other.memory_block_ = nullptr;
      }
      return *this;
    }

    // Reset methods
    void reset() noexcept {
      cleanup();
      sp_.reset();
    }

    template <typename U>
    void reset(U *p) {
      cleanup();
      sp_.reset(p);
      allocate_special_buffer();
    }

    // Swap
    void swap(shared_ptr &other) noexcept {
      sp_.swap(other.sp_);
      std::swap(memory_block_, other.memory_block_);
    }

    // Accessors and operators
    T *get() const noexcept {
      return sp_.get();
    }
    T &operator*() const noexcept {
      return *sp_;
    }
    T *operator->() const noexcept {
      return sp_.get();
    }
    long use_count() const noexcept {
      return sp_.use_count();
    }
    explicit operator bool() const noexcept {
      return bool(sp_);
    }

    // Equality comparison operators
    template <typename U>
    bool operator==(const shared_ptr<U> &other) const noexcept {
      return sp_ == other.sp_;
    }

    template <typename U>
    bool operator!=(const shared_ptr<U> &other) const noexcept {
      return sp_ != other.sp_;
    }

    // Comparison with nullptr
    bool operator==(std::nullptr_t) const noexcept {
      return sp_ == nullptr;
    }
    bool operator!=(std::nullptr_t) const noexcept {
      return sp_ != nullptr;
    }

   private:
    std::shared_ptr<T> sp_;  // The actual shared_ptr from the standard library
    char *memory_block_;     // The required 8KB special memory block

    // Allocates the 8KB buffer associated with this shared_ptr object
    void allocate_special_buffer() {
      try {
        memory_block_ = new char[8192];  // 8 KB
      } catch (...) {
        memory_block_ = nullptr;
        throw;
      }
    }

    // Deallocates the 8KB buffer
    void deallocate_special_buffer() {
      delete[] memory_block_;
      memory_block_ = nullptr;
    }

    // Central cleanup function called by destructor and assignments
    void cleanup() {
      if (memory_block_) {
        deallocate_special_buffer();
      }
    }

    friend class weak_ptr<T>;

    template <typename U, typename... Args>
    friend shared_ptr<U> make_shared(Args &&...args);

    // Add friendship for enable_shared_from_this to access sp_
    template <typename U>
    friend class enable_shared_from_this;
  };

  //==============================================================================
  // Implementation of make_shared
  //==============================================================================
  template <typename T, typename... Args>
  shared_ptr<T> make_shared(Args &&...args) {
    shared_ptr<T> result;
    result.sp_ = std::make_shared<T>(std::forward<Args>(args)...);
    result.allocate_special_buffer();
    result.enable_weak_this(result.get());
    return result;
  }

  //==============================================================================
  // Implementation of enable_shared_from_this methods
  //==============================================================================
  template <typename T>
  shared_ptr<T> enable_shared_from_this<T>::shared_from_this() {
    shared_ptr<T> result;
    result.sp_ = std::enable_shared_from_this<T>::shared_from_this();
    result.allocate_special_buffer();
    return result;
  }

  template <typename T>
  shared_ptr<const T> enable_shared_from_this<T>::shared_from_this() const {
    shared_ptr<const T> result;
    result.sp_ = std::enable_shared_from_this<T>::shared_from_this();
    result.allocate_special_buffer();
    return result;
  }

  template <typename T>
  weak_ptr<T> enable_shared_from_this<T>::weak_from_this() noexcept {
    weak_ptr<T> result;
    result.wp_ = std::enable_shared_from_this<T>::weak_from_this();
    return result;
  }

  template <typename T>
  weak_ptr<const T> enable_shared_from_this<T>::weak_from_this()
      const noexcept {
    weak_ptr<const T> result;
    result.wp_ = std::enable_shared_from_this<T>::weak_from_this();
    return result;
  }

  //==============================================================================
  // Implementation of weak_ptr methods
  //==============================================================================
  template <typename T>
  weak_ptr<T>::weak_ptr(const shared_ptr<T> &sp) noexcept : wp_(sp.sp_) {}

  template <typename T>
  template <typename U, typename>
  weak_ptr<T>::weak_ptr(const weak_ptr<U> &other) noexcept : wp_(other.wp_) {}

  template <typename T>
  weak_ptr<T> &weak_ptr<T>::operator=(const shared_ptr<T> &sp) noexcept {
    wp_ = sp.sp_;
    return *this;
  }

  template <typename T>
  long weak_ptr<T>::use_count() const noexcept {
    return wp_.use_count();
  }

  template <typename T>
  bool weak_ptr<T>::expired() const noexcept {
    return wp_.expired();
  }

  template <typename T>
  shared_ptr<T> weak_ptr<T>::lock() const noexcept {
    shared_ptr<T> result;
    result.sp_ = wp_.lock();
    if (result.sp_) {
      result.allocate_special_buffer();
    }
    return result;
  }

  template <typename T>
  void weak_ptr<T>::reset() noexcept {
    wp_.reset();
  }

  template <typename T>
  void weak_ptr<T>::swap(weak_ptr &other) noexcept {
    wp_.swap(other.wp_);
  }

}  // namespace libp2p::connection

// Specializations in the std namespace for standard library compatibility
namespace std {
  // Specialization of std::hash for libp2p::connection::shared_ptr<T>
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
