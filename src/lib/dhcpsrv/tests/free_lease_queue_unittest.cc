// Copyright (C) 2020 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <config.h>
#include <dhcpsrv/free_lease_queue.h>

#include <gtest/gtest.h>

using namespace isc;
using namespace isc::asiolink;
using namespace isc::dhcp;

namespace {

// This test verifies that an exception is thrown upon an attempt to
// create an address range from invalid values and that no exception
// is thrown when the values are correct.
TEST(FreeLeaseQueueRangeTest, constructorWithInvalidValues) {
    FreeLeaseQueue lq;

    // The start address must be lower or equal the end address.
    EXPECT_THROW(FreeLeaseQueue::Range(IOAddress("192.0.2.100"), IOAddress("192.0.2.99")),
                 BadValue);
    // The start and end address must be of the same family.
    EXPECT_THROW(FreeLeaseQueue::Range(IOAddress("192.0.2.100"), IOAddress("2001:db8:1::1")),
                 BadValue);
    // It is allowed to create address range with a single IP address.
    EXPECT_NO_THROW(FreeLeaseQueue::Range(IOAddress("192.0.2.100"), IOAddress("192.0.2.100")));
}

// This test verifies that it is not allowed to add a range that overlaps with
// any existing range.
TEST(FreeLeaseQueueTest, addRangeOverlapping) {
    FreeLeaseQueue lq;
    // Add the initial range. This should succeed.
    ASSERT_NO_THROW(lq.addRange(IOAddress("192.0.2.10"), IOAddress("192.0.3.100")));

    // Let's assume the following naming convention:
    // - r1s - start of the first range added.
    // - r1e - end of the first range added.
    // - r2s - start of the second range (will be added later in this test).
    // - r2e - end of the second range (will be added later in this test).
    // - ns - start of the new range colliding with existing ones.
    // - ne - end of the new range colliding with existing ones.
    // - #### overlap
    // - ns/ne/r1s - overlap on the edges of the ranges (single address shared).

    // r1s    ns####r2s    ne
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.50"), IOAddress("192.0.3.199")),
                 BadValue);

    // ns     r1s###ne     r1s
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.5"), IOAddress("192.0.2.100")),
                 BadValue);

    // r1s    ns####ne     r2s
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.50"), IOAddress("192.0.3.50")),
                 BadValue);
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.5"), IOAddress("192.0.3.200")),
                 BadValue);

    // ns    ne/r1s    r1e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.5"), IOAddress("192.0.2.10")),
                 BadValue);

    // r1s   r1e/ns    ne
    EXPECT_THROW(lq.addRange(IOAddress("192.0.3.100"), IOAddress("192.0.3.105")),
                 BadValue);

    // ns/ne/r1s    r1e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.10"), IOAddress("192.0.2.10")),
                 BadValue);

    // r1s    r1e/ns/ne
    EXPECT_THROW(lq.addRange(IOAddress("192.0.3.100"), IOAddress("192.0.3.100")),
                 BadValue);

    // Add another range, marked as r2s, r2e.
    ASSERT_NO_THROW(lq.addRange(IOAddress("192.0.4.10"), IOAddress("192.0.5.100")));

    // r1s    ns####r1e    ne    r2s    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.50"), IOAddress("192.0.3.199")),
                 BadValue);

    // r1s    r1e    r2s    ns####r2e    ne
    EXPECT_THROW(lq.addRange(IOAddress("192.0.4.50"), IOAddress("192.0.5.199")),
                 BadValue);

    // r1s    ns####r2s####r2e    ne
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.50"), IOAddress("192.0.5.199")),
                 BadValue);

    // ns    r1s####ne    r1e    r2s    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.5"), IOAddress("192.0.2.100")),
                 BadValue);

    // r1s    r1e    ns    r2s####ne    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.4.5"), IOAddress("192.0.4.100")),
                 BadValue);

    // ns    r1s####r1e    r2s####ne    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.5"), IOAddress("192.0.4.100")),
                 BadValue);

    // r1s    ns####ne    r1e    r2s    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.50"), IOAddress("192.0.3.50")),
                 BadValue);

    // r1s    r1e    r2s    ns####ne    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.4.50"), IOAddress("192.0.5.50")),
                 BadValue);

    // r1s    ns####r1e    r2s####ne    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.50"), IOAddress("192.0.5.50")),
                 BadValue);

    // ns    r1s    r1e   ne    r2s    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.5"), IOAddress("192.0.3.200")),
                 BadValue);

    // r1s    r1e    ns    r2s####r2e   ne
    EXPECT_THROW(lq.addRange(IOAddress("192.0.4.5"), IOAddress("192.0.5.200")),
                 BadValue);

    // ns     r1s####r1e    r2s####r2e    ne
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.5"), IOAddress("192.0.5.200")),
                 BadValue);

    // ns     ne/r1s     r1e    r2s    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.2.5"), IOAddress("192.0.2.10")),
                 BadValue);
    // r1s    r1e/ns     ne     r2s    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.3.100"), IOAddress("192.0.3.105")),
                 BadValue);
    // r1s    r1e    ns    ne/r2s    r2e
    EXPECT_THROW(lq.addRange(IOAddress("192.0.4.5"), IOAddress("192.0.4.10")),
                 BadValue);
    // r1s    r1e    r2s    r2e/ns    ne
    EXPECT_THROW(lq.addRange(IOAddress("192.0.5.100"), IOAddress("192.0.5.105")),
                 BadValue);
}

