#pragma once

#include <memory>
#include <atomic>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <execinfo.h>
#include <iostream>
#include <cstring>

namespace libp2p::connection {

class YamuxedConnection;

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
    
    void* watched_address_{nullptr};
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

} // namespace libp2p::connection 