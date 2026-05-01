#ifndef GIT_HASH
#define GIT_HASH "undef"
#endif

#ifndef GIT_DIRTY
#define GIT_DIRTY "-dirty"
#endif

__attribute__((section(".version"), used)) const char gbalatro_version[] =
    "GBALATRO-VERSION:" GIT_HASH GIT_DIRTY;
