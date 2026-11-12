#import "../shared/report.typ": *
#show: report.with(title: "Related works")

// Compiled on its own for now: chapter 2, sections continue from 2.6.
#counter(heading).update(1)

#todo-list()

// Tables in this chapter: caption underneath, a line around every cell, and
// a shaded, bold header row with a thicker rule below it.
#show figure.where(kind: table): set figure.caption(position: bottom)
#let rule-thin = 0.4pt + luma(170)
#let rule-head = 0.9pt + accent
#set table(
  stroke: (_, y) => (
    x: rule-thin,
    top: if y == 1 { rule-head } else { rule-thin },
    bottom: if y == 0 { rule-head } else { rule-thin },
  ),
  fill: (_, y) => if y == 0 { accent.lighten(88%) },
  inset: (x: 5pt, y: 4.5pt),
  align: left + horizon,
)
#show table.cell.where(y: 0): set text(weight: "bold")
#show table: set text(size: 8.5pt)
#show table: set par(leading: 0.5em)

= Related works

#counter(heading).update((2, 5))

== Multi-tenant forwarding data structures <sec:rw>

The literature proposes various techniques for forwarding with many tables on
one device, which can be categorised into families based on their core approach.
The simplest strategy keeps a dedicated structure per tenant, using a
single-table design such as DIR-24-8 or Poptrie. The remaining families store
state common to several tenants only once. Merged tries combine all tenants’
tables into one trie, so that a prefix shared by several tenants is stored once.
Shape graphs and prefix DAGs go further, storing identical sub-tries once
regardless of their position; both were proposed to compress a single table, and
we examine how they extend to many tenants. Other families, which we discuss
only briefly in @sec:rw-rejected, reshape or compress tables before sharing
them, adapt to specific hardware, or exploit skew in traffic: trie braiding
aligns tries to increase their overlap, forwarding table aggregation removes
redundant entries, Bloom filters avoid probing prefix lengths that cannot match,
hardware designs key a single table on the tenant and the address together or
compress tables to fit limited on-chip memory, and route caching keeps only
popular routes in fast memory.

We examine each family against the challenges and opportunities identified in
Section 2.3. The first is the fixed cost per tenant: the memory a tenant costs
regardless of how many routes it holds, which at a cloud gateway is multiplied
by up to a million tenants, and in a host virtual switch is paid again whenever
a VM arrives. The second is sharing: which state is stored once for several
tenants, and whether this matches how similar their tables are, which is high in
the cloud and likely lower at provider-edge routers. The third is the lookup
working set: the memory a lookup touches, and whether it, or at least the part
serving most traffic, can stay in the last-level cache. The fourth is the cost
of route updates, and whether they can be applied without blocking concurrent
lookups (Section 2.4). The last is the cost of adding and removing whole
tenants, which host virtual switches do as VMs come and go. @sec:rw-summary
summarises how each family fares against these criteria and identifies the gaps
this project addresses.

=== Per-tenant data structures <sec:rw-per-tenant>

The simplest approach to multi-tenancy, and the one used in software routers
today, keeps a separate forwarding structure for each tenant: the Linux kernel
@linuxvrf, VPP @vppfib and DPDK @dpdkfib each keep one forwarding table per VRF.
Any single-table LPM structure can be used in this way.

Linux keeps each table in an LC-trie @linuxfibtrie, a trie that combines path
compression, which skips chains of single-child nodes, with level compression,
which replaces densely populated parts of the trie with single nodes that branch
on several bits at once @nilsson1999lctrie. Linux uses a dynamic variant in
which each internal node holds an array of child pointers whose size is adjusted
as routes are inserted and removed @linuxfibtrie, so a small table needs only a
small trie. Its weakness is its footprint per route. Rétvári et al. measured
Linux’s trie at 26 MB for a single table of Internet routes, too large to fit in
the cache. Their own structure, the prefix DAG (@sec:rw-shape), held the same
table in 180 kB and achieved about three times the lookup rate, even though it
needed more memory accesses per lookup @retvari2016compressing. Poptrie,
discussed below, holds a table of similar size in 1.5 MiB @asai2015poptrie. No
tenant’s table is that large, but the tenants of a cloud gateway together hold a
comparable number of routes (Section 2.3.1).

