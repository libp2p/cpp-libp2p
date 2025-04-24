# specify GITHUB_HUNTER_TOKEN and GITHUB_HUNTER_USERNAME to automatically upload binary cache to github.com/qdrvm/hunter-binary-cache
# https://docs.hunter.sh/en/latest/user-guides/hunter-user/github-cache-server.html
string(COMPARE EQUAL "$ENV{GITHUB_HUNTER_TOKEN}" "" password_is_empty)
string(COMPARE EQUAL "$ENV{GITHUB_HUNTER_USERNAME}" "" username_is_empty)

# binary cache can be uploaded to qdrvm/hunter-binary-cache so others will not build same dependencies twice
if (NOT password_is_empty AND NOT username_is_empty)
  option(HUNTER_RUN_UPLOAD "Upload cache binaries" YES)
  message("Binary cache uploading is ENABLED.")
else()
    option(HUNTER_RUN_UPLOAD "Upload cache binaries" NO)
    message("Binary cache uploading is DISABLED.")
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
    "https://github.com/qdrvm/hunter-binary-cache"
    CACHE
    STRING
    "Binary cache server"
)

include(${CMAKE_CURRENT_LIST_DIR}/HunterGate.cmake)

HunterGate(
    URL  https://github.com/qdrvm/hunter/archive/refs/tags/v0.25.3-qdrvm36.tar.gz
    SHA1 15ca50249cd99e107cb06dc5978fe71118d79a43
    LOCAL
)
