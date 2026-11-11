#import "../shared/report.typ": *
#show: report.with(title: "Background and Related Work")

// This chapter is compiled on its own for now, so start numbering at 2.
#counter(heading).update(1)

#todo-list()
#pagebreak()

= Background & related work <ch:background>

// ===========================================================================
== Longest Prefix Match in software <sec:lpm>

Routers decide where to forward each packet using longest prefix match (LPM)
@ruiz2001survey. Stripped of its networking context, the problem is simple:
given a set of strings $S$ and a query string $x$, find the longest string in
$S$ that is a prefix of $x$. For example, let $S = {#`ABA`, #`ABAC`}$. For
$x = #`ABAS`$, the answer is `ABA`, since `ABA` is a prefix of `ABAS` but `ABAC`
is not. For $x = #`ABACE`$, both are prefixes, and the answer is `ABAC` because
it is longer.

In a router, the strings are bit strings. Lookups are performed in the router’s
forwarding table, which is derived from its routing table and holds all the
routes used to forward packets @ruiz2001survey. Each entry pairs an address
prefix with a next hop. A prefix is written as an address followed by its length
in bits: `10.0.0.0/8` denotes the first 8~bits of `10.0.0.0`, i.e. `00001010`.
Thus, the router finds the longest prefix in the table that matches the packet's
destination address and forwards the packet to that entry's next hop.

This lookup is performed for every packet, so forwarding throughput depends
directly on how fast it is. At 10 Gbps, a link carrying minimum-size packets
delivers about 14.88 million packets per second, leaving roughly 67 ns per
packet @rizzo2012revisiting. Comparing the destination address against every
prefix in the table is far too slow, since a table may hold up to a million
prefixes @huston2025. A lookup structure must therefore narrow the search
without examining every entry.

The classic data structure for this problem is the trie. A binary trie stores
prefixes as paths from the root, with each node branching on one bit of the
address. A node at the end of a prefix stores that prefix’s next hop. Other
nodes hold no value and exist only to connect the rest of the trie. A lookup
follows the destination address bit by bit from the root, remembering the last
node with a value it passes. When the path ends, that node gives the last
matching prefix found, which is the longest one possible. The cost of a lookup
is therefore bounded by the address length rather than by the size of the table.

#figure(
  figure-placeholder[example forwarding table and the corresponding binary
    trie],
  caption: [An example forwarding table and the corresponding binary trie.],
) <fig:trie>

In software, the cost of an operation is often dominated not by computation but
by memory accesses, since a cache miss can take tens to hundreds of times longer
than a hit @drepper2007. Trie lookups are particularly exposed to this. Each
step must fetch the next node, and the address of that node is only known once
the previous one has been read, so the CPU cannot overlap these accesses. Nodes
are often far apart in memory, so successive steps rarely benefit from the
cache.

A binary trie for IPv4 addresses needs up to 32 memory accesses per lookup so it
is usually too slow for packet forwarding. Real implementations reduce this by
matching several bits per step (multibit tries @srinivasan1999), by skipping
chains of single-child nodes (path compression @morrison1968), or by indexing
directly into a large table using the first bits of the address @gupta1998.
These bring a lookup down to a handful of memory accesses. Multibit tries and
direct indexing use more memory than a binary trie, trading it for faster
lookups. Packet forwarding prioritises lookup time, but these structures also
make updates more expensive, since a single route change can rewrite many
entries @ruiz2001survey.

Lookup performance depends heavily on how much of the structure fits in cache.
Cloudflare observed lookups in the Linux kernel's BPF LPM trie slow sharply as
tables grew beyond tens of thousands of entries @fleming2025. Arashloo et al.
measured the cost directly in a software VPN gateway @arashloo2018. Forwarding
64-byte packets at close to 40~Gbps, it needed 6 worker cores when its lookup
table held one million entries, which fit in the processor's 45~MB L3 cache.
With 16 million entries, which did not fit, it needed 10 worker cores to sustain
the same throughput. In multi-tenant settings, a software router must keep a
separate structure for every tenant, so their combined size quickly outgrows the
cache.

// ===========================================================================
== Multi-tenancy <sec:multitenancy>

