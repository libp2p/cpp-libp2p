#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <atomic>
#include <mutex>
#include <sstream>
#include <iomanip>

// Простой тест с 10 соединениями для демонстрации исправления

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

// ИСПРАВЛЕННЫЙ Scheduler с правильной очисткой
class FixedScheduler : public std::enable_shared_from_this<FixedScheduler> {
public:
    struct Handle {
        int id = -1;
        std::weak_ptr<FixedScheduler> scheduler;
        
        void reset() {
            if (auto sched = scheduler.lock()) {
                sched->cancelHandle(id);
            }
            id = -1;
        }
        
        bool has_value() const { return id >= 0; }
        
        Handle() = default;
        Handle(int id_, std::weak_ptr<FixedScheduler> sched) 
            : id(id_), scheduler(sched) {}
    };
    
    Handle scheduleWithHandle(std::function<void()> cb, std::chrono::milliseconds delay) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        int handle_id = next_id_++;
        
        scheduled_callbacks_[handle_id] = {
            .callback = std::move(cb),
            .execute_time = std::chrono::steady_clock::now() + delay,
            .cancelled = false
        };
        
        total_scheduled_++;
        std::cout << "[Scheduler] Scheduled " << handle_id << " (total active: " << scheduled_callbacks_.size() << ")" << std::endl;
        
        return Handle(handle_id, weak_from_this());
    }
    
    void cancelHandle(int handle_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = scheduled_callbacks_.find(handle_id);
        if (it != scheduled_callbacks_.end()) {
            it->second.cancelled = true;
            total_cancelled_++;
            std::cout << "[Scheduler] Cancelled " << handle_id << std::endl;
        }
    }
    
    void processCallbacks() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        std::vector<int> to_execute;
        std::vector<int> to_remove;
        
        // Собираем ID для удаления и выполнения
        for (auto& [id, info] : scheduled_callbacks_) {
            if (info.cancelled) {
                to_remove.push_back(id);
            } else if (info.execute_time <= now) {
                to_execute.push_back(id);
            }
        }
        
        // ИСПРАВЛЕНИЕ 1: Удаляем отмененные колбэки СРАЗУ
        for (int id : to_remove) {
            scheduled_callbacks_.erase(id);
            std::cout << "[Scheduler] Removed cancelled callback " << id << std::endl;
        }
        
        // ИСПРАВЛЕНИЕ 2: Выполняем и удаляем колбэки
        for (int id : to_execute) {
            auto it = scheduled_callbacks_.find(id);
            if (it != scheduled_callbacks_.end()) {
                auto callback = std::move(it->second.callback);
                scheduled_callbacks_.erase(it); // Удаляем ПЕРЕД выполнением
                
                mutex_.unlock();
                std::cout << "[Scheduler] Executing " << id << std::endl;
                callback();
                mutex_.lock();
                
                total_executed_++;
            }
        }
    }
    
    void printStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[Scheduler] Total: sched=" << total_scheduled_ 
                  << ", exec=" << total_executed_ 
                  << ", canc=" << total_cancelled_ 
                  << ", active=" << scheduled_callbacks_.size() << std::endl;
        
        if (!scheduled_callbacks_.empty()) {
            std::cout << "  ⚠️  LEAK: " << scheduled_callbacks_.size() << " active callbacks!" << std::endl;
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

using Scheduler = FixedScheduler;

} // namespace basic

namespace connection {

// Простая версия без многопоточности
class SimpleYamuxedConnection : public std::enable_shared_from_this<SimpleYamuxedConnection> {
public:
    using ConnectionClosedCallback = std::function<void(const peer::PeerId&, std::shared_ptr<SimpleYamuxedConnection>)>;
    
    explicit SimpleYamuxedConnection(
        const std::string& peer_id,
        std::shared_ptr<basic::Scheduler> scheduler,
        ConnectionClosedCallback closed_callback,
        int connection_id)
        : scheduler_(std::move(scheduler))
        , closed_callback_(std::move(closed_callback))
        , remote_peer_(peer_id)
        , connection_id_(connection_id) {
        
        std::cout << "[Connection-" << connection_id_ << "] Created for " << peer_id << std::endl;
    }
    
    ~SimpleYamuxedConnection() {
        std::cout << "[Connection-" << connection_id_ << "] *** DESTROYED ***" << std::endl;
        destroyed_count_++;
    }
    