Nilsson and Karlsson found that the single most important optimisation for the
LC-trie was to branch on a fixed 16 bits at the root @nilsson1999lctrie. The
fastest software designs take this further, indexing a large array with the most
significant bits of the address. DIR-24-8 @gupta1998, implemented in DPDK,
answers a lookup in at most two memory accesses by indexing a table of $2^24$
entries with the top 24 bits. With 2-byte entries, this table takes 32 MB
regardless of how many routes a tenant has, so 1,000 tenants would need about 32
GB before a single route is inserted. Later designs shrink the size of that
array but keep it. DXR indexes a lookup table of $2^k$ 32-bit entries, with k =
16 or 18, and resolves the remaining bits by binary search over address ranges
@zec2012dxr. SAIL splits the lookup at levels 16, 24 and 32, starting from an
array of $2^16$ entries @yang2014sail. These arrays make the designs fast, and
DXR in particular is designed so that a single table fits in the cache
@zec2012dxr. Their size, however, is fixed. DXR’s lookup table alone takes 256
KiB for k = 16, so a gateway with a million tenants would need 256 GiB for
lookup tables alone. A host virtual switch with a hundred tenants would need
about 25 MiB, a large share of the last-level cache it shares with the VMs.
SAIL’s footprint exceeds the last-level cache even for a single table
@asai2015poptrie. SAIL also proposes a variant that merges multiple tables into
one structure, which we discuss in @sec:rw-merged.

Poptrie @asai2015poptrie is a multibit trie that consumes six address bits per
node, so a lookup visits at most six nodes. All internal nodes are kept in one
array and all leaves in another. Instead of a pointer per child, each node
stores only the index at which its block of children begins in each array. A
64-bit bitmap marks which children are internal nodes, and a child’s position
within its block is the number of set bits before it, computed by a single
popcount instruction, hence the name. A second bitmap lets each run of identical
consecutive leaves be stored only once. Together, these keep a node at 24 bytes
despite its 64 children, and the authors attribute Poptrie’s performance to a
footprint small enough to stay in the cache @asai2015poptrie. Like the designs
above, Poptrie can add a direct-indexed array of $2^s$ entries at the root,
which costs 4 × $2^s$ bytes @asai2015poptrie. Without it, Poptrie has no
fixed-size component: if its node and leaf arrays grow on demand, an empty
tenant costs a single node.

Without direct pointing, Poptrie is not the fastest single-table design. On a
real tier-1 table with random destinations, it performs 96 million lookups per
second (Mlps) on one core in 1.49 MiB, against 117 and 180 Mlps for DXR with k =
16 and 18, and 158 Mlps for SAIL @asai2015poptrie. Every faster design, however,
owes its speed to a direct-indexed array, including Poptrie itself, which
reaches 241 Mlps with s = 18 @asai2015poptrie. Among designs without such an
array, Poptrie is the fastest. It is 1.4 to 1.7 times faster than Tree BitMap
@eatherton2004treebitmap, which uses similar bitmaps, and about ten times faster
than a binary radix tree @asai2015poptrie.

Updates do not block lookups @asai2015poptrie. Poptrie keeps each tenant’s
routes in a separate radix tree. A single writer applies a route change to this
tree, rebuilds only the affected part of the Poptrie, and publishes it with one
atomic pointer swap. The replaced part is freed once no lookup can still be
using it, but the paper does not describe how this is determined; doing so
safely is part of this project (@sec:rw-summary). With direct pointing, a change
to a prefix shorter than s replaces the entire top-level array @asai2015poptrie,
a further reason to leave it out.

