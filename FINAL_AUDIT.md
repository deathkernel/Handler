# Handler Final Audit — 0.9.0 security hardening

Audit scope:
1. Bugs
2. Security
3. Safety boundaries
4. Rollback/recovery
5. Dependency handling
6. Windows/Linux/macOS
7. CLI
8. Tests
9. CMake/build
10. README/documentation
11. Dead/stale code
12. False claims / unsupported behavior

## Critical findings fixed during audit

- Snapshot IDs/path handling was hardened against traversal and malformed IDs; regression coverage was added.
- Snapshot filename collisions are handled without overwriting an existing snapshot.
- Environment state capture now reports the 0.8.0 release version and uses portable Unix hostname/user/TMPDIR fallbacks.
- Toolchain Doctor now uses an explicitly approved transaction path rather than a second unfulfilled confirmation gate.
- Transaction rollback callbacks now return a real success/failure result instead of being treated as successful merely because a callback existed.
- Python dependency upgrades require a project-local .venv/venv instead of modifying a global interpreter.
- Python module repair no longer falls back to modifying a global interpreter.
- TEMP cleanup refuses a filesystem-root target.
- Command execution timeout containment was hardened for Windows job-assignment failure and Unix process groups.
- README stale Level 6/uninstall roadmap claims were cleaned up.

## 0.9.0 security hardening completed

- Windows command execution now rejects resolved executables outside trusted installation roots.
- Explicit Windows executable paths are canonicalized and must belong to a trusted installation root, Handler-owned state, or the narrowly-defined project-local Python virtual-environment layout (.venv/venv/Scripts/python.exe).
- Windows npm.cmd execution is routed through a trusted cmd.exe interpreter rather than passed directly to CreateProcessW.
- Windows child-process handle inheritance is restricted to Handler's stdout/stderr pipe via STARTUPINFOEX handle-list attributes.
- Tool discovery applies the same Windows trust-root rule and rejects executables nested under the current directory, preventing project-local executables from masquerading as discovered runtimes.
- PATH and environment baselines are accepted only from Handler-owned persistent state.
- Transaction artifact filenames now include a deterministic source-path digest, preventing same-named artifacts from different projects from overwriting one another.
- Regression tests cover rejection of untrusted baseline paths and Handler state-path boundaries.

## Second-pass audit status

The second pass found and fixed three additional Windows execution-boundary issues:

1. Generic workingDirectory executable trust was narrowed so a project cannot simply place a fake python.exe in an arbitrary directory and have Handler execute it.
2. Windows .cmd execution was corrected for npm; CreateProcessW does not directly execute batch files, so npm is launched through the trusted system command interpreter.
3. Child-process handle inheritance was narrowed to the output pipe instead of inheriting every inheritable handle in the Handler process.
4. Windows process containment now fails closed if the Job Object cannot be created, configured, or assigned; Handler no longer proceeds with an uncontained child process.
5. Automated Node.js dependency repair/upgrade uses npm `--ignore-scripts` so package lifecycle scripts are not executed implicitly by Handler.
6. Node dependency rollback now restores package metadata and reconstructs `node_modules` with `npm ci --ignore-scripts` when a lockfile is present, avoiding a manifest-only rollback.
7. Unix command execution now honors and canonicalizes explicit executable paths, so project-local Python repairs cannot silently fall back to PATH/global Python.

The branch has been updated to 0.9.0 project metadata and main.cpp now uses the centralized Handler state-root implementation.

A local build could not be executed in this audit environment because outbound GitHub cloning/DNS is unavailable. Therefore compile/test status is **not asserted** here; the GitHub Actions run remains the authoritative build gate.

## Remaining bounded limitations

- Toolchain package-manager rollback is intentionally not implemented; Handler retains the recovery snapshot and reports that package downgrade was not attempted.
- Linux/macOS automatic toolchain package repair remains disabled.
- Dependency solving is intentionally lightweight and is not a full lockfile SAT/resolution engine.
- Automated npm repair/upgrade intentionally skips package lifecycle scripts; projects that require install scripts need an explicit package-manager workflow outside this automated repair path.
- Deep cleanup code exists as a low-level foundation but is not exposed as a CLI capability.
- CLI integration coverage is smaller than unit-level coverage; CI validates build + core test executable on all three supported CI operating systems.

## Audit conclusion

The repository is release-oriented within its documented scope, but the limitations above must remain explicit. The final CI run for this audit branch is the release gate; no claim of post-fix green status is made until that run completes.

- Unix Doctor port diagnostics now report `UNKNOWN` when probing is unsupported instead of implying the port is free.

- Dependency compatibility checks now use constraint-derived boundary candidates instead of a fixed 0–20 version search range; high-version regression coverage was added.
