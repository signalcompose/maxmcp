/**
    @file uuid_generator.cpp
    MaxMCP - UUID Generator Implementation

    @ingroup maxmcp
*/

#include "uuid_generator.h"
#include <random>
#include <sstream>

std::string generate_uuid(size_t length) {
    static const char alphanum[] =
        "0123456789"
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
