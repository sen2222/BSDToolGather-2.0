#ifndef LIBUSB_MINGW_CONFIG_H
#define LIBUSB_MINGW_CONFIG_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#define DEFAULT_VISIBILITY __attribute__((visibility("default")))
#define ENABLE_LOGGING 1
#define PLATFORM_WINDOWS 1
#define PRINTF_FORMAT(a, b) __attribute__((__format__(__printf__, a, b)))

#endif // LIBUSB_MINGW_CONFIG_H