Against our criteria, per-tenant Poptrie has a negligible fixed cost per tenant,
and adding or removing a tenant only allocates or frees one structure, which
suits host virtual switches. Its update scheme, once reclamation is resolved,
meets the requirements of Section 2.4. Its weakness is that it shares nothing.
With as few as 10% of prefixes distinct across cloud tenants (Section 2.3.1),
most of the state at a cloud gateway or host virtual switch is duplicated, and
although each tenant’s lookup path is compact, the combined state of many
tenants is likely to outgrow the last-level cache. At provider-edge routers,
where tables are larger and likely less similar, there is less to share, and
per-tenant Poptrie may be hard to beat. It is therefore our baseline in all
three settings. As the fastest per-tenant structure without a fixed cost per
tenant, it is the one a shared structure must outperform, and a shared structure
that does so should also outperform the alternatives above.

=== Merged tries <sec:rw-merged>

The best-known merged trie is that of Fu and Rexford @fu2008shared. It merges
all tenants’ tables into one trie and leaf-pushes it into a common prefix set
that is disjoint and complete, so that every address ends at exactly one leaf.
Each leaf points to a row in a shared next-hop table holding one entry per
tenant, and identical rows are stored only once. A lookup is then a walk of the
trie to a leaf, followed by a single row access indexed by the tenant
identifier. The trie is level-compressed like an LC-trie, with a fixed 16-bit
branch at the root, but this array is now paid for once rather than per tenant.
With 50 routing tables of 160,000 to over 250,000 entries each, the shared
structure requires 4.4 MB instead of 75 MB. Lookups are two to six times faster
than with separate tries, depending on the traffic @fu2008shared.

This design suits its setting of tens of large tables, but not ours, where a
device may serve up to a million tenants. Because the prefix set is made
complete, every row holds an entry for every tenant. With one byte per entry, as
in the paper, each row grows to a megabyte at a million tenants. Deduplication
keeps the number of distinct rows small for 50 tables, about 1,900, because each
table had only a few next hops @fu2008shared. Two rows are identical, however,
only if every tenant’s next hop agrees at both leaves, which becomes unlikely as
the number of tenants grows. SAIL’s multi-table variant follows the same
approach, storing an array with a next hop for every table at each leaf of the
leaf-pushed union @yang2014sail. Leaf pushing also makes updates costly. The
authors note that a change to a default route may require updating the whole
trie, and that prefixes are never deleted from the leaf-pushed trie, so the
structure must periodically be rebuilt from scratch @fu2008shared. Adding a
tenant is no easier, as it adds an entry to every row.

Luo et al. keep the merged trie but avoid leaf pushing, so that each prefix
stays at its own node @luo2013trie. Their trie is binary. Each node holds a
bitmap indexed by tenant, marking whether the prefix belongs to that tenant, and
a pointer to a row of next hops, also indexed by tenant, which is kept in a
separate memory. A lookup walks down the trie, remembers the row of the last
node whose bit is set for its tenant, and reads the next hop from that row once
the walk ends. An update changes a single node: deleting a prefix clears one
bit, and inserting or modifying one also writes one entry in the node’s row.
Implemented as an SRAM-based hardware pipeline, the merged trie for 14 full IPv4
tables takes 10 MB, 87% less than separate binary tries, with a further 14 MB of
rows kept in external memory @luo2013trie. These rows are not deduplicated:
every node holding a prefix has its own row with an entry for every tenant, even
if only one tenant has a prefix there.

Without leaf pushing, a node only needs to describe the tenants that have a
prefix at that node. The bitmap and the row can therefore be replaced by a
single sparse structure holding, for each such tenant, an entry that pairs its
identifier with its next hop. The presence of an entry answers what the bitmap
answered, and its contents what the row answered. The total state then grows
with the number of routes rather than with the number of tenants. In exchange, a
lookup must search this structure at every prefix node on its path, rather than
test a single bit. We consider both representations in Chapter 3 (Design). The
tenant-indexed bitmap and row remain compact for provider-edge routers with tens
to hundreds of tenants (Section 2.3.2), while the sparse variant targets the
larger tenant counts of cloud gateways and host virtual switches.

Against our criteria, the merged trie shares the nodes of prefixes common to
several tenants, which is promising given that as few as 10% of cloud prefixes
are distinct (Section 2.3.1). In the sparse variant, its fixed cost per tenant
is close to zero, with one entry per route. The lookup working set is its main
risk. A binary merged trie may need up to 32 dependent memory accesses per
lookup in software, where Luo et al.’s pipeline hides them in separate stages.
Multibit nodes would reduce this, at the cost of expanding each tenant’s
prefixes within a node, which multiplies its entries and makes a route change
rewrite several of them. In the sparse variant, short prefixes such as default
routes also place nearly all tenants in the structures of a few shallow nodes,
which every lookup must search.

