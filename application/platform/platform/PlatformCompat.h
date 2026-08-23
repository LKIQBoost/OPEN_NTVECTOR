#pragma once

#ifdef _WIN32
  #include <process.h>
  #ifdef __MINGW32__
    #include <sys/types.h>
    using _ssize_t = ssize_t;
  #endif
  #define PLATFORM_GETPID() _getpid()
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <cstdint>
  #define PLATFORM_GETPID() getpid()
  // MSVC 内置类型 __int64 兼容 (unsigned __int64 → unsigned long long)
  #ifndef _MSC_VER
    #define __int64 long long
  #endif
#endif
