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
    spdlog
    URL https://github.com/gabime/spdlog/archive/v1.4.2.zip
    SHA1 f71ea50c7c50ecb7ca0502983ce4542aa6dba0c4
)
