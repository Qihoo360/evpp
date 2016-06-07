// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "evpp/base/inner_pre.h"
#include "evpp/base/ip_address.h"
#include "evpp/base/sys_addrinfo.h"

#include <sstream>

namespace evpp {
    namespace base {
        typedef std::vector<uint8_t> IPAddressNumber;

        namespace {
            const unsigned char kIPv4MappedPrefix[] =
            { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };

            bool IsIPv4Mapped(const IPAddressNumber& address) {
                if (address.size() != IPAddress::kIPv6AddressSize)
                    return false;
                return std::equal(address.begin(),
                    address.begin() + H_ARRAYSIZE(kIPv4MappedPrefix),
                    kIPv4MappedPrefix);
            }

            bool IPNumberPrefixCheck(const IPAddressNumber& ip_number,
                const unsigned char* ip_prefix,
                size_t prefix_length_in_bits) {
                // Compare all the bytes that fall entirely within the prefix.
                int num_entire_bytes_in_prefix = prefix_length_in_bits / 8;
                for (int i = 0; i < num_entire_bytes_in_prefix; ++i) {
                    if (ip_number[i] != ip_prefix[i])
                        return false;
                }

                // In case the prefix was not a multiple of 8, there will be 1 byte
                // which is only partially masked.
                int remaining_bits = prefix_length_in_bits % 8;
                if (remaining_bits != 0) {
                    unsigned char mask = 0xFF << (8 - remaining_bits);
                    int i = num_entire_bytes_in_prefix;
                    if ((ip_number[i] & mask) != (ip_prefix[i] & mask))
                        return false;
                }
                return true;
            }

            // Don't compare IPv4 and IPv6 addresses (they have different range
            // reservations). Keep separate reservation arrays for each IP type, and
            // consolidate adjacent reserved ranges within a reservation array when
            // possible.
            // Sources for info:
            // www.iana.org/assignments/ipv4-address-space/ipv4-address-space.xhtml
            // www.iana.org/assignments/ipv6-address-space/ipv6-address-space.xhtml
            // They're formatted here with the prefix as the last element. For example:
            // 10.0.0.0/8 becomes 10,0,0,0,8 and fec0::/10 becomes 0xfe,0xc0,0,0,0...,10.
            static bool IsIPAddressReserved(const std::vector<uint8_t>& host_addr) {
                static const unsigned char kReservedIPv4[][5] = {
                    { 0, 0, 0, 0, 8 }, { 10, 0, 0, 0, 8 }, { 100, 64, 0, 0, 10 }, { 127, 0, 0, 0, 8 },
                    { 169, 254, 0, 0, 16 }, { 172, 16, 0, 0, 12 }, { 192, 0, 2, 0, 24 },
                    { 192, 88, 99, 0, 24 }, { 192, 168, 0, 0, 16 }, { 198, 18, 0, 0, 15 },
                    { 198, 51, 100, 0, 24 }, { 203, 0, 113, 0, 24 }, { 224, 0, 0, 0, 3 }
                };
                static const unsigned char kReservedIPv6[][17] = {
                    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8 },
                    { 0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
                    { 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 },
                    { 0xc0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3 },
                    { 0xe0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4 },
                    { 0xf0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5 },
                    { 0xf8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6 },
                    { 0xfc, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7 },
                    { 0xfe, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9 },
                    { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10 },
                    { 0xfe, 0xc0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10 },
                };
                size_t array_size = 0;
                const unsigned char* array = NULL;
                switch (host_addr.size()) {
                case IPAddress::kIPv4AddressSize:
                    array_size = H_ARRAYSIZE(kReservedIPv4);
                    array = kReservedIPv4[0];
                    break;
                case IPAddress::kIPv6AddressSize:
                    array_size = H_ARRAYSIZE(kReservedIPv6);
                    array = kReservedIPv6[0];
                    break;
                }
                if (!array)
                    return false;
                size_t width = host_addr.size() + 1;
                for (size_t i = 0; i < array_size; ++i, array += width) {
                    if (IPNumberPrefixCheck(host_addr, array, array[width - 1]))
                        return true;
                }
                return false;
            }
        }

        IPAddress::IPAddress() {}

        IPAddress::IPAddress(const IPAddress& other) = default;

        IPAddress::IPAddress(const uint8_t* address, size_t address_len)
            : ip_address_(address, address + address_len) {}

