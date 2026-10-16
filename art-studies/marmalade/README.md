# Selected woodland cat art direction

`woodland/` preserves the exact B model recipe selected by the user, its palette
pass and native 96×96 rest reference. `poses.json` is the recorded 120-frame,
15 fps procedural pose input used for all three comparison studies. Preserve
this baseline while refining the face or adapting the artwork to runtime rigs.

From the repository root, with Blender installed:

```
rtk proxy /Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup --python art-studies/marmalade/woodland/author_render.py
rtk proxy .venv/bin/python art-studies/marmalade/woodland/palette_pass.py
```

The first command creates the editable Blender source and raw PNG frames; the
second creates exact nine-colour binary-alpha frames. Generated files are
ignored. Rendering is an offline art probe, not a runtime implementation or a
resource benchmark. The leg/depth adapter is heuristic; production needs proper
spatial quadruped contacts and a deforming rig.

See `docs/results/2026-10-16-marmalade-art-direction.md` for the design decision.
