#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <execinfo.h>
#include <atomic>
#include <cstring>

// Простая заглушка для YamuxedConnection для тестирования hardware tracker'а
class YamuxedConnection {
public:
    YamuxedConnection(int id) : id_(id) {
        std::cout << "YamuxedConnection(" << id_ << ") создан по адресу " << this << std::endl;
    }
    
    ~YamuxedConnection() {
        std::cout << "YamuxedConnection(" << id_ << ") удален с адреса " << this << std::endl;
    }
    
    int getId() const { return id_; }
    
private:
    int id_;
};

class HardwareSharedPtrTracker {
public:
    static HardwareSharedPtrTracker& getInstance() {
        static HardwareSharedPtrTracker instance;
        return instance;
    }

    // Start tracking the reference count of a shared_ptr
    void startTracking(std::shared_ptr<YamuxedConnection> ptr);
    
    // Stop current tracking
    void stopTracking();
    
    // Check if tracking is active
    bool isTracking() const { return is_tracking_; }
    
    // Enable/disable tracking
    void enable() { enabled_ = true; }
    void disable() { enabled_ = false; }

private:
    HardwareSharedPtrTracker();
    ~HardwareSharedPtrTracker();
    
    // Get the address of the reference count in a shared_ptr
    void* getRefCountAddress(const std::shared_ptr<YamuxedConnection>& ptr);
    
    // Set hardware watchpoint
    bool setHardwareWatchpoint(void* address);
    
    // Remove hardware watchpoint  
    bool removeHardwareWatchpoint();
    
    // Signal handler for memory changes
    static void signalHandler(int sig, siginfo_t* info, void* context);
    
    // Print stack trace
    void printStackTrace();
    
    // Check counter and switch to the next object if needed
    void checkAndSwitchIfNeeded();

private:
    std::atomic<bool> enabled_{false};
    std::atomic<bool> is_tracking_{false};
    std::atomic<bool> should_stop_{false};  // Flag for signal handler
    
    void* watched_address_{nullptr};
    int watchpoint_fd_{-1};  // Store perf_event fd
    std::weak_ptr<YamuxedConnection> current_tracked_ptr_;
    
    // For debug registers
    static constexpr int DR7_L0 = 1;  // Local enable for DR0
    static constexpr int DR7_RW0_WRITE = (1 << 16); // Watch writes to DR0
    static constexpr int DR7_LEN0_4BYTES = (3 << 18); // 4-byte length for DR0
    
    static HardwareSharedPtrTracker* instance_;
    struct sigaction old_sigtrap_action_;
};

// Global function for setting in yamux.cpp
void trackNextYamuxedConnection(std::shared_ptr<YamuxedConnection> ptr);

// Macros for convenience
#define YAMUX_HARDWARE_TRACK_ENABLE() \
    libp2p::connection::HardwareSharedPtrTracker::getInstance().enable()

#define YAMUX_HARDWARE_TRACK_DISABLE() \
    libp2p::connection::HardwareSharedPtrTracker::getInstance().disable()

#define YAMUX_HARDWARE_TRACK_SHARED_PTR(ptr) \
    libp2p::connection::trackNextYamuxedConnection(ptr)


HardwareSharedPtrTracker* HardwareSharedPtrTracker::instance_ = nullptr;

HardwareSharedPtrTracker::HardwareSharedPtrTracker() {
    instance_ = this;
    
    struct sigaction sa;
    sa.sa_sigaction = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    
    if (sigaction(SIGTRAP, &sa, &old_sigtrap_action_) == -1) {
        std::cerr << "Failed to install SIGTRAP handler\n";
    }
    
    std::cout << "HardwareSharedPtrTracker initialized\n";
}

HardwareSharedPtrTracker::~HardwareSharedPtrTracker() {
    stopTracking();
    sigaction(SIGTRAP, &old_sigtrap_action_, nullptr);
    instance_ = nullptr;
}

void* HardwareSharedPtrTracker::getRefCountAddress(const std::shared_ptr<YamuxedConnection>& ptr) {
    struct shared_ptr_internal {
        void* ptr;
        void* control_block;
    };
    
    auto* internal = reinterpret_cast<const shared_ptr_internal*>(&ptr);
    void* control_block = internal->control_block;
    
    if (!control_block) {
        return nullptr;
    }
    
    void* ref_count_addr = control_block;
    
    std::cout << "Control block address: " << control_block << "\n";
    std::cout << "Reference count address: " << ref_count_addr << "\n";
    std::cout << "Current use_count: " << ptr.use_count() << "\n";
    
    return ref_count_addr;
}

bool HardwareSharedPtrTracker::setHardwareWatchpoint(void* address) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    
    pe.type = PERF_TYPE_BREAKPOINT;
    pe.size = sizeof(pe);
    pe.config = 0;
    pe.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R;
    pe.bp_addr = reinterpret_cast<uint64_t>(address);
    pe.bp_len = sizeof(long);
    pe.disabled = 0;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    
    int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        perror("perf_event_open for hardware watchpoint failed");
        return false;
    }
    
    watchpoint_fd_ = fd;
    
    std::cout << "Hardware watchpoint set on address " << address 
              << " (fd=" << fd << ")\n";
    
    return true;
}

