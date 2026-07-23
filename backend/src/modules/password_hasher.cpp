#include "induspilot/modules/password_hasher.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace induspilot::modules {
namespace {

using Bytes = std::vector<std::uint8_t>;
using Sha256Digest = std::array<std::uint8_t, 32>;

constexpr std::array<std::uint32_t, 64> kSha256RoundConstants{
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

std::uint32_t rotateRight(std::uint32_t value, std::uint32_t bits) {
    return (value >> bits) | (value << (32U - bits));
}

Bytes bytesFromString(const std::string& value) {
    return Bytes(value.begin(), value.end());
}

Sha256Digest sha256(Bytes message) {
    const std::uint64_t bitLength = static_cast<std::uint64_t>(message.size()) * 8U;
    message.push_back(0x80U);
    while ((message.size() % 64U) != 56U) {
        message.push_back(0U);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<std::uint8_t>((bitLength >> shift) & 0xffU));
    }

    std::array<std::uint32_t, 8> hash{
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU, 0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};

    for (std::size_t chunk = 0; chunk < message.size(); chunk += 64U) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16U; ++i) {
            const auto offset = chunk + i * 4U;
            words[i] = (static_cast<std::uint32_t>(message[offset]) << 24U) |
                       (static_cast<std::uint32_t>(message[offset + 1U]) << 16U) |
                       (static_cast<std::uint32_t>(message[offset + 2U]) << 8U) |
                       static_cast<std::uint32_t>(message[offset + 3U]);
        }
        for (std::size_t i = 16U; i < 64U; ++i) {
            const auto s0 = rotateRight(words[i - 15U], 7U) ^ rotateRight(words[i - 15U], 18U) ^ (words[i - 15U] >> 3U);
            const auto s1 = rotateRight(words[i - 2U], 17U) ^ rotateRight(words[i - 2U], 19U) ^ (words[i - 2U] >> 10U);
            words[i] = words[i - 16U] + s0 + words[i - 7U] + s1;
        }

        auto a = hash[0];
        auto b = hash[1];
        auto c = hash[2];
        auto d = hash[3];
        auto e = hash[4];
        auto f = hash[5];
        auto g = hash[6];
        auto h = hash[7];

        for (std::size_t i = 0; i < 64U; ++i) {
            const auto sigma1 = rotateRight(e, 6U) ^ rotateRight(e, 11U) ^ rotateRight(e, 25U);
            const auto choose = (e & f) ^ ((~e) & g);
            const auto temp1 = h + sigma1 + choose + kSha256RoundConstants[i] + words[i];
            const auto sigma0 = rotateRight(a, 2U) ^ rotateRight(a, 13U) ^ rotateRight(a, 22U);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = sigma0 + majority;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        hash[0] += a;
        hash[1] += b;
        hash[2] += c;
        hash[3] += d;
        hash[4] += e;
        hash[5] += f;
        hash[6] += g;
        hash[7] += h;
    }

    Sha256Digest digest{};
    for (std::size_t i = 0; i < hash.size(); ++i) {
        digest[i * 4U] = static_cast<std::uint8_t>((hash[i] >> 24U) & 0xffU);
        digest[i * 4U + 1U] = static_cast<std::uint8_t>((hash[i] >> 16U) & 0xffU);
        digest[i * 4U + 2U] = static_cast<std::uint8_t>((hash[i] >> 8U) & 0xffU);
        digest[i * 4U + 3U] = static_cast<std::uint8_t>(hash[i] & 0xffU);
    }
    return digest;
}

Bytes digestToBytes(const Sha256Digest& digest) {
    return Bytes(digest.begin(), digest.end());
}

Bytes hmacSha256(Bytes key, const Bytes& message) {
    constexpr std::size_t blockSize = 64U;
    if (key.size() > blockSize) {
        key = digestToBytes(sha256(std::move(key)));
    }
    key.resize(blockSize, 0U);

    Bytes outerKey(blockSize);
    Bytes innerKey(blockSize);
    for (std::size_t i = 0; i < blockSize; ++i) {
        outerKey[i] = static_cast<std::uint8_t>(key[i] ^ 0x5cU);
        innerKey[i] = static_cast<std::uint8_t>(key[i] ^ 0x36U);
    }

    Bytes innerInput = innerKey;
    innerInput.insert(innerInput.end(), message.begin(), message.end());
    const auto innerDigest = sha256(std::move(innerInput));

    Bytes outerInput = outerKey;
    outerInput.insert(outerInput.end(), innerDigest.begin(), innerDigest.end());
    return digestToBytes(sha256(std::move(outerInput)));
}

Bytes pbkdf2Sha256Bytes(const std::string& password, const std::string& salt, int iterations) {
    Bytes saltBlock = bytesFromString(salt);
    saltBlock.push_back(0U);
    saltBlock.push_back(0U);
    saltBlock.push_back(0U);
    saltBlock.push_back(1U);

    const auto passwordBytes = bytesFromString(password);
    Bytes block = hmacSha256(passwordBytes, saltBlock);
    Bytes result = block;
    for (int i = 1; i < iterations; ++i) {
        block = hmacSha256(passwordBytes, block);
        for (std::size_t j = 0; j < result.size(); ++j) {
            result[j] = static_cast<std::uint8_t>(result[j] ^ block[j]);
        }
    }
    return result;
}

std::string toHex(const Bytes& bytes) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto byte : bytes) {
        out << std::setw(2) << static_cast<int>(byte);
    }
    return out.str();
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    std::istringstream input(value);
    while (std::getline(input, current, delimiter)) {
        parts.push_back(current);
    }
    return parts;
}

bool constantTimeEquals(const std::string& left, const std::string& right) {
    const auto maxSize = std::max(left.size(), right.size());
    unsigned char diff = static_cast<unsigned char>(left.size() ^ right.size());
    for (std::size_t i = 0; i < maxSize; ++i) {
        const auto l = i < left.size() ? static_cast<unsigned char>(left[i]) : 0U;
        const auto r = i < right.size() ? static_cast<unsigned char>(right[i]) : 0U;
        diff = static_cast<unsigned char>(diff | (l ^ r));
    }
    return diff == 0U;
}

bool parseIterations(const std::string& value, int& iterations) {
    try {
        std::size_t consumed = 0;
        const auto parsed = std::stoi(value, &consumed);
        if (consumed != value.size() || parsed < 1 || parsed > 1000000) {
            return false;
        }
        iterations = parsed;
        return true;
    } catch (...) {
        return false;
    }
}

}  // namespace

std::string pbkdf2Sha256PasswordHash(const std::string& password, const std::string& salt, int iterations) {
    if (iterations < 1) {
        return {};
    }
    return "pbkdf2_sha256$" + std::to_string(iterations) + "$" + salt + "$" + toHex(pbkdf2Sha256Bytes(password, salt, iterations));
}

bool verifyPassword(const std::string& password, const std::string& storedHash) {
    if (storedHash.rfind("plain:", 0) == 0) {
        return constantTimeEquals(password, storedHash.substr(6));
    }

    const auto parts = split(storedHash, '$');
    if (parts.size() == 4U && parts[0] == "pbkdf2_sha256") {
        int iterations = 0;
        if (!parseIterations(parts[1], iterations) || parts[2].empty() || parts[3].empty()) {
            return false;
        }
        const auto computed = pbkdf2Sha256PasswordHash(password, parts[2], iterations);
        return constantTimeEquals(computed, storedHash);
    }

    // 兼容早期内存演示数据，后续生产数据应全部使用版本化哈希格式。
    return constantTimeEquals(password, storedHash);
}

}  // namespace induspilot::modules