#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Mc3ds {
    using TBytes = std::vector<std::uint8_t>;

    TBytes ReadFile(const std::filesystem::path &path, std::uint64_t limit = 64 * 1024 * 1024);
    void WriteFile(const std::filesystem::path &path, const TBytes &data);
    void WriteText(const std::filesystem::path &path, const std::string &text);
    std::uint32_t Read32(const TBytes &data, std::size_t offset);
    std::uint64_t Read64(const TBytes &data, std::size_t offset);
    void Write32(TBytes &data, std::size_t offset, std::uint32_t value);
    void RequireRange(std::size_t size, std::size_t offset, std::size_t length);
    TBytes FromHex(const std::string &text);
    TBytes CreateIpsPatch(const TBytes &before, const TBytes &after);
    TBytes ApplyIpsPatch(const TBytes &before, const TBytes &patch);
    std::string Hex(const TBytes &data);
    std::string HexNumber(std::uint64_t value, int width = 0);
    std::string Sha256(const TBytes &data);
    std::string Sha256File(const std::filesystem::path &path);
    std::string PathText(const std::filesystem::path &path);
}
