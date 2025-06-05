#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <queue>
#include <mutex>

// Реалистичный тест утечек памяти в YamuxedConnection

namespace libp2p {

namespace peer {
struct PeerId {
    std::string id;
    PeerId(const std::string& id_) : id(id_) {}
    std::string toBase58() const { return id; }
    bool operator==(const PeerId& other) const { return id == other.id; }
};
}
}

namespace std {
template <>
struct hash<libp2p::peer::PeerId> {
    size_t operator()(const libp2p::peer::PeerId& peer) const {
        return hash<std::string>()(peer.id);
    }
};
}

namespace libp2p {

namespace basic {

// Реалистичная эмуляция Scheduler с накоплением колбэков
class RealisticScheduler : public std::enable_shared_from_this<RealisticScheduler> {
public:
    struct Handle {
        int id = -1;
        std::weak_ptr<RealisticScheduler> scheduler;
        
        void reset() {
            if (auto sched = scheduler.lock()) {
                sched->cancelHandle(id);
            }
            id = -1;
        }
        
        bool has_value() const { return id >= 0; }
        
        Handle() = default;
        Handle(int id_, std::weak_ptr<RealisticScheduler> sched) 
            : id(id_), scheduler(sched) {}
    };
    
    Handle scheduleWithHandle(std::function<void()> cb, std::chrono::milliseconds delay) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        int handle_id = next_id_++;
        
        // Эмулируем хранение колбэков в Scheduler (это источник утечки!)
        scheduled_callbacks_[handle_id] = {
            .callback = std::move(cb),
            .execute_time = std::chrono::steady_clock::now() + delay,
            .cancelled = false
        };
        
        total_scheduled_++;
        std::cout << "[Scheduler] Scheduled callback " << handle_id 
                  << ", total active: " << scheduled_callbacks_.size() << std::endl;
        
        return Handle(handle_id, weak_from_this());
    }
    
    void cancelHandle(int handle_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = scheduled_callbacks_.find(handle_id);
        if (it != scheduled_callbacks_.end()) {
            it->second.cancelled = true;
            // В реальном Scheduler отмененные колбэки могут не удаляться сразу!
            // scheduled_callbacks_.erase(it);  // Закомментировано для эмуляции проблемы
            total_cancelled_++;
            std::cout << "[Scheduler] Cancelled callback " << handle_id 
                      << ", total active: " << scheduled_callbacks_.size() << std::endl;
        }
    }
    
    // Эмулируем выполнение колбэков
    void processCallbacks() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        std::vector<int> to_execute;
        
        for (auto& [id, info] : scheduled_callbacks_) {
            if (!info.cancelled && info.execute_time <= now) {
                to_execute.push_back(id);
            }
        }
        
        for (int id : to_execute) {
            auto it = scheduled_callbacks_.find(id);
            if (it != scheduled_callbacks_.end() && !it->second.cancelled) {
                auto callback = std::move(it->second.callback);
                scheduled_callbacks_.erase(it);
                
                // Выполняем callback без блокировки
                mutex_.unlock();
                std::cout << "[Scheduler] Executing callback " << id << std::endl;
                callback();
                mutex_.lock();
                
                total_executed_++;
            }
        }
    }
    
    void printStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[Scheduler] Stats:" << std::endl;
        std::cout << "  Total scheduled: " << total_scheduled_ << std::endl;
        std::cout << "  Total executed: " << total_executed_ << std::endl;
        std::cout << "  Total cancelled: " << total_cancelled_ << std::endl;
        std::cout << "  Currently active: " << scheduled_callbacks_.size() << std::endl;
        
        // Проблема: активные колбэки могут держать weak_ptr, которые при выполнении
        // становятся shared_ptr и продлевают жизнь объектов!
        if (!scheduled_callbacks_.empty()) {
            std::cout << "  WARNING: Non-zero active callbacks - potential memory leak!" << std::endl;
        }
    }
    
    size_t getActiveCallbacksCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return scheduled_callbacks_.size();
    }

