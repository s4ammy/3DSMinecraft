#include "Platform.h"

#include <array>
#include <cerrno>
#include <future>
#include <random>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace Mc3ds {
#ifdef _WIN32
    using TPipeHandle = HANDLE;
    std::wstring QuoteArgument(const std::string &text);
#else
    using TPipeHandle = int;
#endif
    std::pair<std::string, bool> ReadPipe(TPipeHandle handle);
}

#ifdef _WIN32
std::wstring Mc3ds::QuoteArgument(const std::string &text) {
    const auto input = std::filesystem::u8path(text).native();
    auto result = std::wstring(L"\"");
    auto slashes = std::size_t{0};
    for (const auto character : input) {
        if (character == L'\\') {
            ++slashes;
        } else {
            result.append(slashes * (character == L'"' ? 2 : 1), L'\\');
            slashes = 0;
            if (character == L'"') {
                result.push_back(L'\\');
            }

            result.push_back(character);
        }
    }

    result.append(slashes * 2, L'\\');
    result.push_back(L'"');

    return result;
}
#endif

std::pair<std::string, bool> Mc3ds::ReadPipe(TPipeHandle handle) {
    auto output = std::string{};
    auto overflow = false;
    auto buffer = std::array<char, 16384>{};
    while (true) {
#ifdef _WIN32
        auto count = DWORD{0};
        if (!::ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr) || count == 0) {
            break;
        }
#else
        const auto count = read(handle, buffer.data(), buffer.size());
        if (count < 0 && errno == EINTR) {
            continue;
        }

        if (count <= 0) {
            break;
        }
#endif
        const auto size = static_cast<std::size_t>(count);
        if (output.size() + size <= 16 * 1024 * 1024) {
            output.append(buffer.data(), size);
        } else {
            overflow = true;
        }
    }
#ifdef _WIN32
    CloseHandle(handle);
#else
    close(handle);
#endif
    return {std::move(output), overflow};
}