Route updates are favourable. In the bitmap variant, updates to existing nodes
reduce to atomic writes of a bit or a row entry. The sparse variant is harder,
as inserting into a structure that concurrent lookups may be reading requires
either a concurrent container or copying it and publishing the copy. Adding and
removing tenants separates the two variants. The bitmap and row are as wide as
the number of tenants, so the bitmap variant must either reserve space for the
largest number of tenants in every node or widen every node when a tenant is
added. The sparse variant adds or removes one entry per route of the tenant. The
merged trie is therefore a strong fit for provider-edge routers, and a candidate
for the other settings, depending on the cost of searching the shared nodes.

=== Shape graphs and prefix DAGs <sec:rw-shape>

Shape graphs and prefix DAGs both compress a trie by storing identical sub-tries
only once, wherever they occur. They differ in what makes two sub-tries
identical: a shape graph compares only their structure, while a prefix DAG also
compares the next hops they hold. This determines how each design stores next
hops, and how much each can share between tenants. We first describe both
designs as proposed, for a single table, and then how each extends to many
tenants.

==== Shape graphs

Song et al. observed that the tries of IP forwarding tables contain many
identical sub-tries @song2009shape. Two sub-tries are identical, or have the
same shape, if one can be laid over the other node for node, with each node
having children in the same positions, regardless of the next hops they store.
In a binary trie of the AS1221 table, which holds more than 210,000 prefixes,
the 576,534 nodes form only 76,276 distinct shapes, 7.6 times fewer
@song2009shape. Song et al. therefore store each shape once, as a vertex whose
edges point to the shapes of its children. This turns the trie into a directed
acyclic graph (DAG), called a shape graph. Identical shapes are found bottom-up,
using a hash table keyed on the identifiers of a node’s children, a technique
known as hash-consing @filliatre2006hashcons.

Since a vertex can be reached along several paths, it cannot tell by itself
whether the path that reached it is a prefix. Song et al. therefore leaf-push
the trie first, so that every prefix ends at a leaf and all leaves share a
single shape. A lookup follows the address through the DAG until it reaches this
terminating vertex, and the path it took is exactly the leaf-pushed prefix
matching the address. Shapes ignore next hops, so these are stored separately,
in a hash table keyed by that prefix. A lookup therefore consists of a walk
through the DAG followed by a single hash-table probe @song2009shape. Leaf
pushing expands the AS1221 table 1.7 times, but it makes the trie more regular,
which shrinks the shape graph by a further 32% @song2009shape.

Updates touch the DAG only when they change a shape. A change of next hop
rewrites only the hash table, and a deletion can be handled lazily, by giving
the deleted prefix the next hop of the prefix covering it. An insertion that
changes the trie’s shape may need new vertices for every node on the path from
the change to the root, including a new starting vertex. Song et al. write the
new vertices first and then switch to the new starting vertex in a single step,
so lookups are never blocked and see either the old graph or the new one
@song2009shape. Vertices that are no longer reachable are recycled periodically
@song2009shape. In their hardware pipeline, lookups that may still be reading
them finish within a fixed number of steps. In software, lookups run on several
cores and can be delayed arbitrarily, so deciding when a vertex can safely be
freed becomes a central problem of this project (@sec:rw-summary).

Song et al. motivate the design by the need to keep forwarding state in the
caches of multi-core processors and to host many virtual routers, but implement
it for the embedded memory of ASICs, FPGAs and network processors
@song2009shape. Only the DAG is kept in this memory. The hash table, with five
bytes per leaf-pushed prefix, is kept off-chip and takes most of the space, and
the writer additionally keeps the uncompressed trie and the hash-consing table
@song2009shape. The total memory may therefore exceed that of a trie, but
lookups touch only the DAG and the next-hop table.

