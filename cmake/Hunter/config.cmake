hunter_config(
    Boost
    VERSION 1.70.0-p0
    )

hunter_config(GSL
    URL https://github.com/microsoft/GSL/archive/v2.0.0.tar.gz
    SHA1 9bbdea551b38d7d09ab7aa2e89b91a66dd032b4a
    CMAKE_ARGS GSL_TEST=OFF
    )
