# Handler 🛡️

> **A deterministic developer-environment protection, diagnosis, repair, maintenance, and recovery system.**

Handler is an actively implemented C++17 developer tool designed to protect a developer's coding environment from problems caused by dependency conflicts, missing tools, broken configurations, failed updates, incomplete installations, and other development-environment failures.

Handler is **not an AI assistant**. Its core intelligence is a deterministic decision-making system built from rules, diagnostics, environment state, known error patterns, recovery strategies, safety policies, command generation, and command execution.

## Vision

Development work should not put the developer's machine at unnecessary risk.

Handler aims to follow a simple lifecycle:

**Prevent → Detect → Diagnose → Decide → Protect → Generate → Execute → Verify → Recover**

When a problem can be safely repaired, Handler should perform the required commands itself. When an operation is potentially destructive or system-impacting, Handler should protect the environment, explain the planned action, and request permission where required.

## Core Features

### 🔍 1. Error Detection
Detect common development errors from command output, tool failures, project execution, and environment checks.

Examples:
- Missing Python/Node/Java/.NET packages
- Dependency conflicts
- Broken PATH entries
- Missing runtimes or compilers
- Configuration errors
- Port conflicts
- Permission problems
- Failed installations and updates

### 🧠 2. Deterministic Decision Engine
Handler makes decisions using explicit rules and observed environment state rather than an LLM.

Example rule:

```text
IF module_missing
AND package_is_known
AND target_environment_is_identified
THEN
  create_snapshot
  install_package
  verify_import
```

The decision engine should select an appropriate recovery strategy, verify the result, and move to the next strategy when a safe recovery path fails.

### 🔧 3. Automatic Error Recovery
Handler can recognize supported Python/Node missing-module errors and route them through guarded, policy-confirmed repair. Package repair is transaction-backed with verification and a best-effort package-level rollback when the package was not previously installed.

Example:

```text
ModuleNotFoundError: pandas
        ↓
Identify missing module
        ↓
Check active Python environment
        ↓
Create recovery point
        ↓
Generate package-install command
        ↓
python -m pip install pandas
        ↓
Verify: import pandas
        ↓
✓ Recovery successful
```

### 🧩 4. Command Formula & Generation Engine
Handler should not require a giant hardcoded list of complete commands.

Instead, reusable actions should define:
- Command templates/formulas
- Required inputs
- Environment variables
- Conditions
- Risk level
- Validation requirements
- Verification steps
- Rollback strategy

Example:

```text
Action:
INSTALL_PYTHON_PACKAGE

Inputs:
PYTHON_EXECUTABLE
PACKAGE_NAME

Formula:
PYTHON_EXECUTABLE + " -m pip install " + PACKAGE_NAME
```

Given:

```text
PYTHON_EXECUTABLE = C:\Project\.venv\Scripts\python.exe
PACKAGE_NAME = pandas
```

Handler generates the environment-specific command:

```text
C:\Project\.venv\Scripts\python.exe -m pip install pandas
```

The generated command must pass validation and safety checks before execution.

### 📦 5. Dependency Management
- Detect missing dependencies
- Detect outdated dependencies
- Detect version conflicts
- Identify indirect dependencies
- Choose compatible versions where possible
- Repair broken project environments
- Prevent unsafe dependency removal

### 🔗 6. Existing Component Discovery & Linking
If a required library, tool, runtime, SDK, compiler, or other component already exists on the machine, Handler should **discover it before installing another copy**.

Example:

```text
Project reports:
ModuleNotFoundError: pandas

        ↓

Search relevant environments

        ↓

pandas found in another environment

        ↓

Check:
• Version compatibility
• Target environment
• Isolation rules
• Dependency requirements
• Whether safe reuse/linking is possible

        ↓

If safe:
Connect/configure existing component

If not safe:
Install or provision a compatible component
```

Handler should never bypass intentional environment isolation simply to avoid an installation. For example, a package installed in a different Python environment should not automatically be copied or exposed to a virtual environment unless the target configuration explicitly supports safe reuse.

