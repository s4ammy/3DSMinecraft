#pragma once

#include "Bytes.h"

namespace Mc3ds {
    struct TProcessResult {
        int exitCode;
        std::string output;
    };

    TProcessResult RunProcess(const std::filesystem::path &program, const std::vector<std::string> &arguments);
    std::filesystem::path ExecutableDirectory();
    void ReplaceOutputFile(const std::filesystem::path &staged, const std::filesystem::path &destination);

    class CWorkspace {
    public:
        explicit CWorkspace(const std::filesystem::path &parent);
        ~CWorkspace();
        CWorkspace(const CWorkspace &) = delete;
        CWorkspace &operator=(const CWorkspace &) = delete;
        const std::filesystem::path &path() const;

    private:
        std::filesystem::path directory;
    };
}
