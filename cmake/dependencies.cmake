# https://docs.hunter.sh/en/latest/packages/pkg/GTest.html
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
find_package(GMock CONFIG REQUIRED)

# https://docs.hunter.sh/en/latest/packages/pkg/Boost.html
hunter_add_package(Boost COMPONENTS random filesystem)
find_package(Boost CONFIG REQUIRED  random filesystem)

# added from hunter_config
hunter_add_package(GSL)

# https://www.openssl.org/
hunter_add_package(OpenSSL)
find_package(OpenSSL REQUIRED)

# https://developers.google.com/protocol-buffers/
hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)
