#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <functional>

// Mock классы для тестирования
namespace libp2p {

namespace peer {
struct PeerId {
    std::string id;
    
    PeerId(const std::string& id_) : id(id_) {}
    
    std::string toBase58() const { return id; }
    
    bool operator==(const PeerId& other) const {
        return id == other.id;
    }
};
}
}

// Хеш для peer::PeerId в unordered_map - должен быть определен до использования
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
struct Scheduler {
    struct Handle {
        bool valid = false;
    };
    
    Handle scheduleWithHandle(std::function<void()> cb, std::chrono::milliseconds delay) {
        // Простая эмуляция - выполняем через delay
        std::thread([cb, delay]() {
            std::this_thread::sleep_for(delay);
            cb();
        }).detach();
        return Handle{true};
    }
    
    void cancel(Handle& h) {
        h.valid = false;
    }
};
}

namespace connection {
struct Stream {
    virtual ~Stream() = default;
};

struct SecureConnection {
    peer::PeerId remote_peer;
    bool closed = false;
    
    SecureConnection(const std::string& peer_id) : remote_peer(peer_id) {}
    
    peer::PeerId remotePeer() const { return remote_peer; }
    bool isClosed() const { return closed; }
    void close() { closed = true; }
};

// Упрощенная версия YamuxedConnection для тестирования
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
        std::cout << "[YamuxedConnection] Destroyed connection to " 
                  << remote_peer_.toBase58() 
                  << " at address " << (void*)this << std::endl;
    }
    
    void start() {
        started_ = true;
        std::cout << "[YamuxedConnection] Started connection to " 
                  << remote_peer_.toBase58() << std::endl;
    }
    
    void close() {
        if (closed_) return;
        
        auto self_ptr = shared_from_this();
        std::cout << "[YamuxedConnection] Closing connection to " 
                  << remote_peer_.toBase58() 
                  << " (use_count: " << self_ptr.use_count() << ")" << std::endl;
        
        closed_ = true;
        connection_->close();
        
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
        std::cout << "[YamuxedConnection] Marked as registered in manager" << std::endl;
    }
    
    long getSharedPtrUseCount() const { 
        auto self_ptr = shared_from_this();
        return self_ptr.use_count(); 
    }

private:
    std::shared_ptr<SecureConnection> connection_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    ConnectionClosedCallback closed_callback_;
    peer::PeerId remote_peer_;
    bool started_ = false;
    bool closed_ = false;
    bool registered_in_manager_ = false;
};

} // namespace connection

namespace network {

// Упрощенная версия ConnectionManager для тестирования
class ConnectionManagerTest {
public:
    using ConnectionSPtr = std::shared_ptr<connection::YamuxedConnection>;
    
    void addConnectionToPeer(const peer::PeerId& peer, ConnectionSPtr conn) {
        std::cout << "[ConnectionManager] === addConnectionToPeer CALLED ===" << std::endl;
        std::cout << "[ConnectionManager] peer: " << peer.toBase58() << std::endl;
        std::cout << "[ConnectionManager] connection address: " << (void*)conn.get() << std::endl;
        std::cout << "[ConnectionManager] connection use_count: " << conn.use_count() << std::endl;
        
        auto it = connections_.find(peer);
        if (it == connections_.end()) {
            std::cout << "[ConnectionManager] Creating new peer entry" << std::endl;
            connections_.insert({peer, {conn}});
        } else {
            std::cout << "[ConnectionManager] Adding to existing peer (current size: " 
                      << it->second.size() << ")" << std::endl;
            connections_[peer].insert(conn);
        }
        
        std::cout << "[ConnectionManager] Total connections for peer: " 
                  << connections_[peer].size() << std::endl;
        std::cout << "[ConnectionManager] === addConnectionToPeer FINISHED ===" << std::endl;
    }
    