        IPAddress::IPAddress(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
            ip_address_.reserve(4);
            ip_address_.push_back(b0);
            ip_address_.push_back(b1);
            ip_address_.push_back(b2);
            ip_address_.push_back(b3);
        }

        IPAddress::IPAddress(uint8_t b0,
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
            uint8_t b15) {
            const uint8_t address[] = { b0, b1, b2, b3, b4, b5, b6, b7,
                b8, b9, b10, b11, b12, b13, b14, b15 };
            //ip_address_ = std::vector<uint8_t>(std::begin(address), std::end(address));
            for (int i = 0; i < 16; ++i) {
                ip_address_.push_back(address[i]);
            }
        }

        IPAddress::~IPAddress() {}

        bool IPAddress::IsIPv4() const {
            return ip_address_.size() == kIPv4AddressSize;
        }

        bool IPAddress::IsIPv6() const {
            return ip_address_.size() == kIPv6AddressSize;
        }

        bool IPAddress::IsValid() const {
            return IsIPv4() || IsIPv6();
        }

        bool IPAddress::IsReserved() const {
            return IsIPAddressReserved(ip_address_);
        }

        bool IPAddress::IsZero() const {
            for (size_t i = 0; i < ip_address_.size(); ++i) {
                if (ip_address_[i] != 0) {
                    return false;
                }
            }
            return !ip_address_.empty();
            //for (auto x : ip_address_) {
            //    if (x != 0)
            //        return false;
            //}

            //return !ip_address_.empty();
        }

        bool IPAddress::IsIPv4MappedIPv6() const {
            return IsIPv4Mapped(ip_address_);
        }

        std::string IPAddress::ToString() const {
            if (empty()) {
                return "";
            }
            return IPAddressToString(&ip_address_[0], ip_address_.size());
        }

        bool IPAddress::AssignFromIPLiteral(const Slice& ip_literal) {
            bool ipv4 = true;
            for (size_t i = 0; i < ip_literal.size(); ++i) {
                if (ip_literal[i] == ':') {
                    ipv4 = false;
                }
            }

            if (ipv4) {
                ip_address_.resize(kIPv4AddressSize);
                if (::inet_pton(AF_INET, ip_literal.data(), &ip_address_[0]) == 1) {
                    return true;
                }
            } else {
                ip_address_.resize(kIPv6AddressSize);
                if (::inet_pton(AF_INET6, ip_literal.data(), &ip_address_[0]) == 1) {
                    return true;
                }
            }

            return false;
            
//              std::vector<uint8_t> number;
//              if (!ParseIPLiteralToNumber(ip_literal, &number))
//                  return false;
//  
//              std::swap(number, ip_address_);
//             //TODO
//             return true;
        }

        // static
        IPAddress IPAddress::IPv4Localhost() {
            static const uint8_t kLocalhostIPv4[] = { 127, 0, 0, 1 };
            return IPAddress(kLocalhostIPv4);
        }

        // static
        IPAddress IPAddress::IPv6Localhost() {
            static const uint8_t kLocalhostIPv6[] = { 0, 0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 1 };
            return IPAddress(kLocalhostIPv6);
        }

        // static
        IPAddress IPAddress::AllZeros(size_t num_zero_bytes) {
            IPAddress ip;
            ip.ip_address_.resize(num_zero_bytes);
            return ip;
        }

        // static
        IPAddress IPAddress::IPv4AllZeros() {
            return AllZeros(kIPv4AddressSize);
        }

        // static
        IPAddress IPAddress::IPv6AllZeros() {
            return AllZeros(kIPv6AddressSize);
        }

        bool IPAddress::operator==(const IPAddress& that) const {
            return ip_address_ == that.ip_address_;
        }

        bool IPAddress::operator!=(const IPAddress& that) const {
            return ip_address_ != that.ip_address_;
        }

        bool IPAddress::operator<(const IPAddress& that) const {
            // Sort IPv4 before IPv6.
            if (ip_address_.size() != that.ip_address_.size()) {
                return ip_address_.size() < that.ip_address_.size();
            }

            return ip_address_ < that.ip_address_;
        }

