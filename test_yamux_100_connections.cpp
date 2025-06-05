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
#include <sstream>
#include <iomanip>

// Нагрузочный тест с 100 соединениями и исправлением мертвых колбэков

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

// ИСПРАВЛЕННЫЙ Scheduler с очисткой мертвых колбэков
class ImprovedScheduler : public std::enable_shared_from_this<ImprovedScheduler> {
public:
    struct Handle {
        int id = -1;
        std::weak_ptr<ImprovedScheduler> scheduler;
        
        void reset() {
            if (auto sched = scheduler.lock()) {
                sched->cancelHandle(id);
            }
            id = -1;
        }
        
        bool has_value() const { return id >= 0; }
        
        Handle() = default;
        Handle(int id_, std::weak_ptr<ImprovedScheduler> sched) 
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
        return Handle(handle_id, weak_from_this());
    }
    
    void cancelHandle(int handle_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = scheduled_callbacks_.find(handle_id);
        if (it != scheduled_callbacks_.end()) {
            it->second.cancelled = true;
            total_cancelled_++;
        }
    }
    
    void processCallbacks() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::steady_clock::now();
        std::vector<int> to_execute;
        std::vector<int> to_remove;
        
        for (auto& [id, info] : scheduled_callbacks_) {
            if (info.cancelled) {
                to_remove.push_back(id);
            } else if (info.execute_time <= now) {
                to_execute.push_back(id);
            }
        }
        
        // ИСПРАВЛЕНИЕ: Удаляем отмененные колбэки
        for (int id : to_remove) {
            scheduled_callbacks_.erase(id);
        }
        
        // Выполняем готовые колбэки
        for (int id : to_execute) {
            auto it = scheduled_callbacks_.find(id);
            if (it != scheduled_callbacks_.end()) {
                auto callback = std::move(it->second.callback);
                scheduled_callbacks_.erase(it); // ИСПРАВЛЕНИЕ: удаляем сразу после выполнения
                
                mutex_.unlock();
                callback();
                mutex_.lock();
                
                total_executed_++;
            }
        }
    }
    
    void printStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[Scheduler] Stats: scheduled=" << total_scheduled_ 
                  << ", executed=" << total_executed_ 
                  << ", cancelled=" << total_cancelled_ 
                  << ", active=" << scheduled_callbacks_.size() << std::endl;
        
        if (!scheduled_callbacks_.empty()) {
            std::cout << "  WARNING: " << scheduled_callbacks_.size() << " active callbacks!" << std::endl;
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

using Scheduler = ImprovedScheduler;

} // namespace basic

namespace connection {

// Оптимизированная эмуляция Stream
class OptimizedYamuxStream : public std::enable_shared_from_this<OptimizedYamuxStream> {
public:
    OptimizedYamuxStream(std::shared_ptr<class YamuxedConnection> connection, uint32_t stream_id)
        : connection_(connection), stream_id_(stream_id) {}
    
    ~OptimizedYamuxStream() = default;
    
    void simulateAsyncRead() {
        auto conn = connection_.lock();
        if (conn) {
            // Меньше логирования для производительности
        }
    }
    
    void close() {
        connection_.reset();
    }
    
    uint32_t getId() const { return stream_id_; }
    
private:
    std::weak_ptr<class YamuxedConnection> connection_;
    uint32_t stream_id_;
};

struct SecureConnection {
    peer::PeerId remote_peer;
    bool closed = false;
    std::atomic<int> async_operations_count{0};
    
    SecureConnection(const std::string& peer_id) : remote_peer(peer_id) {}
    
    peer::PeerId remotePeer() const { return remote_peer; }
    bool isClosed() const { return closed; }
    void close() { 
        closed = true; 
    }
    
    // Оптимизированное асинхронное чтение
    void readSome(std::vector<uint8_t>& buffer, size_t size, 
                  std::function<void(int)> callback) {
        async_operations_count++;
        
        // Симулируем асинхронную операцию с более коротким временем
        std::thread([this, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Быстрее для 100 соединений
            
            if (!closed) {
                callback(42); // 42 байта прочитано
            } else {
                callback(-1); // Ошибка при закрытии
            }
            
            async_operations_count--;
        }).detach();
    }
    
    void writeSome(const std::vector<uint8_t>& data, 
                   std::function<void(int)> callback) {
        async_operations_count++;
        
        std::thread([this, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            callback(closed ? -1 : 100);
            async_operations_count--;
        }).detach();
    }
};

// Оптимизированная версия YamuxedConnection
class YamuxedConnection : public std::enable_shared_from_this<YamuxedConnection> {
public:
    using ConnectionClosedCallback = std::function<void(const peer::PeerId&, std::shared_ptr<YamuxedConnection>)>;
    
    explicit YamuxedConnection(
        std::shared_ptr<SecureConnection> connection,
        std::shared_ptr<basic::Scheduler> scheduler,
        ConnectionClosedCallback closed_callback,
        int connection_id)
        : connection_(std::move(connection))
        , scheduler_(std::move(scheduler))
        , closed_callback_(std::move(closed_callback))
        , remote_peer_(connection_->remotePeer())
        , raw_read_buffer_(1024)
        , connection_id_(connection_id) {}
    
