#include "Pipeline.h"

#include <iostream>
#include <stdexcept>

namespace Mc3ds {
    void PrintHelp();
    int Main(const std::vector<std::string> &arguments);
}

void Mc3ds::PrintHelp() {
    std::cout << "Minecraft Old 3DS Patcher " MC3DS_VERSION "\n\n"
                 "Usage: mc3ds-patcher input.cia [options]\n\n"
                 "  -o, --output DIRECTORY  Output folder for generated files\n"
                 "  --controls MODE        l-circle-pad (default) or circle-pad-pro\n"
                 "  --overlay              Enable FPS/debug overlay (update only; default off)\n"
                 "  --bootstrap-cia        Also build a one-time installable bootstrap CIA\n"
                 "  --tools-dir DIRECTORY   Folder containing ctrtool and makerom\n"
                 "  --seeddb FILE           Local seeddb.bin for encrypted titles\n"
                 "  --seed-file FILE        Local 16-byte title seed instead of seeddb\n"
                 "  --allow-similar         Try unknown executables with strict signature guards\n"
                 "  --dry-run               Decrypt and check all patches without writing files\n"
                 "  -h, --help              Show this help\n\n"
                 "The input is never modified. code.ips and report.txt are replaced.\n"
                 "Bootstrap CIAs are created only with --bootstrap-cia and never overwritten.\n"
                 "Supports the European v0.1.0 base game and v9.11.0 update.\n"
                 "Build the bootstrap base first; add the update if you use it.\n"
                 "For later IPS changes, use the original CIA matching the installed update.\n"
                 "Copy code.ips to /luma/titles/000400000017CA00/ with game patching enabled.\n"
                 "Install bootstrap.cia with FBI once if you generated it. See GUIDE.md.\n";
}

int Mc3ds::Main(const std::vector<std::string> &arguments) {
    try {
        if (arguments.empty()) {
            PrintHelp();
            return 0;
        }

        auto options = TOptions{};
        for (auto index = std::size_t{0}; index < arguments.size(); ++index) {
            const auto &argument = arguments[index];
            const auto value = [&]() {
                if (++index >= arguments.size()) {
                    throw std::runtime_error("Missing value after " + argument);
                }

                return std::filesystem::u8path(arguments[index]);
            };

            if (argument == "--help" || argument == "-h") {
                PrintHelp();
                return 0;
            } else if (argument == "--output" || argument == "-o") {
                options.output = value();
            } else if (argument == "--tools-dir") {
                options.toolsDirectory = value();
            } else if (argument == "--seeddb") {
                options.seedDatabase = value();
            } else if (argument == "--seed-file") {
                options.seedFile = value();
            } else if (argument == "--controls") {
                options.controlMode = ParseControlMode(value().u8string());
            } else if (argument == "--overlay") {
                options.enableOverlay = true;
            } else if (argument == "--bootstrap-cia") {
                options.bootstrapCia = true;
            } else if (argument == "--allow-similar") {
                options.allowSimilar = true;
            } else if (argument == "--dry-run") {
                options.dryRun = true;
            } else if (!argument.empty() && argument.front() == '-') {
                throw std::runtime_error("Unknown option: " + argument);
            } else if (options.input.empty()) {
                options.input = std::filesystem::u8path(argument);
            } else {
                throw std::runtime_error("Only one input CIA is accepted");
            }
        }

        if (options.input.empty()) {
            throw std::runtime_error("An input CIA is required");
        }

        if (!options.seedFile.empty() && !options.seedDatabase.empty()) {
            throw std::runtime_error("Use either --seeddb or --seed-file, not both");
        }

        RunPatcher(options);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << "\nNo changes were made to the input CIA.\n";
        return 1;
    }
}

#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
    auto arguments = std::vector<std::string>{};
    for (auto index = 1; index < argc; ++index) {
        arguments.push_back(std::filesystem::path(argv[index]).u8string());
    }
    return Mc3ds::Main(arguments);
}
#else
int main(int argc, char **argv) {
    return Mc3ds::Main(std::vector<std::string>(argv + 1, argv + argc));
}
#endif
