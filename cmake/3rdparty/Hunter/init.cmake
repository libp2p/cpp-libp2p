# specify GITHUB_HUNTER_TOKEN and GITHUB_HUNTER_USERNAME to automatically upload binary cache to github.com/soramitsu/hunter-binary-cache
# https://docs.hunter.sh/en/latest/user-guides/hunter-user/github-cache-server.html
string(COMPARE EQUAL "$ENV{GITHUB_HUNTER_TOKEN}" "" password_is_empty)
string(COMPARE EQUAL "$ENV{GITHUB_HUNTER_USERNAME}" "" username_is_empty)

# binary cache can be uploaded to soramitsu/hunter-binary-cache so others will not build same dependencies twice
if (NOT password_is_empty AND NOT username_is_empty)
  option(HUNTER_RUN_UPLOAD "Upload cache binaries" YES)
  message("Binary cache uploading is ENABLED.")
endif ()

set(
    HUNTER_PASSWORDS_PATH
    "${CMAKE_CURRENT_LIST_DIR}/passwords.cmake"
    CACHE
    FILEPATH
    "Hunter passwords"
)

set(
    HUNTER_CACHE_SERVERS
    "https://github.com/soramitsu/hunter-binary-cache;https://github.com/Warchant/hunter-binary-cache;https://github.com/elucideye/hunter-cache;https://github.com/ingenue/hunter-cache"
    CACHE
    STRING
    "Binary cache server"
)


include(${CMAKE_CURRENT_LIST_DIR}/HunterGate.cmake)
