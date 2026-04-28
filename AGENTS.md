# AGENTS.md

## Project
This is an Unreal Engine 5 action game project.

## Rules
- Do not edit .uasset or .umap unless explicitly requested.
- Prefer C++ changes in Source/ when possible.
- Do not modify Binaries/, Intermediate/, Saved/, or DerivedDataCache/.
- Do not rename or move assets unless asked.
- Keep changes small and task-focused.
- Do not perform large refactors unless requested.

## Unreal Engine Notes
- Use Unreal Engine 5.
- Expose gameplay tuning values to Blueprint with UPROPERTY when useful.
- Be careful with nullptr, UObject lifetime, and Blueprint references.
- If Blueprint-side setup is needed, explain it instead of editing assets directly.

## Always report
1. Files changed
2. What changed
3. How to test in Unreal Editor
4. Blueprint setup needed
5. Risks
