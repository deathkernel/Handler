#include "handler/developer_setup.h"
#include "handler/command_engine.h"

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cctype>

namespace handler {
namespace {

// Package identifiers are reviewed WinGet IDs, not shell fragments.
// The first increment intentionally supports planning only; installation
// execution will be implemented separately behind Handler's policy layer.
const std::vector<SetupPackage> kPackages{
    {"Python.PythonInstallManager", "Python Install Manager",
     "Manage multiple Python runtimes without replacing an existing interpreter.", true},
    {"Git.Git", "Git", "Source control and repository workflows.", true},
    {"Microsoft.VisualStudioCode", "Visual Studio Code",
     "Editor, debugger, and extension host.", true},
    {"GitHub.cli", "GitHub CLI", "Optional GitHub repository and pull-request workflow.", false},
    {"Microsoft.WindowsTerminal", "Windows Terminal", "Modern terminal host.", false},
    {"OpenJS.NodeJS.LTS", "Node.js LTS", "JavaScript and TypeScript runtime.", false},
    {"EclipseAdoptium.Temurin.21.JDK", "Temurin JDK 21",
     "Java development kit; version is pinned for reproducibility.", false},
    {"Kitware.CMake", "CMake", "Cross-platform native build configuration.", false},
    {"GoLang.Go", "Go", "Go language toolchain.", false},
    {"Rustlang.Rustup", "Rustup", "Rust toolchain manager.", false},
    {"PostgreSQL.PostgreSQL", "PostgreSQL", "Optional local relational database.", false},
    {"Docker.DockerDesktop", "Docker Desktop",
     "Optional containers; requires compatible Windows/virtualization setup.", false}
};

std::string safeProjectName(const std::string& raw) {
    std::string name;
    name.reserve(raw.size());
    for (const unsigned char ch : raw) {
        if (std::isalnum(ch)) {
            name.push_back(static_cast<char>(std::tolower(ch)));
        } else if (ch == '-' || ch == '_') {
            name.push_back(static_cast<char>(ch));
        } else {
            name.push_back('-');
        }
    }
    while (!name.empty() && (name.front() == '-' || name.front() == '_'))
        name.erase(name.begin());
    while (!name.empty() && (name.back() == '-' || name.back() == '_'))
        name.pop_back();
    if (name.empty()) name = "handler-project";
    if (std::isdigit(static_cast<unsigned char>(name.front())))
        name.insert(0, "project-");
    return name;
}

const std::vector<SetupProfile> kProfiles{
    {"base", "Base Developer",
     "Python runtime management, Git, and VS Code.",
     {"Python.PythonInstallManager", "Git.Git", "Microsoft.VisualStudioCode"}},
    {"python", "Python Developer",
     "Base tools plus common Python test and lint workflow (project environment setup is planned separately).",
     {"Python.PythonInstallManager", "Git.Git", "Microsoft.VisualStudioCode"}},
    {"web", "Web Developer",
     "Base tools plus Node.js LTS for JavaScript and TypeScript projects.",
     {"Python.PythonInstallManager", "Git.Git", "Microsoft.VisualStudioCode", "OpenJS.NodeJS.LTS"}},
    {"fullstack", "Full Stack Developer",
     "Base tools plus Node.js LTS; database and containers stay opt-in.",
     {"Python.PythonInstallManager", "Git.Git", "Microsoft.VisualStudioCode", "OpenJS.NodeJS.LTS"}},
    {"native", "Native Build Tools",
     "Base tools plus CMake. A compiler workload must be installed separately and explicitly.",
     {"Python.PythonInstallManager", "Git.Git", "Microsoft.VisualStudioCode", "Kitware.CMake"}},
    {"custom", "Custom (catalog preview)",
     "Lists all known optional packages. This is a preview, not an install command.",
     {"Python.PythonInstallManager", "Git.Git", "Microsoft.VisualStudioCode",
      "GitHub.cli", "Microsoft.WindowsTerminal", "OpenJS.NodeJS.LTS",
      "EclipseAdoptium.Temurin.21.JDK", "Kitware.CMake", "GoLang.Go",
      "Rustlang.Rustup", "PostgreSQL.PostgreSQL", "Docker.DockerDesktop"}}
};

} // namespace

const std::vector<SetupPackage>& developerSetupPackages() {
    return kPackages;
}

const std::vector<SetupProfile>& developerSetupProfiles() {
    return kProfiles;
}

SetupPlan planDeveloperSetup(const std::string& profileId) {
    const auto profile = std::find_if(
        kProfiles.begin(), kProfiles.end(),
        [&](const SetupProfile& candidate) { return candidate.id == profileId; });

    SetupPlan plan;
    plan.profileId = profileId;
    if (profile == kProfiles.end()) {
        plan.warnings.push_back("Unknown profile. Run 'handler setup profiles' to list supported profiles.");
        return plan;
    }

    for (const auto& packageId : profile->packageIds) {
        const auto package = std::find_if(
            kPackages.begin(), kPackages.end(),
            [&](const SetupPackage& candidate) { return candidate.id == packageId; });
        if (package == kPackages.end()) {
            plan.warnings.push_back("Catalog integrity error: unknown package ID " + packageId);
            continue;
        }
        plan.packages.push_back(*package);
    }

    plan.warnings.push_back("Planning only: no package manager was invoked and no system changes were made.");
    plan.warnings.push_back("Actual installation is not enabled in this increment.");
    if (profileId == "native") {
        plan.warnings.push_back("CMake is not a compiler; choose and approve a compatible compiler workload separately.");
    }
    if (profileId == "fullstack" || profileId == "custom") {
        plan.warnings.push_back("Docker and database services can require significant storage, virtualization, or elevated privileges.");
    }
    return plan;
}

void printDeveloperSetupPlan(const SetupPlan& plan) {
    std::cout << "Handler Developer Setup Plan\n"
              << "============================\n"
              << "Profile: " << (plan.profileId.empty() ? "(none)" : plan.profileId) << "\n";
    if (plan.packages.empty()) {
        std::cout << "No packages selected.\n";
    } else {
        for (const auto& package : plan.packages) {
            std::cout << "  [PLAN] " << package.name << " [" << package.id << "]\n"
                      << "         " << package.purpose << "\n";
        }
    }
    for (const auto& warning : plan.warnings)
        std::cout << "[INFO] " << warning << "\n";
}


std::vector<HostCheck> inspectDeveloperHost() {
    std::vector<HostCheck> checks;
#ifdef _WIN32
    checks.push_back({"OS", "OK", "Windows"});
#else
    checks.push_back({"OS", "INFO", "Non-Windows platform; WinGet installation is unavailable"});
#endif
#if defined(_M_X64) || defined(__x86_64__)
    checks.push_back({"Architecture", "OK", "x64"});
#elif defined(_M_ARM64) || defined(__aarch64__)
    checks.push_back({"Architecture", "OK", "ARM64"});
#elif defined(_M_IX86) || defined(__i386__)
    checks.push_back({"Architecture", "OK", "x86"});
#else
    checks.push_back({"Architecture", "UNKNOWN", "Compiler did not expose a known architecture"});
#endif
    std::error_code ec;
    const auto space = std::filesystem::space(std::filesystem::current_path(), ec);
    if (!ec) {
        const auto gib = static_cast<double>(space.available) / (1024.0 * 1024.0 * 1024.0);
        checks.push_back({"Disk space", "OK",
            std::to_string(static_cast<unsigned long long>(gib * 100.0) / 100.0) +
            " GiB available on current volume"});
    } else {
        checks.push_back({"Disk space", "UNKNOWN", "Unable to query free space"});
    }
    for (const auto& tool : std::vector<std::string>{"python", "git", "winget"}) {
#ifndef _WIN32
        if (tool == "winget") {
            checks.push_back({"WinGet", "UNAVAILABLE", "WinGet is Windows-only"});
            continue;
        }
#endif
        CommandSpec spec{"developer-setup-preflight", tool, {"--version"},
                         RiskLevel::Low, 10000};
        const auto result = executeCommand(spec);
        const bool ok = result.started && result.exitCode == 0;
        std::string details = result.output.empty() ? result.error : result.output;
        const auto newline = details.find_first_of("\r\n");
        if (newline != std::string::npos) details.resize(newline);
        if (details.empty()) details = ok ? "version check passed" : "not available or version check failed";
        checks.push_back({tool, ok ? "OK" : "MISSING", details});
    }
    return checks;
}

bool createDeveloperProject(const std::string& templateId,
                            const std::string& targetDirectory,
                            std::string& details) {
    namespace fs = std::filesystem;
    if (templateId != "python" && templateId != "web" && templateId != "native") {
        details = "Unsupported template. Choose python, web, or native.";
        return false;
    }
    if (targetDirectory.empty()) {
        details = "Project directory must not be empty.";
        return false;
    }
    const fs::path target = fs::path(targetDirectory).lexically_normal();
    if (target == target.root_path() || target.filename().empty()) {
        details = "Refusing to create a project at a filesystem root.";
        return false;
    }
    std::error_code ec;
    if (fs::exists(target, ec)) {
        details = "Target already exists; Handler will not overwrite existing files.";
        return false;
    }
    const std::string projectName = safeProjectName(target.filename().string());
    const auto parent = target.parent_path().empty() ? fs::current_path() : target.parent_path();
    if (!fs::exists(parent, ec) || !fs::is_directory(parent, ec)) {
        details = "Parent directory does not exist or is not a directory.";
        return false;
    }
    if (!fs::create_directory(target, ec) || ec) {
        details = "Could not create project directory: " + ec.message();
        return false;
    }
    auto writeFile = [&](const fs::path& relative, const std::string& content) {
        std::ofstream out(target / relative, std::ios::binary | std::ios::out);
        if (!out) return false;
        out << content;
        out.close();
        return static_cast<bool>(out);
    };
    bool ok = false;
    if (templateId == "python") {
        ok = writeFile("pyproject.toml",
            "[project]\nname = \"" + projectName +
            "\"\nversion = \"0.1.0\"\ndescription = \"\"\nrequires-python = \">=3.11\"\n"
            "\n[tool.pytest.ini_options]\ntestpaths = [\"tests\"]\n");
        ok = ok && writeFile(".gitignore", ".venv/\n__pycache__/\n.pytest_cache/\n.ruff_cache/\n");
        ok = ok && writeFile("README.md", "# " + projectName +
            "\n\nPython project created by Handler.\nCreate a virtual environment with python -m venv .venv.\n");
        ok = ok && fs::create_directory(target / "src", ec) && !ec;
        ec.clear();
        ok = ok && fs::create_directory(target / "tests", ec) && !ec;
        ok = ok && writeFile(fs::path("src") / "main.py",
            "def main() -> None:\n    print(\"Hello from " + projectName +
            "!\")\n\nif __name__ == \"__main__\":\n    main()\n");
        ok = ok && writeFile(fs::path("tests") / "test_smoke.py",
            "def test_smoke() -> None:\n    assert True\n");
    } else if (templateId == "web") {
        ok = writeFile("package.json", "{\n  \"name\": \"" + projectName +
            "\",\n  \"version\": \"0.1.0\",\n  \"private\": true,\n  \"scripts\": { \"test\": \"node --test\" }\n}\n");
        ok = ok && writeFile(".gitignore", "node_modules/\n.env\ndist/\n");
        ok = ok && writeFile("README.md", "# " + projectName +
            "\n\nWeb project scaffold created by Handler.\n");
        ec.clear();
        ok = ok && fs::create_directory(target / "src", ec) && !ec;
        ok = ok && writeFile(fs::path("src") / "index.js", "console.log('Hello from Handler');\n");
        ok = ok && writeFile("index.html", "<!doctype html>\n<html lang=\"en\"><meta charset=\"utf-8\"><title>" +
            projectName + "</title><body><script src=\"src/index.js\"></script></body></html>\n");
    } else {
        ok = writeFile("CMakeLists.txt", "cmake_minimum_required(VERSION 3.20)\nproject(" +
            projectName + " LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 17)\n"
            "set(CMAKE_CXX_STANDARD_REQUIRED ON)\nadd_executable(app src/main.cpp)\n");
        ok = ok && writeFile(".gitignore", "build/\ncmake-build-*/\n");
        ok = ok && writeFile("README.md", "# " + projectName +
            "\n\nC++ project scaffold created by Handler.\n");
        ec.clear();
        ok = ok && fs::create_directory(target / "src", ec) && !ec;
        ok = ok && writeFile(fs::path("src") / "main.cpp",
            "#include <iostream>\nint main() { std::cout << \"Hello from Handler\\n\"; return 0; }\n");
    }
    if (!ok) {
        std::error_code cleanupError;
        fs::remove_all(target, cleanupError);
        details = "Project scaffold failed; newly created target removed where possible.";
        if (cleanupError) details += " Cleanup warning: " + cleanupError.message();
        return false;
    }
    details = "Created " + templateId + " project at " + target.string() +
              ". No packages were installed.";
    return true;
}

} // namespace handler