The principle is:

**Discover first → determine safe reuse → install only when necessary.**

### 💻 7. Development Language & Tool Updates
Detect installed development languages, runtimes, SDKs, compilers, and developer tools that have updates available.

Planned support may include:
- Python
- Node.js
- Java
- .NET
- Go
- Rust
- PHP
- C/C++ toolchains

Major or potentially breaking updates should not be treated the same as ordinary maintenance updates.

### 🛡️ 8. Environment Protection
Monitor important development-environment state, including:
- PATH
- Environment variables
- Installed runtimes
- Package versions
- Developer tools
- Project configuration
- Relevant services
- Virtual environments

Handler should detect unexpected or risky changes and protect against avoidable damage.

### 💾 9. Snapshots & Known-Good State
Create recovery points before meaningful or risky operations.

A snapshot may capture the environment state required to reproduce or restore a working configuration.

Handler should maintain a **Last Known Good State** whenever possible.

### 🔄 10. Rollback & Recovery
When an update, installation, cleanup, or repair causes a verified regression, Handler can roll back the affected changes when a safe restoration path exists.

Planned capabilities:
- Last operation undo
- Snapshot restore
- Package version rollback
- Virtual-environment recreation
- Configuration restoration
- Interrupted-recovery recovery

### 🧪 11. Verification Engine
A change is not considered successful merely because a command returned successfully.

Handler should verify outcomes using appropriate checks:
- Import/package checks
- Version checks
- Project startup/build checks
- Dependency consistency checks
- Tool availability checks
- Environment integrity checks

### ⚡ 12. Modular On-Demand Execution
Handler should **not run its entire system continuously**. Only the components required for the current task, event, project, or recovery operation should be activated.

Examples:

```text
Missing Python package
        ↓
Activate:
Error Parser
Dependency Resolver
Python Environment Manager
Command Formula Engine
Command Validator
Command Executor
Verification
        ↓
Keep unrelated modules inactive
```

This reduces:
- CPU usage
- RAM usage
- background processes
- unnecessary disk activity
- unnecessary environment scanning

Handler should also avoid refreshing or rescanning everything after every change.

### 🔄 13. Targeted Refresh
When something changes, Handler should refresh **only the affected component/state**.

Examples:

```text
Python package changed
→ Refresh Python dependency state

PATH changed
→ Refresh PATH state

Node.js updated
→ Refresh Node.js/toolchain state

Project A changed
→ Refresh Project A state
→ Do not rescan unrelated projects
```

A full environment scan should be reserved for explicit requests, startup/initial discovery, recovery situations, or cases where targeted state is no longer trustworthy.

### 🧩 14. Lazy Module Loading
Handler components should be loaded only when needed.

The core process should remain lightweight while specialized modules remain dormant until their capabilities are required.

### 💤 15. Idle / Sleep State
When no relevant work is happening, Handler should enter a low-activity state rather than continuously executing diagnostics.

It should wake when a relevant event, scheduled check, developer action, or recovery workflow requires it.

### 🧹 16. Deep Cleanup
Find development components that appear unnecessary and, **with user permission**, remove them completely where it is safe to do so.

Potential cleanup targets:
- Unused libraries
- Orphaned dependencies
- Old virtual environments
- Caches
- Old tool installations
- Related configuration
- Obsolete PATH entries
- Associated services/components where applicable

Handler should distinguish between:
**unused by one project** and **unused by the entire machine**.

Before deep removal, Handler should check whether a component is:
- Directly used
- Required by another dependency
- Used by another project
- Part of a runtime/toolchain
- Globally configured
- Safe to remove

### 🗑️ 17. Supported Project Package Uninstallation
Handler currently supports permission-based removal of direct Python/Node.js project dependencies with project-local scope, dependency-impact checks, backups, verification, and truthful rollback reporting. Deep OS-level application removal is intentionally not enabled.

Where appropriate, this can include:
- Main installation
- Package-manager components
- Caches
- Configuration
- PATH references
- Related services
- Associated files

