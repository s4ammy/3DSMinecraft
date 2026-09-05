#include "Bytes.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <openssl/evp.h>
#endif

namespace Mc3ds {
    class CSha256 {
    public:
        CSha256();
        ~CSha256();

        CSha256(const CSha256 &) = delete;
        CSha256 &operator=(const CSha256 &) = delete;

        void update(const std::uint8_t *data, std::size_t size);
        std::string finish();

    private:
#ifdef _WIN32
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        BCRYPT_HASH_HANDLE context = nullptr;
#else
        EVP_MD_CTX *context = nullptr;
#endif
    };
}

Mc3ds::CSha256::CSha256() {
#ifdef _WIN32
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        throw std::runtime_error("Cannot initialize SHA-256");
    }

    if (BCryptCreateHash(algorithm, &context, nullptr, 0, nullptr, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        throw std::runtime_error("Cannot create SHA-256 context");
    }
#else
    context = EVP_MD_CTX_new();
    if (context == nullptr || EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(context);
        throw std::runtime_error("Cannot initialize SHA-256");
    }
#endif
}

Mc3ds::CSha256::~CSha256() {
#ifdef _WIN32
    BCryptDestroyHash(context);
    BCryptCloseAlgorithmProvider(algorithm, 0);
#else
    EVP_MD_CTX_free(context);
#endif
}

void Mc3ds::CSha256::update(const std::uint8_t *data, std::size_t size) {
    if (size == 0) {
        return;
    }
#ifdef _WIN32
    if (size > 0xffffffffULL || BCryptHashData(context, const_cast<PUCHAR>(data), static_cast<ULONG>(size), 0) != 0) {
        throw std::runtime_error("SHA-256 update failed");
    }
#else
    if (EVP_DigestUpdate(context, data, size) != 1) {
        throw std::runtime_error("SHA-256 update failed");
    }
#endif
}

std::string Mc3ds::CSha256::finish() {
    auto result = TBytes(32);
#ifdef _WIN32
    if (BCryptFinishHash(context, result.data(), 32, 0) != 0) {
        throw std::runtime_error("SHA-256 finalization failed");
    }
#else
    auto size = 0U;
    if (EVP_DigestFinal_ex(context, result.data(), &size) != 1 || size != 32) {
        throw std::runtime_error("SHA-256 finalization failed");
    }
#endif
    return Hex(result);
}

std::string Mc3ds::PathText(const std::filesystem::path &path) {
    return path.u8string();
}

void Mc3ds::RequireRange(std::size_t size, std::size_t offset, std::size_t length) {
    if (offset > size || length > size - offset) {
        throw std::runtime_error("Truncated file or out-of-range field");
    }
}

Mc3ds::TBytes Mc3ds::ReadFile(const std::filesystem::path &path, std::uint64_t limit) {
    const auto size = std::filesystem::file_size(path);
    if (size > limit) {
        throw std::runtime_error("File exceeds the supported size: " + PathText(path.filename()));
    }

    auto data = TBytes(static_cast<std::size_t>(size));
    auto file = std::ifstream(path, std::ios::binary);
    if (!file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()))) {
        throw std::runtime_error("Cannot read " + PathText(path.filename()));
    }

    return data;
}

void Mc3ds::WriteFile(const std::filesystem::path &path, const TBytes &data) {
    auto file = std::ofstream(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    file.close();
    if (!file) {
        throw std::runtime_error("Cannot write " + PathText(path.filename()));
    }
}

void Mc3ds::WriteText(const std::filesystem::path &path, const std::string &text) {
    WriteFile(path, TBytes(text.begin(), text.end()));
}

std::uint32_t Mc3ds::Read32(const TBytes &data, std::size_t offset) {
    RequireRange(data.size(), offset, 4);
    auto value = std::uint32_t{0};
    for (auto index = std::size_t{0}; index < 4; ++index) {
        value |= static_cast<std::uint32_t>(data[offset + index]) << (index * 8);
    }

    return value;
}

std::uint64_t Mc3ds::Read64(const TBytes &data, std::size_t offset) {
    return Read32(data, offset) | (static_cast<std::uint64_t>(Read32(data, offset + 4)) << 32);
}

void Mc3ds::Write32(TBytes &data, std::size_t offset, std::uint32_t value) {
    RequireRange(data.size(), offset, 4);
    for (auto index = std::size_t{0}; index < 4; ++index) {
        data[offset + index] = static_cast<std::uint8_t>(value >> (index * 8));
    }
}

Mc3ds::TBytes Mc3ds::FromHex(const std::string &text) {
    if (text.size() % 2 != 0) {
        throw std::runtime_error("Odd-length hexadecimal data");
    }

    auto result = TBytes{};
    for (auto offset = std::size_t{0}; offset < text.size(); offset += 2) {
        const auto piece = text.substr(offset, 2);
        if (piece.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) {
            throw std::runtime_error("Invalid hexadecimal data");
        }

        result.push_back(static_cast<std::uint8_t>(std::stoul(piece, nullptr, 16)));
    }

    return result;
}

std::string Mc3ds::Hex(const TBytes &data) {
    auto output = std::ostringstream{};
    output << std::hex << std::setfill('0');
    for (const auto value : data) {
        output << std::setw(2) << static_cast<unsigned>(value);
    }

    return output.str();
}

std::string Mc3ds::HexNumber(std::uint64_t value, int width) {
    auto output = std::ostringstream{};
    output << std::hex << std::setfill('0') << std::setw(width) << value;

    return output.str();
}

std::string Mc3ds::Sha256(const TBytes &data) {
    auto hash = CSha256{};
    hash.update(data.data(), data.size());

    return hash.finish();
}

std::string Mc3ds::Sha256File(const std::filesystem::path &path) {
    auto file = std::ifstream(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file for hashing");
    }

    auto hash = CSha256{};
    auto buffer = std::array<std::uint8_t, 65536>{};
    while (file) {
        file.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        hash.update(buffer.data(), static_cast<std::size_t>(file.gcount()));
    }

    if (!file.eof()) {
        throw std::runtime_error("File read failed during hashing");
    }

    return hash.finish();
}
