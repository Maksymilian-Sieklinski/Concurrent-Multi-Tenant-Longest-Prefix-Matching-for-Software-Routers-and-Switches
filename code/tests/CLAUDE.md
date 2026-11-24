# Tests

Rules for writing tests. They grow as the suite does.

## Naming

- **Suite**: one behaviour area, a PascalCase noun phrase (`LongestPrefixMatch`, `PrefixUpdates`,
  `TenantsHandling`). The fixture class has the same name. The file is the suite name in snake_case plus
  `_test.cpp`.
- **Test**: the expected behaviour stated as a fact, PascalCase, present tense, situation then
  outcome (`LongerPrefixWins`, `RemovedPrefixFallsBackToShorter`, `UpdateOnMissingTenantThrows`).
  Read together with the suite it forms a sentence.
- No filler: no `Test` suffix, no `Should`/`Check` prefix, and no method names as test names.
- One behaviour per test. If the name needs "And", split the test.
- Never put a structure name in a test name; the name generator adds it.

## Test structure

- Three sections in order, separated by a blank line: data (prefixes, addresses, next hops),
  apply (calls on the structure), check (lookups and expectations). One cycle per test: no apply
  after a check.
- The anonymous namespace holds only values the outcome doesn't depend on, such as `TENANT` in a
  test that isn't about tenants. Values that decide the outcome, including the next hops of
  competing prefixes, are local to the test.
- The fixture holds state only (`theTable`), no constants. Bind `auto &myTable = this->theTable;`
  at the top of each test.
- Every test calls `addTenant` itself. Routes go in through `addTenant` with the `routes(...)`
  helper, unless the test is about `update`.
- Every route in a test has a distinct next hop.
- `EXPECT_EQ(expected, actual)`, with the lookup inline.

## Writing new tests (Claude)

When asked to write new tests (not to fix existing ones), spawn a subagent that is blind to the
implementations. Give it the behaviour area under test and point it at this file.

- It reads only: the concept headers (`forwarding_tables/*/*_forwarding_table.hpp`), `core/`,
  `utils/`, this file, the fixtures and helpers in `tests/`, `implementations.hpp`, and the
  suite's existing test file. Nothing else under `forwarding_tables/`.
- Expected values come only from the semantics block above each concept.
- It builds and runs the tests. A failing test is reported together with the semantics it relies
  on; expectations are never changed to make a test pass.
- It writes no comments, and it reports every file it read.

Review its tests against this file before handing them over.
