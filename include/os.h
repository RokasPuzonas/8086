
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
	#define IS_WINDOWS
#elif defined(__linux__)
	#define IS_LINUX
#else
	#error Unable to determine platform
#endif

#if defined(IS_LINUX)
    #include <stdio.h>
    #define MAX_PATH_SIZE PATH_MAX
#elif defined(IS_WINDOWS)
    #define MAX_PATH_SIZE 260
#endif