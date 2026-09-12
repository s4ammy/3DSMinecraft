#include "Pipeline.h"
#include "BootstrapData.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace Mc3ds {
    struct TCiaMetadata {
        std::uint64_t titleId;
        std::uint16_t version;
        std::uint16_t ticketVersion;
        std::uint64_t tmdVersionOffset;
        std::uint64_t ticketVersionOffset;
    };

    TBytes ReadAt(std::ifstream &file, std::uint64_t offset, std::size_t size, std::uint64_t fileSize);
    std::uint64_t ReadBig(const TBytes &data, std::size_t offset, std::size_t size);
    std::size_t SignatureSize(const TBytes &data);
    TCiaMetadata ReadCiaMetadata(const std::filesystem::path &path);
    void PreserveTitleVersion(const std::filesystem::path &path, const TCiaMetadata &original);
    std::string MakeRebuildSettings(const std::string &productCode, std::uint64_t titleId, std::uint16_t remasterVersion);
    std::filesystem::path FindTool(const std::filesystem::path &requested, const std::string &name);
    bool IsSignatureFailure(const std::string &line);
    void CheckToolResult(const TProcessResult &result, const std::string &stage, const std::vector<std::string> &markers);
    std::vector<std::string> HashMarkers();
    std::string BuildBootstrapCia(const std::filesystem::path &work, const std::filesystem::path &ctrtool,
        const std::filesystem::path &makerom, const std::string &productCode, const TCiaMetadata &metadata,
        const TBytes &originalCode, const TPatchResult &patched);
}

Mc3ds::TBytes Mc3ds::ReadAt(std::ifstream &file, std::uint64_t offset, std::size_t size, std::uint64_t fileSize) {
    if (offset > fileSize || size > fileSize - offset) {
        throw std::runtime_error("CIA metadata extends outside the file");
    }

    auto data = TBytes(size);
    file.seekg(static_cast<std::streamoff>(offset));
    if (!file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()))) {
        throw std::runtime_error("Cannot read CIA metadata");
    }

    return data;
}

std::uint64_t Mc3ds::ReadBig(const TBytes &data, std::size_t offset, std::size_t size) {
    RequireRange(data.size(), offset, size);
    auto result = std::uint64_t{0};
    for (auto index = std::size_t{0}; index < size; ++index) {
        result = (result << 8) | data[offset + index];
    }

    return result;
}

std::size_t Mc3ds::SignatureSize(const TBytes &data) {
    const auto type = ReadBig(data, 0, 4);
    if (type == 0x10003 || type == 0x10000) {
        return 0x240;
    }

    if (type == 0x10004 || type == 0x10001) {
        return 0x140;
    }

    if (type == 0x10005 || type == 0x10002) {
        return 0x80;
    }

    throw std::runtime_error("Unsupported metadata signature type");
}

Mc3ds::TCiaMetadata Mc3ds::ReadCiaMetadata(const std::filesystem::path &path) {
    const auto size = std::filesystem::file_size(path);
    if (size < 0x2020 || size > 0x100000000ULL) {
        throw std::runtime_error("Input is not a supported-size CIA");
    }

    auto file = std::ifstream(path, std::ios::binary);
    const auto header = ReadAt(file, 0, 0x20, size);
    if (Read32(header, 0) != 0x2020 || Read32(header, 4) != 0) {
        throw std::runtime_error("Expected a normal CIA header");
    }

    const auto align = [](std::uint64_t value) {
        return (value + 63) & ~std::uint64_t{63};
    };
    const auto certificateOffset = align(Read32(header, 0));
    const auto ticketOffset = align(certificateOffset + Read32(header, 8));
    const auto tmdOffset = align(ticketOffset + Read32(header, 0xc));
    const auto tmdSize = Read32(header, 0x10);
    if (tmdSize < 0x180 || tmdSize > 4 * 1024 * 1024) {
        throw std::runtime_error("Unsupported TMD size");
    }

    const auto tmd = ReadAt(file, tmdOffset, tmdSize, size);
    const auto contentOffset = align(tmdOffset + tmdSize);
    const auto contentSize = Read64(header, 0x18);
    if (contentOffset > size || contentSize > size - contentOffset) {
        throw std::runtime_error("CIA content size exceeds the file");
    }

    const auto signatureSize = SignatureSize(tmd);
    const auto titleId = ReadBig(tmd, signatureSize + 0x4c, 8);
    if (((titleId >> 32) != 0x00040000 && (titleId >> 32) != 0x0004000e) || (titleId & 0xff) != 0) {
        throw std::runtime_error("Only base-game and update CIAs are supported, not DLC");
    }

    const auto ticketSize = Read32(header, 0xc);
    if (ticketSize < 0x180 || ticketSize > 4 * 1024 * 1024) {
        throw std::runtime_error("Unsupported ticket size");
    }

    const auto ticket = ReadAt(file, ticketOffset, ticketSize, size);
    const auto ticketSignatureSize = SignatureSize(ticket);
    if (ReadBig(ticket, ticketSignatureSize + 0x9c, 8) != titleId) {
        throw std::runtime_error("Ticket and TMD title IDs disagree");
    }

    return {titleId, static_cast<std::uint16_t>(ReadBig(tmd, signatureSize + 0x9c, 2)),
        static_cast<std::uint16_t>(ReadBig(ticket, ticketSignatureSize + 0xa6, 2)),
        tmdOffset + signatureSize + 0x9c, ticketOffset + ticketSignatureSize + 0xa6};
}

