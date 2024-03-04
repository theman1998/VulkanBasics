#pragma once



#if defined(_WIN32) && defined(_MGE_BUILD_DLL)
 /* We are building the Win32 DLL */
 #define MGE_API __declspec(dllexport)
#elif defined(_WIN32) && defined(MGE_DLL)
 /* We are calling the Win32 DLL */
 #define MGE_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_MGE_BUILD_DLL)
 /* We are building the Unix shared library */
 #define MGE_API __attribute__((visibility("default")))
#else
 #define MGE_API
#endif











