# Handler 🛡️

> **A deterministic developer-environment protection, diagnosis, repair, maintenance, and recovery system.**

Handler is a planned developer tool designed to protect a developer's coding environment from problems caused by dependency conflicts, missing tools, broken configurations, failed updates, incomplete installations, and other development-environment failures.

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
Handler can execute required commands to repair recognized problems.

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

### 🗑️ 17. Complete Uninstallation
For supported software and development components, Handler should be able to perform a permission-based deep uninstall rather than removing only the visible application.

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

The cleanup must remain conservative: Handler should not blindly delete files outside the intended TEMP scope.

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

### Level 6 — Maintenance & Advanced Operations

33. **Development Language & Tool Updates** — update runtimes, SDKs, compilers, and developer tools using compatibility checks and recovery protection.
34. **Risky Command Interception** — apply the safety and decision systems to operations that could affect the wider machine or multiple projects.
35. **Safe Mode / Isolation** — test or stage risky changes in isolation where practical.
36. **Scheduled TEMP Cleanup** — use the maintenance/scheduling layer, scope rules, safety checks, execution, and verification to perform periodic %TEMP% cleanup.
37. **Deep Cleanup** — safely identify and remove genuinely unused development components using dependency and impact analysis.
38. **Complete Uninstallation** — perform permission-based deep removal using discovery, dependency analysis, safety, transactions, and verification.

### Dependency Rule

A feature should not be considered complete merely because its code exists. Its **required foundation must already be reliable**.

For example:

`Automatic Error Recovery`
→ requires Error Detection + Component Discovery + Verification + Safety + Snapshots + Command Execution + Decision Rules.

`Deep Cleanup`
→ requires Dependency Management + Impact Analysis + Discovery + Safety + Snapshots + Verification + Transaction/Recovery support.

`Complete Uninstallation`
→ requires the same foundations plus stronger permission and rollback safeguards.

This ordering is the canonical implementation sequence. Individual low-level tasks may be developed earlier for testing, but production capabilities should follow these dependency levels.

## Level 2 Implementation Notes

Level 2 is intentionally **observation-only**. It discovers and models the environment without installing, removing, linking, or repairing components.

### Error Detection
- Deterministic pattern matching for missing modules/commands, dependency conflicts, permission failures, and resource conflicts.
- Confidence-scored observations.
- No automatic repair yet.

### Component Discovery
- Windows PATH-based discovery for common developer tools.
- Default scan covers Python, Node.js, Git, CMake, .NET, Java, Go, and Cargo.
- Custom tool names can be supplied.
- Discovery happens before future installation/recovery decisions.

### Existing Component Reuse / Linking Foundation
- Discovered components are represented with name, kind, path, and execution status.
- Current stage is observation-only.
- No unsafe copying or linking is performed.

### Project Environment Awareness
- Walks upward from the current directory to detect supported project manifests.
- Recognizes Python, Node.js, Rust, Go, and C/C++ project contexts.
- PC/System remains the primary scope; project context only refines future decisions.

### Dependency Management Foundation
- Lightweight manifest inspection for supported ecosystems.
- Reads requirements.txt, package.json, Cargo.toml, and go.mod.
- Does not install, upgrade, or remove anything.

### Dependency Graph Foundation
- Creates deterministic project → declared-dependency edges.
- Provides the base for later impact analysis.
- Deeper transitive/runtime impact analysis remains future work.

### Level 2 CLI
- handler detect-error <error text>
- handler discover [tool ...]
- handler project
- handler deps
- handler graph

## Level 3 Implementation Notes

Level 3 establishes Handler's safety and verification foundation before any automatic repair is introduced.

Implemented:
- **Verification Engine** — file/directory outcome checks.
- **Permission & Safety Policies** — Auto, Confirm, and Strict modes with risk levels.
- **Snapshots** — serialized environment-state recovery points.
- **Transaction Foundation** — action → verification → commit, with rollback callback on failure.
- **Recovery Journal** — timestamped observation/action/recovery records.
- Handler version bumped to **0.3.0**.

Level 3 remains a **foundation**, not a full recovery system. Snapshot restore, richer verification, interactive confirmation, and complete transactional rollback will be expanded before production automatic repair.

## Level 4 Implementation Notes

Level 4 adds the deterministic command and recovery decision foundation.

Implemented:
- **Command Formula & Generation Engine** — structured command specs, safe argument quoting, and an allowlisted executable set.
- **Action / Command Execution Engine** — executes only allowlisted commands and captures output/exit status.
- **Deterministic Decision Engine** — maps detected error categories to bounded next-step decisions.
- **Recovery Strategy Selection** — ranks safe recovery directions before execution.
- **Automatic Error Recovery Foundation** — recovery permission is evaluated through the Level 3 safety policy.
- **Recovery Circuit Breaker** — stops repeated recovery attempts after a configurable failure limit.
- Handler version bumped to **0.4.0**.

Level 4 is intentionally conservative: it does not silently install packages, modify PATH, kill processes, or perform destructive repairs. Those actions require later policy-backed capabilities and stronger verification.

## Level 5 Implementation Notes

Level 5 adds PC protection and diagnostics on top of the Level 4 decision system.

Implemented:
- **Environment Health Check** — validates key environment areas.
- **PATH Guardian** — detects PATH entries that no longer resolve to directories.
- **Environment Variable Protection** — observes important variables without modifying them.
- **Port & Resource Conflict Detection** — probes common development ports.
- **Toolchain Doctor** — checks availability of common development tools through component discovery.
- New diagnostic command: `handler protect`.
- Handler version bumped to **0.5.0**.

Level 5 remains observation-first: it reports risks and conflicts but does not silently modify PATH/environment variables, terminate processes, or change system configuration.

## Level 6 Implementation Notes

Level 6 adds maintenance and advanced-operation foundations.

Implemented:
- **Development Language & Tool Updates** — discovers installed tool locations and produces update-review candidates.
- **Risky Command Interception** — classifies destructive/system-changing command patterns as Safe, Review, or Blocked.
- **Safe Mode / Isolation** — creates a Handler sandbox context and validates paths against it.
- **Deep Cleanup Foundation** — scoped cleanup primitive with an explicit backup gate.
- **Complete Uninstallation Foundation** — validates uninstall targets and rejects filesystem roots.
- Existing scheduled TEMP cleanup remains part of the maintenance layer.
- New commands: `handler updates`, `handler risk <command>`, `handler safe-mode`.
- Handler version bumped to **0.6.0**.

Level 6 is intentionally conservative. Update discovery does not silently upgrade software, risky-command detection does not execute or rewrite commands, deep cleanup is scoped, and uninstallation requires explicit confirmation. Production-grade backup/restore and full Windows installer/package-manager integration remain hardening work.

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

## Project Status

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
- Updates
- Deep cleanup
- Complete uninstallation
- Project-aware dependency maintenance
- Safe component reuse

### Phase 5 — Performance & Hardening
- Modular/lazy execution
- Idle state
- Transactional operations
- Recovery circuit breaker
- Permission model
- Audit/recovery journal
- Extensive testing

## Status of the Feature List

This README is intentionally a living specification. Features will be added, refined, prioritized, and moved into implementation phases as Handler evolves.

---

**Handler** — *protect the environment, fix the problem, verify the result.* 🛡️