==== Multibit shape graphs

Sharing reduces the size of a trie but not its depth: a binary shape graph takes
as many steps per lookup as the binary trie it was built from @song2009shape.
Song et al. therefore also build multibit shape graphs, in which each vertex
consumes several address bits, as in a multibit trie such as Poptrie. Larger
strides reduce both the number of steps and the number of vertices, but each
vertex grows exponentially with the stride. Even so, overall memory grows far
more slowly than that of a multibit trie @song2009shape.

Choosing the stride is therefore a trade-off between depth and size, and in
software both determine speed. Size matters because, as @sec:rw-per-tenant
showed, a structure that fits in the cache can be faster than one that needs
fewer memory accesses but is too big. Depth matters because each step is a
dependent memory access, and Song et al. estimate that a stride of s raises
throughput roughly s-fold @song2009shape. A larger stride, however, also
requires two vertices to match over more address bits before they can be shared.
How much sharing remains at a given stride on tenants’ tables has not been
measured; we measure it in our evaluation.

==== Prefix DAGs

Rétvári et al. later proposed prefix DAGs, which merge two sub-tries only if
both their shapes and the next hops at their leaves are identical
@retvari2016compressing. The next hops can then be stored in the DAG itself, and
the separate hash table disappears. Rétvári et al. criticise shape graphs
precisely for this table, and for the cost of updating a fully leaf-pushed trie.
To keep updates cheap, they leaf-push and merge only below a chosen depth, and
keep the levels above it as an ordinary trie @retvari2016compressing.

A prefix DAG holds a full Internet table in 180 kB, and a lookup is a single
walk, with no separate table to probe @retvari2016compressing. This compression
relies on the table having few distinct next hops: the authors attribute their
results mainly to the low entropy of the next-hop distribution
@retvari2016compressing. Within one tenant this is plausible, as a tenant with
fewer than ten routes has at most as many next hops; across tenants, it depends
on how their next hops are labelled, as discussed below.

==== Sharing across tenants

Neither design was developed for many tenants, but both extend to them
naturally. In a multi-tenant shape graph, all tenants share one hash-consing
table, and hence one DAG. Each tenant has its own root in the DAG and its own
next-hop table, and identical sub-tries are stored once, whichever tenant or
position they come from. A merged trie shares a node only for the same prefix
and stores per-tenant state in it. A shape graph instead shares sub-tries
wherever their shapes match, at the cost of keeping next hops outside the graph.

A prefix DAG extends in the same way, but it shares a sub-trie between two
tenants only if the labels at its leaves are also identical. As in Fu and
Rexford’s design, a multi-tenant prefix DAG would most naturally label leaves
with indices into a small next-hop table per tenant @fu2008shared, so that
labels stay small whatever the next hops are. Two tenants’ sub-tries can then be
merged whenever their next hops are numbered alike. How often this happens has
not been measured. It depends on how many next hops each tenant has and how they
are numbered, and with tables of fewer than ten routes, it may be frequent.

The two designs therefore trade sharing against per-tenant state. A shape graph
shares every sub-trie of the same shape, but keeps a next-hop entry for every
leaf of each tenant. A prefix DAG keeps only one entry per distinct next hop for
each tenant, but shares a sub-trie only when its labels coincide. Which balance
is preferable depends on how often tenants’ labels coincide, and this has not
been measured. Since the two designs differ only in whether labels are part of
the key used to detect identical sub-tries, we evaluate both.

==== Assessment

Against our criteria, both designs have a small fixed cost per tenant: a root in
the shared DAG and a next-hop table, which grows with the tenant’s leaves in a
shape graph and with its distinct next hops in a prefix DAG. Their sharing is
promising but unmeasured. Small cloud tables draw their addresses from the same
few private ranges and should therefore produce heavily repeated structure.
However, the figure in Section 2.3.1 counts identical prefixes, not sub-tries,
and both Song et al. and Rétvári et al. measured repetition only within single
tables. In both designs, a lookup takes a few steps through a DAG shared by all
tenants, which should stay in the cache, and both inherit the cost of leaf
pushing. Unlike in Fu and Rexford’s design, however, each tenant’s trie is
leaf-pushed separately rather than their union, so this cost depends only on the
tenant’s own routes and not on the number of tenants. Updates that change the
structure create new vertices along the path to the root and switch the tenant’s
root. Adding a tenant adds a root. Removing one drops its root, after which
vertices no longer reachable from any root must be freed safely.