Multi-tenancy describes a situation where a single forwarding device is shared
by many independent users, called tenants. A common example is a public cloud.
Each customer rents an isolated virtual network, in which it runs its virtual
machines and chooses its own addresses and routes. Traffic leaving these
networks, for example to the internet, to other cloud services such as storage
or to the customer’s own offices, passes through gateways shared by all
customers @pan2021sailfish. Each gateway therefore forwards traffic for many
customers at once, and each customer is one of its tenants. Other examples
include customers of a virtual private network (VPN) service sharing a
provider’s routers. @sec:settings describes these settings in detail.

Each tenant expects its network to behave as if it were the only one, even
though it shares the underlying infrastructure with many others. Two
requirements follow. First, tenants commonly use the same private address ranges
@rfc1918, so the same address can mean different things to different tenants
@rfc4364. Second, tenants must be isolated, so one tenant's routes must not
affect how another tenant's packets are forwarded. For example, two tenants may
both define a route for `10.0.0.0/24`, where the first tenant’s route leads to
virtual machines in its own network and the second tenant’s leads to its office
in London. If the second tenant adds a route `10.0.0.5/32` for a printer in that
office, packets of the first tenant must be unaffected.

This is usually achieved by keeping a separate routing and forwarding instance,
called a virtual routing and forwarding (VRF) instance, for each tenant
@rfc4364. Each VRF has its own routing table and the forwarding table derived
from it. The router identifies each packet’s tenant, for example from the
interface it arrived on or from a tag carried in the packet, and performs LPM in
that tenant’s forwarding table. In the rest of this report, “VRF” refers to this
forwarding table.

#figure(
  figure-placeholder[one device with several VRFs; packets tagged with their
    tenant],
  caption: [A single forwarding device holding several VRFs. Each packet is
    tagged with its tenant and looked up in that tenant's VRF. This should
    support the earlier example from the cloud setting],
) <fig:vrfs>

Because tenants usually draw their addresses from the same private ranges, their
tables often contain the same prefixes, and storing them separately duplicates
this data. This adds to the memory and cache pressure described earlier. For
examaple, in five real production cloud gateway tables, as few as 10% of
prefixes were distinct across tenants @xu2024map.

// ===========================================================================
== Multi-tenant settings <sec:settings>

We focus on three representative settings in which multi-tenant LPM is performed
in software: software cloud gateways, provider-edge routers for L3VPNs, and host
virtual switches. All three forward traffic for many tenants from a single
device, but together they cover a wide range of workloads. Cloud gateways serve
up to a million tenants, most with very small VRFs. Provider-edge routers serve
tens to hundreds of VPN customers, each with VRFs of thousands of routes. Host
virtual switches serve tens to about a hundred tenants on each server of a data
centre. For each setting, we describe the system, why it runs or could run in
software, and its workload: the number of tenants, the size and similarity of
their tables, and how often routes change and are looked up. We end each with
the challenges this workload poses for a lookup structure and the opportunities
it offers; Section 2.6 examines how existing structures fare against them.

// ---------------------------------------------------------------------------
=== Software cloud gateways <sec:gateways>

#figure(
  figure-placeholder[redraw the Sailfish VXLAN routing / VM-NC mapping figure:
    two packets from VPC A, one to a VM in the same VPC and one to VPC B via a
    peering route],
  caption: [Forwarding in a cloud gateway. Adapted from @pan2021sailfish.],
) <fig:cloud-gateway>

Large public clouds let tenants rent virtual private clouds (VPCs), which are
isolated virtual networks of virtual machines. Traffic that leaves a VPC to the
internet, to cloud services such as storage, to the tenant's own offices, or to
another VPC, passes through a cloud gateway @pan2021sailfish. The gateway
therefore holds the complete routing state of every tenant @pan2021sailfish.
Such gateways are commonly built in software on ordinary x86 servers. Alibaba's
gateway is built on DPDK, a framework that lets packet-processing software
bypass the operating system kernel and run directly on the server's cores
@pan2021sailfish, @dpdk, and Google's runs entirely in software
@dalton2018andromeda. Some providers offload part of this traffic to
programmable switches @pan2021sailfish, but we focus on gateways implemented in
software.