        std::string IPAddressToStringWithPort(const IPAddress& address, uint16_t port) {
            if (address.empty()) {
                return "";
            }
            std::string output = IPAddressToString(&address.bytes()[0], address.size());
            std::ostringstream ss;
            if (address.size() == IPAddress::kIPv4AddressSize) {
                ss << output << ":" << port;
                return ss.str();
            } else if (address.size() == IPAddress::kIPv6AddressSize) {
                ss << "[" << output << "]:" << port;
                return ss.str();
            }
            return "";
        }
// 
//         std::string IPAddressToPackedString(const IPAddress& address) {
//             //return IPAddressToPackedString(address.bytes());
//             //TODO
//             return std::string();
//         }
// 
//         IPAddress ConvertIPv4ToIPv4MappedIPv6(const IPAddress& address) {
//             //return IPAddress(ConvertIPv4NumberToIPv6Number(address.bytes()));
//             //TODO
//             return IPAddress();
//         }
// 
//         IPAddress ConvertIPv4MappedIPv6ToIPv4(const IPAddress& address) {
//             //return IPAddress(ConvertIPv4MappedToIPv4(address.bytes()));
//             //TODO
//             return IPAddress();
//         }
// 
//         bool IPAddressMatchesPrefix(const IPAddress& ip_address,
//                          const IPAddress& ip_prefix,
//                          size_t prefix_length_in_bits) {
//             //             return IPNumberMatchesPrefix(ip_address.bytes(), ip_prefix.bytes(),
//             //                 prefix_length_in_bits);
//             //TODO 
//             return true;
//         }
// 
//         bool ParseCIDRBlock(const std::string& cidr_literal,
//             IPAddress* ip_address,
//             size_t* prefix_length_in_bits) {
//             //TODO
//             // We expect CIDR notation to match one of these two templates:
//             //   <IPv4-literal> "/" <number of bits>
//             //   <IPv6-literal> "/" <number of bits>
// 
// //             std::vector<Slice> parts = Slice(
// //                 cidr_literal, "/", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
// //             if (parts.size() != 2)
// //                 return false;
// // 
// //             // Parse the IP address.
// //             if (!ip_address->AssignFromIPLiteral(parts[0]))
// //                 return false;
// // 
// //             // TODO(martijnc): Find a more general solution for the overly permissive
// //             // base::StringToInt() parsing. https://crbug.com/596523.
// //             const base::StringPiece& prefix_length = parts[1];
// //             if (prefix_length.starts_with("+"))
// //                 return false;
// // 
// //             // Parse the prefix length.
// //             int number_of_bits = -1;
// //             if (!base::StringToInt(prefix_length, &number_of_bits))
// //                 return false;
// // 
// //             // Make sure the prefix length is in a valid range.
// //             if (number_of_bits < 0 ||
// //                 number_of_bits > static_cast<int>(ip_address->size() * 8))
// //                 return false;
// // 
// //             *prefix_length_in_bits = static_cast<size_t>(number_of_bits);
//             return true;
//         }


        //TODO
//         unsigned CommonPrefixLength(const IPAddress& a1, const IPAddress& a2) {
//             return CommonPrefixLength(a1.bytes(), a2.bytes());
//         }
// 
//         unsigned MaskPrefixLength(const IPAddress& mask) {
//             return MaskPrefixLength(mask.bytes());
//         }
    } // namespace base
}  // namespace evpp

#ifndef H_OS_WINDOWS
int _itoa_s(int value, char* buffer, size_t size_in_chars, int radix) {
    const char* format_str;
    if (radix == 10)
        format_str = "%d";
    else if (radix == 16)
        format_str = "%x";
    else
        return EINVAL;

    int written = snprintf(buffer, size_in_chars, format_str, value);
    if (static_cast<size_t>(written) >= size_in_chars) {
        // Output was truncated, or written was negative.
        return EINVAL;
    }
    return 0;
}

int _itow_s(int value, uint16_t* buffer, size_t size_in_chars, int radix) {
    if (radix != 10)
        return EINVAL;

    // No more than 12 characters will be required for a 32-bit integer.
    // Add an extra byte for the terminating null.
    char temp[13];
    int written = snprintf(temp, sizeof(temp), "%d", value);
    if (static_cast<size_t>(written) >= size_in_chars) {
        // Output was truncated, or written was negative.
        return EINVAL;
    }

    for (int i = 0; i < written; ++i) {
        buffer[i] = static_cast<uint16_t>(temp[i]);
    }
    buffer[written] = '\0';
    return 0;
}
#endif