private:
    mutable std::mutex mutex_;
    
    struct CallbackInfo {
        std::function<void()> callback;
        std::chrono::steady_clock::time_point execute_time;
        bool cancelled;
    };
    
    std::unordered_map<int, CallbackInfo> scheduled_callbacks_;
    int next_id_ = 1;
    int total_scheduled_ = 0;
    int total_executed_ = 0;
    int total_cancelled_ = 0;
};

using Scheduler = RealisticScheduler;

} // namespace basic

namespace connection {

struct SecureConnection {
    peer::PeerId remote_peer;
    bool closed = false;
    
    SecureConnection(const std::string& peer_id) : remote_peer(peer_id) {}
    
    peer::PeerId remotePeer() const { return remote_peer; }
    bool isClosed() const { return closed; }
    void close() { closed = true; }
};

// Реалистичная версия YamuxedConnection с таймерами
class YamuxedConnection : public std::enable_shared_from_this<YamuxedConnection> {
public:
    using ConnectionClosedCallback = std::function<void(const peer::PeerId&, std::shared_ptr<YamuxedConnection>)>;
    
    explicit YamuxedConnection(
        std::shared_ptr<SecureConnection> connection,
        std::shared_ptr<basic::Scheduler> scheduler,
        ConnectionClosedCallback closed_callback)
        : connection_(std::move(connection))
        , scheduler_(std::move(scheduler))
        , closed_callback_(std::move(closed_callback))
        , remote_peer_(connection_->remotePeer()) {
        
        std::cout << "[YamuxedConnection] Created connection to " 
                  << remote_peer_.toBase58() 
                  << " at address " << (void*)this << std::endl;
    }
    
    ~YamuxedConnection() {
        std::cout << "[YamuxedConnection] *** DESTRUCTOR *** for " 
                  << remote_peer_.toBase58() 
                  << " at address " << (void*)this << std::endl;
    }
    
    void start() {
        started_ = true;
        std::cout << "[YamuxedConnection] Started connection to " 
                  << remote_peer_.toBase58() << std::endl;
        
        // Запускаем таймеры (источник проблемы!)
        setTimerCleanup();
        setTimerPing();
        setTimerInactivity();
    }
    
    void stop() {
        if (!started_) return;
        
        started_ = false;
        std::cout << "[YamuxedConnection] Stopping connection to " 
                  << remote_peer_.toBase58() << std::endl;
        
        // ДО ИСПРАВЛЕНИЯ: таймеры не отменялись!
        // ПОСЛЕ ИСПРАВЛЕНИЯ: отменяем все таймеры
        cancelAllTimers();
    }
    
    void close() {
        if (closed_) return;
        
        auto self_ptr = shared_from_this();
        std::cout << "[YamuxedConnection] Closing connection to " 
                  << remote_peer_.toBase58() 
                  << " (use_count: " << self_ptr.use_count() << ")" << std::endl;
        
        closed_ = true;
        connection_->close();
        
        // ДО ИСПРАВЛЕНИЯ: таймеры не отменялись!
        // ПОСЛЕ ИСПРАВЛЕНИЯ: отменяем все таймеры
        cancelAllTimers();
        
        // Вызываем callback для ConnectionManager
        if (closed_callback_ && registered_in_manager_) {
            std::cout << "[YamuxedConnection] Calling closed_callback_ with use_count: " 
                      << self_ptr.use_count() << std::endl;
            
            closed_callback_(remote_peer_, self_ptr);
        }
    }
    
    bool isClosed() const { return closed_; }
    peer::PeerId remotePeer() const { return remote_peer_; }
    
    void markAsRegistered() { 
        registered_in_manager_ = true; 
    }
    
