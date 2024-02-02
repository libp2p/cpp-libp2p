#pragma once

// Generic helper definitions for shared library support
#if defined _WIN32 || defined __CYGWIN__
  #define LIBP2P_QUIC_HELPER_DLL_IMPORT __declspec(dllimport)
  #define LIBP2P_QUIC_HELPER_DLL_EXPORT __declspec(dllexport)
  #define LIBP2P_QUIC_HELPER_DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define LIBP2P_QUIC_HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
    #define LIBP2P_QUIC_HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
    #define LIBP2P_QUIC_HELPER_DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define LIBP2P_QUIC_HELPER_DLL_IMPORT
    #define LIBP2P_QUIC_HELPER_DLL_EXPORT
    #define LIBP2P_QUIC_HELPER_DLL_LOCAL
  #endif
#endif

// Now we use the generic helper definitions above to define LIBP2P_QUIC_API and LIBP2P_QUIC_LOCAL.
// LIBP2P_QUIC_API is used for the public API symbols. It either DLL imports or DLL exports (or does nothing for static build)
// LIBP2P_QUIC_LOCAL is used for non-api symbols.

// use CMAKE_BUILD_SHARED_LIBS here ?!
//#ifdef BUILD_SHARED_LIBS // defined if LIBP2P is compiled as a DLL
  #ifdef LIBP2P_QUIC_DLL_EXPORTS // defined if we are building the LIBP2P DLL (instead of using it)
    #define LIBP2P_QUIC_API LIBP2P_QUIC_HELPER_DLL_EXPORT
    #define LIBP2P_QUIC_API_FUNC extern LIBP2P_QUIC_HELPER_DLL_EXPORT
  #else
    #define LIBP2P_QUIC_API LIBP2P_QUIC_HELPER_DLL_IMPORT
    #define LIBP2P_QUIC_API_FUNC  extern LIBP2P_QUIC_HELPER_DLL_IMPORT
  #endif // LIBP2P_QUIC_DLL_EXPORTS
  #define LIBP2P_QUIC_LOCAL LIBP2P_QUIC_HELPER_DLL_LOCAL
  #define LIBP2P_QUIC_LOCAL_FUNC extern LIBP2P_QUIC_HELPER_DLL_LOCAL
/*#else // LIBP2P_QUIC_DLL is not defined: this means LIBP2P is a static lib.
  #define LIBP2P_QUIC_API_FUNC
  #define LIBP2P_QUIC_API
  #define LIBP2P_QUIC_LOCAL
  #define LIBP2P_QUIC_LOCAL_FUNC
#endif // LIBP2P_QUIC_DLL */