Within the cloud, tenant traffic is carried using Virtual Extensible LAN
(VXLAN), which tags each packet with a VXLAN network identifier (VNI) @rfc7348.
The gateway reads the VNI to determine which tenant the packet belongs to, then
performs LPM on the destination address in that tenant's VRF @pan2021sailfish.
#todo[Walk through the two packets in @fig:cloud-gateway: one to a VM in the
  same VPC, one to VPC B via a peering route]

A large cloud region has up to a million tenants @pan2021sailfish, with between
177K and 1.16M routing entries in total across the regions measured by
QuarkTable @lu2024quarktable. Most tenants' tables are very small: 80% have
fewer than 10 routes, fewer than 5% have more than 100, and the largest has
about 1,500 @lu2024quarktable. The tables are also highly similar. Almost 80% of
tenants draw their addresses from `10.0.0.0/8` or `172.16.0.0/12`
@lu2024quarktable, and as few as 10% of prefixes are distinct across tenants
@xu2024map. Prefix lengths range from `/16` to `/32`, peaking at `/16`, `/20`
and `/24` @lu2024quarktable.

On the read side, a single x86 core processes about 1~million packets per
second, and a whole server tens of millions @pan2021sailfish. Traffic is heavily
skewed: about 5% of entries carry 95% of traffic @pan2021sailfish, so a
structure that keeps the busiest entries in cache could benefit considerably.
The tables are large relative to the cache. Server processors provide only a few
megabytes of last-level cache per core; the Xeon used by Arashloo et al., for
example, has 45~MB shared by 18~cores @arashloo2018. Although in this setting
most tenants' tables are small, their combined size is comparable to that of a
single large table. The largest table measured by Xu et al. holds over two
million entries for nearly a million tenants @xu2024map, and a single table of
two million routes in VPP (Vector Packet Processing), an open-source software
router, already takes about 47~MB @vppscale. Keeping a separate structure per
tenant can only add to this, since each may carry a fixed overhead regardless of
how many routes it holds. Updates arrive at about 1,200 per second on average
across a region (over 100~million per day) @wei2023achelous, but often in
bursts, for example when a large tenant joins or adds many routes at once
@pan2021sailfish. The resulting workload combines a very large number of tenants
with small, highly similar tables, traffic concentrated on a few entries, and
updates that are modest on average but bursty.

This workload poses two challenges for a lookup structure. With up to a million
tenants, any cost paid per tenant is multiplied a millionfold, and the combined
state already exceeds the cache. Updates arrive in bursts, which must not slow
lookups down. It also offers two opportunities. First, tenants draw their
addresses from the same few ranges, so much of the structure could be shared
between them. The result of a lookup, however, belongs to each tenant: the same
address means different things to different tenants (Section 2.2), so two
tenants with the same prefix send matching packets to different destinations in
their own networks. A shared structure can therefore store such a prefix once,
but must still return a separate result for each tenant. Whether these results
can themselves be stored in a shared form depends on how they are represented,
which Section 2.6.3 discusses.

// ---------------------------------------------------------------------------
=== L3VPN provider-edge routers <sec:pe>

#figure(
  figure-placeholder[two VPN customers, each with sites connected through CE
    routers to PEs; PEs connected across the provider's MPLS network; each PE
    holds one VRF per customer, and a customer's VRF is repeated on every PE its
    sites connect to],
  caption: [L3VPN provider-edge routers. Each PE keeps a VRF for every customer
    with a site connected to it, holding routes to all of that customer's
    sites.],
) <fig:l3vpn>

Here, tenants are customers of a VPN provider who want to connect their sites,
such as offices or data centres, to each other. Each site has a customer edge
(CE) router, which connects to a provider edge (PE) router maintained by the VPN
provider. The PE forwards the customer's traffic across the provider's network
using multiprotocol label switching (MPLS) to the customer's other sites
@rfc4364. Each PE serves several customers and keeps one VRF per customer. That
VRF holds all of the customer's VPN routes, so that any site can reach any other
site @kim2008relaying. Since a customer needs a VRF on every PE to which one of
its sites connects, and each must hold routes to all of its sites, the same
routes are repeated on every one of these PEs. The size of a VRF grows with the
number of sites the customer has, not with the traffic at this PE. The PE learns
each customer's routes from the customer's CE router, either through static
configuration or a routing protocol such as BGP @rfc4364, and exchanges them
with the other PEs using multiprotocol BGP @rfc4364.
#todo[Point to @fig:l3vpn, showing two customers with sites at several PEs, and
  each customer's VRF repeated on every PE its sites connect to.]