void Mc3ds::PreserveTitleVersion(const std::filesystem::path &path, const TCiaMetadata &original) {
    const auto rebuilt = ReadCiaMetadata(path);
    if (rebuilt.titleId != original.titleId) {
        throw std::runtime_error("Rebuild changed the title ID");
    }

    auto file = std::fstream(path, std::ios::binary | std::ios::in | std::ios::out);
    const auto version = std::array<char, 2>{static_cast<char>(original.version >> 8),
        static_cast<char>(original.version & 0xff)};
    for (const auto offset : {rebuilt.tmdVersionOffset, rebuilt.ticketVersionOffset}) {
        file.seekp(static_cast<std::streamoff>(offset));
        file.write(version.data(), static_cast<std::streamsize>(version.size()));
    }

    file.close();
    if (!file) {
        throw std::runtime_error("Cannot preserve the title version in the bootstrap CIA");
    }
}

std::string Mc3ds::MakeRebuildSettings(const std::string &productCode, std::uint64_t titleId, std::uint16_t remasterVersion) {
    const auto update = (titleId >> 32) == 0x0004000e;
    if (productCode.size() != 10 || productCode.substr(0, 9) != (update ? "KTR-U-BD3" : "KTR-P-BD3") ||
        productCode.back() < 'A' || productCode.back() > 'Z') {
        throw std::runtime_error("Unexpected Minecraft product code");
    }

    if ((!update && (titleId >> 32) != 0x00040000) || (titleId & 0xff) != 0) {
        throw std::runtime_error("Only base applications and updates are supported");
    }

    auto settings = rebuildTemplate;
    const auto replace = [&](const std::string &name, const std::string &value) {
        auto position = std::size_t{0};
        while ((position = settings.find(name, position)) != std::string::npos) {
            settings.replace(position, name.size(), value);
            position += value.size();
        }
    };

    replace("@PRODUCT_CODE@", productCode);
    replace("@UNIQUE_ID@", "0x" + HexNumber((titleId >> 8) & 0xffffff));
    replace("Category: Application", update ? "Category: Patch\n  TargetCategory: Application" : "Category: Application");
    replace("RemasterVersion: 0", "RemasterVersion: " + std::to_string(remasterVersion));

    return settings;
}

std::filesystem::path Mc3ds::FindTool(const std::filesystem::path &requested, const std::string &name) {
#ifdef _WIN32
    const auto filename = name + ".exe";
#else
    const auto filename = name;
#endif
    const auto directories = requested.empty() ? std::vector<std::filesystem::path>{
                                                     ExecutableDirectory() / "tools", ExecutableDirectory()} :
                                                 std::vector<std::filesystem::path>{requested};
    for (const auto &directory : directories) {
        const auto candidate = directory / filename;
        if (std::filesystem::is_regular_file(candidate)) {
            return std::filesystem::canonical(candidate);
        }
    }

    throw std::runtime_error("Missing " + filename + ". Put the pinned Project_CTR tools in the tools folder, "
                                                     "use --tools-dir, or build with MC3DS_FETCH_TOOLS=ON. See GUIDE.md.");
}