System-level deletion should require stronger safeguards and explicit permission.

### 🚧 18. Risky Command Interception
Identify operations that could affect multiple projects or the wider machine.

Examples:
- Global package changes
- Runtime upgrades
- PATH modifications
- System-wide configuration changes
- Toolchain replacement
- Large-scale cleanup

Handler can block, isolate, or request confirmation based on configured policy.

### 🧪 19. Safe Mode / Isolation
Test risky changes in an isolated environment before applying them to the primary environment when practical.

### 🏥 20. Environment Health Check
Provide a diagnostic scan of the developer environment.

Example:

```text
Developer Environment Health

Python        ✓
Node.js       ✓
Git           ✓
PATH          ✓
Dependencies  ⚠
Configuration ⚠
Recovery      ✓
```

### 🔌 21. Toolchain Doctor
Detect and repair broken developer tooling such as runtimes, compilers, package managers, and command-line tools.

### 🛣️ 22. PATH Guardian
Track PATH changes, detect missing or suspicious entries, and restore valid configuration when a safe recovery point exists.

### 🔐 23. Environment Variable Protection
Detect missing or unexpectedly changed environment variables while keeping secrets protected from plaintext logs and snapshots.

### 🚦 24. Port & Resource Conflict Detection
Identify common development conflicts such as occupied ports, locked files, and conflicting running services/processes.

### 📊 25. Environment & Change History
Keep a clear timeline of meaningful Handler actions:

```text
18:42  Snapshot created
18:43  Dependency installed
18:44  Verification passed
18:47  Runtime updated
18:48  Project verification failed
18:48  Rollback started
18:49  Environment restored
```

### 📦 26. Project Environment Awareness
Understand that different projects can require different language/runtime versions.

Example:

```text
Project A → Python 3.11
Project B → Python 3.13
Project C → Python 3.14
```

Global changes should consider their potential impact on all known projects.

### 🌳 27. Dependency Graph & Impact Analysis
Show relationships between projects, packages, runtimes, and tools and estimate what could break before making a change.

### 🧯 28. Recovery Circuit Breaker
If repeated recovery attempts are failing or changes are producing unexpected results, Handler should stop further automatic modifications and preserve the current state for investigation or rollback.

### 🔒 29. Permission & Safety Policies
Configurable operating modes:

- **Auto** — execute only pre-approved low-risk repairs
- **Confirm** — ask before meaningful changes
- **Strict** — require confirmation for modifications

Safety levels should be based on actual impact, not just the command name.

### 🔁 30. Transaction-Based Operations
Treat multi-step changes as transactions:

```text
START
  ↓
Snapshot
  ↓
Action(s)
  ↓
Verify
  ↓
COMMIT ✓

Failure
  ↓
ROLLBACK
```

### 📝 31. Recovery Journal
Record what Handler detected, what decision it made, what commands/actions it performed, and whether verification succeeded.

The journal should be understandable to developers and useful for debugging Handler itself.

### 🧹 32. Scheduled TEMP Cleanup
Handler includes a native maintenance capability for Windows temporary files.

Planned behavior:
- Inspect the current user's `%TEMP%` directory
- Clean eligible temporary files and directories
- Skip locked/in-use items instead of forcing deletion
- Report removed items, skipped items, and recovered space
- Run the cleanup on a **2-hour maintenance interval** when Handler maintenance mode is active
- Keep cleanup focused on the TEMP directory rather than performing unrelated system cleanup

The cleanup is conservative: destructive runs require confirmation, entries modified within 24 hours are skipped, protected Handler directories are skipped, and failed/locked entries are reported. Maintenance mode uses dry-run cleanup because it is non-interactive.

## Dependency-First Implementation Order

Handler features will be implemented in the following order. This order is **dependency-driven**, not popularity-driven: each stage should provide the foundation required by the next stage.

### Level 1 — Foundation