PEs have traditionally been hardware routers, whose forwarding tables sit in
limited on-chip memory. Kim et al. describe this memory as "a key bottleneck in
provisioning new customers" @kim2008relaying. PEs can also be implemented in
software, such as Juniper's vMX, which runs on x86 servers using DPDK
@dinham2021vmx. In this report we focus on PEs implemented in software.

A PE may serve from tens to hundreds of tenants; vMX, for example, supports up
to 250 VPN instances on its highest licence tier, and up to 2~million routes on
its Advance tier @dinham2021vmx. Each tenant may have thousands of routes. In a
large US VPN provider, the largest VPN, belonging to a single customer, had more
than 20,000 routes in each of its VRFs @kim2008relaying. For example, a chain of
7,200 stores connected by a single VPN needs at least 14,400 routes in the VRF
of every PE that serves even a single one of its stores @kim2008relaying. This
is because every store announces at least two routes, one for its own network
and one for the link connecting it to the provider’s PE @kim2008relaying.

Tenants’ tables are likely less similar than in the cloud setting. Sharing
requires identical prefixes, not merely addresses from the same range. Cloud
tenants mostly create their networks from the provider’s defaults, so many end
up with exactly the same prefixes @lu2024quarktable. VPN customers instead
connect existing networks, whose address plans were developed independently over
years and divide the private ranges in different ways. Some overlap is still
likely, but to our knowledge no published data measures it, nor the distribution
of prefix lengths in VPN tables.

On the read side, traffic is concentrated on a small part of each table: over
periods of a week to a month, in 80% of VRFs only 10% of prefixes or fewer
received any traffic, and these active sets stayed stable across periods
@kim2008relaying. Updates are driven by BGP. To our knowledge, no public data
describes their frequency in VPNs. A VPN’s routes change only when its
customer’s sites or links change, so updates are likely far less frequent than
in the Internet routing table, which receives up to about 160 updates per second
@li2014 and can serve as a pessimistic reference point. Compared with cloud
gateways, PEs serve fewer tenants with larger and less similar tables, and
traffic remains concentrated on a small part of each table.

This workload poses two challenges for a lookup structure. Tens to hundreds of
VRFs of thousands of routes each can together outgrow the cache, which, as
@sec:lpm showed, directly costs cores. And since tenants’ tables are likely less
similar than in the cloud, less of their state can be expected to be shared. It
also offers two opportunities. First, with fewer tenants, a fixed cost per
tenant matters far less than in the cloud, so per-tenant state can be afforded
where it speeds up lookups. Second, active sets are small and stable, and
updates are likely infrequent, so a structure can keep the busiest part of each
table in cache and spend more effort on each update to do so.

// ---------------------------------------------------------------------------
=== Host virtual switches <sec:vswitch>

#figure(
  figure-placeholder[a server hosting VMs of several tenants, a VPN endpoint
    elsewhere in the network, and the tenant's VRF with routes for `10.2.0.0/16`
    (next hop: the VPN endpoint) and `10.3.0.0/16` (part of the virtual network,
    no next hop). Two packets from the same VM: (1) LPM in the tenant's VRF
    decides how to forward each, directly to the VM for `10.3.1.5` and via the
    VPN endpoint for `10.2.7.9`; (2) an exact-match lookup of (VNI, address)
    gives the server hosting the destination VM or the VPN endpoint; the packet
    is then encapsulated and sent across the physical network to that server],
  caption: [A host virtual switch. LPM in the tenant's VRF decides how to
    forward a packet; an exact-match table then finds the server it must be sent
    to.],
  kind: image,
) <fig:vswitch>

