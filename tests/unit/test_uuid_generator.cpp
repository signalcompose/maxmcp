/**
 * @file test_uuid_generator.cpp
 * Unit tests for UUID Generator
 */

#include "uuid_generator.h"

#include <set>

#include <gtest/gtest.h>

/**
 * Test: UUID generation with default length (8)
 */
TEST(UUIDGenerator, DefaultLength) {
    auto uuid = generate_uuid();
    EXPECT_EQ(uuid.length(), 8);
}

/**
 * Test: UUID generation with custom length
 */
TEST(UUIDGenerator, CustomLength) {
    auto uuid4 = generate_uuid(4);
    EXPECT_EQ(uuid4.length(), 4);

    auto uuid16 = generate_uuid(16);
    EXPECT_EQ(uuid16.length(), 16);
}

/**
 * Test: UUID contains only alphanumeric characters
 */
TEST(UUIDGenerator, AlphanumericOnly) {
    auto uuid = generate_uuid(100);

    for (char c : uuid) {
        EXPECT_TRUE(std::isalnum(c)) << "Non-alphanumeric character found: " << c;
    }
}

/**
 * Test: UUID uniqueness (generate 1000 UUIDs, all should be unique)
 */
TEST(UUIDGenerator, Uniqueness) {
    std::set<std::string> uuids;
    const int count = 1000;

    for (int i = 0; i < count; i++) {
        auto uuid = generate_uuid(8);
        uuids.insert(uuid);
    }

    // All 1000 UUIDs should be unique
    EXPECT_EQ(uuids.size(), count) << "Duplicate UUIDs detected";
}

/**
 * Test: Zero length (edge case)
 */
TEST(UUIDGenerator, ZeroLength) {
    auto uuid = generate_uuid(0);
    EXPECT_EQ(uuid.length(), 0);
}

/**
 * Test: Large length (performance test)
 */
TEST(UUIDGenerator, LargeLength) {
    auto uuid = generate_uuid(10000);
    EXPECT_EQ(uuid.length(), 10000);
}

/**
 * Test: Remove extension from filename
 */
TEST(UUIDGenerator, RemoveExtension) {
    EXPECT_EQ(remove_extension("synth.maxpat"), "synth");
    EXPECT_EQ(remove_extension("effect.maxhelp"), "effect");
    EXPECT_EQ(remove_extension("patch.txt"), "patch");
    EXPECT_EQ(remove_extension("noextension"), "noextension");
    EXPECT_EQ(remove_extension("multiple.dots.maxpat"), "multiple.dots");
}

/**
 * Test: Generate patch ID format
 */
TEST(UUIDGenerator, PatchIDFormat) {
    auto patch_id = generate_patch_id("synth.maxpat", 8);

    // Should start with "synth_"
    EXPECT_TRUE(patch_id.rfind("synth_", 0) == 0) << "Patch ID should start with 'synth_'";

    // Total length should be "synth" (5) + "_" (1) + uuid (8) = 14
    EXPECT_EQ(patch_id.length(), 14);

    // UUID part should be 8 characters
    std::string uuid_part = patch_id.substr(6);  // After "synth_"
    EXPECT_EQ(uuid_part.length(), 8);
}

/**
 * Test: Patch ID with different filenames
 */
TEST(UUIDGenerator, PatchIDVariousNames) {
    auto id1 = generate_patch_id("myeffect.maxpat", 8);
    EXPECT_TRUE(id1.rfind("myeffect_", 0) == 0);

    auto id2 = generate_patch_id("master.maxpat", 8);
    EXPECT_TRUE(id2.rfind("master_", 0) == 0);

    auto id3 = generate_patch_id("test", 8);  // No extension
    EXPECT_TRUE(id3.rfind("test_", 0) == 0);
}

/**
 * Test: Untitled patch handling
 */
TEST(UUIDGenerator, UntitledPatch) {
    auto patch_id = generate_patch_id("Untitled", 8);

    // Should use "patch" as default name
    EXPECT_TRUE(patch_id.rfind("patch_", 0) == 0);
}

/**
 * Test: Empty patcher name handling
 */
TEST(UUIDGenerator, EmptyPatcherName) {
    auto patch_id = generate_patch_id("", 8);

    // Should use "patch" as default name
    EXPECT_TRUE(patch_id.rfind("patch_", 0) == 0);
}

/**
 * Test: Patch ID uniqueness (multiple calls with same patcher name)
 */
TEST(UUIDGenerator, PatchIDUniqueness) {
    std::set<std::string> patch_ids;
    const int count = 100;

    for (int i = 0; i < count; i++) {
        auto patch_id = generate_patch_id("synth.maxpat", 8);
        patch_ids.insert(patch_id);
    }

    // All 100 patch IDs should be unique
    EXPECT_EQ(patch_ids.size(), count) << "Duplicate patch IDs detected";
}