1. **Native Handler Core / CLI** — process lifecycle, command routing, exit codes, configuration entry point.
2. **PC/System Target Model** — establish the machine as Handler's primary scope.
3. **System Environment Observation** — inspect OS, user context, TEMP, PATH, installed locations, processes, and basic system state.
4. **Environment State Store** — represent and persist the observed PC environment in a structured form.
5. **Change History / State Tracking** — record meaningful environment changes so later features can compare and reason about state.
6. **Targeted State Refresh** — refresh only the state affected by an operation instead of rescanning the whole PC.
7. **Modular On-Demand Execution** — provide the routing mechanism that activates only the capabilities required by a task.
8. **Lazy Module Loading** — make specialized modules loadable only when required.
9. **Idle / Sleep State** — allow scheduled/event-driven capabilities to remain inactive between tasks.

### Level 2 — Observation & Discovery

10. **Error Detection** — capture and normalize errors from commands, tools, processes, and environment checks.
11. **Component Discovery** — discover existing runtimes, tools, SDKs, compilers, packages, and other relevant components.
12. **Existing Component Reuse / Linking** — determine whether a discovered component can safely satisfy a requirement before installing another copy.
13. **Project Environment Awareness** — understand project-specific runtime and dependency requirements while keeping PC/System as the primary scope.
14. **Dependency Management** — model missing, outdated, conflicting, and indirect dependencies.
15. **Dependency Graph & Impact Analysis** — connect projects, packages, runtimes, and tools and estimate change impact.

### Level 3 — Verification & Safety Foundation

16. **Verification Engine** — independently prove whether an action actually succeeded.
17. **Permission & Safety Policies** — define Auto, Confirm, and Strict operating modes plus risk-based permissions.
18. **Snapshots & Last Known Good State** — create restoration points before meaningful changes.
19. **Transaction-Based Operations** — combine snapshot, action, verification, commit, and failure handling into one safe operation.
20. **Recovery Journal / Audit Trail** — record observations, decisions, actions, verification, and recovery results.

### Level 4 — Command & Decision System

21. **Command Formula & Generation Engine** — generate environment-specific commands from reusable formulas and validated inputs.
22. **Action / Command Execution Engine** — execute approved generated commands with controlled permissions and result capture.
23. **Deterministic Decision Engine** — select actions from observed state, rules, safety policies, and available recovery paths.
24. **Recovery Strategy Selection** — rank/select the safest viable recovery path instead of blindly trying commands.
25. **Automatic Error Recovery** — connect detection → decision → protection → command execution → verification.
26. **Recovery Circuit Breaker** — stop automatic changes when repeated recovery attempts fail or behavior becomes unsafe.
27. **Rollback & Recovery** — restore snapshots, reverse supported operations, and recover from verified regressions.

### Level 5 — PC Protection & Diagnostics

28. **Environment Health Check** — combine observation and verification into a machine-wide developer-environment health report.
29. **PATH Guardian** — use the state model, history, snapshots, verification, and safety layer to detect and safely repair PATH problems.
30. **Environment Variable Protection** — protect configuration using the same state, snapshot, permission, and verification foundations.
31. **Port & Resource Conflict Detection** — diagnose ports, locked files, processes, and related developer resource conflicts.
32. **Toolchain Doctor** — combine discovery, diagnostics, decision rules, commands, verification, and recovery to repair developer tooling.

### Level 7 — Native Execution Hardening

39. **Windows-Native Process Execution** — replace shell-based _popen() execution with controlled CreateProcessW execution, explicit executable resolution, inherited-output pipe handling, and direct process exit-code retrieval.
40. **Risky Command Execution Boundary** — prevent the command executor from delegating to a generic shell such as cmd.exe; shell syntax must never become an unintended execution escape hatch.
41. **Windows Argument Quoting Hardening** — use Windows-compatible quoting rules for spaces, quotes, and trailing backslashes so generated argument boundaries remain intact.
42. **Controlled Process Output Capture** — capture stdout/stderr through explicit inherited handles instead of shell redirection.
43. **Allowlisted Executable Enforcement** — resolve and execute only explicitly supported developer executables before process creation.

### Level 7 Implementation Notes