A shape graph shares the most structure, since its sharing does not depend on
next hops. Route changes that keep the shape also leave the shared DAG
untouched, including moving a route to a different next hop. Its costs lie in
the per-tenant table. The table holds one entry per leaf of the tenant’s
leaf-pushed trie, so changing a short route rewrites the entries of all leaves
below it. In Song et al.’s design, it is also a hash table keyed by prefix,
which gains nothing from the sharing and whose probe is the part of each lookup
most likely to miss in the cache. Avoiding this probe is one of the problems we
address in Chapter 3.

A prefix DAG keeps per-tenant state minimal. After the walk, a lookup only reads
a table of a few entries, which should rarely miss in the cache. Its costs lie
in the shared structure. Sub-tries are shared only when tenants’ labels
coincide, so its sharing depends on how many next hops tenants have and how they
are numbered. Moving a route to a different next hop relabels its leaves, and so
alters the shared DAG on the path to the tenant’s root; for a short route, this
affects many leaves at once. Changing a next hop used by several routes, in
contrast, only updates the tenant’s table. Rétvári et al.’s leaf-push barrier
would limit the cost of relabelling, but the nodes above the barrier hold
per-tenant next hops and could not be shared.

Shape graphs and prefix DAGs are therefore both strong candidates for cloud
gateways and host virtual switches, with their many small and similar tables.
They place their costs differently: shape graphs in per-tenant state, and prefix
DAGs in the shared structure. Their benefit for provider-edge routers depends on
how much structure their larger tables share.

=== Summary and gaps <sec:rw-summary>

@tab:rw-criteria summarises how each design fares against our criteria, and
@tab:rw-settings how well each suits our three settings. No existing design
suits all three settings, and the gaps that remain fall into four groups.

*Scale*

None of the multi-tenant designs was evaluated at our tenant counts, and several
were evaluated only in hardware. Fu and Rexford merge at most 50 tables
@fu2008shared and Luo et al. 14 @luo2013trie, and trie braiding is argued for 16
to 32 virtual routers @song2010braiding. Shape graphs and prefix DAGs were
evaluated only on individual tables @song2009shape @retvari2016compressing.
Cloud gateways, in contrast, serve up to a million tenants (Section 2.3.1), and
none of these papers shows how its design behaves when most tables hold fewer
than ten routes.

*Concurrency and reclamation*

None of the shared designs addresses concurrent operation in software. Among
per-tenant structures, Linux’s tries are read under RCU (Section 2.4), and
Poptrie publishes rebuilt parts with an atomic swap without blocking lookups,
although it does not say when the replaced memory may be freed @asai2015poptrie.
Song et al. also switch to a new graph without blocking lookups, but in a
hardware pipeline, where every lookup completes within a fixed number of steps,
and they recycle unused vertices periodically @song2009shape. Luo et al.
interleave updates with lookups in a similar pipeline @luo2013trie. Fu and
Rexford apply updates incrementally and periodically rebuild the structure, but
do not consider lookups that run during an update @fu2008shared. Rétvári et al.
do not discuss concurrency at all @retvari2016compressing. In software, lookups
run on many cores and a reader may be delayed arbitrarily, so replaced memory
can only be freed with one of the reclamation methods of Section 2.5. For shared
structures, this is harder than for a per-tenant one. A vertex may be reachable
from the roots of many tenants, so it can be freed only once no tenant’s
structure reaches it and no lookup of any tenant may still be reading it.

*Unmeasured sharing*

The property that decides between the shared designs has not been measured. The
only measurement across tenants counts identical prefixes @xu2024map, which
bears on merged tries. How often sub-tries repeat across tenants, which decides
the benefit of shape graphs, and how often their labels coincide, which decides
that of prefix DAGs, are both unknown.

*The combination*

