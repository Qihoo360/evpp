
#include "evpp/exp.h"
#include "./test_common.h"

#include "evpp/base/ip_address.h"

TEST_UNIT(testIPAddress) {

}


// Helper to stringize an IP address (used to define expectations).
// std::string DumpIPAddress(const IPAddress& v) {
//     std::string out;
//     for (size_t i = 0; i < v.bytes().size(); ++i) {
//         if (i != 0)
//             out.append(",");
//         out.append(base::UintToString(v.bytes()[i]));
//     }
//     return out;
// }

using evpp::base::IPAddress;

TEST_UNIT(ConstructIPv4) {
    H_TEST_EQUAL("127.0.0.1", IPAddress::IPv4Localhost().ToString());

    IPAddress ipv4_ctor(192, 168, 1, 1);
    H_TEST_EQUAL("192.168.1.1", ipv4_ctor.ToString());
}

TEST_UNIT(ConstructIPv6) {
    H_TEST_EQUAL("::1", IPAddress::IPv6Localhost().ToString());

    IPAddress ipv6_ctor(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16);
    H_TEST_EQUAL("102:304:506:708:90a:b0c:d0e:f10", ipv6_ctor.ToString());
}

TEST_UNIT(IsIPVersion) {
    uint8_t addr1[4] = { 192, 168, 0, 1 };
    IPAddress ip_address1(addr1);
    H_TEST_ASSERT(ip_address1.IsIPv4());
    H_TEST_ASSERT(!ip_address1.IsIPv6());

    uint8_t addr2[16] = { 0xFE, 0xDC, 0xBA, 0x98 };
    IPAddress ip_address2(addr2);
    H_TEST_ASSERT(ip_address2.IsIPv6());
    H_TEST_ASSERT(!ip_address2.IsIPv4());

    IPAddress ip_address3;
    H_TEST_ASSERT(!ip_address3.IsIPv6());
    H_TEST_ASSERT(!ip_address3.IsIPv4());
}

TEST_UNIT(IsValid) {
    uint8_t addr1[4] = { 192, 168, 0, 1 };
    IPAddress ip_address1(addr1);
    H_TEST_ASSERT(ip_address1.IsValid());
    H_TEST_ASSERT(!ip_address1.empty());

    uint8_t addr2[16] = { 0xFE, 0xDC, 0xBA, 0x98 };
    IPAddress ip_address2(addr2);
    H_TEST_ASSERT(ip_address2.IsValid());
    H_TEST_ASSERT(!ip_address2.empty());

    uint8_t addr3[5] = { 0xFE, 0xDC, 0xBA, 0x98 };
    IPAddress ip_address3(addr3);
    H_TEST_ASSERT(!ip_address3.IsValid());
    H_TEST_ASSERT(!ip_address3.empty());

    IPAddress ip_address4;
    H_TEST_ASSERT(!ip_address4.IsValid());
    H_TEST_ASSERT(ip_address4.empty());
}

TEST_UNIT(IsZero) {
    uint8_t address1[4] = {};
    IPAddress zero_ipv4_address(address1);
    H_TEST_ASSERT(zero_ipv4_address.IsZero());

    uint8_t address2[4] = { 10 };
    IPAddress non_zero_ipv4_address(address2);
    H_TEST_ASSERT(!non_zero_ipv4_address.IsZero());

    uint8_t address3[16] = {};
    IPAddress zero_ipv6_address(address3);
    H_TEST_ASSERT(zero_ipv6_address.IsZero());

    uint8_t address4[16] = { 10 };
    IPAddress non_zero_ipv6_address(address4);
    H_TEST_ASSERT(!non_zero_ipv6_address.IsZero());

    IPAddress empty_address;
    H_TEST_ASSERT(!empty_address.IsZero());
}

TEST_UNIT(AllZeros) {
    H_TEST_ASSERT(IPAddress::AllZeros(0).empty());

    H_TEST_EQUAL(3u, IPAddress::AllZeros(3).size());
    H_TEST_ASSERT(IPAddress::AllZeros(3).IsZero());

    H_TEST_EQUAL("0.0.0.0", IPAddress::IPv4AllZeros().ToString());
    H_TEST_EQUAL("::", IPAddress::IPv6AllZeros().ToString());
}