Implemented:
- **Windows-native process execution** now uses CreateProcessW instead of _popen() on Windows.
- Executables are resolved with SearchPathW before launch.
- stdout and stderr are captured through a dedicated inherited pipe.
- Process completion and exit status are obtained directly with GetExitCodeProcess.
- The generic cmd executable was removed from the allowlist so Handler's command executor cannot intentionally fall back to arbitrary shell syntax.
- Command argument quoting now handles embedded quotes and trailing backslashes using Windows command-line quoting rules.
- Non-Windows builds retain the existing portable fallback path; production Handler execution remains Windows-native.

Level 7 is still a hardening foundation. Timeout handling, process-tree containment, job-object isolation, and broader execution-policy enforcement remain later hardening work.

### ## Current Release Status

Handler **0.8.0** is the current development release. It is a deterministic C++17 developer-environment protection and recovery tool with Windows-first repair capabilities and portable observation/testing on Linux and macOS.

### Implemented release capabilities

- Deterministic diagnostics, decision routing, command risk classification, and controlled command execution.
- Environment state, history, snapshots, recovery journal, verification, and transactional operations.
- Python and Node.js dependency inspection, compatibility analysis, guarded upgrades, and project-local uninstall.
- Python/Node repair workflows with verification and recovery handling.
- Toolchain Doctor with guarded Windows/winget upgrades for supported installed tools.
- PATH and environment protection/repair foundations.
- TEMP cleanup with dry-run and confirmation safeguards.
- Linux/macOS core observation and CI support.
- Explicit safety boundaries that avoid claiming unsupported OS-level or filesystem-wide rollback.

### Deliberate safety boundaries

Handler does not silently remove global packages, runtimes, operating-system applications, or arbitrary files. Package uninstall is limited to direct Python/Node.js project dependencies and requires project-local environments plus verification. Toolchain repair is limited to supported installed tools on Windows; missing runtimes and unsupported package-manager paths remain blocked or review-only.

### Platform support

| Capability | Windows | Linux | macOS |
|---|---|---|---|
| Build + core tests | ✅ | ✅ | ✅ |
| Environment observation | ✅ | ✅ | ✅ |
| PATH/component discovery | ✅ | ✅ | ✅ |
| Python/Node dependency analysis | ✅ | ✅ | ✅ |
| Project package uninstall | ✅ | ✅ | ✅ |
| Guarded Python/Node repair | ✅ | Foundation/limited | Foundation/limited |
| Toolchain automatic repair | Windows/winget | ❌ | ❌ |
| Windows-native process containment | ✅ | N/A | N/A |

### CLI quick reference

```text
handler help
handler version
handler health
handler self-check
handler status
handler state
handler history
handler discover [tools...]
handler project
handler deps [--upgrade <package>]
handler dependency-upgrade <package>
handler graph
handler detect-error <text>
handler decide <error text>
handler recover <error text>
handler repair python-module <package>
handler repair node-module <package>
handler doctor
handler doctor-repair
handler toolchain-repair <tool>
handler updates
handler risk <command>
handler path-audit
handler path-repair <baseline-file>
handler env-baseline <file> <name...>
handler env-audit <file>
handler env-repair <file> [--user]
handler protect
handler safe-mode
handler temp-cleanup [--dry-run]
handler maintenance
handler uninstall <python|node> <package> [--dry-run]
handler snapshots
handler rollback <snapshot-id>
handler modules
```

Real uninstall is confirmation-gated; `--dry-run` only plans the operation. Rollback is reported as verified only after the captured package version is checked again.

`handler version` reports **0.8.0**.

## Performance Model


Handler should be designed around **minimum necessary execution**.

It should not behave like a large application where every subsystem is active all the time. Instead:

**Event → Determine required capability → Load/activate capability → Perform task → Verify → Refresh affected state → Release capability**

For example, an npm dependency problem should not activate Python diagnostics, Java management, unrelated project scanners, or system-wide cleanup.

The goal is a **lightweight core with specialized capabilities activated on demand**.

### Command Generation Performance

