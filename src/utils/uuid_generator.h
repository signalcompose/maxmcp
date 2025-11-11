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

/**
 * @brief Generate a patch ID from patcher name and UUID
 *
 * Generates patch ID in format: {patchname}_{uuid8}
 * Examples: "synth_a7f2b3c1", "myeffect_d4e5f6a7"
 *
 * @param patcher_name Full patcher name (e.g., "synth.maxpat")
 * @param uuid_length Length of UUID suffix (default: 8)
 * @return Formatted patch ID
 */
std::string generate_patch_id(const std::string& patcher_name, size_t uuid_length = 8);

/**
 * @brief Remove file extension from filename
 *
 * Utility function to strip extension from patcher name.
 * Examples: "synth.maxpat" -> "synth", "effect.maxhelp" -> "effect"
 *
 * @param filename Filename with extension
 * @return Filename without extension
 */
std::string remove_extension(const std::string& filename);

#endif  // UUID_GENERATOR_H