    ~YamuxedConnection() {
        // Меньше логирования для 100 соединений
        destroyed_count_++;
    }
    
    void start() {
        started_ = true;
        
        // Запускаем таймеры
        setTimerCleanup();
        setTimerPing();
        
        // Запускаем чтение
        continueReading();
        
        // Создаем меньше потоков для производительности
        createMockStreams();
    }
    
    void stop() {
        if (!started_) return;
        
        started_ = false;
        
        // ИСПРАВЛЕНИЕ: отменяем все таймеры
        cancelAllTimers();
        
        // Закрываем потоки
        closeAllStreams();
    }
    
    void close() {
        if (closed_) return;
        
        auto self_ptr = shared_from_this();
        
        closed_ = true;
        connection_->close();
        
        // ИСПРАВЛЕНИЕ: отменяем все таймеры
        cancelAllTimers();
        
        // Закрываем потоки
        closeAllStreams();
        
        // Вызываем callback для ConnectionManager
        if (closed_callback_ && registered_in_manager_) {
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
    
    int getConnectionId() const { return connection_id_; }
    
    static int getDestroyedCount() { return destroyed_count_; }

private:
    // ИСПРАВЛЕННОЕ continueReading с проверкой состояния
    void continueReading() {
        if (!started_ || closed_) return;
        
        auto weak_self = std::weak_ptr<YamuxedConnection>(shared_from_this());
        
        connection_->readSome(raw_read_buffer_, raw_read_buffer_.size(),
            [weak_self](int bytes_read) {
                auto self = weak_self.lock();
                if (!self) {
                    // ИСПРАВЛЕНИЕ: не создаем лишних операций для мертвых объектов
                    return;
                }
                self->onRead(bytes_read);
            });
    }
    
    void onRead(int bytes_read) {
        if (!started_ || closed_) return; // ИСПРАВЛЕНИЕ: дополнительная проверка
        
        if (bytes_read < 0) {
            close();
            return;
        }
        
        // Продолжаем чтение только если соединение активно
        if (started_ && !closed_) {
            continueReading();
        }
    }
    
    void createMockStreams() {
        // Создаем только 1 поток для производительности
        auto stream = std::make_shared<OptimizedYamuxStream>(shared_from_this(), 1);
        streams_[1] = stream;
        stream->simulateAsyncRead();
    }
    
    void closeAllStreams() {
        for (auto& [id, stream] : streams_) {
            if (stream) {
                stream->close();
            }
        }
        streams_.clear();
    }
    
    void setTimerCleanup() {
        auto weak_self = std::weak_ptr<YamuxedConnection>(shared_from_this());
        
        cleanup_handle_ = scheduler_->scheduleWithHandle(
            [weak_self]() {
                auto self = weak_self.lock();
                if (!self || !self->started_) return;
                
                // Очистка потоков
                self->cleanupAbandonedStreams();
                
                // Рекурсивный вызов только если соединение активно
                if (self->started_ && !self->closed_) {
                    self->setTimerCleanup();
                }
            },
            std::chrono::milliseconds(200) // Увеличили интервал для производительности
        );
    }
    
    void setTimerPing() {
        auto weak_self = std::weak_ptr<YamuxedConnection>(shared_from_this());
        
        ping_handle_ = scheduler_->scheduleWithHandle(
            [weak_self]() {
                auto self = weak_self.lock();
                if (!self || !self->started_) return;
                
                // Отправляем ping
                self->sendPing();
                
                // Рекурсивный вызов только если соединение активно
                if (self->started_ && !self->closed_) {
                    self->setTimerPing();
                }
            },
            std::chrono::milliseconds(150) // Увеличили интервал
        );
    }
    
    void cleanupAbandonedStreams() {
        // Оптимизированная очистка
    }
    
    void sendPing() {
        std::vector<uint8_t> ping_data = {0x01};
        auto weak_self = std::weak_ptr<YamuxedConnection>(shared_from_this());
        
        connection_->writeSome(ping_data, [weak_self](int result) {
            // Минимальная обработка для производительности
        });
    }
    
    void cancelAllTimers() {
        ping_handle_.reset();
        cleanup_handle_.reset();
    }

private:
    std::shared_ptr<SecureConnection> connection_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    ConnectionClosedCallback closed_callback_;
    peer::PeerId remote_peer_;
    bool started_ = false;
    bool closed_ = false;
    bool registered_in_manager_ = false;
    int connection_id_;
    
    std::vector<uint8_t> raw_read_buffer_;
    std::unordered_map<uint32_t, std::shared_ptr<OptimizedYamuxStream>> streams_;
    
    basic::Scheduler::Handle ping_handle_;
    basic::Scheduler::Handle cleanup_handle_;
    
    static std::atomic<int> destroyed_count_;
};

std::atomic<int> YamuxedConnection::destroyed_count_{0};

} // namespace connection

namespace network {

class ConnectionManagerTest {
public:
    using ConnectionSPtr = std::shared_ptr<connection::YamuxedConnection>;
    