Command formulas and action definitions should remain available as lightweight definitions, while expensive resolvers and specialized command-generation logic should be loaded only when the relevant action is selected.

Handler should not generate, validate, or resolve commands that are unrelated to the current operation.

## Design Principles

1. **Deterministic over opaque** — decisions should come from explicit rules and observable state.
2. **Verify every important action** — successful command execution does not automatically mean successful recovery.
3. **Least privilege** — Handler should request only the permissions required for the current operation.
4. **Protect before modifying** — create an appropriate recovery point before risky changes.
5. **Permission before destruction** — destructive cleanup must be user-approved.
6. **Dependency awareness** — never remove something merely because it is not directly imported by the current project.
7. **Project awareness** — changes must consider other projects sharing the environment.
8. **Fail safely** — repeated recovery failures should stop automatic action rather than escalate blindly.
9. **Transparent actions** — developers should be able to see what Handler changed.
10. **Recoverable operations** — important modifications should have a restoration path whenever technically possible.
11. **On-demand execution** — do not run components that are irrelevant to the current task.
12. **Targeted refresh** — update only the state affected by a change unless a full rescan is required.
13. **Low overhead** — Handler should minimize CPU, memory, disk, and background activity.
14. **Reuse before reinstall** — discover existing compatible components before creating unnecessary duplicates.
15. **Never break intentional isolation** — safe reuse must respect project and environment boundaries.

## Planned Architecture

Handler is expected to evolve around these logical components:

```text
                    Handler Core
                         │
                 Event / Task Router
                         │
             ┌───────────┴───────────┐
             ↓                       ↓
       Required Modules         Dormant Modules
             │
             ↓
      Diagnostics Engine
             ↓
       Decision Engine
             ↓
   Component Discovery Engine
             ↓
   Command Formula Engine
             ↓
   Safety & Permission Layer
             ↓
      Action / Command Engine
             ↓
      Verification Engine
             ↓
   Recovery / Rollback Engine
             ↓
      Targeted State Refresh
             ↓
       History & State Store
```

Specialized modules should be independently activatable. The architecture should support loading only what a task requires and releasing or idling components after completion.

## Scope

Handler is intended to focus on **developer environments and development tooling**.

It is not intended to replace:
- Antivirus software
- Full system backup software
- Enterprise endpoint management
- General-purpose AI coding assistants


## Batch 1 — Dependency Intelligence

Batch 1 adds the first production-oriented dependency intelligence layer:

- **Version constraint solving** for exact/comparison ranges plus common `^` and `~` ranges.
- **Constraint intersection checks** so conflicting requirements are reported only when no supported version satisfies the combined constraints.
- **Compatible-version selection** chooses the highest compatible version from registry-discovered candidates.
- **Transitive dependency closure** expands known dependency edges and performs reverse impact traversal for affected projects/packages.
- **Guarded package upgrades** support Python and Node.js through registry lookup, explicit user confirmation, Handler transactions, artifact backups, post-install verification, and recovery journaling.
- CLI support: `handler deps --upgrade <package>` and `handler dependency-upgrade <package>`.

The solver is intentionally deterministic and conservative. It does not silently upgrade packages, and registry-backed changes remain confirmation-gated. Full ecosystem-specific lockfile solving and installer support for Rust/Go/C++ remain later work.


## Batch 2 — Recovery & Repair Hardening

Batch 2 strengthens the recovery foundation without broadening Handler into an unrestricted system cleaner:

- **Centralized Handler state/recovery roots** prevent transaction, repair, and dependency workflows from drifting to different storage locations.
- **Artifact backup integrity metadata** records original/backup sizes and validates the backup before restoring it.
- **Explicit repair cancellation semantics** use exit code `2` instead of reporting a cancelled repair as success.
- **Stronger Python repair verification** now requires both package visibility and `pip check` consistency before a repair commits.
- Regression coverage now exercises the shared state root and artifact restoration path.

The recovery layer still does not claim full filesystem rollback. Snapshots capture Handler environment state, while artifact backups cover only files explicitly registered by a transaction.