TEST_UNIT(ToString) {
    uint8_t addr1[4] = { 0, 0, 0, 0 };
    IPAddress ip_address1(addr1);
    H_TEST_EQUAL("0.0.0.0", ip_address1.ToString());

    uint8_t addr2[4] = { 192, 168, 0, 1 };
    IPAddress ip_address2(addr2);
    H_TEST_EQUAL("192.168.0.1", ip_address2.ToString());

    uint8_t addr3[16] = { 0xFE, 0xDC, 0xBA, 0x98 };
    IPAddress ip_address3(addr3);
    H_TEST_EQUAL("fedc:ba98::", ip_address3.ToString());

    // ToString() shouldn't crash on invalid addresses.
    uint8_t addr4[2];
    IPAddress ip_address4(addr4);
    H_TEST_EQUAL("", ip_address4.ToString());

    IPAddress ip_address5;
    H_TEST_EQUAL("", ip_address5.ToString());
}

// Test that invalid IP literals fail to parse.
TEST_UNIT(AssignFromIPLiteral_FailParse) {
    IPAddress address;

    H_TEST_ASSERT(!address.AssignFromIPLiteral("bad value"));
    H_TEST_ASSERT(!address.AssignFromIPLiteral("bad:value"));
    H_TEST_ASSERT(!address.AssignFromIPLiteral(std::string()));
    H_TEST_ASSERT(!address.AssignFromIPLiteral("192.168.0.1:30"));
    H_TEST_ASSERT(!address.AssignFromIPLiteral("  192.168.0.1  "));
    H_TEST_ASSERT(!address.AssignFromIPLiteral("[::1]"));
}

// Test parsing an IPv4 literal.
TEST_UNIT(AssignFromIPLiteral_IPv4) {
    IPAddress address;
    H_TEST_ASSERT(address.AssignFromIPLiteral("192.168.0.1"));
    //H_TEST_EQUAL("192,168,0,1", DumpIPAddress(address));
    H_TEST_EQUAL("192.168.0.1", address.ToString());
}

// Test parsing an IPv6 literal.
TEST_UNIT(AssignFromIPLiteral_IPv6) {
    IPAddress address;
    H_TEST_ASSERT(address.AssignFromIPLiteral("1:abcd::3:4:ff"));
    //H_TEST_EQUAL("0,1,171,205,0,0,0,0,0,0,0,3,0,4,0,255", DumpIPAddress(address));
    H_TEST_EQUAL("1:abcd::3:4:ff", address.ToString());
}

TEST_UNIT(IsIPv4MappedIPv6) {
    IPAddress ipv4_address;
    H_TEST_ASSERT(ipv4_address.AssignFromIPLiteral("192.168.0.1"));
    H_TEST_ASSERT(!ipv4_address.IsIPv4MappedIPv6());

    IPAddress ipv6_address;
    H_TEST_ASSERT(ipv6_address.AssignFromIPLiteral("::1"));
    H_TEST_ASSERT(!ipv6_address.IsIPv4MappedIPv6());

    IPAddress ipv4mapped_address;
    H_TEST_ASSERT(ipv4mapped_address.AssignFromIPLiteral("::ffff:0101:1"));
    H_TEST_ASSERT(ipv4mapped_address.IsIPv4MappedIPv6());
}

TEST_UNIT(IsEqual) {
    IPAddress ip_address1;
    H_TEST_ASSERT(ip_address1.AssignFromIPLiteral("127.0.0.1"));
    IPAddress ip_address2;
    H_TEST_ASSERT(ip_address2.AssignFromIPLiteral("2001:db8:0::42"));
    IPAddress ip_address3;
    H_TEST_ASSERT(ip_address3.AssignFromIPLiteral("127.0.0.1"));

    H_TEST_ASSERT(ip_address1 != ip_address2);
    H_TEST_ASSERT(ip_address1 == ip_address3);
}

