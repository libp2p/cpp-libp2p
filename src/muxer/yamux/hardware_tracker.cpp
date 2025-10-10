#include <libp2p/muxer/yamux/hardware_tracker.hpp>
#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <cstdio>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <execinfo.h>
#include <atomic>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>

namespace libp2p::connection {

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
    
    uint32_t* ref_count_addr = (uint32_t*)((uint8_t*)control_block + sizeof(void*));
    
    std::cout << "Control block address: " << control_block << "\n";
    std::cout << "Reference count address: " << ref_count_addr << "\n";
    std::cout << "Current use_count: " << ptr.use_count() << "\n";
    assert(*ref_count_addr == ptr.use_count());
    
    return ref_count_addr;
}

bool HardwareSharedPtrTracker::setHardwareWatchpoint(void* address) {
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    
    pe.type = PERF_TYPE_BREAKPOINT;
    pe.size = sizeof(pe);
    pe.config = 0;
    pe.bp_type = HW_BREAKPOINT_W;  // Только запись, чтение может генерировать много событий
    pe.bp_addr = reinterpret_cast<uint64_t>(address);
    pe.bp_len = HW_BREAKPOINT_LEN_4;  // 4 байта для int
    pe.disabled = 0;  // Включен сразу
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.exclude_user = 0;  // Отслеживаем user space
    pe.sample_period = 1;  // Генерировать событие на каждое изменение
    pe.wakeup_events = 1;  // Пробуждать на каждое событие
    
    std::cout << "🔧 Настройка hardware watchpoint:\n";
    std::cout << "   Адрес: " << address << "\n";
    std::cout << "   bp_addr: 0x" << std::hex << pe.bp_addr << std::dec << "\n";
    std::cout << "   bp_len: " << pe.bp_len << "\n";
    std::cout << "   bp_type: " << pe.bp_type << " (только запись)\n";
    
    int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        perror("❌ perf_event_open неудачен");
        std::cout << "   Ошибка: " << strerror(errno) << "\n";
        std::cout << "   Код ошибки: " << errno << "\n";
        return false;
    }
    
    // Настраиваем доставку сигнала
    struct f_owner_ex owner;
    owner.type = F_OWNER_TID;
    owner.pid = syscall(SYS_gettid);
    
    if (fcntl(fd, F_SETOWN_EX, &owner) == -1) {
        perror("⚠️ fcntl F_SETOWN_EX failed");
    }
    
    if (fcntl(fd, F_SETFL, O_ASYNC) == -1) {
        perror("⚠️ fcntl F_SETFL failed");
    }
    
    if (fcntl(fd, F_SETSIG, SIGTRAP) == -1) {
        perror("⚠️ fcntl F_SETSIG failed");
    }
    
    watchpoint_fd_ = fd;
    
    std::cout << "✅ Hardware watchpoint установлен на адрес " << address 
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
    
    static int call_number = 0;
    call_number++;
    
    const char msg[] = "\n🚨 === HARDWARE BREAKPOINT: REFERENCE COUNT CHANGED === 🚨\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    
    char call_msg[200];
    int len = snprintf(call_msg, sizeof(call_msg), "📍 Изменение #%d - Адрес: %p\n", call_number, info->si_addr);
    write(STDOUT_FILENO, call_msg, len);
    
    const char stack_msg[] = "📚 Стек трассировки (точное место изменения счетчика ссылок):\n";
    write(STDOUT_FILENO, stack_msg, sizeof(stack_msg) - 1);
    
    // Получаем стек трассировки
    const int max_frames = 7;
    void* buffer[max_frames];
    int nframes = backtrace(buffer, max_frames);
    
    // Используем backtrace_symbols для получения символов
    char** symbols = backtrace_symbols(buffer, nframes);
    
    if (symbols) {
        for (int i = 0; i < nframes; ++i) {
            char frame_msg[500];
            int frame_len = snprintf(frame_msg, sizeof(frame_msg), 
                                   "   [%2d] %s\n", i, symbols[i]);
            write(STDOUT_FILENO, frame_msg, frame_len);
        }
        free(symbols);
    } else {
        // Fallback - используем backtrace_symbols_fd если symbols == NULL
        backtrace_symbols_fd(buffer, nframes, STDOUT_FILENO);
    }
    
    const char end_msg[] = "🔚 ============================================== 🔚\n\n";
    write(STDOUT_FILENO, end_msg, sizeof(end_msg) - 1);
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

void HardwareSharedPtrTracker::startTracking(const std::shared_ptr<YamuxedConnection>& ptr) {
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

void trackNextYamuxedConnection(const std::shared_ptr<YamuxedConnection>& ptr) {
    auto& tracker = HardwareSharedPtrTracker::getInstance();
    tracker.startTracking(ptr);
}    

} // namespace libp2p::connection 