    void start() {
        started_ = true;
        std::cout << "[Connection-" << connection_id_ << "] Started" << std::endl;
        
        // Запускаем таймеры
        setTimerCleanup();
        setTimerPing();
    }
    
    void close() {
        if (closed_) return;
        
        auto self_ptr = shared_from_this();
        std::cout << "[Connection-" << connection_id_ << "] Closing (use_count: " << self_ptr.use_count() << ")" << std::endl;
        
        closed_ = true;
        started_ = false; // Останавливаем таймеры
        
        // ИСПРАВЛЕНИЕ: отменяем все таймеры
        cancelAllTimers();
        
        // Вызываем callback для ConnectionManager
        if (closed_callback_ && registered_in_manager_) {
            std::cout << "[Connection-" << connection_id_ << "] Calling closed_callback_" << std::endl;
            closed_callback_(remote_peer_, self_ptr);
        }
        
        std::cout << "[Connection-" << connection_id_ << "] Closed (final use_count: " << self_ptr.use_count() << ")" << std::endl;
    }
    
    bool isClosed() const { return closed_; }
    peer::PeerId remotePeer() const { return remote_peer_; }
    
    void markAsRegistered() { 
        registered_in_manager_ = true; 
    }
    
    int getConnectionId() const { return connection_id_; }
    
    static int getDestroyedCount() { return destroyed_count_; }

private:
    void setTimerCleanup() {
        auto weak_self = std::weak_ptr<SimpleYamuxedConnection>(shared_from_this());
        
        cleanup_handle_ = scheduler_->scheduleWithHandle(
            [weak_self, id = connection_id_]() {
                auto self = weak_self.lock();
                if (!self) {
                    std::cout << "[Timer] Cleanup-" << id << ": object destroyed" << std::endl;
                    return;
                }
                if (!self->started_) {
                    std::cout << "[Timer] Cleanup-" << id << ": connection stopped" << std::endl;
                    return;
                }
                
                std::cout << "[Timer] Cleanup-" << id << ": executed" << std::endl;
                
                // Рекурсивный вызов только если активно
                if (self->started_ && !self->closed_) {
                    self->setTimerCleanup();
                }
            },
            std::chrono::milliseconds(300)
        );
    }
    
    void setTimerPing() {
        auto weak_self = std::weak_ptr<SimpleYamuxedConnection>(shared_from_this());
        
        ping_handle_ = scheduler_->scheduleWithHandle(
            [weak_self, id = connection_id_]() {
                auto self = weak_self.lock();
                if (!self) {
                    std::cout << "[Timer] Ping-" << id << ": object destroyed" << std::endl;
                    return;
                }
                if (!self->started_) {
                    std::cout << "[Timer] Ping-" << id << ": connection stopped" << std::endl;
                    return;
                }
                
                std::cout << "[Timer] Ping-" << id << ": executed" << std::endl;
                
                // Рекурсивный вызов только если активно
                if (self->started_ && !self->closed_) {
                    self->setTimerPing();
                }
            },
            std::chrono::milliseconds(200)
        );
    }
    
    void cancelAllTimers() {
        std::cout << "[Connection-" << connection_id_ << "] === CANCELLING TIMERS ===" << std::endl;
        
        ping_handle_.reset();
        cleanup_handle_.reset();
        
        std::cout << "[Connection-" << connection_id_ << "] === TIMERS CANCELLED ===" << std::endl;
    }

private:
    std::shared_ptr<basic::Scheduler> scheduler_;
    ConnectionClosedCallback closed_callback_;
    peer::PeerId remote_peer_;
    bool started_ = false;
    bool closed_ = false;
    bool registered_in_manager_ = false;
    int connection_id_;
    
    basic::Scheduler::Handle ping_handle_;
    basic::Scheduler::Handle cleanup_handle_;
    
    static std::atomic<int> destroyed_count_;
};

std::atomic<int> SimpleYamuxedConnection::destroyed_count_{0};

} // namespace connection

namespace network {

class ConnectionManagerTest {
public:
    using ConnectionSPtr = std::shared_ptr<connection::SimpleYamuxedConnection>;
    
    void addConnectionToPeer(const peer::PeerId& peer, ConnectionSPtr conn) {
        connections_[peer].insert(conn);
        std::cout << "[Manager] Added connection " << conn->getConnectionId() << " for " << peer.toBase58() << std::endl;
    }
    