A host virtual switch (vSwitch) is a software component running on every server
in a cloud data centre @firestone2017vfp, @dalton2018andromeda. It typically
acts as the tunnel endpoint for the server's virtual machines, encapsulating
their packets, for example with VXLAN, before they enter the physical network
and decapsulating packets on arrival. VXLAN was originally designed to carry
layer-2 traffic between machines @rfc7348, but in clouds the vSwitch also routes
packets at layer~3 on behalf of each tenant @firestone2017vfp. This leaves the
physical network unaware of tenants; multi-tenancy is handled entirely by the
vSwitches.

When a vSwitch receives a packet from a tenant VM, it encapsulates it with the
VM's VNI and sends it across the physical network to the right server. Finding
that server takes two lookups, because the destination is not always another VM.
Each virtual network has a set of routes in its VRF describing where ranges of
destination addresses should go. A virtual network might, for example, send
`10.1.0.0/16` to cloud storage, `10.2.0.0/16` to its VPN endpoint, and
`10.3.0.0/16` to other VMs in the same network.

The vSwitch first performs LPM in the tenant's VRF to decide how to forward the
packet @firestone2017vfp. The route for `10.3.0.0/16` only marks this range as
part of the virtual network, so a packet to `10.3.1.5` goes directly to the VM
with that address. The route for `10.2.0.0/16` instead names the VPN endpoint as
the next hop, so a packet to `10.2.7.9` goes there. Either way, the vSwitch then
needs to know which server hosts the VM or endpoint the packet goes to. For
this, it keeps an exact-match table that maps (VNI, private address) pairs to
server addresses @firestone2017vfp, @pan2021sailfish. The exact-match table
alone is not enough. It only knows where individual VMs are, so it cannot tell
whether `10.2.7.9` is a VM or a machine behind the VPN endpoint. Only the VRF
can decide this.
#todo[Worked example on @fig:vswitch: follow a packet to `10.3.1.5` and a packet
  to `10.2.7.9`. Both go through LPM and then the exact-match lookup. For
  `10.3.1.5`, the route only says the range belongs to the virtual network, so
  the destination address itself is looked up. For `10.2.7.9`, the route names
  the VPN endpoint, so its address is looked up instead. Show which server the
  packet is encapsulated towards in each case.]

As a result, the vSwitch must hold a VRF for every virtual network with a VM on
the server. Providers differ in how much of this state they keep on hosts.
Google installs only part of it and sends other traffic through dedicated
gateways @dalton2018andromeda, while Azure's vSwitch holds the routing tables
directly @firestone2017vfp. We assume the latter, so each VRF on a host has the
same contents as at the cloud gateway.

This may sound similar to the cloud gateway, but the vSwitch operates under
different constraints. Above all, it shares the server's cache and memory with
the tenants' VMs. Co-located VMs already compete for the shared last-level cache
@hu2026cacheman, and the vSwitch adds to this contention. Memory also has a
direct cost: DRAM can account for half of a server's cost @li2023pond, and
memory used by the vSwitch cannot be sold to tenants. Google, for example, caps
its host dataplane's memory and plans to account for it when placing VMs
@dalton2018andromeda. In addition, VMs are created and destroyed frequently,
with most living only minutes @hadary2020protean, and a server may host only one
or a few VMs from each virtual network. The vSwitch must therefore add and
remove whole VRFs as VMs arrive and leave, which can be costly for structures
with a large fixed size per table.

Current server processors have up to 192 cores @amd9965, and most cloud VMs need
only one to four cores @hadary2020protean. A single server can therefore host
roughly 50--100 VMs, and its vSwitch may hold tens to about a hundred VRFs, at
most one per VM. Since the VRFs have the same contents as at the gateway, most
hold fewer than 10 routes @lu2024quarktable. On the read side, the vSwitch
processes all traffic of the server's VMs, often on a small, dedicated share of
the CPU. Google reserves a single physical core for its dataplane, which
processes over 3~million packets per second within a budget of 300~ns per packet
@dalton2018andromeda. Updates take two forms. Route changes within a VRF arrive
from the cloud's control plane, as at the gateway, but a host sees only those
for the virtual networks it serves. VRFs themselves are added and removed as VMs
arrive and leave. No published figures give the rate per host, but with VM
lifetimes of minutes @hadary2020protean, this happens regularly on a busy
server. In summary, a vSwitch serves a modest number of tenants with very small,
likely overlapping tables, but must add and remove VRFs dynamically and has a
direct financial incentive to minimise memory use.

