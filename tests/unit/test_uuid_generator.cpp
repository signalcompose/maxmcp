/**
 * @file test_uuid_generator.cpp
 * Unit tests for UUID Generator
 */

#include <gtest/gtest.h>
#include "uuid_generator.h"
#include <set>

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