Mc3ds::TProcessResult Mc3ds::RunProcess(const std::filesystem::path &program, const std::vector<std::string> &arguments) {
    auto result = TProcessResult{-1, {}};
#ifdef _WIN32
    auto attributes = SECURITY_ATTRIBUTES{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    auto readHandle = HANDLE{};
    auto writeHandle = HANDLE{};
    if (!CreatePipe(&readHandle, &writeHandle, &attributes, 0)) {
        throw std::runtime_error("Cannot create subprocess pipe");
    }

    SetHandleInformation(readHandle, HANDLE_FLAG_INHERIT, 0);
    auto errorRead = HANDLE{};
    auto errorWrite = HANDLE{};
    if (!CreatePipe(&errorRead, &errorWrite, &attributes, 0)) {
        CloseHandle(readHandle);
        CloseHandle(writeHandle);
        throw std::runtime_error("Cannot create subprocess error pipe");
    }

    SetHandleInformation(errorRead, HANDLE_FLAG_INHERIT, 0);
    auto command = QuoteArgument(PathText(program));
    for (const auto &argument : arguments) {
        command += L" " + QuoteArgument(argument);
    }

    auto startup = STARTUPINFOW{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = writeHandle;
    startup.hStdError = errorWrite;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    auto process = PROCESS_INFORMATION{};
    const auto started = CreateProcessW(program.c_str(), command.data(), nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(writeHandle);
    CloseHandle(errorWrite);
    if (!started) {
        CloseHandle(readHandle);
        CloseHandle(errorRead);
        throw std::runtime_error("Cannot start " + PathText(program.filename()));
    }

    auto errorReader = std::async(std::launch::async, ReadPipe, errorRead);
    auto standardOutput = ReadPipe(readHandle);
    auto errorOutput = errorReader.get();
    WaitForSingleObject(process.hProcess, INFINITE);
    auto exitCode = DWORD{1};
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    result.exitCode = static_cast<int>(exitCode);
#else
    auto pipes = std::array<int, 2>{};
    if (pipe(pipes.data()) != 0) {
        throw std::runtime_error("Cannot create subprocess pipe");
    }

    auto errorPipes = std::array<int, 2>{};
    if (pipe(errorPipes.data()) != 0) {
        close(pipes[0]);
        close(pipes[1]);
        throw std::runtime_error("Cannot create subprocess error pipe");
    }

    auto storage = std::vector<std::string>{PathText(program)};
    storage.insert(storage.end(), arguments.begin(), arguments.end());
    auto argv = std::vector<char *>{};
    for (auto &argument : storage) {
        argv.push_back(argument.data());
    }

    argv.push_back(nullptr);
    const auto child = fork();
    if (child == 0) {
        close(pipes[0]);
        close(errorPipes[0]);
        dup2(pipes[1], STDOUT_FILENO);
        dup2(errorPipes[1], STDERR_FILENO);
        close(pipes[1]);
        close(errorPipes[1]);
        const auto nullInput = open("/dev/null", O_RDONLY);
        if (nullInput >= 0) {
            dup2(nullInput, STDIN_FILENO);
            close(nullInput);
        }

        execv(program.c_str(), argv.data());
        _exit(127);
    }

    close(pipes[1]);
    close(errorPipes[1]);
    if (child < 0) {
        close(pipes[0]);
        close(errorPipes[0]);
        throw std::runtime_error("Cannot start subprocess");
    }

    auto errorReader = std::async(std::launch::async, ReadPipe, errorPipes[0]);
    auto standardOutput = ReadPipe(pipes[0]);
    auto errorOutput = errorReader.get();
    auto status = 0;
    while (waitpid(child, &status, 0) < 0) {
        if (errno != EINTR) {
            throw std::runtime_error("Cannot wait for subprocess");
        }
    }

    result.exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : 128;
#endif
    if (standardOutput.second || errorOutput.second) {
        throw std::runtime_error("Subprocess output exceeded its limit");
    }

    result.output = std::move(standardOutput.first);
    result.output += "\n" + errorOutput.first;

    return result;
}

std::filesystem::path Mc3ds::ExecutableDirectory() {
#ifdef _WIN32
    auto buffer = std::vector<wchar_t>(32768);
    const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) {
        throw std::runtime_error("Cannot locate patcher executable");
    }

    return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
#else
    return std::filesystem::canonical("/proc/self/exe").parent_path();
#endif
}

void Mc3ds::ReplaceOutputFile(const std::filesystem::path &staged, const std::filesystem::path &destination) {
#ifdef _WIN32
    if (!MoveFileExW(staged.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw std::runtime_error("Cannot replace output. Check free space and permissions.");
    }
#else
    if (rename(staged.c_str(), destination.c_str()) != 0) {
        throw std::runtime_error("Cannot replace output file: " +
            std::error_code(errno, std::generic_category()).message());
    }
#endif
}

Mc3ds::CWorkspace::CWorkspace(const std::filesystem::path &parent) {
    auto random = std::random_device{};
    for (auto attempt = 0; attempt < 64; ++attempt) {
        const auto candidate = parent / ("mc3ds-patcher-" + HexNumber(random(), 8) + HexNumber(random(), 8));
#ifdef _WIN32
        auto error = std::error_code{};
        const auto created = std::filesystem::create_directory(candidate, error);
#else
        const auto created = mkdir(candidate.c_str(), 0700) == 0;
#endif
        if (created) {
            directory = candidate;
            return;
        }
    }

    throw std::runtime_error("Cannot create a private temporary directory");
}

Mc3ds::CWorkspace::~CWorkspace() {
    if (!directory.empty()) {
        auto error = std::error_code{};
        std::filesystem::remove_all(directory, error);
    }
}

const std::filesystem::path &Mc3ds::CWorkspace::path() const {
    return directory;
}