// This test verifies that a range can be removed from the container.
TEST(FreeLeaseQueueTest, removeRange) {
    FreeLeaseQueue lq;

    // Add two ranges.
    FreeLeaseQueue::Range range1(IOAddress("192.0.2.99"), IOAddress("192.0.2.100"));
    FreeLeaseQueue::Range range2(IOAddress("192.0.3.99"), IOAddress("192.0.3.100"));
    ASSERT_NO_THROW(lq.addRange(range1));
    ASSERT_NO_THROW(lq.addRange(range2));

    bool removed;

    // Expect true to be returned when the range is removed.
    ASSERT_NO_THROW(removed = lq.removeRange(range1));
    EXPECT_TRUE(removed);

    // An attempt to append an address to the removed range should not succeed.
    EXPECT_FALSE(lq.append(IOAddress("192.0.2.99")));

    // Removing it the second time should return false to indicate that the range
    // was no longer there.
    ASSERT_NO_THROW(removed = lq.removeRange(range1));
    EXPECT_FALSE(removed);

    // Same for the second range.
    ASSERT_NO_THROW(removed = lq.removeRange(range2));
    EXPECT_TRUE(removed);

    ASSERT_NO_THROW(removed = lq.removeRange(range2));
    EXPECT_FALSE(removed);
}

// This test verifies that an attempt to use an address from outside the
// given range throws and that an attempt to use non-existing in-range
// address returns false.
TEST(FreeLeaseQueueTest, useInvalidAddress) {
    FreeLeaseQueue::Range range(IOAddress("192.0.2.99"), IOAddress("192.0.2.100"));

    FreeLeaseQueue lq;
    ASSERT_NO_THROW(lq.addRange(range));

    // Using out of range address.
    EXPECT_THROW(lq.use(range, IOAddress("192.0.2.1")), BadValue);

    // Using in-range address but not existing in the container.
    bool used = true;
    ASSERT_NO_THROW(used = lq.use(range, IOAddress("192.0.2.99")));
    EXPECT_FALSE(used);
}

// Check that duplicates are eliminated when appending free address to
// the range queue.
TEST(FreeLeaseQueueTest, appendDuplicates) {
    FreeLeaseQueue lq;

    FreeLeaseQueue::Range range(IOAddress("192.0.2.1"), IOAddress("192.0.2.255"));
    ASSERT_NO_THROW(lq.addRange(range));

    ASSERT_NO_THROW(lq.append(range, IOAddress("192.0.2.10")));
    // Append the duplicate of the first address.
    ASSERT_NO_THROW(lq.append(range, IOAddress("192.0.2.10")));
    ASSERT_NO_THROW(lq.append(range, IOAddress("192.0.2.5")));

    IOAddress next(0);
    // The first address should be returned first.
    ASSERT_NO_THROW(next = lq.next(range));
    EXPECT_EQ("192.0.2.10", next.toText());

    // The duplicate should not be returned. Instead, the second address should
    // be returned.
    ASSERT_NO_THROW(next = lq.next(range));
    EXPECT_EQ("192.0.2.5", next.toText());
}