    void onConnectionClosed(const peer::PeerId& peer, ConnectionSPtr connection) {
        std::cout << "[Manager] onConnectionClosed for " << peer.toBase58() 
                  << " (connection " << connection->getConnectionId() << ")" << std::endl;
        
        auto it = connections_.find(peer);
        if (it != connections_.end()) {
            it->second.erase(connection);
            if (it->second.empty()) {
                connections_.erase(it);
                std::cout << "[Manager] Removed peer " << peer.toBase58() << std::endl;
            }
        }
    }
    
    size_t getTotalConnections() const {
        size_t total = 0;
        for (const auto& [peer, conns] : connections_) {
            total += conns.size();
        }
        return total;
    }
    
    void printStats() const {
        std::cout << "[Manager] Active connections: " << getTotalConnections() << std::endl;
    }

private:
    std::unordered_map<peer::PeerId, std::unordered_set<ConnectionSPtr>> connections_;
};

} // namespace network
} // namespace libp2p

class SimpleTest10Connections {
public:
    void runTest() {
        std::cout << "\n=== SIMPLE YAMUX TEST: 10 CONNECTIONS ===" << std::endl;
        
        auto scheduler = std::make_shared<libp2p::basic::FixedScheduler>();
        auto connection_manager = std::make_shared<libp2p::network::ConnectionManagerTest>();
        
        std::vector<std::shared_ptr<libp2p::connection::SimpleYamuxedConnection>> connections;
        
        // Создаем 10 соединений
        std::cout << "\n--- Creating 10 connections ---" << std::endl;
        {
            for (int i = 0; i < 10; ++i) {
                std::ostringstream oss;
                oss << "peer_" << std::setfill('0') << std::setw(2) << i;
                auto peer_id = oss.str();
                
                auto callback = [connection_manager](const libp2p::peer::PeerId& peer, 
                                                    std::shared_ptr<libp2p::connection::SimpleYamuxedConnection> conn) {
                    connection_manager->onConnectionClosed(peer, conn);
                };
                
                auto yamux_conn = std::make_shared<libp2p::connection::SimpleYamuxedConnection>(
                    peer_id, scheduler, callback, i);
                
                yamux_conn->markAsRegistered();
                connection_manager->addConnectionToPeer(libp2p::peer::PeerId(peer_id), yamux_conn);
                
                yamux_conn->start();
                connections.push_back(yamux_conn);
            }
        }
        
        std::cout << "\n--- Initial state ---" << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        // Работа системы
        std::cout << "\n--- Running system (500ms) ---" << std::endl;
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            scheduler->processCallbacks();
        }
        
        std::cout << "\n--- After workload ---" << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        // Закрываем все соединения
        std::cout << "\n--- Closing all connections ---" << std::endl;
        for (auto& conn : connections) {
            conn->close();
        }
        
        std::cout << "\n--- After close() calls ---" << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        // Очищаем локальные ссылки
        connections.clear();
        
        std::cout << "\n--- After clearing local references ---" << std::endl;
        std::cout << "Destroyed objects: " << libp2p::connection::SimpleYamuxedConnection::getDestroyedCount() << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        // Финальная очистка
        std::cout << "\n--- Final cleanup (1 second) ---" << std::endl;
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            scheduler->processCallbacks();
        }
        
        std::cout << "\n=== FINAL RESULTS ===" << std::endl;
        std::cout << "Destroyed objects: " << libp2p::connection::SimpleYamuxedConnection::getDestroyedCount() << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        if (scheduler->getActiveCallbacksCount() > 0) {
            std::cout << "\n❌ MEMORY LEAK DETECTED!" << std::endl;
            std::cout << "Active callbacks: " << scheduler->getActiveCallbacksCount() << std::endl;
        } else {
            std::cout << "\n✅ NO MEMORY LEAKS!" << std::endl;
            std::cout << "All callbacks properly cleaned up!" << std::endl;
        }
        
        if (libp2p::connection::SimpleYamuxedConnection::getDestroyedCount() == 10) {
            std::cout << "✅ All 10 connections properly destroyed!" << std::endl;
        } else {
            std::cout << "❌ Some connections were not destroyed!" << std::endl;
        }
    }
};

int main() {
    SimpleTest10Connections test;
    test.runTest();
    return 0;
} 