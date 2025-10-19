/**
    @file uuid_generator.h
    MaxMCP - UUID Generator

    Simple UUID generator for patch IDs.

    @ingroup maxmcp
*/

#ifndef UUID_GENERATOR_H
#define UUID_GENERATOR_H

#include <string>

/**
 * @brief Generate a random UUID-like string
 *
 * Generates a simple random alphanumeric string for use as patch ID.
 * Not cryptographically secure, but sufficient for patch identification.
 *
 * @param length Length of UUID string to generate (default: 8)
 * @return Random alphanumeric string
 */
std::string generate_uuid(size_t length = 8);

#endif // UUID_GENERATOR_H
