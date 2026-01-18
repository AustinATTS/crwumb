#ifndef UWUCC_PLATFORM_H
#define UWUCC_PLATFORM_H

//smartass
#if defined(__APPLE__) && defined(__MACH__)
    #define UWUCC_PLATFORM_MACOS 1
    #define UWUCC_PLATFORM_NAME "macOS"
#elif defined(__linux__)
    #define UWUCC_PLATFORM_LINUX 1
    #define UWUCC_PLATFORM_NAME "Linux"
#else
    #error "Unsupported platform"
#endif


#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
    #define UWUCC_ARCH_X86_64 1
    #define UWUCC_ARCH_NAME "x86-64"
    #define UWUCC_REGISTER_WIDTH 64
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
    #define UWUCC_ARCH_ARM64 1
    #define UWUCC_ARCH_NAME "ARM64"
    #define UWUCC_REGISTER_WIDTH 64
#else
    #error "Unsupported architecture"
#endif


#if UWUCC_REGISTER_WIDTH == 64
    #define UWUCC_PTR_SIZE 8
#else
    #define UWUCC_PTR_SIZE 4
#endif


#ifdef UWUCC_PLATFORM_MACOS
    #define UWUCC_ASM_PREFIX "_"
    #define UWUCC_LINKER "ld"
#else
    #define UWUCC_ASM_PREFIX ""
    #define UWUCC_LINKER "ld"
#endif

#endif
