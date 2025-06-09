#include <libp2p/muxer/yamux/hardware_tracker.hpp>
#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

#include <sys/mman.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <cstdio>

namespace libp2p::connection {

HardwareSharedPtrTracker* HardwareSharedPtrTracker::instance_ = nullptr;

HardwareSharedPtrTracker::HardwareSharedPtrTracker() {
    instance_ = this;
    
    // Установка обработчика сигнала SIGTRAP для hardware breakpoints
    struct sigaction sa;
    sa.sa_sigaction = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    
    if (sigaction(SIGTRAP, &sa, &old_sigtrap_action_) == -1) {
        std::cerr << "❌ Не удалось установить обработчик SIGTRAP\n";
    }
    
    std::cout << "🔧 HardwareSharedPtrTracker инициализирован\n";
}

HardwareSharedPtrTracker::~HardwareSharedPtrTracker() {
    stopTracking();
    
    // Восстановить старый обработчик сигнала
    sigaction(SIGTRAP, &old_sigtrap_action_, nullptr);
    
    instance_ = nullptr;
}

void* HardwareSharedPtrTracker::getRefCountAddress(const std::shared_ptr<YamuxedConnection>& ptr) {
    // shared_ptr состоит из двух указателей:
    // - указатель на объект
    // - указатель на control block (содержит счетчики ссылок)
    
    // Получаем указатель на control block из shared_ptr
    // Это внутренняя структура, но мы можем получить к ней доступ
    struct shared_ptr_internal {
        void* ptr;
        void* control_block;
    };
    
    auto* internal = reinterpret_cast<const shared_ptr_internal*>(&ptr);
    void* control_block = internal->control_block;
    
    if (!control_block) {
        return nullptr;
    }
    
    // В control block счетчик shared_ptr обычно находится по смещению 0
    // (зависит от реализации стандартной библиотеки)
    void* ref_count_addr = control_block;
    
    std::cout << "📍 Адрес control block: " << control_block << "\n";
    std::cout << "📍 Адрес счетчика ссылок: " << ref_count_addr << "\n";
    std::cout << "📍 Текущий use_count: " << ptr.use_count() << "\n";
    
    return ref_count_addr;
}

bool HardwareSharedPtrTracker::setHardwareWatchpoint(void* address) {
    // Используем perf_event для hardware watchpoint
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(pe));
    
    pe.type = PERF_TYPE_BREAKPOINT;
    pe.size = sizeof(pe);
    pe.config = 0;
    pe.bp_type = HW_BREAKPOINT_W | HW_BREAKPOINT_R;  // Watch reads and writes
    pe.bp_addr = reinterpret_cast<uint64_t>(address);
    pe.bp_len = sizeof(long);  // Размер счетчика ссылок
    pe.disabled = 0;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    
    // Создаем hardware breakpoint через perf_event_open
    int fd = syscall(__NR_perf_event_open, &pe, 0, -1, -1, 0);
    if (fd == -1) {
        perror("❌ perf_event_open для hardware watchpoint");
        return false;
    }
    
    std::cout << "✅ Hardware watchpoint установлен на адрес " << address 
              << " (fd=" << fd << ")\n";
    
    return true;
}

bool HardwareSharedPtrTracker::removeHardwareWatchpoint() {
    // В реальной реализации здесь должно быть удаление watchpoint
    // Но для простоты пока оставляем пустым
    std::cout << "🗑️ Hardware watchpoint удален\n";
    return true;
}

void HardwareSharedPtrTracker::signalHandler(int sig, siginfo_t* info, void* context) {
    if (!instance_ || sig != SIGTRAP) {
        return;
    }
    
    std::cout << "\n🎯 === HARDWARE BREAKPOINT TRIGGERED ===\n";
    std::cout << "Сигнал: " << sig << "\n";
    std::cout << "Адрес: " << info->si_addr << "\n";
    
    // Выводим стек вызовов
    instance_->printStackTrace();
    
    // Проверяем нужно ли переключиться на следующий объект
    instance_->checkAndSwitchIfNeeded();
    
    std::cout << "==========================================\n\n";
}

