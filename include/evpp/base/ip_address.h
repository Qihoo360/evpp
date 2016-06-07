// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <string>
#include <vector>

#include "evpp/base/evppbase_export.h"
#include "evpp/base/slice.h"


#ifndef H_OS_WINDOWS
#ifdef __cplusplus
extern "C" {
#endif
    // Implementations of Windows' int-to-string conversions
    int _itoa_s(int value, char* buffer, size_t size_in_chars, int radix);
    int _itow_s(int value, uint16_t* buffer, size_t size_in_chars, int radix);
#ifdef __cplusplus
}
#endif

// Secure template overloads for these functions
template<size_t N>
inline int _itoa_s(int value, char(&buffer)[N], int radix) {
    return _itoa_s(value, buffer, N, radix);
}

#endif


namespace evpp {
    namespace base {
        class EVPPBASE_EXPORT IPAddress {
        public:
            enum { kIPv4AddressSize = 4, kIPv6AddressSize = 16 };

            // Creates a zero-sized, invalid address.
            IPAddress();

            IPAddress(const IPAddress& other);

            // Copies the input address to |ip_address_|. The input is expected to be in
            // network byte order.
            template <size_t N>
            IPAddress(const uint8_t(&address)[N]) {
                for (size_t i = 0; i < N; i++) {
                    ip_address_.push_back(address[i]);
                }
            }

            // Copies the input address to |ip_address_| taking an additional length
            // parameter. The input is expected to be in network byte order.
            IPAddress(const uint8_t* address, size_t address_len);

            // Initializes |ip_address_| from the 4 bX bytes to form an IPv4 address.
            // The bytes are expected to be in network byte order.
            IPAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);

            // Initializes |ip_address_| from the 16 bX bytes to form an IPv6 address.
            // The bytes are expected to be in network byte order.
            IPAddress(uint8_t b0,
                uint8_t b1,
                uint8_t b2,
                uint8_t b3,
                uint8_t b4,
                uint8_t b5,
                uint8_t b6,
                uint8_t b7,
                uint8_t b8,
                uint8_t b9,
                uint8_t b10,
                uint8_t b11,
                uint8_t b12,
                uint8_t b13,
                uint8_t b14,
                uint8_t b15);

            ~IPAddress();

            // Returns true if the IP has |kIPv4AddressSize| elements.
            bool IsIPv4() const;

            // Returns true if the IP has |kIPv6AddressSize| elements.
            bool IsIPv6() const;

            // Returns true if the IP is either an IPv4 or IPv6 address. This function
            // only checks the address length.
            bool IsValid() const;

            // Returns true if an IP address hostname is in a range reserved by the IANA.
            // Works with both IPv4 and IPv6 addresses, and only compares against a given
            // protocol's reserved ranges.
            bool IsReserved() const;

            // Returns true if the IP is "zero" (e.g. the 0.0.0.0 IPv4 address).
            bool IsZero() const;

            // Returns true if |ip_address_| is an IPv4-mapped IPv6 address.
            bool IsIPv4MappedIPv6() const;

            // The size in bytes of |ip_address_|.
            size_t size() const { return ip_address_.size(); }

            // Returns true if the IP is an empty, zero-sized (invalid) address.
            bool empty() const { return ip_address_.empty(); }

            // Returns the canonical string representation of an IP address.
            // For example: "192.168.0.1" or "::1". Returns the empty string when
            // |ip_address_| is invalid.
            std::string ToString() const;

            // Parses an IP address literal (either IPv4 or IPv6) to its numeric value.
            // Returns true on success and fills |ip_address_| with the numeric value.
            bool AssignFromIPLiteral(const Slice& ip_literal);

            // Returns the underlying byte vector.
            const std::vector<uint8_t>& bytes() const { return ip_address_; };

            // Returns an IPAddress instance representing the 127.0.0.1 address.
            static IPAddress IPv4Localhost();

            // Returns an IPAddress instance representing the ::1 address.
            static IPAddress IPv6Localhost();

            // Returns an IPAddress made up of |num_zero_bytes| zeros.
            static IPAddress AllZeros(size_t num_zero_bytes);

            // Returns an IPAddress instance representing the 0.0.0.0 address.
            static IPAddress IPv4AllZeros();

            // Returns an IPAddress instance representing the :: address.
            static IPAddress IPv6AllZeros();