    long getSharedPtrUseCount() const { 
        auto self_ptr = shared_from_this();
        return self_ptr.use_count(); 
    }

private:
    void setTimerCleanup() {
        auto weak_self = std::weak_ptr<YamuxedConnection>(shared_from_this());
        
        cleanup_handle_ = scheduler_->scheduleWithHandle(
            [weak_self]() {
                auto self = weak_self.lock();
                if (!self) {
                    std::cout << "[Timer] Cleanup: object already destroyed" << std::endl;
                    return;
                }
                if (!self->started_) {
                    std::cout << "[Timer] Cleanup: connection stopped" << std::endl;
                    return;
                }
                
                std::cout << "[Timer] Cleanup executed for " << self->remote_peer_.toBase58() << std::endl;
                
                // ПРОБЛЕМА: рекурсивный вызов создает новый таймер!
                self->setTimerCleanup();
            },
            std::chrono::milliseconds(150)
        );
        
        std::cout << "[YamuxedConnection] Set cleanup timer for " << remote_peer_.toBase58() << std::endl;
    }
    
    void setTimerPing() {
        auto weak_self = std::weak_ptr<YamuxedConnection>(shared_from_this());
        
        ping_handle_ = scheduler_->scheduleWithHandle(
            [weak_self]() {
                auto self = weak_self.lock();
                if (!self) {
                    std::cout << "[Timer] Ping: object already destroyed" << std::endl;
                    return;
                }
                if (!self->started_) {
                    std::cout << "[Timer] Ping: connection stopped" << std::endl;
                    return;
                }
                
                std::cout << "[Timer] Ping executed for " << self->remote_peer_.toBase58() << std::endl;
                
                // ПРОБЛЕМА: рекурсивный вызов создает новый таймер!
                self->setTimerPing();
            },
            std::chrono::milliseconds(100)
        );
        
        std::cout << "[YamuxedConnection] Set ping timer for " << remote_peer_.toBase58() << std::endl;
    }
    
    void setTimerInactivity() {
        auto weak_self = std::weak_ptr<YamuxedConnection>(shared_from_this());
        
        inactivity_handle_ = scheduler_->scheduleWithHandle(
            [weak_self]() {
                auto self = weak_self.lock();
                if (!self) {
                    std::cout << "[Timer] Inactivity: object already destroyed" << std::endl;
                    return;
                }
                if (!self->started_) {
                    std::cout << "[Timer] Inactivity: connection stopped" << std::endl;
                    return;
                }
                
                std::cout << "[Timer] Inactivity executed for " << self->remote_peer_.toBase58() << std::endl;
                
                // Этот таймер может закрыть соединение
                self->close();
            },
            std::chrono::milliseconds(500)
        );
        
        std::cout << "[YamuxedConnection] Set inactivity timer for " << remote_peer_.toBase58() << std::endl;
    }
    
    void cancelAllTimers() {
        std::cout << "[YamuxedConnection] === CANCELLING ALL TIMERS === for " << remote_peer_.toBase58() << std::endl;
        
        ping_handle_.reset();
        cleanup_handle_.reset();
        inactivity_handle_.reset();
        
        std::cout << "[YamuxedConnection] === ALL TIMERS CANCELLED ===" << std::endl;
    }

private:
    std::shared_ptr<SecureConnection> connection_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    ConnectionClosedCallback closed_callback_;
    peer::PeerId remote_peer_;
    bool started_ = false;
    bool closed_ = false;
    bool registered_in_manager_ = false;
    
    // Таймеры
    basic::Scheduler::Handle ping_handle_;
    basic::Scheduler::Handle cleanup_handle_;
    basic::Scheduler::Handle inactivity_handle_;
};

} // namespace connection

namespace network {

class ConnectionManagerTest {
public:
    using ConnectionSPtr = std::shared_ptr<connection::YamuxedConnection>;
    
    void addConnectionToPeer(const peer::PeerId& peer, ConnectionSPtr conn) {
        connections_[peer].insert(conn);
        std::cout << "[ConnectionManager] Added connection for " << peer.toBase58() 
                  << " (total: " << connections_[peer].size() << ")" << std::endl;
    }
    
    void onConnectionClosed(const peer::PeerId& peer, ConnectionSPtr connection) {
        std::cout << "[ConnectionManager] onConnectionClosed for " << peer.toBase58() 
                  << " (use_count: " << connection.use_count() << ")" << std::endl;
        
        auto it = connections_.find(peer);
        if (it != connections_.end()) {
            it->second.erase(connection);
            if (it->second.empty()) {
                connections_.erase(it);
                std::cout << "[ConnectionManager] Removed peer " << peer.toBase58() << std::endl;
            }
        }
        
        // ВАЖНО: в этом месте connection должен быть единственной ссылкой!
        // Если use_count > 1, значит есть утечка
        std::cout << "[ConnectionManager] Final use_count: " << connection.use_count() << std::endl;
    }
    