This workload poses two challenges for a lookup structure. First, the memory and
cache it uses are not its own. Every byte it occupies is memory that cannot be
sold to tenants, and every cache line it touches competes with the VMs on the
same server. This makes compactness matter more than in the other settings, even
though the total state is much smaller. Second, whole VRFs come and go as VMs
arrive and leave. Adding or removing a VRF must therefore be cheap and must not
disrupt lookups for the other tenants on the server. Unlike at a PE, a fixed
cost per VRF is not paid once but again with every VM that arrives. It also
offers two opportunities. First, the state is small: tens of VRFs of mostly
fewer than ten routes each amount to at most a few thousand routes. A compact
structure could keep all of them in cache, so that lookups rarely compete with
the VMs for memory at all. Second, the VRFs have the same contents as at the
cloud gateway, where tenants' tables are highly similar. If this holds on a
single host as well, much of the structure could be shared between tenants.

// ---------------------------------------------------------------------------
=== Comparison <sec:comparison>

#figure(
  {
    set text(size: 0.9em)
    table(
      columns: (3.3cm, 1fr, 1fr, 1fr),
      inset: (x: 6pt, y: 10pt),
      toprule,
      table.header[][*Cloud Gateway*][*L3VPN PEs*][*Virtual host switch*],
      midrule,

      [*Tenants per device*],
      [Up to 1M],
      [Tens to hundreds],
      [Tens to about a hundred],

      [*Routes per tenant*],
      [Mostly $<$10, up to 1500],
      [Thousands, more than 20,000],
      [As cloud gateway],

      [*Similarity between tenants*],
      [As few as 10% of prefixes are distinct],
      [Lower than cloud; not measured],
      [Likely high; not measured],

      [*Prefix lengths*],
      [From 16 to 32, mostly `/16`, `/20` and `/24`],
      [Customer subnets; no published distribution],
      [As cloud gateway],

      [*Number of active routes*],
      [5% is used by 95% of traffic],
      [80% of VRFs use $<$10% of their prefixes],
      [Not measured],

      [*Update frequency*],
      [$tilde$1,200/s],
      [BGP-driven; likely far below 160/s (Internet table)],
      [VRFs added and removed with VMs (lifetimes of minutes); rate not
        measured],

      [*Main constraint*],
      [Per-tenant cost, multiplied by up to 1M tenants],
      [Large, less similar tables outgrow the cache],
      [Memory and cache shared with VMs and high VRF churn],

      bottomrule,
    )
  },
  caption: [Workload characteristics of the three multi-tenant settings.],
) <tab:settings>

@tab:settings summarises the workloads of the three settings. Cloud gateways
combine a very large number of tenants with small, highly similar tables and
traffic concentrated on a few entries. PEs serve far fewer tenants, but with
much larger tables, and although customers still reuse private address ranges,
their tables are likely less similar, since each VPN's routes lead to its own
sites. Host virtual switches hold the same kind of small tables as cloud
gateways, but for at most about a hundred tenants, and under tighter
constraints: they share cache and memory with the tenants' VMs, memory they use
cannot be sold, and whole VRFs must be added and removed as VMs come and go.
Despite these differences, all three share the features that motivate this
project: a single device holds many tables at once, and the tables change while
they must be looked up to forward traffic. Where it has been measured, lookups
also concentrate on a small part of each table.

// ===========================================================================
== Concurrent updates and progress guarantees <sec:concurrency>