bool Mc3ds::IsSignatureFailure(const std::string &line) {
    return line.find("Signature") != std::string::npos || line.find("signature") != std::string::npos;
}

void Mc3ds::CheckToolResult(const TProcessResult &result, const std::string &stage, const std::vector<std::string> &markers) {
    if (result.exitCode != 0) {
        throw std::runtime_error(stage + " failed (tool exit " + std::to_string(result.exitCode) + ")");
    }

    if (result.output.find("no seed is set") != std::string::npos) {
        throw std::runtime_error("This encrypted CIA needs its title seed. Put seeddb.bin beside the input CIA "
                                 "or use --seeddb FILE / --seed-file FILE. A decrypted CIA does not need a seed.");
    }

    if (result.output.find("Seed check mismatch") != std::string::npos) {
        throw std::runtime_error("The supplied seed does not match this title");
    }

    auto lines = std::istringstream(result.output);
    auto line = std::string{};
    while (std::getline(lines, line)) {
        if ((line.find("(FAIL)") != std::string::npos || line.find("ERROR]") != std::string::npos) && !IsSignatureFailure(line)) {
            throw std::runtime_error(stage + " reported an integrity or decoding error. "
                                             "Check that the CIA is complete and any supplied seed is correct.");
        }
    }

    for (const auto &marker : markers) {
        if (result.output.find(marker) == std::string::npos) {
            throw std::runtime_error(stage + " did not confirm: " + marker);
        }
    }
}

std::vector<std::string> Mc3ds::HashMarkers() {
    return {"ContentInfo:", "Hash: (GOOD)", "Exheader hash: (GOOD)", "ExeFS hash: (GOOD)",
        "RomFS hash: (GOOD)", "Section hash: (GOOD)", "Level 0: (GOOD)", "Level 1: (GOOD)", "Level 2: (GOOD)"};
}

std::string Mc3ds::BuildBootstrapCia(const std::filesystem::path &work, const std::filesystem::path &ctrtool,
    const std::filesystem::path &makerom, const std::string &productCode, const TCiaMetadata &metadata,
    const TBytes &originalCode, const TPatchResult &patched) {
    if (originalCode.size() != patched.code.size() || Read32(patched.exheader, 0x18) > originalCode.size()) {
        throw std::runtime_error("Original executable does not fit the bootstrap memory layout");
    }

    const auto originalRomfsHash = Sha256File(work / "romfs.bin");
    WriteFile(work / "bootstrap-code.bin", originalCode);
    WriteFile(work / "bootstrap-exheader.bin", patched.exheader);
    WriteFile(work / "bootstrap-icon.bin", patched.icon);
    WriteText(work / "rebuild.rsf", MakeRebuildSettings(productCode, metadata.titleId,
        static_cast<std::uint16_t>(Read32(patched.exheader, 0xc) >> 16)));

    std::cout << "Building the one-time bootstrap CIA..." << std::endl;
    const auto built = RunProcess(makerom, {"-f", "cia", "-o", PathText(work / "bootstrap.cia"),
        "-rsf", PathText(work / "rebuild.rsf"), "-target", "t", "-exheader", PathText(work / "bootstrap-exheader.bin"),
        "-code", PathText(work / "bootstrap-code.bin"), "-romfs", PathText(work / "romfs.bin"),
        "-icon", PathText(work / "bootstrap-icon.bin")});
    CheckToolResult(built, "Bootstrap CIA build", {});
    PreserveTitleVersion(work / "bootstrap.cia", metadata);

    std::cout << "Verifying the bootstrap CIA..." << std::endl;
    std::filesystem::create_directory(work / "verify-exefs");
    const auto verification = RunProcess(ctrtool, {"-v", "-y", "--exefsdir=" + PathText(work / "verify-exefs"),
        "--exheader=" + PathText(work / "verify-exheader.bin"), "--romfs=" + PathText(work / "verify-romfs.bin"),
        PathText(work / "bootstrap.cia")});
    auto markers = HashMarkers();
    const auto modeMarkers = std::vector<std::string>{"IsSnakeOnly: false",
        "System mode:            dev2 (AppMemory: 80MB) (GOOD)",
        "System mode (New3DS):   ctr dev2 (AppMemory: 80MB) (GOOD)",
        "CPU Speed (New3DS):     268MHz (GOOD)", "Enable L2 Cache:        NO (GOOD)",
        "Affinity mask:          1 (GOOD)", "Access Core 2:       NO"};
    markers.insert(markers.end(), modeMarkers.begin(), modeMarkers.end());
    CheckToolResult(verification, "Bootstrap CIA verification", markers);
    if (Sha256File(work / "verify-exefs/code.bin") != Sha256(originalCode) ||
        Sha256File(work / "verify-exefs/icon.bin") != Sha256(patched.icon) ||
        Sha256File(work / "verify-romfs.bin") != originalRomfsHash) {
        throw std::runtime_error("Bootstrap executable, icon, or RomFS differs from the intended output");
    }

    const auto outputMetadata = ReadCiaMetadata(work / "bootstrap.cia");
    if (outputMetadata.titleId != metadata.titleId || outputMetadata.version != metadata.version ||
        outputMetadata.ticketVersion != metadata.version) {
        throw std::runtime_error("Bootstrap CIA changed the title identity or version");
    }

    const auto verifiedExheader = ReadFile(work / "verify-exheader.bin");
    if (Read64(verifiedExheader, 0x200) != Read64(patched.exheader, 0x200) ||
        Read64(verifiedExheader, 0x1c8) != Read64(patched.exheader, 0x1c8) ||
        Read64(verifiedExheader, 0x230) != Read64(patched.exheader, 0x230) ||
        (Read32(verifiedExheader, 0xc) >> 16) != (Read32(patched.exheader, 0xc) >> 16)) {
        throw std::runtime_error("Bootstrap CIA changed the application identity or save mapping");
    }

    for (const auto offset : {0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x30, 0x34, 0x38, 0x3c}) {
        if (Read32(verifiedExheader, static_cast<std::size_t>(offset)) !=
            Read32(patched.exheader, static_cast<std::size_t>(offset))) {
            throw std::runtime_error("Bootstrap CIA changed the executable memory layout");
        }
    }

    return Sha256File(work / "bootstrap.cia");
}

