/* Platform-specific functions */

#if defined(_WIN32)
    #include <libloaderapi.h>
    #include <windef.h>
    #include <winbase.h>
    typedef HMODULE DLHandle;
#else
    #include <dlfcn.h>
    typedef void* DLHandle;
#endif

static DLHandle platform_dlopen(char* path);
static void* platform_dlsym(DLHandle lib, char* name);
static int platform_dlclose(DLHandle lib); /* Returns zero on success */

/* Contents and size will be non-zero on success. Contents must be freed after use */
static void platform_read(char* path, char** contents, size_t* size);
