# Explicit command outcome producer

`tools/pet_run.py` runs an actual command and sends its lifecycle through the
[version-1 session API](session-v1.md). It publishes `working` immediately before
launch and `completed` for exit zero or `failed` for any other outcome. This is a
small real command adapter, with no coding-chat integration or inactivity-based
completion inference.

Start the resident, then run a command from its project directory:

```sh
.venv/bin/python tools/pet_run.py \
  --label "Run project tests" -- .venv/bin/python -m unittest discover -s tests
```

The wrapper's default `petctl` path is resolved relative to its own checkout,
so it also works from another project's directory. Each invocation generates a
unique session ID under source `command`; two concurrent commands remain separate.
The command runs with inherited working directory, environment, stdin, stdout
and stderr. Arguments after `--` pass directly to the executable, with no shell
interpretation. For shell syntax, explicitly invoke your shell after `--`.

The default Open source target is a `file:` URI for the command's canonical
working directory. Override it with a specific file or web URL when useful:

```sh
.venv/bin/python tools/pet_run.py \
  --source build --label "Build release" \
  --source-url https://example.com/builds/current \
  -- cmake --build build/qt
```

Other options are `--petctl PATH`, `--state-dir ABS_DIR`, `--session ID`, and
`--ttl SECONDS` (1–3600, default 3600). The source URL uses the additive
`petctl update --source-url` option. Both lifecycle updates include the same
identity, label and source URL, with sequences 1 and 2. Prefer the generated
session ID: reusing `--session` while an earlier record exists can reject the
sequence as stale.

TTL bounds retained state; the wrapper has no refresh timer or polling loop.
For a command longer than TTL, its working card can expire. The terminal event
still comes only from the actual process outcome, and is retained for a fresh
TTL. Resident sleep/wake can also clear its current working record. No elapsed
deadline means that the command completed.

The wrapper returns the command's exit code. A signal-terminated child returns
the conventional `128 + signal` code. A missing executable returns 127; other
launch errors return 126. An interrupted wrapper reports failure with 130 when
Python receives `KeyboardInterrupt`. Force-killing the wrapper cannot guarantee
a terminal notification; retained state expires normally.

Notification failure prints a diagnostic to stderr and does not prevent the
command from running or alter its result. Each publication has a four-second
transport deadline and no retries. Successful `petctl` response JSON is suppressed,
preserving the command's stdout. Transport failures never imply completion.

## Verification limits

`tests/test_pet_run.py` exercises real command processes and a fake executable
`petctl`, checking lifecycle order, success/failure/signal exit outcomes,
source metadata, launch errors and transport failure isolation. This proves the
producer boundary only. Live resident/socket delivery, Open source and
Acknowledge interaction still require their separate integration/native checks;
these tests establish neither desktop behavior nor battery impact.