To increase the throughput of forwarding in software, routers process packets on
multiple threads, each typically running on its own core @vppthreads. Updates to
the forwarding table, however, are usually serialised: at most one writer
modifies a table at a time, while many packet-processing threads perform lookups
in it concurrently. In the Linux kernel, a single global lock (RTNL) is used for
all changes to network configuration, while readers access the shared state
under RCU without taking it @rtnetlink. DPDK's LPM library allows lookups from
many threads in parallel, but requires that additions and deletions are not
performed by multiple threads at once @dpdkthreads. Google's Andromeda goes
further: a control thread builds an updated copy of the flow table and then
points the packet-processing threads to it, so these threads never modify the
table and no locks are needed @dalton2018andromeda. We assume the same model: a
single writer and many concurrent readers.

Even with a single writer, readers and the writer still access the same table at
the same time. In all three settings of @sec:settings, every packet needs a
lookup, so lookups lie on the critical path and vastly outnumber updates. This
leads to two requirements. First, updates must not stop or slow down lookups.
This must hold even if the writer stalls for an arbitrary time, for example
because it is descheduled in the middle of an update. Likewise, a stalled reader
must not stop the writer. Second, updates must be atomically visible: a reader
must see the table as it was either before or after an update, never in between.

These requirements can be stated precisely using the progress guarantees defined
by Herlihy and Shavit @herlihy2008art. Under a _blocking_ guarantee, a stalled
thread can stop the progress of others, so a read might not be able to complete
until an update has finished. Under a _lock-free_ guarantee, at least one thread
is always guaranteed to make progress, so a complete stall of the whole program
is impossible. Under a _wait-free_ guarantee, every operation finishes in a
bounded number of its own steps, so each thread completes its operation
regardless of what the other threads are doing @herlihy1991.

A blocking guarantee is the easiest to implement, and the usual choice for most
applications. It does not meet our first requirement, however: a writer that
stalls while holding a lock stops every reader of the table. Ideally, both sides
would be wait-free. Readers would then complete regardless of the writer, and
the writer would never wait for readers. Whether readers are wait-free or only
lock-free depends on how memory is reclaimed, as the next paragraph and
@sec:reclamation explain. In either case, neither readers nor the writer block.

Giving up blocking makes memory management harder. With locks, a thread that
frees memory has exclusive access to it, so it is certain that no other thread
is reading it. Without them, even if the writer decides that part of the data
structure should be deleted, it cannot free that memory immediately, as readers
might still be accessing it. @sec:reclamation describes how such memory can be
reclaimed safely.

// ===========================================================================
== Memory reclamation methods <sec:reclamation>

Concurrent memory reclamation methods split freeing memory into two steps:
_retirement_ and _reclamation_. First, memory is retired: it is unlinked from
the data structure, so new lookups can no longer reach it, but lookups that
started earlier may still be using it. Then it is reclaimed, once the method can
guarantee that no thread can still be accessing it, at which point it is safe to
free.

Common methods for this are reference counting, quiescent-state-based
reclamation (QSBR), epoch-based reclamation (EBR) and hazard pointers (HP)
@hart2007. Reference counting keeps, in each node, a count of the readers
currently using it. It is simple, but inefficient, as every read must write to
shared memory. QSBR, the approach behind read-copy-update (RCU) in the Linux
kernel, waits until every reader has passed a point at which it holds no
references, guaranteeing that none can still access retired memory
@mckenney1998. EBR maintains a global epoch, which readers announce when they
start an operation, and memory retired in an epoch is freed once no reader can
still be in that epoch @fraser2004. With hazard pointers, each reader publishes
the pointers it is about to access in shared slots, and a node is freed only
when no thread has published a hazard pointer to it @michael2004.

EBR and QSBR are similar in concept: both wait for all threads to pass through
some state before deciding that memory is safe to reclaim. This is why both add
very little overhead to readers, but can leave an unbounded amount of
unreclaimed memory if a reader stalls @hart2007. HP bounds unreclaimed memory,
but adds a cost to every read @hart2007, @michael2004. The two approaches also
give readers different guarantees. Under EBR, a lookup only announces the epoch
and then traverses a structure of bounded depth, so it is wait-free. Under HP, a
lookup must re-check each pointer after publishing it and retry if it has
changed, so it is lock-free @michael2004. In both cases, the writer is
wait-free, as it never waits for readers. Because EBR and QSBR are so similar,
implementing both would add little, so we consider only EBR and HP, which sit at
opposite ends of this trade-off.

#references()
