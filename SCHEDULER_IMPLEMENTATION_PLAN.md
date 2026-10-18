# Handler Scheduled Task Runner — Implementation Plan

## Goal

Add command-based recurring tasks to Handler, execute due tasks sequentially, and start the scheduler automatically for the current Windows user. Keep the existing deterministic C++17 architecture; this does not introduce an LLM or arbitrary shell execution.

## Repository audit

- The application already has a CLI router and a module registry with on-demand callbacks.
- State and history are stored below the per-user Handler state directory.
- Existing command and repair paths already have risk classification and interactive confirmations.
- The current source tree has no dedicated persistent recurring-task scheduler.
- The README describes idle/event-driven work and scheduled checks as part of the intended architecture, but the recurring task queue and Windows startup registration are not implemented as a distinct feature.

## Implementation

1. **Persistent schedule store**
   - Save task ID, interval in minutes, allowlisted Handler command, and last-run epoch in the user state directory.
   - Validate IDs, intervals (1–10,080 minutes), records, and duplicate IDs.
   - Write via a temporary file and replace the schedule file to reduce corruption risk.
   - Fail closed if a schedule record is malformed.

2. **Sequential due-task runner**
   - Check the due time on each poll.
   - Execute due commands one at a time through registered Handler modules; never pass schedule data to a shell.
   - Persist the run timestamp before executing to avoid repeated execution if a process crashes mid-task.
   - Use a process lock to prevent overlapping scheduler ticks.
   - Record outcomes in Handler history.

3. **Safety boundary**
   - Only allow non-destructive diagnostics/state reporting to run unattended.
   - Reject cleanup, install/update, repair, uninstall, rollback, arbitrary process execution, and unknown commands from the scheduler.
   - Those operations remain on the interactive CLI path with the existing explicit confirmation policy.

4. **Windows startup**
   - Provide a per-user PowerShell installer that registers one Task Scheduler task to check every minute and another to check immediately at logon.
   - Provide a matching uninstall script.
   - Tasks run with limited user privileges; no administrator elevation is requested by the installer.

5. **CLI**
   - handler schedule list
   - handler schedule add <id> <interval-minutes> <safe-command>
   - handler schedule remove <id>
   - handler schedule run-due

## Test and acceptance criteria

- Accept valid safe tasks and persist/reload them.
- Reject unsafe commands, invalid IDs, invalid intervals, and duplicate IDs.
- Verify tasks are not due before their interval and become due at the boundary.
- Verify run timestamps defer the next run.
- Verify removing tasks works and malformed state fails closed.
- Build and run the existing CTest suite on supported CI platforms.
- Inspect the GitHub Actions result before treating the change as verified.

## Follow-up work (not part of this first slice)

- Exact wall-clock daily/weekly triggers and time-zone/DST semantics.
- User-visible approval inbox for scheduled operations that are intentionally risky.
- Retry/backoff policies, missed-run policy, task priorities, and richer task arguments.
- Installer packaging so startup registration can be offered from a signed/released installer.
