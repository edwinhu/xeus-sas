#ifndef XEUS_SAS_CONFIG_HPP
#define XEUS_SAS_CONFIG_HPP

// Project version
#define XEUS_SAS_VERSION_MAJOR 0
#define XEUS_SAS_VERSION_MINOR 1
#define XEUS_SAS_VERSION_PATCH 0

// Default SAS path (if found during build)
#ifdef DEFAULT_SAS_PATH
    #define XEUS_SAS_DEFAULT_PATH DEFAULT_SAS_PATH
#else
    #define XEUS_SAS_DEFAULT_PATH ""
#endif

namespace xeus_sas
{
    // Version information
    constexpr const char* version = "0.1.0";
    constexpr int version_major = XEUS_SAS_VERSION_MAJOR;
    constexpr int version_minor = XEUS_SAS_VERSION_MINOR;
    constexpr int version_patch = XEUS_SAS_VERSION_PATCH;

    // Default SAS path
    constexpr const char* default_sas_path = XEUS_SAS_DEFAULT_PATH;
}

#endif // XEUS_SAS_CONFIG_HPP
