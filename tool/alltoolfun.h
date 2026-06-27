#ifndef ALLTOOLFUN
#define ALLTOOLFUN

#include <QtGlobal>
#include "ConfigBase/configbase.h"
#include "SystemLog/logoutput.h"

#define CONFIG_READ_INT(section, key, outValue) ConfigBase::getInstance()->ConfigReadInt(section, key, outValue)
#define CONFIG_WRITE_INT(section, key, inValue) ConfigBase::getInstance()->ConfigWriteInt(section, key, inValue)
#define CONFIG_READ_STRING(section, key, outValue) ConfigBase::getInstance()->ConfigReadString(section, key, outValue)
#define CONFIG_WRITE_STRING(section, key, inValue) ConfigBase::getInstance()->ConfigWriteString(section, key, inValue)


#define BSD_LOG(level, message) LogOutput::getInstance()->logOut(level, message)

#define BSD_LOG_DEBUG(level, message) \
    LogOutput::getInstance()->logOut(level, \
        QString("(%2)%3: %4").arg(__FUNCTION__).arg(__LINE__).arg(message))


#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    #define IS_LITTLE_ENDIAN 1
    #define IS_BIG_ENDIAN 0
#else
    #define IS_LITTLE_ENDIAN 0
    #define IS_BIG_ENDIAN 1
#endif

#define SWAP16(x) ((((x) >> 8) & 0x00FF) | (((x) << 8) & 0xFF00))
#define SWAP32(x) ((((x) >> 24) & 0x000000FF) | (((x) >> 8) & 0x0000FF00) | \
                   (((x) << 8) & 0x00FF0000) | (((x) << 24) & 0xFF000000))

#if IS_LITTLE_ENDIAN
    #define TO_LE16(x) (x)
    #define TO_LE32(x) (x)
    #define TO_BE16(x) SWAP16(x)
    #define TO_BE32(x) SWAP32(x)
    // 小端主机：大端序→主机序需交换，小端序→主机序直接返回
    #define BE_TO_HOST16(x) SWAP16(x)
    #define BE_TO_HOST32(x) SWAP32(x)
    #define LE_TO_HOST16(x) (x)
    #define LE_TO_HOST32(x) (x)
#else
    #define TO_LE16(x) SWAP16(x)
    #define TO_LE32(x) SWAP32(x)
    #define TO_BE16(x) (x)
    #define TO_BE32(x) (x)
    // 大端主机：大端序→主机序直接返回，小端序→主机序需交换
    #define BE_TO_HOST16(x) (x)
    #define BE_TO_HOST32(x) (x)
    #define LE_TO_HOST16(x) SWAP16(x)
    #define LE_TO_HOST32(x) SWAP32(x)
#endif




#endif