TEST_UNIT(LessThan) {
    // IPv4 vs IPv6
    IPAddress ip_address1;
    H_TEST_ASSERT(ip_address1.AssignFromIPLiteral("127.0.0.1"));
    IPAddress ip_address2;
    H_TEST_ASSERT(ip_address2.AssignFromIPLiteral("2001:db8:0::42"));
    H_TEST_ASSERT(ip_address1 < ip_address2);
    H_TEST_ASSERT(!(ip_address2 < ip_address1));

    // Compare equivalent addresses.
    IPAddress ip_address3;
    H_TEST_ASSERT(ip_address3.AssignFromIPLiteral("127.0.0.1"));
    H_TEST_ASSERT(!(ip_address1 < ip_address3));
    H_TEST_ASSERT(!(ip_address3 < ip_address1));
}

TEST_UNIT(IPAddressToStringWithPort) {
    IPAddress address1;
    H_TEST_ASSERT(address1.AssignFromIPLiteral("0.0.0.0"));
    H_TEST_EQUAL("0.0.0.0:3", IPAddressToStringWithPort(address1, 3));

    IPAddress address2;
    H_TEST_ASSERT(address2.AssignFromIPLiteral("192.168.0.1"));
    H_TEST_EQUAL("192.168.0.1:99", IPAddressToStringWithPort(address2, 99));

    IPAddress address3;
    H_TEST_ASSERT(address3.AssignFromIPLiteral("fedc:ba98::"));
    H_TEST_EQUAL("[fedc:ba98::]:8080", IPAddressToStringWithPort(address3, 8080));

    // ToString() shouldn't crash on invalid addresses.
    IPAddress address4;
    H_TEST_EQUAL("", IPAddressToStringWithPort(address4, 8080));
}

// TEST_UNIT(IPAddressToPackedString) {
//     IPAddress ipv4_address;
//     H_TEST_ASSERT(ipv4_address.AssignFromIPLiteral("4.31.198.44"));
//     std::string expected_ipv4_address("\x04\x1f\xc6\x2c", 4);
//     H_TEST_EQUAL(expected_ipv4_address, IPAddressToPackedString(ipv4_address));
// 
//     IPAddress ipv6_address;
//     H_TEST_ASSERT(ipv6_address.AssignFromIPLiteral("2001:0700:0300:1800::000f"));
//     std::string expected_ipv6_address(
//         "\x20\x01\x07\x00\x03\x00\x18\x00"
//         "\x00\x00\x00\x00\x00\x00\x00\x0f",
//         16);
//     H_TEST_EQUAL(expected_ipv6_address, IPAddressToPackedString(ipv6_address));
// }
// 
// TEST_UNIT(ConvertIPv4ToIPv4MappedIPv6) {
//     IPAddress ipv4_address;
//     H_TEST_ASSERT(ipv4_address.AssignFromIPLiteral("192.168.0.1"));
// 
//     IPAddress ipv6_address = ConvertIPv4ToIPv4MappedIPv6(ipv4_address);
// 
//     // ::ffff:192.168.0.1
//     H_TEST_EQUAL("::ffff:c0a8:1", ipv6_address.ToString());
// }
// 
// TEST_UNIT(ConvertIPv4MappedIPv6ToIPv4) {
//     IPAddress ipv4mapped_address;
//     H_TEST_ASSERT(ipv4mapped_address.AssignFromIPLiteral("::ffff:c0a8:1"));
// 
//     IPAddress expected;
//     H_TEST_ASSERT(expected.AssignFromIPLiteral("192.168.0.1"));
// 
//     IPAddress result = ConvertIPv4MappedIPv6ToIPv4(ipv4mapped_address);
//     H_TEST_EQUAL(expected, result);
// }
// 
// // Test parsing invalid CIDR notation literals.
// TEST_UNIT(ParseCIDRBlock_Invalid) {
//     const char* const bad_literals[] = { "foobar",
//         "",
//         "192.168.0.1",
//         "::1",
//         "/",
//         "/1",
//         "1",
//         "192.168.1.1/-1",
//         "192.168.1.1/33",
//         "::1/-3",
//         "a::3/129",
//         "::1/x",
//         "192.168.0.1//11",
//         "192.168.1.1/+1",
//         "192.168.1.1/ +1",
//         "192.168.1.1/" };
// 
//     for (const auto& bad_literal : bad_literals) {
//         IPAddress ip_address;
//         size_t prefix_length_in_bits;
// 
//         H_TEST_ASSERT(!
//             ParseCIDRBlock(bad_literal, &ip_address, &prefix_length_in_bits));
//     }
// }
// 
// // Test parsing a valid CIDR notation literal.
// TEST_UNIT(ParseCIDRBlock_Valid) {
//     IPAddress ip_address;
//     size_t prefix_length_in_bits;
// 
//     H_TEST_ASSERT(
//         ParseCIDRBlock("192.168.0.1/11", &ip_address, &prefix_length_in_bits));
// 
//     H_TEST_EQUAL("192,168,0,1", DumpIPAddress(ip_address));
//     H_TEST_EQUAL(11u, prefix_length_in_bits);
// 
//     H_TEST_ASSERT(ParseCIDRBlock("::ffff:192.168.0.1/112", &ip_address,
//         &prefix_length_in_bits));
// 
//     H_TEST_EQUAL("0,0,0,0,0,0,0,0,0,0,255,255,192,168,0,1",
//         DumpIPAddress(ip_address));
//     H_TEST_EQUAL(112u, prefix_length_in_bits);
// }