    void addConnectionToPeer(const peer::PeerId& peer, ConnectionSPtr conn) {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_[peer].insert(conn);
        total_connections_++;
    }
    
    void onConnectionClosed(const peer::PeerId& peer, ConnectionSPtr connection) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = connections_.find(peer);
        if (it != connections_.end()) {
            it->second.erase(connection);
            if (it->second.empty()) {
                connections_.erase(it);
            }
        }
        
        total_connections_--;
        closed_connections_++;
    }
    
    size_t getTotalConnections() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t total = 0;
        for (const auto& [peer, conns] : connections_) {
            total += conns.size();
        }
        return total;
    }
    
    void printStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "[ConnectionManager] Active: " << getTotalConnections() 
                  << ", Closed: " << closed_connections_ << std::endl;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<peer::PeerId, std::unordered_set<ConnectionSPtr>> connections_;
    int total_connections_ = 0;
    int closed_connections_ = 0;
};

} // namespace network
} // namespace libp2p

class LoadTestYamux100Connections {
public:
    void runTest() {
        std::cout << "\n=== YAMUX LOAD TEST: 100 CONNECTIONS ===" << std::endl;
        
        auto scheduler = std::make_shared<libp2p::basic::ImprovedScheduler>();
        auto connection_manager = std::make_shared<libp2p::network::ConnectionManagerTest>();
        
        std::vector<std::shared_ptr<libp2p::connection::YamuxedConnection>> connections;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Создаем 100 соединений
        std::cout << "\n--- Creating 100 connections ---" << std::endl;
        {
            for (int i = 0; i < 100; ++i) {
                std::ostringstream oss;
                oss << "peer_" << std::setfill('0') << std::setw(3) << i;
                auto peer_id = oss.str();
                
                auto secure_conn = std::make_shared<libp2p::connection::SecureConnection>(peer_id);
                
                auto callback = [connection_manager](const libp2p::peer::PeerId& peer, 
                                                    std::shared_ptr<libp2p::connection::YamuxedConnection> conn) {
                    connection_manager->onConnectionClosed(peer, conn);
                };
                
                auto yamux_conn = std::make_shared<libp2p::connection::YamuxedConnection>(
                    secure_conn, scheduler, callback, i);
                
                yamux_conn->markAsRegistered();
                connection_manager->addConnectionToPeer(libp2p::peer::PeerId(peer_id), yamux_conn);
                
                yamux_conn->start();
                connections.push_back(yamux_conn);
                
                if ((i + 1) % 20 == 0) {
                    std::cout << "Created " << (i + 1) << " connections..." << std::endl;
                }
            }
        }
        
        auto creation_time = std::chrono::high_resolution_clock::now();
        auto creation_duration = std::chrono::duration_cast<std::chrono::milliseconds>(creation_time - start_time);
        
        std::cout << "\n--- Initial state ---" << std::endl;
        std::cout << "Created 100 connections in " << creation_duration.count() << "ms" << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        // Даем время на работу системы
        std::cout << "\n--- Running system for 1 second ---" << std::endl;
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            scheduler->processCallbacks();
        }
        
        std::cout << "\n--- After initial workload ---" << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        // Закрываем все соединения
        std::cout << "\n--- Closing all connections ---" << std::endl;
        auto close_start = std::chrono::high_resolution_clock::now();
        
        for (auto& conn : connections) {
            conn->close();
        }
        
        auto close_time = std::chrono::high_resolution_clock::now();
        auto close_duration = std::chrono::duration_cast<std::chrono::milliseconds>(close_time - close_start);
        
        std::cout << "Closed 100 connections in " << close_duration.count() << "ms" << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        // Очищаем локальные ссылки
        connections.clear();
        
        std::cout << "\n--- After clearing local references ---" << std::endl;
        std::cout << "Destroyed objects: " << libp2p::connection::YamuxedConnection::getDestroyedCount() << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        // Финальная очистка
        std::cout << "\n--- Final cleanup (2 seconds) ---" << std::endl;
        for (int i = 0; i < 40; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            scheduler->processCallbacks();
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n=== FINAL RESULTS ===" << std::endl;
        std::cout << "Total test time: " << total_duration.count() << "ms" << std::endl;
        std::cout << "Destroyed objects: " << libp2p::connection::YamuxedConnection::getDestroyedCount() << std::endl;
        connection_manager->printStats();
        scheduler->printStats();
        
        if (scheduler->getActiveCallbacksCount() > 0) {
            std::cout << "\n*** MEMORY LEAK DETECTED ***" << std::endl;
            std::cout << "Active callbacks in scheduler: " << scheduler->getActiveCallbacksCount() << std::endl;
        } else {
            std::cout << "\n*** NO MEMORY LEAKS DETECTED ***" << std::endl;
            std::cout << "All callbacks properly cleaned up!" << std::endl;
        }
        
        if (libp2p::connection::YamuxedConnection::getDestroyedCount() == 100) {
            std::cout << "✅ All 100 connections properly destroyed!" << std::endl;
        } else {
            std::cout << "❌ Some connections were not destroyed!" << std::endl;
        }
    }
};

int main() {
    LoadTestYamux100Connections test;
    test.runTest();
    return 0;
} 