            bool operator==(const IPAddress& that) const;
            bool operator!=(const IPAddress& that) const;
            bool operator<(const IPAddress& that) const;

        private:
            // IPv4 addresses will have length kIPv4AddressSize, whereas IPv6 address
            // will have length kIPv6AddressSize.
            std::vector<uint8_t> ip_address_;

            // This class is copyable and assignable.
        };

        typedef std::vector<IPAddress> IPAddressList;

        // TODO(Martijnc): These utility functions currently forward the calls to
        // the IPAddressNumber implementations. Move the implementations over when
        // the IPAddressNumber migration is complete. https://crbug.com/496258.

        // Returns the canonical string representation of an IP address along with its
        // port. For example: "192.168.0.1:99" or "[::1]:80".
        NET_EXPORT std::string IPAddressToStringWithPort(const IPAddress& address,
            uint16_t port);

        // Returns the address as a sequence of bytes in network-byte-order.
        //NET_EXPORT std::string IPAddressToPackedString(const IPAddress& address);

        // Converts an IPv4 address to an IPv4-mapped IPv6 address.
        // For example 192.168.0.1 would be converted to ::ffff:192.168.0.1.
        //NET_EXPORT IPAddress ConvertIPv4ToIPv4MappedIPv6(const IPAddress& address);

        // Converts an IPv4-mapped IPv6 address to IPv4 address. Should only be called
        // on IPv4-mapped IPv6 addresses.
        //NET_EXPORT IPAddress ConvertIPv4MappedIPv6ToIPv4(const IPAddress& address);

        // Compares an IP address to see if it falls within the specified IP block.
        // Returns true if it does, false otherwise.
        //
        // The IP block is given by (|ip_prefix|, |prefix_length_in_bits|) -- any
        // IP address whose |prefix_length_in_bits| most significant bits match
        // |ip_prefix| will be matched.
        //
        // In cases when an IPv4 address is being compared to an IPv6 address prefix
        // and vice versa, the IPv4 addresses will be converted to IPv4-mapped
        // (IPv6) addresses.
        NET_EXPORT bool IPAddressMatchesPrefix(const IPAddress& ip_address,
            const IPAddress& ip_prefix,
            size_t prefix_length_in_bits);

        // Parses an IP block specifier from CIDR notation to an
        // (IP address, prefix length) pair. Returns true on success and fills
        // |*ip_address| with the numeric value of the IP address and sets
        // |*prefix_length_in_bits| with the length of the prefix.
        //
        // CIDR notation literals can use either IPv4 or IPv6 literals. Some examples:
        //
        //    10.10.3.1/20
        //    a:b:c::/46
        //    ::1/128
        NET_EXPORT bool ParseCIDRBlock(const std::string& cidr_literal,
            IPAddress* ip_address,
            size_t* prefix_length_in_bits);

        // Returns number of matching initial bits between the addresses |a1| and |a2|.
        unsigned CommonPrefixLength(const IPAddress& a1, const IPAddress& a2);

        // Computes the number of leading 1-bits in |mask|.
        unsigned MaskPrefixLength(const IPAddress& mask);

        // Checks whether |address| starts with |prefix|. This provides similar
        // functionality as IPAddressMatchesPrefix() but doesn't perform automatic IPv4
        // to IPv4MappedIPv6 conversions and only checks against full bytes.
        template <size_t N>
        bool IPAddressStartsWith(const IPAddress& address, const uint8_t(&prefix)[N]) {
            if (address.size() < N)
                return false;
            return std::equal(prefix, prefix + N, address.bytes().begin());
        }

        inline void AppendIPv4Address(const unsigned char address[4], std::string* output) {
            for (int i = 0; i < 4; i++) {
                char str[16];
                _itoa_s(address[i], str, 10);

                for (int ch = 0; str[ch] != 0; ch++)
                    output->push_back(str[ch]);

                if (i != 3)
                    output->push_back('.');
            }
        }

        // Represents a substring for URL parsing.
        struct Component {
            Component() : begin(0), len(-1) {}

            // Normal constructor: takes an offset and a length.
            Component(int b, int l) : begin(b), len(l) {}

            int end() const {
                return begin + len;
            }

            // Returns true if this component is valid, meaning the length is given. Even
            // valid components may be empty to record the fact that they exist.
            bool is_valid() const {
                return (len != -1);
            }