The properties these designs offer have not been combined. Poptrie provides
compact multibit nodes suited to the CPU cache and non-blocking updates, but
shares nothing between tenants. Shape graphs and prefix DAGs share sub-tries
wherever they occur, and could share them between tenants, but they were
designed for hardware or for single tables, and neither addresses concurrent
updates in software. No existing design combines the sharing of sub-tries
between tenants with cache-oriented multibit nodes, and with non-blocking
updates and safe memory reclamation in software. Chapter 3 describes how we
build such designs, and our evaluation compares them in the three settings of
Section 2.3.

#[
  #figure(
    table(
      columns: (1.1fr, 1fr, 1fr, 1.15fr, 1fr, 1.05fr),
      table.header(
        [Design],
        [Fixed cost per tenant],
        [Sharing],
        [Lookup working set],
        [Route updates],
        [Adding/removing tenants],
      ),
      [DIR-24-8, DXR, SAIL],
      [Large direct-indexed array],
      [None],
      [1–2 accesses; all tenants' arrays exceed the cache],
      [Rewrite a range or rebuild a chunk],
      [Allocate/free the array],

      [LC-trie (Linux)],
      [Small],
      [None],
      [Pointer-based; large footprint per route],
      [Local],
      [Cheap],

      [Poptrie],
      [One node],
      [None],
      [≤ 6 nodes; many tenants' state likely exceeds the cache],
      [Rebuild affected part; atomic swap],
      [Cheap],

      [Fu and Rexford],
      [Entry in every row],
      [Prefixes of the leaf-pushed union; identical rows],
      [Walk, then one row],
      [May touch whole trie; periodic rebuild],
      [Entry added to every row],

      [Merged trie, bitmap (Luo~et~al.)],
      [Bit + row entry per prefix node, sized for max.~tenants],
      [Nodes of common prefixes],
      [≤ 32 accesses if binary; bit test per node],
      [One bit or row entry],
      [Widen every node, or reserve space],

      [Merged trie, sparse],
      [≈ 0],
      [Nodes of common prefixes],
      [Search shared nodes; shallow nodes crowded],
      [Concurrent set or copy],
      [One entry per route],

      [Shape graph],
      [Root + entry per leaf],
      [Same-shape sub-tries, any position],
      [Shared DAG walk, then probe tenant's table],
      [Table only if shape kept; else path to root],
      [Add/drop a root; reclaim unreachable vertices],

      [Prefix DAG],
      [Root + entry per distinct next hop],
      [Sub-tries with same shape and labels],
      [Shared DAG walk, then small table],
      [New next hop alters path to root],
      [Add/drop a root; reclaim unreachable vertices],
    ),
    caption: [Existing designs against the criteria of @sec:rw.],
  ) <tab:rw-criteria>
]

#figure(
  table(
    columns: (auto, auto, auto, auto),
    align: (x, _) => (if x == 0 { left } else { center }) + horizon,
    table.header(
      [Design], [Cloud gateway], [Provider-edge router], [Host virtual switch]
    ),
    [DIR-24-8, DXR, SAIL], [✗], [✗], [✗],
    [LC-trie (Linux)], [✗], [\~], [\~],
    [Poptrie], [\~], [✓], [\~],
    [Fu and Rexford], [✗], [\~], [✗],
    [Merged trie, bitmap], [✗], [✓], [\~],
    [Merged trie, sparse], [\~], [✓], [\~],
    [Shape graph], [✓], [?], [✓],
    [Prefix DAG], [✓], [?], [✓],
  ),
  caption: [Expected fit of each design for the settings of Section 2.3: ✓
    promising fit, \~ workable, ✗ poor fit, ? depends on properties of the
    tables that have not been measured. Promising candidates will be tested and
    evaluated to confirm those claims.],
) <tab:rw-settings>


=== Rejected designs <sec:rw-rejected>

*Trie braiding*