    void onConnectionClosed(const peer::PeerId& peer, ConnectionSPtr connection) {
        std::cout << "[ConnectionManager] === onConnectionClosed CALLED ===" << std::endl;
        std::cout << "[ConnectionManager] peer: " << peer.toBase58() << std::endl;
        std::cout << "[ConnectionManager] connection address: " << (void*)connection.get() << std::endl;
        std::cout << "[ConnectionManager] connection use_count: " << connection.use_count() << std::endl;
        
        auto it = connections_.find(peer);
        if (it == connections_.end()) {
            std::cout << "[ConnectionManager] WARNING: Peer not found in connections_!" << std::endl;
            return;
        }
        
        std::cout << "[ConnectionManager] Found peer, current connections: " << it->second.size() << std::endl;
        
        // Показываем все соединения для сравнения
        for (const auto& conn : it->second) {
            std::cout << "[ConnectionManager] Existing connection: " 
                      << (void*)conn.get() 
                      << " (use_count: " << conn.use_count() << ")" << std::endl;
        }
        
        auto erased = it->second.erase(connection);
        std::cout << "[ConnectionManager] Erased count: " << erased << std::endl;
        
        if (erased == 0) {
            std::cout << "[ConnectionManager] ERROR: Connection was NOT found in set!" << std::endl;
        } else {
            std::cout << "[ConnectionManager] SUCCESS: Connection removed" << std::endl;
        }
        
        if (it->second.empty()) {
            connections_.erase(peer);
            std::cout << "[ConnectionManager] Peer removed from connections_ (no more connections)" << std::endl;
        }
        
        std::cout << "[ConnectionManager] === onConnectionClosed FINISHED ===" << std::endl;
    }
    
    size_t getTotalConnections() const {
        size_t total = 0;
        for (const auto& [peer, conns] : connections_) {
            total += conns.size();
        }
        return total;
    }
    
    void printStatus() const {
        std::cout << "\n[ConnectionManager] === STATUS ===" << std::endl;
        std::cout << "[ConnectionManager] Total peers: " << connections_.size() << std::endl;
        std::cout << "[ConnectionManager] Total connections: " << getTotalConnections() << std::endl;
        
        for (const auto& [peer, conns] : connections_) {
            std::cout << "[ConnectionManager] Peer " << peer.toBase58() 
                      << " has " << conns.size() << " connections:" << std::endl;
            for (const auto& conn : conns) {
                std::cout << "[ConnectionManager]   - " << (void*)conn.get() 
                          << " (use_count: " << conn.use_count() << ")" << std::endl;
            }
        }
        std::cout << "[ConnectionManager] ===================" << std::endl;
    }

private:
    std::unordered_map<peer::PeerId, std::unordered_set<ConnectionSPtr>> connections_;
};

} // namespace network
} // namespace libp2p

// Тест множественных соединений
class YamuxLeakTest {
public:
    void runTest() {
        std::cout << "\n=== YAMUX LEAK TEST STARTED ===" << std::endl;
        
        auto scheduler = std::make_shared<libp2p::basic::Scheduler>();
        auto connection_manager = std::make_shared<libp2p::network::ConnectionManagerTest>();
        
        // Создаем множественные соединения
        const int num_connections = 5;
        std::vector<std::shared_ptr<libp2p::connection::YamuxedConnection>> connections;
        
        for (int i = 0; i < num_connections; ++i) {
            auto peer_id = "peer_" + std::to_string(i);
            auto secure_conn = std::make_shared<libp2p::connection::SecureConnection>(peer_id);
            
            // Создаем callback для ConnectionManager
            auto callback = [connection_manager](const libp2p::peer::PeerId& peer, 
                                                std::shared_ptr<libp2p::connection::YamuxedConnection> conn) {
                connection_manager->onConnectionClosed(peer, conn);
            };
            
            auto yamux_conn = std::make_shared<libp2p::connection::YamuxedConnection>(
                secure_conn, scheduler, callback);
            
            yamux_conn->start();
            yamux_conn->markAsRegistered();
            
            connection_manager->addConnectionToPeer(libp2p::peer::PeerId(peer_id), yamux_conn);
            connections.push_back(yamux_conn);
            
            std::cout << "\n--- Created connection " << (i+1) << "/" << num_connections << " ---" << std::endl;
        }
        
        connection_manager->printStatus();
        
        // Даем время на обработку
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "\n=== CLOSING ALL CONNECTIONS ===" << std::endl;
        
        // Закрываем все соединения
        for (int i = 0; i < connections.size(); ++i) {
            std::cout << "\n--- Closing connection " << (i+1) << "/" << connections.size() << " ---" << std::endl;
            connections[i]->close();
            
            connection_manager->printStatus();
        }
        
        // Очищаем наши ссылки
        connections.clear();
        
        std::cout << "\n=== FINAL STATUS ===" << std::endl;
        connection_manager->printStatus();
        
        std::cout << "\n=== YAMUX LEAK TEST FINISHED ===" << std::endl;
    }
};

int main() {
    YamuxLeakTest test;
    test.runTest();
    
    std::cout << "\nWaiting for potential cleanup..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    return 0;
} 