void Mc3ds::RunPatcher(const TOptions &options) {
    const auto input = std::filesystem::canonical(options.input);
    if (!std::filesystem::is_regular_file(input)) {
        throw std::runtime_error("Input must be a regular CIA file");
    }

    const auto suffix = options.controlMode == EControlMode::CIRCLE_PAD_PRO ? "-old3ds-luma-circle-pad-pro" : "-old3ds-luma";
    auto outputDirectory = options.output.empty() ? input.parent_path() / (input.stem().u8string() + suffix) :
                                                    std::filesystem::absolute(options.output);
    outputDirectory = std::filesystem::weakly_canonical(outputDirectory);
    if (outputDirectory == outputDirectory.root_path()) {
        throw std::runtime_error("The output directory cannot be a filesystem root");
    }

    if (std::filesystem::exists(outputDirectory) && !std::filesystem::is_directory(outputDirectory)) {
        throw std::runtime_error("Output must be a directory");
    }

    if (!options.dryRun && !std::filesystem::is_directory(outputDirectory.parent_path())) {
        throw std::runtime_error("The output parent directory does not exist");
    }

    const auto bootstrapPath = outputDirectory / "bootstrap.cia";
    if (options.bootstrapCia && !options.dryRun && std::filesystem::exists(bootstrapPath)) {
        throw std::runtime_error("bootstrap.cia already exists. Choose a fresh output folder; it is never overwritten.");
    }

    const auto ctrtool = FindTool(options.toolsDirectory, "ctrtool");
    const auto makerom = options.bootstrapCia && !options.dryRun ? FindTool(options.toolsDirectory, "makerom") :
                                                            std::filesystem::path{};
    const auto metadata = ReadCiaMetadata(input);
    const auto update = (metadata.titleId >> 32) == 0x0004000e;
    const auto applicationTitleId = (std::uint64_t{0x00040000} << 32) | (metadata.titleId & 0xffffffff);
    std::cout << "Hashing input CIA..." << std::endl;
    const auto inputHash = Sha256File(input);
    std::cout << "Input SHA-256: " << inputHash << "\n";
    std::cout << (inputHash == testedCiaHash || inputHash == updateCiaHash ? "This is a reference CIA.\n" :
                                                                             "Different CIA package. The executable still has to pass the profile checks.\n");
    if (update) {
        std::cout << "Update package: this replacement will override the installed update executable.\n";
    }

    auto workspace = CWorkspace(std::filesystem::temp_directory_path());
    const auto &work = workspace.path();
    std::filesystem::create_directory(work / "exefs");
    auto seedDatabase = options.seedDatabase;
    if (!options.seedFile.empty()) {
        const auto seed = ReadFile(options.seedFile, 16);
        if (seed.size() != 16) {
            throw std::runtime_error("A raw title seed must be exactly 16 bytes");
        }

        auto database = TBytes(48);
        Write32(database, 0, 1);
        Write32(database, 16, static_cast<std::uint32_t>(metadata.titleId));
        Write32(database, 20, static_cast<std::uint32_t>(metadata.titleId >> 32));
        std::copy(seed.begin(), seed.end(), database.begin() + 24);
        seedDatabase = work / "seeddb.bin";
        WriteFile(seedDatabase, database);
    } else if (seedDatabase.empty()) {
        for (const auto &candidate : {input.parent_path() / "seeddb.bin", ExecutableDirectory() / "seeddb.bin"}) {
            if (std::filesystem::is_regular_file(candidate)) {
                seedDatabase = candidate;
                break;
            }
        }
    }

    auto arguments = std::vector<std::string>{"-v", "-y", "-n", "0", "--exheader=" + PathText(work / "exheader.bin"),
        "--exefsdir=" + PathText(work / "exefs")};
    if (options.bootstrapCia && !options.dryRun) {
        arguments.push_back("--romfs=" + PathText(work / "romfs.bin"));
    }
    if (!seedDatabase.empty()) {
        seedDatabase = std::filesystem::canonical(seedDatabase);
        if (!std::filesystem::is_regular_file(seedDatabase) || std::filesystem::file_size(seedDatabase) > 16 * 1024 * 1024) {
            throw std::runtime_error("Invalid or oversized seed database");
        }

        arguments.push_back("--seeddb=" + PathText(seedDatabase));
    }

    arguments.push_back(PathText(input));
    std::cout << "Decrypting, decompressing, and checking the input..." << std::endl;
    const auto extraction = RunProcess(ctrtool, arguments);
    CheckToolResult(extraction, "Input extraction", HashMarkers());
    auto productMatch = std::smatch{};
    if (!std::regex_search(extraction.output, productMatch, std::regex("Product code:[ \\t]+(KTR-[PU]-BD3[A-Z])"))) {
        throw std::runtime_error("This is not a recognized Minecraft product-code family");
    }

    const auto productCode = productMatch[1].str();
    const auto exheader = ReadFile(work / "exheader.bin");
    if (Read64(exheader, 0x200) != applicationTitleId || Read64(exheader, 0x1c8) != metadata.titleId ||
        Read64(exheader, 0x230) != ((metadata.titleId >> 8) & 0xffffff) ||
        productCode[4] != (update ? 'U' : 'P')) {
        throw std::runtime_error("Title identity or save-data mapping is inconsistent");
    }

    const auto originalCode = ReadFile(work / "exefs/code.bin");
    const auto icon = ReadFile(work / "exefs/icon.bin");
    const auto snakeOnly = (Read32(icon, 0x2028) & 0x1000U) != 0;
    std::cout << "Scanning and checking the complete patch profile..." << std::endl;
    const auto patched = PatchGame(originalCode, exheader, icon,
        options.allowSimilar, options.controlMode, options.enableOverlay);
    std::cout << patched.report;
    if (options.dryRun) {
        std::cout << "Dry run passed. No replacement files were written.\n";
        return;
    }

    if (patched.exheader.size() != 0x800) {
        throw std::runtime_error("Patched extended header has an unexpected size");
    }

    const auto ipsPatch = CreateIpsPatch(originalCode, patched.code);
    if (ApplyIpsPatch(originalCode, ipsPatch) != patched.code) {
        throw std::runtime_error("Generated IPS did not reproduce the patched executable");
    }

    const auto bootstrapHash = options.bootstrapCia ?
        BuildBootstrapCia(work, ctrtool, makerom, productCode, metadata, originalCode, patched) : std::string{};
    if (Sha256File(input) != inputHash) {
        throw std::runtime_error("Input CIA changed while the patcher was running");
    }

    const auto codeHash = Sha256(patched.code);
    const auto ipsHash = Sha256(ipsPatch);
    auto applicationTitleIdText = HexNumber(applicationTitleId, 16);
    std::transform(applicationTitleIdText.begin(), applicationTitleIdText.end(), applicationTitleIdText.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::toupper(character));
        });
    const auto titleDirectory = "/luma/titles/" + applicationTitleIdText + "/";
    auto report = std::ostringstream{};
    report << "Minecraft Old 3DS Patcher " << MC3DS_VERSION
           << (options.bootstrapCia ? "\nOutput type: Luma IPS and one-time bootstrap CIA" :
                                      "\nOutput type: Luma IPS executable patch")
           << "\nInput CIA SHA-256: " << inputHash
           << "\ncode.ips SHA-256: " << ipsHash
           << (options.bootstrapCia ? "\nbootstrap.cia SHA-256: " + bootstrapHash : "")
           << "\nSource title ID: " << HexNumber(metadata.titleId, 16)
           << "\nLuma title ID: " << applicationTitleIdText
           << "\nTitle version: " << metadata.version
           << "\nProduct code: " << productCode << "\n"
           << patched.report
           << "Patched executable SHA-256 after IPS: " << codeHash << "\n"
           << "Install code.ips in " << titleDirectory << " with Luma game patching enabled.\n"
           << "Existing romfs, locale, and unrelated files in the title directory are not changed.\n"
           << "Do not install external code.bin or exheader.bin beside this IPS.\n"
           << (options.bootstrapCia ? "Install bootstrap.cia once with FBI, then apply code.ips before launching. Keep the CIA installed.\n" : "")
           << (update ? "Generated from update content. The matching Old 3DS bootstrap update must be installed.\n" :
                        "Generated from base content. The Old 3DS bootstrap base must be installed without an update.\n");
    if (snakeOnly) {
        report << "The source SMDH is New 3DS-only. The installed bootstrap base must contain the Old 3DS SMDH change.\n";
    }

    report << "This report contains no ticket keys or title seeds.\n";
    const auto reportText = report.str();
    if (!std::filesystem::exists(outputDirectory)) {
        auto error = std::error_code{};
        if (!std::filesystem::create_directory(outputDirectory, error)) {
            throw std::runtime_error("Cannot create output directory: " + error.message());
        }
    }

    const auto ipsPath = outputDirectory / "code.ips";
    const auto reportPath = outputDirectory / "report.txt";
    for (const auto &path : {ipsPath, reportPath}) {
        if (std::filesystem::is_directory(path)) {
            throw std::runtime_error("A generated output path is already a directory: " + PathText(path));
        }
    }

    for (const auto &path : {outputDirectory / "code.bin", outputDirectory / "code.bps", outputDirectory / "exheader.bin"}) {
        if (std::filesystem::exists(path)) {
            std::cout << "Warning: remove stale executable or ExHeader override " << PathText(path) << ".\n";
        }
    }

    auto outputStage = CWorkspace(outputDirectory);
    WriteFile(outputStage.path() / "code.ips", ipsPatch);
    WriteText(outputStage.path() / "report.txt", reportText);
    if (options.bootstrapCia) {
        std::filesystem::copy_file(work / "bootstrap.cia", outputStage.path() / "bootstrap.cia");
        if (Sha256File(outputStage.path() / "bootstrap.cia") != bootstrapHash) {
            throw std::runtime_error("Staged bootstrap CIA failed its checksum");
        }
    }

    if (Sha256File(outputStage.path() / "code.ips") != ipsHash) {
        throw std::runtime_error("Staged replacement files failed their checksums");
    }

    if (options.bootstrapCia) {
        PublishFile(outputStage.path() / "bootstrap.cia", bootstrapPath);
    }

    ReplaceOutputFile(outputStage.path() / "report.txt", reportPath);
    ReplaceOutputFile(outputStage.path() / "code.ips", ipsPath);
    if (Sha256File(ipsPath) != ipsHash) {
        throw std::runtime_error("Published replacement files failed their checksums");
    }

    std::cout << "Done: " << PathText(outputDirectory)
              << (options.bootstrapCia ? "\nBootstrap CIA: " + PathText(bootstrapPath) : "")
              << (options.bootstrapCia ? "\nBootstrap CIA SHA-256: " + bootstrapHash : "")
              << "\nLuma destination: " << titleDirectory
              << "\ncode.ips SHA-256: " << ipsHash
              << "\nPatched executable SHA-256: " << codeHash
              << "\nReport: " << PathText(reportPath) << "\n";
}