Song et al. reshape tenants’ tries before merging them, allowing each node to
swap its children so that more nodes overlap. This helps most when tables are
dissimilar @song2010braiding. Trie shifting with depth adjusting pursues the
same goal, with 4.7–7.8% fewer nodes than braiding @zhou2012shifting. Recovering
each tenant’s trie requires a braiding bit per tenant at every node. The authors
argue that bits for 16 to 32 virtual routers fit in the unused part of an SRAM
burst @song2010braiding, but at our tenant counts the bits would outgrow the
node itself. Finding the optimal braiding is exponential in the number of
tables, so tables are merged one at a time. Route changes are applied directly
to the braided trie, which is re-optimised only periodically, as is the shifted
trie @song2010braiding @zhou2012shifting. Nor does the idea carry over to shape
graphs. Song et al. note that allowing children to be swapped would leave fewer
distinct sub-tries, 48,762 rather than 76,276 in AS1221. A graph built from
them, however, could no longer be walked with an address, as its edges would
lose which branch is which @song2009shape.

*Forwarding table aggregation*

Aggregation replaces a table with an equivalent one with fewer prefixes. ORTC
computes the minimal such table, reducing backbone tables to about 60% of their
original size @draves1999ortc, and FIFA maintains an aggregated table under
updates @liu2013fifa. Poptrie already applies a simple form by default, merging
prefixes with the same next hop @asai2015poptrie. Aggregation is orthogonal to
the choice of structure and could precede any of the designs above, although by
changing each tenant’s prefixes it may also change how much the tenants share.
It offers little for cloud tenants, most of whom have fewer than ten routes, but
may reduce the larger tables of provider-edge routers.

*Bloom-filter front ends*

Dharmapurikar et al. keep one Bloom filter per prefix length in on-chip memory,
so that only lengths whose filter reports a match are probed in off-chip hash
tables @dharmapurikar2003bloom. The benefit relies on the filters residing in
memory much faster than the tables. In software, a filter is itself a memory
access, and the design reduces to the hash-based LPM rejected below, with false
positives in addition.

*Combined tenant and address keys*

In hardware, multi-tenant routes are often kept in a single table keyed on the
tenant identifier together with the destination address. Sailfish’s
programmable-switch gateway, for example, performs LPM on the pair (VNI, inner
destination IP) @pan2021sailfish. In TCAM or a switch’s LPM unit, extending the
key in this way costs little, as all entries are matched in parallel. In
software, the idea does not carry over. A trie over the combined key places the
tenant bits first and therefore reduces to a directory of per-tenant tries, the
approach of @sec:rw-per-tenant. The tenant bits cannot be placed last, as a
tenant’s prefix would then no longer be a prefix of the combined key. Recording
the tenants at each node instead leads to the merged trie of @sec:rw-merged.
Hash-based LPM over (tenant, prefix) pairs @waldvogel1997scalable is either
uncompetitive, when prefix lengths are probed one by one, or makes updates
non-local, when binary search over lengths is used to reduce the number of
probes. In that case, a route change may require adding or recomputing marker
entries at several prefix lengths.

*Hardware table compression*

Other hardware designs compress tables to fit switch memory. Luo et al.
partition virtual routers’ tables across TCAM and SRAM @luo2012hybrid. MaP
merges similar entries across VPCs and partitions the result to fit a
programmable switch’s memory blocks @xu2024map. QuarkTable shortens entries by
encoding the upper part of long prefixes @lu2024quarktable. These techniques
target the costs of TCAM and match-action tables rather than CPU caches, but
they confirm that real cloud routing tables contain considerable redundancy
across tenants.

*Route caching*

Route caching keeps only the popular routes in fast memory and fetches the rest
from slower memory on a miss, exploiting the skew in traffic that Section 2.3
reports @kim2009routecaching. Caching prefixes is complicated by longest-prefix
matching, since a cached short prefix can hide a more specific route that is not
cached. Kim et al. therefore cache only fixed-length /24 sub-prefixes, relying
on providers filtering routes more specific than /24 @kim2009routecaching. Cloud
tables, whose prefixes reach /32 (Section 2.3.1), do not allow this. More
fundamentally, in software the CPU cache already keeps the busiest part of
whatever structure is used. A separate route cache adds a lookup in front of the
structure, and its misses still reach it. A compact structure benefits from the
skew without one.


#references()