            // Returns true if the given component is specified on false, the component
            // is either empty or invalid.
            bool is_nonempty() const {
                return (len > 0);
            }

            void reset() {
                begin = 0;
                len = -1;
            }

            bool operator==(const Component& other) const {
                return begin == other.begin && len == other.len;
            }

            int begin;  // Byte offset in the string of this component.
            int len;    // Will be -1 if the component is unspecified.
        };

        // Searches for the longest sequence of zeros in |address|, and writes the
        // range into |contraction_range|. The run of zeros must be at least 16 bits,
        // and if there is a tie the first is chosen.
        void ChooseIPv6ContractionRange(const unsigned char address[16],
            Component* contraction_range) {
            // The longest run of zeros in |address| seen so far.
            Component max_range;

            // The current run of zeros in |address| being iterated over.
            Component cur_range;

            for (int i = 0; i < 16; i += 2) {
                // Test for 16 bits worth of zero.
                bool is_zero = (address[i] == 0 && address[i + 1] == 0);

                if (is_zero) {
                    // Add the zero to the current range (or start a new one).
                    if (!cur_range.is_valid())
                        cur_range = Component(i, 0);
                    cur_range.len += 2;
                }

                if (!is_zero || i == 14) {
                    // Just completed a run of zeros. If the run is greater than 16 bits,
                    // it is a candidate for the contraction.
                    if (cur_range.len > 2 && cur_range.len > max_range.len) {
                        max_range = cur_range;
                    }
                    cur_range.reset();
                }
            }
            *contraction_range = max_range;
        }

        inline void AppendIPv6Address(const unsigned char address[16], std::string* output) {
            // We will output the address according to the rules in:
            // http://tools.ietf.org/html/draft-kawamura-ipv6-text-representation-01#section-4

            // Start by finding where to place the "::" contraction (if any).
            Component contraction_range;
            ChooseIPv6ContractionRange(address, &contraction_range);

            for (int i = 0; i <= 14;) {
                // We check 2 bytes at a time, from bytes (0, 1) to (14, 15), inclusive.
                assert(i % 2 == 0);
                if (i == contraction_range.begin && contraction_range.len > 0) {
                    // Jump over the contraction.
                    if (i == 0)
                        output->push_back(':');
                    output->push_back(':');
                    i = contraction_range.end();
                } else {
                    // Consume the next 16 bits from |address|.
                    int x = address[i] << 8 | address[i + 1];

                    i += 2;

                    // Stringify the 16 bit number (at most requires 4 hex digits).
                    char str[5];
                    _itoa_s(x, str, 16);
                    for (int ch = 0; str[ch] != 0; ++ch)
                        output->push_back(str[ch]);

                    // Put a colon after each number, except the last.
                    if (i < 16)
                        output->push_back(':');
                }
            }
        }

        inline std::string IPAddressToString(const uint8_t* address, size_t address_len) {
            std::string output;

            if (address_len == IPAddress::kIPv4AddressSize) {
                AppendIPv4Address(address, &output);
            } else if (address_len == IPAddress::kIPv6AddressSize) {
                AppendIPv6Address(address, &output);
            }

            return output;
        }


//         bool ParseIPLiteralToNumber(const Slice& ip_literal,
//             IPAddressNumber* ip_number) {
//             // |ip_literal| could be either a IPv4 or an IPv6 literal. If it contains
//             // a colon however, it must be an IPv6 address.
//             if (ip_literal.find(':') != Slice::npos) {
//                 // GURL expects IPv6 hostnames to be surrounded with brackets.
//                 std::string host_brackets = "[";
//                 ip_literal.AppendToString(&host_brackets);
//                 host_brackets.push_back(']');
//                 url::Component host_comp(0, host_brackets.size());
// 
//                 // Try parsing the hostname as an IPv6 literal.
//                 ip_number->resize(16);  // 128 bits.
//                 return url::IPv6AddressToNumber(host_brackets.data(), host_comp,
//                     &(*ip_number)[0]);
//             }
// 
//             // Otherwise the string is an IPv4 address.
//             ip_number->resize(4);  // 32 bits.
//             url::Component host_comp(0, ip_literal.size());
//             int num_components;
//             url::CanonHostInfo::Family family = url::IPv4AddressToNumber(
//                 ip_literal.data(), host_comp, &(*ip_number)[0], &num_components);
//             return family == url::CanonHostInfo::IPV4;
//        
    }
}  // namespace net