## Phase 3 — Linux & macOS Portability

The portability layer now supports Unix-like environments for core observation workflows:

- PATH-based developer-tool discovery on Linux/macOS.
- XDG state storage with `$XDG_STATE_HOME`, then `$HOME/.local/state/handler`.
- Portable hostname/user/TMPDIR detection.
- Cross-platform CMake/CTest CI on Windows, Ubuntu, and macOS.

Windows remains the strongest platform for package-manager-backed repair. Linux/macOS package installation and automatic repair remain intentionally disabled until platform-specific installer policies and rollback semantics are implemented.


🚧 **Level 6 — Maintenance & Advanced Operations Foundation Implemented**

Current repository is a **native C++17 / Windows CMD-first** implementation with **PC/System Environment** as the primary target.

Implemented:
- Native CLI and CMake build
- PC/system environment observation
- State store and history
- Modular command routing
- Maintenance/idle foundation
- TEMP cleanup capability
- Deterministic error-pattern detection
- Windows PATH component discovery
- Existing-component observation
- Project context detection
- Lightweight dependency manifest inspection
- Project → dependency graph foundation

Not yet production-complete:
- Automatic repair
- Package installation/removal
- Transitive dependency resolution
- Compatibility solving
- Snapshots/rollback
- Full verification and permission engine
- Transactional recovery

## Phase 4 — Guarded Project Package Uninstallation

Phase 4 begins the production uninstall system with a deliberately narrow boundary:

- **Python and Node.js project dependencies only**; operating-system application removal is not enabled by this engine.
- **Project-local scope only**; global package removal is blocked by design.
- **Direct dependencies only**; transitive dependency removal is blocked until the dependency graph can prove that removal is safe.
- **Preflight planning** verifies the project manifest, direct dependency declaration, installed package version, and expected package-manager command before any modification.
- **Manifest/lockfile backups** are created before the destructive package operation.
- **High-risk transaction protection** creates a Handler recovery snapshot before uninstall.
- **Post-action verification** confirms the package is absent from the project environment.
- **Rollback attempt** reinstalls the exact captured version and restores backed-up project artifacts if verification fails. Handler does not claim filesystem-level rollback beyond the artifacts and package-manager operation it can actually restore.
- **Dry-run support**: `handler uninstall <python|node> <package> --dry-run`.
- **Explicit confirmation** is required immediately before the uninstall operation.

Supported commands: `handler uninstall python <package>`, `handler uninstall node <package>`, and their `--dry-run` variants.

The first Phase 4 batch intentionally does not remove global tools, runtimes, or operating-system applications. Those operations require separate package-manager policy, privilege handling, cross-project impact analysis, and truthful rollback semantics.

## Phase 4 — Batch 2: Uninstall Safety Hardening

Batch 2 strengthens the guarded uninstall boundary:

- Python package removal now requires a project-local `.venv` or `venv`; Handler will not fall back to a global Python interpreter for destructive uninstall.
- Node.js package removal requires project-local `node_modules`.
- Pre-uninstall impact analysis checks other declared Python dependencies for a dependency relationship to the target package and blocks removal when a declared dependent is detected.
- Rollback status is now surfaced separately from transaction failure so Handler does not report an unverified recovery as successful.
- Uninstall regression tests cover project-local scope enforcement and deterministic safety boundaries.

The uninstall engine still does **not** remove global packages, runtimes, or operating-system applications.

## Phase 4 — Batch 3: Final Uninstall Hardening

The final Phase 4 batch closes the guarded project-uninstall implementation:

- Rollback is no longer reported as verified merely because a rollback callback ran; Handler re-checks the exact captured package version after recovery.
- CLI output explicitly reports whether a rollback was attempted and whether it was verified.
- Additional regression tests cover missing manifests and malformed Node project manifests.
- Project-local scope remains mandatory: Python uses `.venv`/`venv`, and Node.js requires `node_modules`.
- Global packages, runtimes, operating-system applications, and transitive-only removal remain blocked.

### Phase 4 completion boundary

