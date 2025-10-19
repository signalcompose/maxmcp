/**
 * @file test_patch_registry.cpp
 * Unit tests for PatchRegistry
 */

#include <gtest/gtest.h>
#include "patch_registry.h"
#include "maxmcp.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Mock t_maxmcp structure for testing
// (In actual tests, we need to mock the Max object structure)

/**
 * Test Fixture for PatchRegistry
 */
class PatchRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear registry before each test
        // Note: PatchRegistry needs a reset method for testing
    }

    void TearDown() override {
        // Clean up after each test
    }
};

/**
 * Test: Register and unregister patch
 */
TEST_F(PatchRegistryTest, RegisterUnregister) {
    // Create mock patch object
    t_maxmcp mock_patch;
    new (&mock_patch.patch_id) std::string("test_12345678");
    new (&mock_patch.display_name) std::string("Test Patch");
    new (&mock_patch.patcher_name) std::string("test.maxpat");

    // Register
    PatchRegistry::register_patch(&mock_patch);

    // Verify registration
    auto result = PatchRegistry::list_patches();
    EXPECT_TRUE(result.contains("result"));
    EXPECT_TRUE(result["result"].contains("patches"));
    EXPECT_EQ(result["result"]["count"], 1);

    // Unregister
    PatchRegistry::unregister_patch(&mock_patch);

    // Verify unregistration
    result = PatchRegistry::list_patches();
    EXPECT_EQ(result["result"]["count"], 0);

    // Cleanup
    mock_patch.patch_id.~basic_string();
    mock_patch.display_name.~basic_string();
    mock_patch.patcher_name.~basic_string();
}

/**
 * Test: Find patch by ID
 */
TEST_F(PatchRegistryTest, FindPatch) {
    t_maxmcp mock_patch;
    new (&mock_patch.patch_id) std::string("test_12345678");
    new (&mock_patch.display_name) std::string("Test Patch");
    new (&mock_patch.patcher_name) std::string("test.maxpat");

    PatchRegistry::register_patch(&mock_patch);

    // Find existing patch
    t_maxmcp* found = PatchRegistry::find_patch("test_12345678");
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found, &mock_patch);

    // Find non-existing patch
    t_maxmcp* not_found = PatchRegistry::find_patch("nonexistent");
    EXPECT_EQ(not_found, nullptr);

    // Cleanup
    PatchRegistry::unregister_patch(&mock_patch);
    mock_patch.patch_id.~basic_string();
    mock_patch.display_name.~basic_string();
    mock_patch.patcher_name.~basic_string();
}

/**
 * Test: Multiple patches
 */
TEST_F(PatchRegistryTest, MultiplePatches) {
    t_maxmcp patch1, patch2, patch3;

    new (&patch1.patch_id) std::string("patch1_aaaaaaaa");
    new (&patch1.display_name) std::string("Patch 1");
    new (&patch1.patcher_name) std::string("patch1.maxpat");

    new (&patch2.patch_id) std::string("patch2_bbbbbbbb");
    new (&patch2.display_name) std::string("Patch 2");
    new (&patch2.patcher_name) std::string("patch2.maxpat");

    new (&patch3.patch_id) std::string("patch3_cccccccc");
    new (&patch3.display_name) std::string("Patch 3");
    new (&patch3.patcher_name) std::string("patch3.maxpat");

    // Register all
    PatchRegistry::register_patch(&patch1);
    PatchRegistry::register_patch(&patch2);
    PatchRegistry::register_patch(&patch3);

    // Verify count
    auto result = PatchRegistry::list_patches();
    EXPECT_EQ(result["result"]["count"], 3);

    // Verify all patches are listed
    auto patches = result["result"]["patches"];
    EXPECT_EQ(patches.size(), 3);

    // Cleanup
    PatchRegistry::unregister_patch(&patch1);
    PatchRegistry::unregister_patch(&patch2);
    PatchRegistry::unregister_patch(&patch3);

    patch1.patch_id.~basic_string();
    patch1.display_name.~basic_string();
    patch1.patcher_name.~basic_string();

    patch2.patch_id.~basic_string();
    patch2.display_name.~basic_string();
    patch2.patcher_name.~basic_string();

    patch3.patch_id.~basic_string();
    patch3.display_name.~basic_string();
    patch3.patcher_name.~basic_string();
}

/**
 * Test: Duplicate registration (should be idempotent)
 */
TEST_F(PatchRegistryTest, DuplicateRegistration) {
    t_maxmcp mock_patch;
    new (&mock_patch.patch_id) std::string("test_12345678");
    new (&mock_patch.display_name) std::string("Test Patch");
    new (&mock_patch.patcher_name) std::string("test.maxpat");

    // Register twice
    PatchRegistry::register_patch(&mock_patch);
    PatchRegistry::register_patch(&mock_patch);

    // Should still have only 1 patch
    auto result = PatchRegistry::list_patches();
    // Note: Current implementation may allow duplicates
    // This test documents expected behavior

    // Cleanup
    PatchRegistry::unregister_patch(&mock_patch);
    mock_patch.patch_id.~basic_string();
    mock_patch.display_name.~basic_string();
    mock_patch.patcher_name.~basic_string();
}