TEST_UNIT(IPAddressStartsWith) {
    IPAddress ipv4_address(192, 168, 10, 5);

    uint8_t ipv4_prefix1[] = { 192, 168, 10 };
    H_TEST_ASSERT(IPAddressStartsWith(ipv4_address, ipv4_prefix1));

    uint8_t ipv4_prefix3[] = { 192, 168, 10, 5 };
    H_TEST_ASSERT(IPAddressStartsWith(ipv4_address, ipv4_prefix3));

    uint8_t ipv4_prefix2[] = { 192, 168, 10, 10 };
    H_TEST_ASSERT(!IPAddressStartsWith(ipv4_address, ipv4_prefix2));

    // Prefix is longer than the address.
    uint8_t ipv4_prefix4[] = { 192, 168, 10, 10, 0 };
    H_TEST_ASSERT(!IPAddressStartsWith(ipv4_address, ipv4_prefix4));

    IPAddress ipv6_address;
    H_TEST_ASSERT(ipv6_address.AssignFromIPLiteral("2a00:1450:400c:c09::64"));

    uint8_t ipv6_prefix1[] = { 42, 0, 20, 80, 64, 12, 12, 9 };
    H_TEST_ASSERT(IPAddressStartsWith(ipv6_address, ipv6_prefix1));

    uint8_t ipv6_prefix2[] = { 41, 0, 20, 80, 64, 12, 12, 9,
        0, 0, 0, 0, 0, 0, 100 };
    H_TEST_ASSERT(!IPAddressStartsWith(ipv6_address, ipv6_prefix2));

    uint8_t ipv6_prefix3[] = { 42, 0, 20, 80, 64, 12, 12, 9,
        0, 0, 0, 0, 0, 0, 0, 100 };
    H_TEST_ASSERT(IPAddressStartsWith(ipv6_address, ipv6_prefix3));

    uint8_t ipv6_prefix4[] = { 42, 0, 20, 80, 64, 12, 12, 9,
        0, 0, 0, 0, 0, 0, 0, 0 };
    H_TEST_ASSERT(!IPAddressStartsWith(ipv6_address, ipv6_prefix4));

    // Prefix is longer than the address.
    uint8_t ipv6_prefix5[] = { 42, 0, 20, 80, 64, 12, 12, 9, 0,
        0, 0, 0, 0, 0, 0, 0, 10 };
    H_TEST_ASSERT(!IPAddressStartsWith(ipv6_address, ipv6_prefix5));
}