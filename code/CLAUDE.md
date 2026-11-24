# code/

Rules for all C++ in this directory. Test-specific rules are in `tests/CLAUDE.md`.

## Language and build

- C++23, CMake + Ninja. Warnings are errors.
- Tests use GoogleTest. They also run under ASan/UBSan (TSan once concurrency lands).

## C++ engineering

Write modern, high-performance C++23 following the C++ Core Guidelines.

- RAII everywhere. `unique_ptr` by default; `shared_ptr` only for genuinely shared ownership,
  never on hot paths. Lock-free structures reclaim memory via EBR / hazard pointers.
- Const correctness and `noexcept` wherever they hold. State exception-safety guarantees.
- Prefer compile-time errors over runtime errors: concepts with documented constraints,
  `constexpr`, strong types.
- Prefer STL algorithms and ranges over raw loops, except where a loop is measurably faster.
- `#pragma once` in every header.
- Code must be ASan/UBSan clean, and TSan clean once concurrent.
- Match existing patterns in the codebase before introducing new ones.

## Naming (strict, no exceptions)

| Kind | Prefix | Example |
|---|---|---|
| Local variables (incl. loop variables) | `my` | `myNode`, `myIndex` |
| Instance (member) variables | `the` | `theRoot`, `theTenantCount` |
| Function arguments | `a` / `an` | `aPrefix`, `anAddress` |

Types are `PascalCase`; functions and the part after a prefix are `camelCase`.

## Design principles

- Strong encapsulation and separation of concerns. If a component can be fully independent
  of the rest of the system, make it so. Expose narrow interfaces; hide representation.
- These rules hold for everything that isn't performance-critical. Inside the forwarding
  structures themselves they may be relaxed where it measurably matters; say why in a comment.
- Prefer compile-time polymorphism (concepts/templates) over virtual dispatch, so the harnesses
  add no overhead to what they measure.

## Architecture

Dependencies point one way:

```
harnesses → generator → core
harnesses → concept   → core
structures → concept
```

- **Utils** — helpers that know nothing about this project.
- **Core** — shared vocabulary used by every other component.
- **Concept** — the contract every forwarding structure satisfies. Harnesses depend on it, never
  on a concrete structure; structures are named only where a harness is instantiated.
  Semantics the concept can't express (throws, misses, overwrite rules) live in a semantics
  comment block above each concept. Never change a concept's semantics without asking me first.
- **Reference structure** — `LinearLengthSearch` (single-tenant, wrapped in `PerTenantTable`):
  one hash map keyed by normalised `Prefix`; a lookup probes every length from /32 down to /0.
  No optimisations, written to be obviously correct. Once it passes the integration tests it is
  the oracle for everything else. Structures that share `PerTenantTable` with it can't expose
  bugs in that adapter through differential tests, so the integration tests must cover tenant
  handling.
- **Workload generator** — produces operation streams: the read/write mix and data parameters
  (tenant count, prefixes shared across tenants, next-hop set, prefix-length distribution, …).
  Deterministic given a seed, so every failure and measurement is reproducible.
  Knows nothing about any structure.
- **Correctness harness**
  - Integration tests: hand-designed inputs with hand-written expected outputs. These validate
    the reference structure.
  - Differential tests: run the same generated workload on an implementation and the reference
    (single-threaded) and compare every result. On mismatch, report the seed and operation index.
- **Performance harness** — measures only, never verifies: mean, p95, p99 of lookup and update
  latency. Metrics are pluggable so memory use, cache misses, and multi-threaded runs can be
  added later without touching the existing ones.
- **Structures** — the implementations under comparison. Each depends only on the concept and
  core.
