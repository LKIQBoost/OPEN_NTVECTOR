# SNB Migration Baseline

This migration changes only this OPEN-VECTOR working tree.
`C:\Users\26962\source\minecraft\StaticNeteaseBot` is a read-only reference.

## Source Baseline

- Repository: `https://github.com/LKIQBoost/OPEN_NTVECTOR.git`
- Reference branch: `main`
- Reference commit: `653fdba723e311b9bbc55b0962269c4382b8ff5a`
- Target tree source: GitHub `main` source archive

The target tree was obtained from the source archive because a full Git fetch
was not reliable in this environment. The local `.git` metadata is therefore
not treated as a complete history. Each migration stage must remain buildable
and independently reviewable.

## Initial Comparison

- OPEN-VECTOR application files: 1241
- SNB application files: 1276
- Same application paths: 1044
- Same-path files with identical content: 1038
- Same-path files with different content: 6
- SNB-only application paths: 232
- OPEN-VECTOR-only application paths: 197

## Rules

1. Only files below `OPEN-VECTOR` may be edited.
2. Existing OPEN-VECTOR behavior is preserved unless the change is explicitly
   required by an SNB improvement or additional feature.
3. Functional migration and directory reorganization are separate stages.
4. Every migrated native API includes implementation, declaration, test, and
   documentation updates where applicable.
5. A failed build or an incompatible legacy behavior stops the stage for
   investigation; it is not silently overwritten.

## Stage Status

- [x] Baseline and comparison
- [x] Build and test foundation (Ninja + MinGW baseline links `Program.exe`)
- [x] NBT and native Python API
- [x] Packet objects and generic codec
- [x] Structured protocol events
- [x] Async requests and timeout handling
- [x] Player, world, entity, and inventory state
- [ ] Type declarations, documentation, and final regression