// This test verifies that it is possible to pick next address from the given
// range.
TEST(FreeLeaseQueueTest, next) {
    FreeLeaseQueue lq;

    // Let's create two distinct address ranges.
    FreeLeaseQueue::Range range1(IOAddress("192.0.2.1"), IOAddress("192.0.2.255"));
    FreeLeaseQueue::Range range2(IOAddress("192.0.3.1"), IOAddress("192.0.3.255"));
    ASSERT_NO_THROW(lq.addRange(range1));
    ASSERT_NO_THROW(lq.addRange(range2));

    // Append some IP addresses to those address ranges.
    ASSERT_NO_THROW(lq.append(FreeLeaseQueue::Range(range1), IOAddress("192.0.2.10")));
    ASSERT_NO_THROW(lq.append(FreeLeaseQueue::Range(range1), IOAddress("192.0.2.5")));
    ASSERT_NO_THROW(lq.append(FreeLeaseQueue::Range(range2), IOAddress("192.0.3.23")));
    ASSERT_NO_THROW(lq.append(FreeLeaseQueue::Range(range2), IOAddress("192.0.3.46")));

    // Get the first address from the first range.
    IOAddress next(0);
    ASSERT_NO_THROW(next = lq.next(range1));
    EXPECT_EQ("192.0.2.10", next.toText());

    // Get the next one.
    ASSERT_NO_THROW(next = lq.next(range1));
    EXPECT_EQ("192.0.2.5", next.toText());

    // After iterating over all addresses we should get the first one again.
    ASSERT_NO_THROW(next = lq.next(range1));
    EXPECT_EQ("192.0.2.10", next.toText());

    // Repeat that test for the second address range.
    ASSERT_NO_THROW(next = lq.next(range2));
    EXPECT_EQ("192.0.3.23", next.toText());

    ASSERT_NO_THROW(next = lq.next(range2));
    EXPECT_EQ("192.0.3.46", next.toText());

    ASSERT_NO_THROW(next = lq.next(range2));
    EXPECT_EQ("192.0.3.23", next.toText());

    // Use (remove) the address from the first range.
    bool used = false;
    ASSERT_NO_THROW(used = lq.use(range1, IOAddress("192.0.2.5")));
    EXPECT_TRUE(used);

    // We should now be getting the sole address from that range.
    ASSERT_NO_THROW(next = lq.next(range1));
    EXPECT_EQ("192.0.2.10", next.toText());

    ASSERT_NO_THROW(next = lq.next(range1));
    EXPECT_EQ("192.0.2.10", next.toText());

    // Check the same for the second range.
    ASSERT_NO_THROW(lq.use(range2, IOAddress("192.0.3.46")));

    ASSERT_NO_THROW(next = lq.next(range2));
    EXPECT_EQ("192.0.3.23", next.toText());
}

// This test verifies that it is possible to pop next address from the given
// range.

TEST(FreeLeaseQueueTest, pop) {
    FreeLeaseQueue lq;

    // Let's create two distinct address ranges.
    FreeLeaseQueue::Range range1(IOAddress("192.0.2.1"), IOAddress("192.0.2.255"));
    FreeLeaseQueue::Range range2(IOAddress("192.0.3.1"), IOAddress("192.0.3.255"));
    ASSERT_NO_THROW(lq.addRange(range1));
    ASSERT_NO_THROW(lq.addRange(range2));

    // Append some IP addresses to those address ranges.
    ASSERT_NO_THROW(lq.append(FreeLeaseQueue::Range(range1), IOAddress("192.0.2.10")));
    ASSERT_NO_THROW(lq.append(FreeLeaseQueue::Range(range1), IOAddress("192.0.2.5")));
    ASSERT_NO_THROW(lq.append(FreeLeaseQueue::Range(range2), IOAddress("192.0.3.23")));
    ASSERT_NO_THROW(lq.append(FreeLeaseQueue::Range(range2), IOAddress("192.0.3.46")));

    // Pop first from first range.
    IOAddress next(0);
    ASSERT_NO_THROW(next = lq.pop(range1));
    EXPECT_EQ("192.0.2.10", next.toText());

    // Pop second from the first range.
    ASSERT_NO_THROW(next = lq.pop(range1));
    EXPECT_EQ("192.0.2.5", next.toText());

    // After iterating over all addresses we should get empty queue.
    ASSERT_NO_THROW(next = lq.pop(range1));
    EXPECT_TRUE(next.isV4Zero());

    // Repeat that test for the second address range.
    ASSERT_NO_THROW(next = lq.pop(range2));
    EXPECT_EQ("192.0.3.23", next.toText());

    ASSERT_NO_THROW(next = lq.pop(range2));
    EXPECT_EQ("192.0.3.46", next.toText());

    ASSERT_NO_THROW(next = lq.pop(range2));
    EXPECT_TRUE(next.isV4Zero());
}

// Check that out of bounds address can't be appended to the range.
TEST(FreeLeaseQueueTest, nextRangeMismatch) {
    FreeLeaseQueue::Range range(IOAddress("192.0.2.1"), IOAddress("192.0.2.255"));

    FreeLeaseQueue lq;
    EXPECT_THROW(lq.append(FreeLeaseQueue::Range(range), IOAddress("192.0.3.1")),
                 isc::BadValue);
}

