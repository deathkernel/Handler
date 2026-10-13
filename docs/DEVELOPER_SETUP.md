# Developer Environment Setup (v1.0)

Handler's Windows-first setup workflow provides named profiles, a read-only host preflight, and minimal project scaffolds. The setup planner remains read-only: it does not install packages.

## Commands

```text
handler setup scan
handler setup profiles
handler setup plan base
handler setup plan python
handler setup plan web
handler setup plan fullstack
handler setup plan native
handler setup plan custom
handler setup project python <path>
handler setup project web <path>
handler setup project native <path>
```

## Host preflight

`handler setup scan` reports the current platform, compiler-visible architecture, free space on the current working-directory volume, and trusted executable-path discovery for Python launchers/interpreters, Git, CMake, Node/npm, and (on Windows) WinGet. Version-qualified Python candidates are shown when present so side-by-side installations can be spotted without running a package manager. It is observational only and does not invoke detected tools. Free space is not an installer-size estimate. Discovery is limited to trusted executable paths exposed by the current PATH; it is not a complete registry-based inventory of every Python installation.

## Profiles

- `base`: Python Install Manager, Git, Visual Studio Code.
- `python`: base tools; Python project scaffolding is available, but dependencies are not installed automatically.
- `web`: base tools plus Node.js LTS.
- `fullstack`: base tools plus Node.js LTS. Database and Docker remain opt-in.
- `native`: base tools plus CMake. CMake is not a compiler; an MSVC/Build Tools workload requires separate compatibility review and explicit approval.
- `custom`: catalog preview covering optional GitHub CLI, Windows Terminal, Node.js, JDK, CMake, Go, Rustup, PostgreSQL, and Docker Desktop.

## Project scaffolds

Project creation supports `python`, `web`, and `native`. It refuses to overwrite an existing target, requires an existing parent directory, and removes the newly created target on scaffold failure where possible. It writes starter manifests, README/.gitignore files, and minimal source/test files. It does not create a Python virtual environment or install dependencies; run those steps explicitly after reviewing the generated files.

## Installer boundary

WinGet package installation is not enabled by this feature increment. Package identifiers and source metadata still require live validation, installed-package detection, disk/privilege preflight, explicit confirmation, transaction/recovery integration, and post-install verification before installation execution is safe to enable.

## Safety requirements for future installation execution

- Never execute arbitrary IDs, shell fragments, or package names as raw command strings.
- Build process arguments as structured arguments; do not use generic shell execution.
- Require explicit confirmation for changes and elevation only when necessary.
- Preserve existing Python installations and report PATH conflicts instead of silently overwriting them.
- Detect already installed packages before proposing installation.
- Verify installed package/version after the package manager exits.
- Log action outcome and distinguish installation failure from rollback. Do not claim OS-level rollback unless it was verified.
- Do not install C/C++ build workloads, databases, or Docker by default.
- Fail closed if Handler has an unfinished high-risk transaction.
- Keep plan mode free of package-manager invocations and machine changes.

Disk-space estimates should be shown only when a reliable installer estimate is available; otherwise report that the size is unknown. Python libraries such as pytest and Ruff belong in a project virtual environment rather than a global Python environment.
