/**
 * @file test_patch_helpers.cpp
 * Unit tests for PatchHelpers utility functions
 */

#include <gtest/gtest.h>

// Provide complete type for t_atom before patch_helpers.h
// (forward declaration alone is insufficient for std::vector<t_atom>)
struct _atom {};
typedef struct _atom t_atom;
#define MAXMCP_TEST_ATOM_DEFINED

#include "patch_helpers.h"

using json = nlohmann::json;

// =============================================================================
// build_text_from_arguments (SDK-independent, fully testable)
// =============================================================================

TEST(BuildTextFromArguments, NullArguments) {
    json args = nullptr;
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args), "");
}

TEST(BuildTextFromArguments, EmptyArray) {
    json args = json::array();
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args), "");
}

TEST(BuildTextFromArguments, NotArray) {
    json args = 42;
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args), "");

    json args_str = "hello";
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args_str), "");

    json args_obj = json::object();
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args_obj), "");
}

TEST(BuildTextFromArguments, SingleString) {
    json args = json::array({"hello"});
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args), "hello");
}

TEST(BuildTextFromArguments, MultipleStrings) {
    json args = json::array({"hello", "world"});
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args), "hello world");
}

TEST(BuildTextFromArguments, SingleInteger) {
    json args = json::array({42});
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args), "42");
}

TEST(BuildTextFromArguments, SingleFloat) {
    json args = json::array({3.14});
    auto result = PatchHelpers::build_text_from_arguments(args);
    EXPECT_TRUE(result.find("3.14") == 0) << "Expected to start with '3.14', got: " << result;
}

TEST(BuildTextFromArguments, MixedTypes) {
    json args = json::array({"set", 440, 1.5});
    auto result = PatchHelpers::build_text_from_arguments(args);
    EXPECT_TRUE(result.find("set 440") == 0) << "Unexpected result: " << result;
}

TEST(BuildTextFromArguments, SpaceSeparation) {
    json args = json::array({"a", "b", "c"});
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args), "a b c");
}

TEST(BuildTextFromArguments, UnsupportedTypeSkipped) {
    // Boolean and object types are silently skipped
    json args = json::array({true, "hello"});
    EXPECT_EQ(PatchHelpers::build_text_from_arguments(args), " hello");
}

// =============================================================================
// Test mode stubs (verify expected stub behavior)
// =============================================================================

TEST(PatchHelpersStubs, FindBoxByVarnameReturnsNull) {
    EXPECT_EQ(PatchHelpers::find_box_by_varname(nullptr, "test"), nullptr);
}

TEST(PatchHelpersStubs, GetInletCountReturnsNegative) {
    EXPECT_EQ(PatchHelpers::get_inlet_count(nullptr), -1);
}

TEST(PatchHelpersStubs, GetOutletCountReturnsNegative) {
    EXPECT_EQ(PatchHelpers::get_outlet_count(nullptr), -1);
}

TEST(PatchHelpersStubs, JsonToAtomsReturnsEmpty) {
    json value = json::array({1, 2, 3});
    auto atoms = PatchHelpers::json_to_atoms(value);
    EXPECT_TRUE(atoms.empty());
}

TEST(PatchHelpersStubs, SetBoxAttributeReturnsFalse) {
    json value = 42;
    EXPECT_FALSE(PatchHelpers::set_box_attribute(nullptr, "test", value));
}

TEST(PatchHelpersStubs, SetTextfieldContentReturnsFalse) {
    EXPECT_FALSE(PatchHelpers::set_textfield_content(nullptr, "test"));
}
