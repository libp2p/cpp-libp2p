hunter_config(
    Boost
    VERSION 1.70.0-p0
)

hunter_config(GSL
    URL https://github.com/microsoft/GSL/archive/v2.0.0.tar.gz
    SHA1 9bbdea551b38d7d09ab7aa2e89b91a66dd032b4a
    CMAKE_ARGS GSL_TEST=OFF
    )

hunter_config(
    GTest
    VERSION 1.8.0-hunter-p11
    CMAKE_ARGS "CMAKE_CXX_FLAGS=-Wno-deprecated-copy"
)

hunter_config(
    spdlog
    URL https://github.com/gabime/spdlog/archive/v1.4.2.zip
    SHA1 4b10e9aa17f7d568e24f464b48358ab46cb6f39c
)

hunter_config(
    tsl_hat_trie
    URL https://github.com/masterjedy/hat-trie/archive/343e0dac54fc8491065e8a059a02db9a2b1248ab.zip
    SHA1 7b0051e9388d629f382752dd6a12aa8918cdc022
)

hunter_config(
        Boost.DI
        URL https://github.com/masterjedy/di/archive/a20eebb6decfba55867a6920e00443a9485f50cd.zip
        SHA1 459660ae0f34a95eccc738f6b98195a2650185e1
        # disable building examples and tests, disable testing
        CMAKE_ARGS BOOST_DI_OPT_BUILD_TESTS=OFF BOOST_DI_OPT_BUILD_EXAMPLES=OFF
)

hunter_config(
    SQLiteModernCpp
    URL https://github.com/soramitsu/libp2p-sqlite-modern-cpp/archive/fc3b700064cb57ab6b598c9bc7a12b2842f78da2.zip
    SHA1 d913f2a0360892a30bc7cd8820a0475800b47d76
)
