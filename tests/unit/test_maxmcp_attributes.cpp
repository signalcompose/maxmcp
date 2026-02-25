/**
 * @file test_maxmcp_attributes.cpp
 * Unit tests for maxmcp client object attribute logic (@alias, @group)
 *
 * Note: These tests focus on the UUID generation and patch ID logic.
 * Max SDK attribute handling (gensym, attr_args_process) cannot be tested
 * without Max environment, so we test the core logic that drives the attributes.
 */

#include "uuid_generator.h"

#include <gtest/gtest.h>

/**
 * Test Fixture for maxmcp attributes logic
 */
class MaxMCPAttributesTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // Test setup if needed
    }

    void TearDown() override {
        // Test cleanup if needed
    }

    // Helper to simulate patch_id generation logic from maxmcp_new
    std::string generate_test_patch_id(const std::string& patcher_name,
                                       const std::string& alias = "") {
        if (!alias.empty()) {
            // Alias overrides auto-generated ID (simulates maxmcp_new logic)
            return alias;
        } else {
            // Auto-generate: patchname_uuid8
            return generate_patch_id(patcher_name, 8);
        }
    }
};

/**
 * Test: Default patch ID generation (no alias)
 * Simulates: [maxmcp] with no @alias attribute
 */
TEST_F(MaxMCPAttributesTest, DefaultPatchIDGeneration) {
    auto patch_id = generate_test_patch_id("synth.maxpat");

    // Should start with "synth_"
    EXPECT_TRUE(patch_id.rfind("synth_", 0) == 0)
        << "Default patch ID should start with patcher name";

    // Should be "synth" (5) + "_" (1) + uuid (8) = 14 chars
    EXPECT_EQ(patch_id.length(), 14);
}

/**
 * Test: Custom patch ID via @alias attribute
 * Simulates: [maxmcp @alias my_custom_synth]
 */
TEST_F(MaxMCPAttributesTest, CustomPatchIDWithAlias) {
    auto patch_id = generate_test_patch_id("synth.maxpat", "my_custom_synth");

    // Patch ID should be overridden by alias
    EXPECT_EQ(patch_id, "my_custom_synth");
}

/**
 * Test: Empty alias (should use default patch ID)
 * Simulates: [maxmcp @alias ""]
 */
TEST_F(MaxMCPAttributesTest, EmptyAlias) {
    auto patch_id = generate_test_patch_id("test.maxpat", "");

    // Should use default auto-generated patch ID
    EXPECT_TRUE(patch_id.rfind("test_", 0) == 0);
}

/**
 * Test: Alias enables multiple instances with same patcher name
 * Simulates:
 *   [maxmcp @alias synth_osc]
 *   [maxmcp @alias synth_filter]
 *   [maxmcp]  (no alias)
 * All in the same patch file "synth.maxpat"
 */
TEST_F(MaxMCPAttributesTest, AliasEnablesMultipleInstances) {
    auto patch_id1 = generate_test_patch_id("synth.maxpat", "synth_osc");
    auto patch_id2 = generate_test_patch_id("synth.maxpat", "synth_filter");
    auto patch_id3 = generate_test_patch_id("synth.maxpat");  // No alias

    // All should have different patch IDs
    EXPECT_EQ(patch_id1, "synth_osc");
    EXPECT_EQ(patch_id2, "synth_filter");
    EXPECT_TRUE(patch_id3.rfind("synth_", 0) == 0);

    // Verify they're all unique
    EXPECT_NE(patch_id1, patch_id2);
    EXPECT_NE(patch_id1, patch_id3);
    EXPECT_NE(patch_id2, patch_id3);
}

/**
 * Test: Special characters in alias
 * Simulates: [maxmcp @alias my-synth_v2.0]
 */
TEST_F(MaxMCPAttributesTest, SpecialCharactersInAlias) {
    auto patch_id = generate_test_patch_id("test.maxpat", "my-synth_v2.0");

    // Alias should preserve special characters
    EXPECT_EQ(patch_id, "my-synth_v2.0");
}

/**
 * Test: Untitled patch with alias
 * Simulates: [maxmcp @alias my_patch] in "Untitled" patch
 */
