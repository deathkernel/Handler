#pragma once

#include <string>
#include <vector>

namespace handler {

struct SetupPackage {
    std::string id;
    std::string name;
    std::string purpose;
    bool requiredByBaseProfile{false};
    std::string platform{"windows"};
};

struct SetupProfile {
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> packageIds;
};

struct SetupPlan {
    std::string profileId;
    std::vector<SetupPackage> packages;
    std::vector<std::string> warnings;
};

// Return the built-in, reviewed catalog. This function is read-only.
const std::vector<SetupPackage>& developerSetupPackages();

// Return the built-in profile catalog.
const std::vector<SetupProfile>& developerSetupProfiles();

// Build a read-only installation plan. Unknown profiles are rejected.
SetupPlan planDeveloperSetup(const std::string& profileId);

// Print a plan in a human-readable form. Planning never installs packages.
void printDeveloperSetupPlan(const SetupPlan& plan);

} // namespace handler
