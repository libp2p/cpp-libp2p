# https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
find_package(GMock CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost COMPONENTS random filesystem program_options)
find_package(Boost CONFIG REQUIRED  random filesystem program_options)

# added from hunter_config
hunter_add_package(GSL)

# https://www.openssl.org/
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

# https://developers.google.com/protocol-buffers/
hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/spdlog.html
hunter_add_package(spdlog)
find_package(spdlog CONFIG REQUIRED)

# https://github.com/masterjedy/hat-trie
hunter_add_package(tsl_hat_trie)
find_package(tsl_hat_trie CONFIG REQUIRED)

# https://github.com/masterjedy/di
hunter_add_package(Boost.DI)
find_package(Boost.DI CONFIG REQUIRED)

# https://github.com/soramitsu/libp2p-sqlite-modern-cpp/tree/hunter
hunter_add_package(SQLiteModernCpp)
find_package(SQLiteModernCpp CONFIG REQUIRED)