bool HardwareSharedPtrTracker::removeHardwareWatchpoint() {
    if (watchpoint_fd_ != -1) {
        close(watchpoint_fd_);
        watchpoint_fd_ = -1;
        std::cout << "Hardware watchpoint removed\n";
        return true;
    }
    return false;
}

void HardwareSharedPtrTracker::signalHandler(int sig, siginfo_t* info, void* context) {
    if (!instance_ || sig != SIGTRAP) {
        return;
    }

    // printStackTrace();
    
    static int call_number = 0;
    call_number++;
    
    const char msg[] = "\n=== HARDWARE BREAKPOINT: REFERENCE COUNT CHANGED ===\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    
    char call_msg[100];
    int len = snprintf(call_msg, sizeof(call_msg), "Call #%d - Address: %p\n", call_number, info->si_addr);
    write(STDOUT_FILENO, call_msg, len);
    
    const int max_frames = 15;
    void* buffer[max_frames];
    
    const char stack_msg[] = "Stack trace (exact location of reference count change):\n";
    write(STDOUT_FILENO, stack_msg, sizeof(stack_msg) - 1);
    
    int nframes = backtrace(buffer, max_frames);
    
    backtrace_symbols_fd(buffer, nframes, STDOUT_FILENO);
    
    const char end_msg[] = "================================================\n\n";
    write(STDOUT_FILENO, end_msg, sizeof(end_msg) - 1);
    
    //instance_->signal_count_.fetch_add(1, std::memory_order_relaxed);
}

void HardwareSharedPtrTracker::printStackTrace() {
    const int max_frames = 7;
    void* buffer[max_frames];
    
    int nframes = backtrace(buffer, max_frames);
    char** symbols = backtrace_symbols(buffer, nframes);
    
    std::cout << "Stack trace (reference count change):\n";
    
    for (int i = 0; i < nframes; ++i) {
        std::cout << "  [" << i << "] " << (symbols ? symbols[i] : "???") << "\n";
    }
    
    if (symbols) {
        free(symbols);
    }
}

void HardwareSharedPtrTracker::checkAndSwitchIfNeeded() {
    if (should_stop_.exchange(false)) {
        std::cout << "\n=== HARDWARE BREAKPOINT TRIGGERED ===\n";
        printStackTrace();
        
        if (auto ptr = current_tracked_ptr_.lock()) {
            long count = ptr.use_count();
            std::cout << "Current use_count: " << count << "\n";
            
            if (count <= 1) {
                std::cout << "Object will be deleted soon (use_count=" << count << ")\n";
                std::cout << "Stopping hardware tracking\n";
                stopTracking();
            }
        } else {
            std::cout << "Tracked object already deleted\n";
            std::cout << "Stopping hardware tracking\n";
            stopTracking();
        }
        
        std::cout << "====================================\n\n";
    }
}

void HardwareSharedPtrTracker::startTracking(std::shared_ptr<YamuxedConnection> ptr) {
    if (!enabled_) {
        return;
    }
    
    if (!current_tracked_ptr_.expired()) {
        std::cout << "Already tracking another YamuxedConnection (address: active" 
                  << "), ignoring new request\n";
        return;
    }
    
    stopTracking();
    std::cout << "\n=== HARDWARE TRACKING STARTED ===\n";
    std::cout << "YamuxedConnection address: " << ptr.get() << "\n";
    std::cout << "shared_ptr use_count: " << ptr.use_count() << "\n";
    
    void* ref_count_addr = getRefCountAddress(ptr);
    if (!ref_count_addr) {
        std::cerr << "Failed to get reference count address\n";
        return;
    }
    
    if (!setHardwareWatchpoint(ref_count_addr)) {
        std::cerr << "Failed to set hardware watchpoint\n";
        return;
    }
    
    watched_address_ = ref_count_addr;
    current_tracked_ptr_ = ptr;
    is_tracking_ = true;
    should_stop_ = false;
    
    std::cout << "Hardware tracking activated\n";
    std::cout << "=================================\n\n";
}

void HardwareSharedPtrTracker::stopTracking() {
    if (!is_tracking_) {
        return;
    }
    
    std::cout << "\n=== HARDWARE TRACKING STOPPED ===\n";
    
    removeHardwareWatchpoint();
    
    watched_address_ = nullptr;
    current_tracked_ptr_.reset();
    is_tracking_ = false;
    should_stop_ = false;
    
    std::cout << "Hardware tracking stopped\n";
    std::cout << "=================================\n\n";
}

void trackNextYamuxedConnection(std::shared_ptr<YamuxedConnection> ptr) {
    auto& tracker = HardwareSharedPtrTracker::getInstance();
    tracker.startTracking(std::move(ptr));
}    


int main() {
    auto& tracker = HardwareSharedPtrTracker::getInstance();
    tracker.enable();
    
    std::shared_ptr<YamuxedConnection> ptr1 = std::make_shared<YamuxedConnection>(1);
    trackNextYamuxedConnection(ptr1);

    std::shared_ptr<YamuxedConnection> ptr2 = ptr1;
    ptr2.reset();
    ptr1.reset();

    std::shared_ptr<YamuxedConnection> ptr = std::make_shared<YamuxedConnection>(10);
    trackNextYamuxedConnection(ptr);
    ptr.reset();
    return 0;
} 