    size_t getTotalPeers() const { return connections_.size(); }
    
    size_t getTotalConnections() const {
        size_t total = 0;
        for (const auto& [peer, conns] : connections_) {
            total += conns.size();
        }
        return total;
    }

private:
    std::unordered_map<peer::PeerId, std::unordered_set<ConnectionSPtr>> connections_;
};

} // namespace network
} // namespace libp2p

class YamuxMemoryLeakTest {
public:
    void runTest() {
        std::cout << "\n=== YAMUX MEMORY LEAK TEST (REALISTIC) ===" << std::endl;
        
        auto scheduler = std::make_shared<libp2p::basic::RealisticScheduler>();
        auto connection_manager = std::make_shared<libp2p::network::ConnectionManagerTest>();
        
        // Создаем соединение
        auto peer_id = "leak_test_peer";
        auto secure_conn = std::make_shared<libp2p::connection::SecureConnection>(peer_id);
        
        auto callback = [connection_manager](const libp2p::peer::PeerId& peer, 
                                            std::shared_ptr<libp2p::connection::YamuxedConnection> conn) {
            connection_manager->onConnectionClosed(peer, conn);
        };
        
        {
            // Область видимости для соединения
            auto yamux_conn = std::make_shared<libp2p::connection::YamuxedConnection>(
                secure_conn, scheduler, callback);
            
            yamux_conn->markAsRegistered();
            connection_manager->addConnectionToPeer(libp2p::peer::PeerId(peer_id), yamux_conn);
            
            std::cout << "\n--- Starting connection (timers will be created) ---" << std::endl;
            yamux_conn->start();
            
            std::cout << "\n--- Initial state ---" << std::endl;
            std::cout << "Connections count: " << connection_manager->getTotalConnections() << std::endl;
            std::cout << "YamuxedConnection use_count: " << yamux_conn.use_count() << std::endl;
            scheduler->printStats();
            
            // Даем время таймерам сработать
            std::cout << "\n--- Waiting for timers (250ms) ---" << std::endl;
            for (int i = 0; i < 5; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                scheduler->processCallbacks();
            }
            
            std::cout << "\n--- After timer execution ---" << std::endl;
            scheduler->printStats();
            
            std::cout << "\n--- Closing connection ---" << std::endl;
            yamux_conn->close();
            
            std::cout << "\n--- After close() call ---" << std::endl;
            std::cout << "YamuxedConnection use_count: " << yamux_conn.use_count() << std::endl;
            scheduler->printStats();
        }
        
        std::cout << "\n--- After yamux_conn goes out of scope ---" << std::endl;
        std::cout << "Connections count: " << connection_manager->getTotalConnections() << std::endl;
        scheduler->printStats();
        
        // Даем время на финальную очистку
        std::cout << "\n--- Final cleanup (500ms) ---" << std::endl;
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            scheduler->processCallbacks();
        }
        
        std::cout << "\n=== FINAL RESULTS ===" << std::endl;
        std::cout << "Active connections: " << connection_manager->getTotalConnections() << std::endl;
        scheduler->printStats();
        
        if (scheduler->getActiveCallbacksCount() > 0) {
            std::cout << "\n*** MEMORY LEAK DETECTED ***" << std::endl;
            std::cout << "Active callbacks in scheduler: " << scheduler->getActiveCallbacksCount() << std::endl;
            std::cout << "This indicates that timers were not properly cancelled!" << std::endl;
        } else {
            std::cout << "\n*** NO MEMORY LEAKS DETECTED ***" << std::endl;
            std::cout << "All timers were properly cancelled." << std::endl;
        }
    }
};

int main() {
    YamuxMemoryLeakTest test;
    test.runTest();
    return 0;
} 