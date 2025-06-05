#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// Простая диагностика утечек для shared_ptr
class LeakDetector {
public:
    static LeakDetector& instance() {
        static LeakDetector detector;
        return detector;
    }
    
    void registerPointer(void* ptr, const char* type) {
        std::lock_guard<std::mutex> lock(mutex_);
        active_pointers_[ptr] = type;
        std::cout << "[LEAK_DEBUG] Created " << type << " at " << ptr << std::endl;
    }
    
    void unregisterPointer(void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = active_pointers_.find(ptr);
        if (it != active_pointers_.end()) {
            std::cout << "[LEAK_DEBUG] Destroyed " << it->second << " at " << ptr << std::endl;
            active_pointers_.erase(it);
        }
    }
    
    void printActivePointers() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_pointers_.empty()) {
            std::cout << "[LEAK_DEBUG] No active pointers detected!" << std::endl;
        } else {
            std::cout << "[LEAK_DEBUG] Active pointers (" << active_pointers_.size() << "):" << std::endl;
            for (const auto& [ptr, type] : active_pointers_) {
                std::cout << "  " << type << " at " << ptr << std::endl;
            }
        }
    }
    
private:
    std::mutex mutex_;
    std::unordered_map<void*, std::string> active_pointers_;
};

// Макросы для отладки утечек
#define REGISTER_LEAK_DEBUG(ptr, type) LeakDetector::instance().registerPointer(ptr, type)
#define UNREGISTER_LEAK_DEBUG(ptr) LeakDetector::instance().unregisterPointer(ptr)
#define PRINT_ACTIVE_LEAKS() LeakDetector::instance().printActivePointers()

// Пример использования для диагностики YamuxedConnection
class YamuxedConnectionDebug : public std::enable_shared_from_this<YamuxedConnectionDebug> {
public:
    YamuxedConnectionDebug() {
        REGISTER_LEAK_DEBUG(this, "YamuxedConnection");
    }
    
    ~YamuxedConnectionDebug() {
        UNREGISTER_LEAK_DEBUG(this);
    }
    
    void debugSharedPtrReferences() {
        auto self = shared_from_this();
        std::cout << "[DEBUG] shared_ptr use_count: " << self.use_count() << std::endl;
        
        // Симулируем различные места, где могут держаться ссылки:
        
        // 1. В streams_ (как было ранее)
        streams_["test"] = std::make_shared<int>(42);
        std::cout << "[DEBUG] After adding to streams_, use_count: " << self.use_count() << std::endl;
        
        // 2. В callback'ах
        auto callback = [self](){ /* do something */ };
        std::cout << "[DEBUG] After capturing in callback, use_count: " << self.use_count() << std::endl;
        
        // 3. В timer'ах
        auto timer_callback = [weak_self = std::weak_ptr<YamuxedConnectionDebug>(self)](){ 
            if (auto strong = weak_self.lock()) {
                /* do something */
            }
        };
        std::cout << "[DEBUG] After weak capture in timer, use_count: " << self.use_count() << std::endl;
        
        // Очистка
        streams_.clear();
        std::cout << "[DEBUG] After clearing streams_, use_count: " << self.use_count() << std::endl;
    }
    
private:
    std::unordered_map<std::string, std::shared_ptr<int>> streams_;
};

// Mock classes for simple testing
namespace libp2p {
namespace peer {
struct PeerId {
    std::string id;
    std::string toBase58() const { return id; }
};
}

namespace connection {
struct CapableConnection {
    virtual ~CapableConnection() = default;
    virtual bool isClosed() const = 0;
};
}

namespace muxer::yamux {
// Simple test for shared_ptr behavior in unordered_set
class YamuxedConnection {
public:
    void debugPrintMemoryLeakSources() {
        std::cout << "=== MEMORY LEAK DEBUG INFO ===" << std::endl;
        std::cout << "This is a test method" << std::endl;
    }
};
}
}

// Test shared_ptr behavior in unordered_set
void testSharedPtrInSet() {
    std::cout << "\n=== Testing shared_ptr behavior in unordered_set ===" << std::endl;
    
    auto obj = std::make_shared<int>(42);
    auto obj2 = obj;  // same object, different shared_ptr instance
    
    std::unordered_set<std::shared_ptr<int>> set;
    
    // Add first shared_ptr
    set.insert(obj);
    std::cout << "Added obj to set, size: " << set.size() << std::endl;
    
    // Try to find with different shared_ptr to same object
    auto found = set.find(obj2);
    if (found != set.end()) {
        std::cout << "SUCCESS: obj2 found in set (pointing to same object)" << std::endl;
    } else {
        std::cout << "FAIL: obj2 NOT found in set" << std::endl;
    }
    
    // Try to erase with different shared_ptr to same object
    auto erased = set.erase(obj2);
    std::cout << "Erased count using obj2: " << erased << std::endl;
    std::cout << "Set size after erase: " << set.size() << std::endl;
    
    // Test with shared_from_this equivalent
    struct TestClass : std::enable_shared_from_this<TestClass> {
        int value = 123;
    };
    
    auto test_obj = std::make_shared<TestClass>();
    auto self_ptr = test_obj->shared_from_this();
    
    std::unordered_set<std::shared_ptr<TestClass>> test_set;
    test_set.insert(test_obj);
    
    auto found2 = test_set.find(self_ptr);
    if (found2 != test_set.end()) {
        std::cout << "SUCCESS: shared_from_this() found in set" << std::endl;
    } else {
        std::cout << "FAIL: shared_from_this() NOT found in set" << std::endl;
    }
    
    auto erased2 = test_set.erase(self_ptr);
    std::cout << "Erased count using shared_from_this(): " << erased2 << std::endl;
    std::cout << "Test set size after erase: " << test_set.size() << std::endl;
}

int main() {
    std::cout << "=== Yamux Memory Leak Debug Tool ===" << std::endl;
    
    // Test shared_ptr behavior
    testSharedPtrInSet();
    
    // Test our debug method
    auto yamux_conn = std::make_shared<libp2p::muxer::yamux::YamuxedConnection>();
    yamux_conn->debugPrintMemoryLeakSources();
    
    std::cout << "\n=== Debug tool completed ===" << std::endl;
    return 0;
} 