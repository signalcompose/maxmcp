/**
    @file uuid_generator.cpp
    MaxMCP - UUID Generator Implementation

    @ingroup maxmcp
*/

#include "uuid_generator.h"

#include <random>
#include <sstream>

std::string generate_uuid(size_t length) {
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

    std::string uuid;
    uuid.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        uuid += alphanum[dis(gen)];
    }

    return uuid;
}

std::string remove_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');

    if (dot_pos == std::string::npos) {
        // No extension found
        return filename;
    }

    return filename.substr(0, dot_pos);
}

std::string generate_patch_id(const std::string& patcher_name, size_t uuid_length) {
    // Remove extension from patcher name
    std::string name = remove_extension(patcher_name);

    // Handle empty or "Untitled" patches
    if (name.empty() || name == "Untitled") {
        name = "patch";
    }

    // Generate UUID suffix
    std::string uuid = generate_uuid(uuid_length);

    // Combine: {name}_{uuid}
    return name + "_" + uuid;
}
