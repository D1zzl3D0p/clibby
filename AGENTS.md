# Clibby session context

Start with `docs/handoff.md`, then `docs/architecture.md` and
`docs/results/2026-10-08-comparison.md`.

- User direction: lightweight procedural desktop pets, modular characters and
  behaviors, stable language-neutral input interfaces, low battery impact.
- Qt Quick is the active direction. Godot and the original study are archived;
  do not treat old archived plans as current implementation requirements.
- The modular product architecture is incremental. `qt/` contains the C++17
  resident runtime/host/shell and preserved benchmark; `shared/` is a frozen
  test fixture. Read the current handoff for implemented stages and open checks.
- Preserve raw evidence in `docs/evidence/` and `docs/archive/`. New benchmark
  output belongs in ignored `results/` directories with new run names.
- CPU/RSS and offscreen tests do not establish battery impact or desktop parity.
- The user requested effective use of subagents; delegate independent work with
  explicit file ownership when useful.
- In the maintainer's agent environment, prefix shell commands with `rtk`
  (`rtk proxy <command>` for passthrough). RTK is an optional local wrapper,
  not a project dependency; public setup instructions use ordinary commands.
- Use `.venv/bin/python` for Python tests; system Python may lack Pillow/psutil.
- Follow current user instructions over archived process notes. Do not create
  extra approval rounds for work the user has already authorized.