Phase 4 is complete for its defined scope: safe removal of direct Python/Node.js project dependencies with confirmation, preflight checks, backups, transaction protection, verification, and truthful rollback reporting. It does not claim universal package-manager rollback or operating-system application removal.

## Roadmap

### Phase 0 — Specification
- Feature definition
- Safety model
- Error taxonomy
- Recovery rules
- Supported platforms
- Environment state model
- Modular execution model
- Targeted refresh model
- Command formula model
- Existing component discovery model

### Phase 1 — Environment Observation
- Native C++/Windows CLI foundation
- PC/System target model
- Basic system environment inspection
- Environment state store
- Change history
- Task routing / on-demand module registry
- Maintenance idle loop
- Handler self-check
- TEMP cleanup capability
- Two-hour maintenance mode
- Detect installed tools/runtimes
- Inspect project environments
- Build environment state snapshots
- Track changes
- On-demand discovery
- Targeted state refresh
- Existing component discovery

### Phase 2 — Diagnostics
- Error parsing
- Dependency analysis
- Environment health checks
- Toolchain diagnostics
- Resource conflict detection
- Component availability and compatibility analysis

### Phase 3 — Safe Recovery
- Deterministic rules
- Command formula generation
- Action execution
- Verification
- Snapshots
- Rollback
- Recovery circuit breaker

### Phase 4 — Maintenance
- Updates / toolchain inspection and guarded Windows repair
- TEMP cleanup
- Guarded direct Python/Node.js project uninstallation
- Project-aware dependency maintenance
- Safe component reuse foundations

### Phase 5 — Performance & Hardening
- Cross-platform execution hardening
- Transactional operations and recovery journal
- Permission and risk model
- CI/build/test hardening
- Release documentation and final audit

## Status of the Feature List

This README is intentionally a living specification. Features will be added, refined, prioritized, and moved into implementation phases as Handler evolves.

---

**Handler** — *protect the environment, fix the problem, verify the result.* 🛡️

## Batch 3 — Guarded Toolchain Repair

Batch 3 moves Toolchain Doctor from observation-only repair proposals to a narrowly scoped, package-manager-backed repair path on Windows:

- **Verified winget boundary** — `winget` is now explicitly allowlisted; generic shell execution remains outside Handler's command boundary.
- **Supported package IDs** — Python, Node.js, Git, CMake, and .NET SDK use fixed winget package IDs rather than free-form package input.
- **Installed-tool-only repair** — missing runtimes are still blocked from automatic installation. Repair operates only on an already discovered toolchain.
- **Source pinning** — upgrades use the explicit `winget` source and accept source/package agreements non-interactively after the user has approved the Handler operation.
- **Transactional execution** — upgrades run through Handler's High-risk transaction layer with a recovery snapshot and post-action health verification.
- **Safe failure semantics** — package downgrade is not fabricated as rollback; if verification fails, Handler records the retained snapshot and reports that package-level downgrade was not attempted.
- **Cancellation semantics** — an explicit cancellation returns exit code `2` rather than success.
- **Regression coverage** — tests cover the winget allowlist/risk boundary and avoid executing real package-manager upgrades during CI.

### Toolchain Repair Boundary

Handler intentionally does **not** claim that a snapshot can undo an operating-system package installation. The recovery snapshot protects Handler's saved environment baseline; package-manager rollback is a separate capability that is not silently attempted. This keeps the repair path honest and prevents a false sense of filesystem-level rollback.

Supported automatic repair targets in this batch:

| Tool | Package ID | Automatic action |
|---|---|---|
| Python | `Python.Python.3` | Upgrade installed package |
| Node.js | `OpenJS.NodeJS` | Upgrade installed package |
| Git | `Git.Git` | Upgrade installed package |
| CMake | `Kitware.CMake` | Upgrade installed package |
| .NET SDK | `Microsoft.DotNet.SDK` | Upgrade installed package |

Missing tools and unsupported ecosystems remain review-only until an explicit installer/source policy exists.

Handler version is now **0.8.0**.

