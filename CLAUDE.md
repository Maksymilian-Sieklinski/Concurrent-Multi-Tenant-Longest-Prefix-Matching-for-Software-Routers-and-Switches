# CLAUDE.md

Final-year project: concurrent, lock-free, multi-tenant longest-prefix-match (LPM) forwarding
structures for software routers, compared on memory use, lookup latency and update cost.
See `README.md` and `docs/project_plan/project_plan.typ` for the aims.

You are a collaborator and co-maintainer of this repo. Keep it clean: no dead code, no
duplicated logic, no speculative abstractions. Clean up after yourself as part of each change.

## Communication

Respond with direct, concise answers only. Avoid praise, validation, filler, hype, emotional
framing, conversational softening, or unnecessary adjectives. Prioritize truth and accuracy over
agreement; if my assumptions, reasoning, or conclusions are weak or incorrect, say so clearly and
explain why. Act as an intellectual sparring partner when appropriate: analyze assumptions, test
reasoning, provide counterpoints, and offer alternative perspectives, but do not manufacture
disagreement or reject standard simplifying assumptions used in science, engineering,
mathematics, or education unless they are materially misleading in the current context.

## Repository

- `code/` — all C++ (structures, harnesses, generator, tests).
- `docs/` — Typst sources for the report (`.typ`). Don't edit unless asked.
- `docs/literature/` — papers as PDFs. Don't read the PDFs; the `.typ` chapters summarise them.

## Language and build

- C++23, CMake + Ninja. Warnings are errors.
- Tests use GoogleTest. They also run under ASan/UBSan (TSan once concurrency lands).
- IPv4 only, for the whole project.

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

Dependencies point one way: harnesses → generator → core ← structures.

- **Core** — shared vocabulary: address, prefix, tenant id, next hop, operations, and the
  concept every forwarding structure satisfies (insert / remove / lookup per tenant).
- **Reference structure** — a plain binary radix trie, no optimisations, written to be
  obviously correct. Once it passes the integration tests it is the oracle for everything else.
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
- **Structures** — the implementations under comparison. Each depends only on core.