// Check that it is possible to return an address to the range and that the
// appropriate range is detected.
TEST(FreeLeaseQueueTest, detectRange) {
    FreeLeaseQueue lq;

    // Create three ranges.
    FreeLeaseQueue::Range range1(IOAddress("192.0.2.1"), IOAddress("192.0.2.255"));
    FreeLeaseQueue::Range range2(IOAddress("192.0.3.1"), IOAddress("192.0.3.255"));
    FreeLeaseQueue::Range range3(IOAddress("10.0.0.1"), IOAddress("10.8.1.45"));
    ASSERT_NO_THROW(lq.addRange(range1));
    ASSERT_NO_THROW(lq.addRange(range2));
    ASSERT_NO_THROW(lq.addRange(range3));

    // Append some addresses matching the first range.
    ASSERT_NO_THROW(lq.append(IOAddress("192.0.2.7")));
    ASSERT_NO_THROW(lq.append(IOAddress("192.0.2.1")));
    ASSERT_NO_THROW(lq.append(IOAddress("192.0.2.255")));

    // Make sure that these addresses have been appended to that range.
    IOAddress next(0);
    ASSERT_NO_THROW(next = lq.next(range1));
    EXPECT_EQ("192.0.2.7", next.toText());

    ASSERT_NO_THROW(next = lq.next(range1));
    EXPECT_EQ("192.0.2.1", next.toText());

    ASSERT_NO_THROW(next = lq.next(range1));
    EXPECT_EQ("192.0.2.255", next.toText());

    // Append some addresses matching the remaining two ranges.
    ASSERT_NO_THROW(lq.append(IOAddress("10.5.4.3")));
    ASSERT_NO_THROW(lq.append(IOAddress("192.0.3.98")));

    ASSERT_NO_THROW(next = lq.next(range3));
    EXPECT_EQ("10.5.4.3", next.toText());

    ASSERT_NO_THROW(next = lq.next(range2));
    EXPECT_EQ("192.0.3.98", next.toText());

    // Appending out of bounds address should not succeed.
    EXPECT_FALSE(lq.append(IOAddress("10.0.0.0")));
}

// This test verifies that false is returned if the specified address to be
// appended does not belong to any of the existing ranges.
TEST(FreeLeaseQueueTest, detectRangeFailed) {
    FreeLeaseQueue::Range range(IOAddress("192.0.2.1"), IOAddress("192.0.2.255"));

    FreeLeaseQueue lq;
    ASSERT_NO_THROW(lq.addRange(range));

    EXPECT_FALSE(lq.append(IOAddress("192.0.3.7")));
}

// This test verifies that it is possible to append IP addresses to the
// selected range via random access index.
TEST(FreeLeaseQueueTest, appendThroughRangeIndex) {
    FreeLeaseQueue lq;

    FreeLeaseQueue::Range range1(IOAddress("192.0.2.1"), IOAddress("192.0.2.255"));
    FreeLeaseQueue::Range range2(IOAddress("192.0.3.1"), IOAddress("192.0.3.255"));
    FreeLeaseQueue::Range range3(IOAddress("10.0.0.1"), IOAddress("10.8.1.45"));
    ASSERT_NO_THROW(lq.addRange(range1));
    ASSERT_NO_THROW(lq.addRange(range2));
    ASSERT_NO_THROW(lq.addRange(range3));

    uint64_t index1 = 0;
    ASSERT_NO_THROW(index1 = lq.getRangeIndex(range1));
    uint64_t index2 = 0;
    ASSERT_NO_THROW(index2 = lq.getRangeIndex(range2));
    uint64_t index3 = 0;
    ASSERT_NO_THROW(index3 = lq.getRangeIndex(range3));

    EXPECT_NE(index1, index2);
    EXPECT_NE(index2, index3);
    EXPECT_NE(index1, index3);

    ASSERT_NO_THROW(lq.append(index1, IOAddress("192.0.2.50")));
    ASSERT_NO_THROW(lq.append(index2, IOAddress("192.0.3.50")));
    ASSERT_NO_THROW(lq.append(index3, IOAddress("10.1.1.50")));

    ASSERT_THROW(lq.append(index2, IOAddress("10.1.1.51")), BadValue);
    ASSERT_THROW(lq.append(index3, IOAddress("192.0.3.34")), BadValue);
}

} // end of anonymous namespace