TEST_F(MaxMCPAttributesTest, UntitledPatchWithAlias) {
    auto patch_id = generate_test_patch_id("Untitled", "my_patch");

    // Patch ID should be custom alias (not "patch_xxxxxx")
    EXPECT_EQ(patch_id, "my_patch");
}

/**
 * Test: Untitled patch without alias
 * Simulates: [maxmcp] in "Untitled" patch
 */
TEST_F(MaxMCPAttributesTest, UntitledPatchWithoutAlias) {
    auto patch_id = generate_test_patch_id("Untitled");

    // Should use "patch_" as default prefix (from generate_patch_id logic)
    EXPECT_TRUE(patch_id.rfind("patch_", 0) == 0);
}

/**
 * Test: Long alias names
 * Simulates: [maxmcp @alias very_long_identifier...]
 */
TEST_F(MaxMCPAttributesTest, LongAliasName) {
    std::string long_alias = "very_long_patch_identifier_with_many_characters_for_testing_purposes";
    auto patch_id = generate_test_patch_id("test.maxpat", long_alias);

    // Should accept long alias
    EXPECT_EQ(patch_id, long_alias);
}

/**
 * Test: Display name extraction (remove extension)
 * Tests the remove_extension() helper used in maxmcp_new
 */
TEST_F(MaxMCPAttributesTest, DisplayNameExtraction) {
    EXPECT_EQ(remove_extension("synth.maxpat"), "synth");
    EXPECT_EQ(remove_extension("effect.maxhelp"), "effect");
    EXPECT_EQ(remove_extension("patch.txt"), "patch");
    EXPECT_EQ(remove_extension("noextension"), "noextension");
    EXPECT_EQ(remove_extension("multiple.dots.maxpat"), "multiple.dots");
}

/**
 * Test: Patch ID format consistency
 * Verifies the format: {patchname}_{uuid8}
 */
TEST_F(MaxMCPAttributesTest, PatchIDFormat) {
    auto patch_id = generate_patch_id("synth.maxpat", 8);

    // Should start with "synth_"
    EXPECT_TRUE(patch_id.rfind("synth_", 0) == 0);

    // Total length should be "synth" (5) + "_" (1) + uuid (8) = 14
    EXPECT_EQ(patch_id.length(), 14);

    // UUID part should be 8 characters and alphanumeric
    std::string uuid_part = patch_id.substr(6);  // After "synth_"
    EXPECT_EQ(uuid_part.length(), 8);

    for (char c : uuid_part) {
        EXPECT_TRUE(std::isalnum(c)) << "UUID should be alphanumeric";
    }
}

/**
 * Test: Patch ID uniqueness
 * Multiple calls with same patcher name should produce unique IDs
 */
TEST_F(MaxMCPAttributesTest, PatchIDUniqueness) {
    std::set<std::string> patch_ids;
    const int count = 100;

    for (int i = 0; i < count; i++) {
        auto patch_id = generate_test_patch_id("synth.maxpat");
        patch_ids.insert(patch_id);
    }

    // All 100 patch IDs should be unique
    EXPECT_EQ(patch_ids.size(), count) << "Duplicate patch IDs detected";
}

/**
 * Test: Empty patcher name handling
 */
TEST_F(MaxMCPAttributesTest, EmptyPatcherName) {
    auto patch_id = generate_test_patch_id("");

    // Should use "patch_" as default prefix
    EXPECT_TRUE(patch_id.rfind("patch_", 0) == 0);
}

/**
 * Integration Test: Alias override behavior
 * Documents the complete logic flow in maxmcp_new:
 * 1. Generate auto ID: {patchname}_{uuid8}
 * 2. If @alias is set, override with custom ID
 */
TEST_F(MaxMCPAttributesTest, AliasOverrideBehavior) {
    // Scenario 1: No alias - use auto-generated ID
    auto id_without_alias = generate_test_patch_id("synth.maxpat");
    EXPECT_TRUE(id_without_alias.rfind("synth_", 0) == 0);
    EXPECT_EQ(id_without_alias.length(), 14);

    // Scenario 2: With alias - override with custom ID
    auto id_with_alias = generate_test_patch_id("synth.maxpat", "custom_synth");
    EXPECT_EQ(id_with_alias, "custom_synth");
    EXPECT_NE(id_with_alias.length(), 14);  // Different length from auto ID

    // Verify they're different
    EXPECT_NE(id_without_alias, id_with_alias);
}