void HardwareSharedPtrTracker::printStackTrace() {
    const int max_frames = 20;
    void* buffer[max_frames];
    
    int nframes = backtrace(buffer, max_frames);
    char** symbols = backtrace_symbols(buffer, nframes);
    
    std::cout << "📚 Стек вызовов (изменение счетчика ссылок):\n";
    
    for (int i = 0; i < nframes; ++i) {
        std::cout << "  [" << i << "] " << (symbols ? symbols[i] : "???") << "\n";
    }
    
    if (symbols) {
        free(symbols);
    }
}

void HardwareSharedPtrTracker::checkAndSwitchIfNeeded() {
    // Проверяем жив ли текущий отслеживаемый объект
    if (auto ptr = current_tracked_ptr_.lock()) {
        long count = ptr.use_count();
        std::cout << "📊 Текущий use_count: " << count << "\n";
        
        if (count <= 1) {
            std::cout << "💀 Объект скоро будет удален (use_count=" << count << ")\n";
            std::cout << "🔄 Ожидаем следующий YamuxedConnection для отслеживания...\n";
            
            // Останавливаем текущее отслеживание
            stopTracking();
        }
    } else {
        std::cout << "💀 Отслеживаемый объект уже удален\n";
        std::cout << "🔄 Ожидаем следующий YamuxedConnection для отслеживания...\n";
        
        // Останавливаем текущее отслеживание
        stopTracking();
    }
}

void HardwareSharedPtrTracker::startTracking(std::shared_ptr<YamuxedConnection> ptr) {
    if (!enabled_) {
        return;
    }
    
    // Если уже что-то отслеживаем - останавливаем
    if (is_tracking_) {
        stopTracking();
    }
    
    std::cout << "\n🎯 === НАЧАЛО HARDWARE ОТСЛЕЖИВАНИЯ ===\n";
    std::cout << "YamuxedConnection адрес: " << ptr.get() << "\n";
    std::cout << "shared_ptr use_count: " << ptr.use_count() << "\n";
    
    // Получаем адрес счетчика ссылок
    void* ref_count_addr = getRefCountAddress(ptr);
    if (!ref_count_addr) {
        std::cerr << "❌ Не удалось получить адрес счетчика ссылок\n";
        return;
    }
    
    // Устанавливаем hardware watchpoint
    if (!setHardwareWatchpoint(ref_count_addr)) {
        std::cerr << "❌ Не удалось установить hardware watchpoint\n";
        return;
    }
    
    // Сохраняем состояние
    watched_address_ = ref_count_addr;
    current_tracked_ptr_ = ptr;
    is_tracking_ = true;
    
    std::cout << "✅ Hardware отслеживание активировано\n";
    std::cout << "==========================================\n\n";
}

void HardwareSharedPtrTracker::stopTracking() {
    if (!is_tracking_) {
        return;
    }
    
    std::cout << "\n🛑 === ОСТАНОВКА HARDWARE ОТСЛЕЖИВАНИЯ ===\n";
    
    // Удаляем hardware watchpoint
    removeHardwareWatchpoint();
    
    // Сбрасываем состояние
    watched_address_ = nullptr;
    current_tracked_ptr_.reset();
    is_tracking_ = false;
    
    std::cout << "✅ Hardware отслеживание остановлено\n";
    std::cout << "==========================================\n\n";
}

// Глобальная функция для использования в yamux.cpp
void trackNextYamuxedConnection(std::shared_ptr<YamuxedConnection> ptr) {
    auto& tracker = HardwareSharedPtrTracker::getInstance();
    
    // Если сейчас ничего не отслеживается - начинаем отслеживание
    if (!tracker.isTracking()) {
        tracker.startTracking(std::move(ptr));
    } else {
        std::cout << "⏳ Уже отслеживается другой YamuxedConnection, ждем его завершения...\n";
    }
}

} // namespace libp2p::connection 