#include "handler/developer_setup.h"

#include <algorithm>
#include <iostream>

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

} // namespace handler
