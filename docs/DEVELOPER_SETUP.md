# Developer Environment Setup (v1.0 plan)

Handler's developer setup feature is designed to prepare a Windows development environment through named, reviewable profiles.

## First increment

The first increment provides a **read-only plan preview**:

```text
handler setup profiles
handler setup plan base
handler setup plan python
handler setup plan web
handler setup plan fullstack
handler setup plan native
handler setup plan custom
```

It does not execute WinGet or change the machine. Package identifiers in the code are reviewed catalog entries and must be revalidated against current WinGet metadata before a future installation implementation is enabled.

## Profiles

- `base`: Python Install Manager, Git, Visual Studio Code.
- `python`: base tools; project-local virtual environment and Python quality-tool setup are later increments.
- `web`: base tools plus Node.js LTS.
- `fullstack`: base tools plus Node.js LTS. Database and Docker remain opt-in.
- `native`: base tools plus CMake. CMake is not a compiler; an MSVC/Build Tools workload requires separate compatibility review and explicit approval.
- `custom`: catalog preview covering optional GitHub CLI, Windows Terminal, Node.js, JDK, CMake, Go, Rustup, PostgreSQL, and Docker Desktop.

## Intended follow-up increments

1. Read-only host scan: OS version, architecture, WinGet availability, existing package/runtimes, and free disk space.
2. Dependency and compatibility resolver; revalidate package IDs, installer architecture, and source.
3. Explicit install preview showing version/source/estimated disk footprint and privilege requirements.
4. WinGet installation behind Handler's allowlist, risk policy, transaction lock, recovery journal, confirmation, timeout, and result verification.
5. Project templates and virtual environments.
6. Repair diagnostics and supported recovery paths.

## Safety requirements before installation execution

- Never execute arbitrary IDs, shell fragments, or package names provided as raw command strings.
- Build process arguments as structured arguments; do not use generic shell execution.
- Require explicit confirmation for changes and elevation only when necessary.
- Preserve existing Python installations and report PATH conflicts instead of silently overwriting them.
- Detect already installed packages before proposing installation.
- Verify installed package/version after the package manager exits.
- Log action outcome and distinguish installation failure from rollback. Do not claim OS-level rollback unless it was verified.
- Do not install C/C++ build workloads, databases, or Docker by default.
- Fail closed if Handler has an unfinished high-risk transaction.
- Keep plan mode free of package manager invocations and machine changes.

## Planning notes

Disk-space numbers should be presented only when a reliable package-manager or installer estimate is available. Otherwise display "unknown; check vendor requirements" rather than invent a precise footprint. Python libraries such as pytest and Ruff belong in a project virtual environment rather than a global Python environment.
