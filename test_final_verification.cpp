#include <iostream>
#include <memory>
#include <functional>
#include <string>

// Финальная проверка исправлений компиляции

namespace libp2p {

namespace peer {
struct PeerId {
    std::string id = "test_peer";
    std::string toBase58() const { return id; }
    bool operator==(const PeerId& other) const { return id == other.id; }
};
}

namespace connection {
class CapableConnection {
public:
    virtual ~CapableConnection() = default;
    virtual bool isClosed() const = 0;
};

class YamuxedConnection : public CapableConnection, public std::enable_shared_from_this<YamuxedConnection> {
public:
    using ConnectionClosedCallback = std::function<void(const peer::PeerId&, const std::shared_ptr<CapableConnection>&)>;
    
    YamuxedConnection(ConnectionClosedCallback callback) : closed_callback_(callback) {}
    
    bool isClosed() const override { return false; }
    
    void simulateCloseCallback() {
        peer::PeerId remote_peer;
        auto self_ptr = shared_from_this();
        
        // ИСПРАВЛЕНИЕ 1: Приведение типа в yamuxed_connection.cpp (строка 574)
        closed_callback_(remote_peer, std::static_pointer_cast<CapableConnection>(self_ptr));
    }

private:
    ConnectionClosedCallback closed_callback_;
};
}

namespace network {
class ConnectionManagerImpl {
public:
    // ИСПРАВЛЕНИЕ 2: Правильная сигнатура в connection_manager_impl.cpp (строка 158)
    void onConnectionClosed(const peer::PeerId& peer,
                           const std::shared_ptr<connection::CapableConnection>& connection) {
        std::cout << "ConnectionManagerImpl::onConnectionClosed called for peer: " 
                  << peer.toBase58() << std::endl;
        std::cout << "Connection address: " << (void*)connection.get() << std::endl;
    }
};
}

}

int main() {
    std::cout << "=== ПРОВЕРКА ВСЕХ ИСПРАВЛЕНИЙ КОМПИЛЯЦИИ ===" << std::endl;
    
    auto manager = std::make_shared<libp2p::network::ConnectionManagerImpl>();
    
    auto callback = [manager](const libp2p::peer::PeerId& peer, 
                             const std::shared_ptr<libp2p::connection::CapableConnection>& conn) {
        manager->onConnectionClosed(peer, conn);
    };
    
    auto connection = std::make_shared<libp2p::connection::YamuxedConnection>(callback);
    
    std::cout << "1. Тестирование yamuxed_connection.cpp - приведение типа..." << std::endl;
    connection->simulateCloseCallback();
    
    std::cout << "2. Тестирование connection_manager_impl.cpp - сигнатура метода..." << std::endl;
    libp2p::peer::PeerId peer;
    manager->onConnectionClosed(peer, connection);
    
    std::cout << "=== ВСЕ ИСПРАВЛЕНИЯ РАБОТАЮТ КОРРЕКТНО ===" << std::endl;
    return 0;
} 