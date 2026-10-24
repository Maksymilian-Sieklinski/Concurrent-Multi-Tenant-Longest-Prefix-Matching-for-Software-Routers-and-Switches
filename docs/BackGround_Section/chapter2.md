# 2. Background & related work

## 2.1 Longest Prefix Match in software

Routers decide where to forward each packet using longest prefix match (LPM) [1]. Stripped of its networking context, the problem is simple: given a set of strings S and a query string x, find the longest string in S that is a prefix of x. For example, let S = {ABA, ABAC}. For x = ABAS, the answer is ABA, since ABA is a prefix of ABAS but ABAC is not. For x = ABACE, both are prefixes, and the answer is ABAC because it is longer.

In a router, the strings are bit strings. A routing table is a set of routes, each pairing an address prefix with a next hop. A prefix is written as an address followed by its length in bits: 10.0.0.0/8 denotes the first 8 bits of 10.0.0.0, i.e. 00001010. For each packet, the router finds the longest prefix in the table that matches the packet's destination address and forwards the packet to that route's next hop.

[Figure 2.1: example routing table and the corresponding binary trie]

A binary trie for IPv4 addresses needs up to 32 memory accesses per lookup, which is usually too slow for packet forwarding. Real implementations reduce this by matching several bits per step (multibit tries [2]), by skipping chains of single-child nodes (path compression [3]), or by indexing directly into a large table using the first bits of the address [4]. These bring a lookup down to a handful of memory accesses. Multibit tries and direct indexing use more memory than a binary trie, trading it for faster lookups. With millions of packets per second to process, packet forwarding prioritises lookup time, but these structures also make updates more expensive, since a single route change can rewrite many entries [1].

In software, lookup time is dominated not by computation but by memory accesses [5]. Each step must fetch the next node, and nodes are often not close to each other in memory, which hurts cache locality. This matters because a cache miss can take tens to hundreds of times longer than a hit [5]. Lookup performance depends heavily on how much of the structure fits in cache. Cloudflare observed lookups in the Linux kernel's BPF LPM trie slow sharply as tables grew beyond tens of thousands of entries [6], and Arashloo et al. found that a lookup table too large for the L3 cache needed almost twice as many cores to sustain the same packet rate [7]. In multi-tenant settings, a software router must keep a separate structure for every tenant, so their combined size quickly outgrows the cache.

## 2.2 Multi-tenancy

Multi-tenancy describes a single forwarding device shared by many independent users, called tenants. Tenants may be virtual networks in a data centre or cloud, or customers of a virtual private network (VPN) service.

Each tenant expects its network to behave as if it were the only one, even though it shares the underlying infrastructure with many others. Two requirements follow. First, tenants commonly use the same private address ranges [8], so the same address can mean different things to different tenants [9]. Second, tenants must be isolated, so one tenant's routes must not affect how another tenant's packets are forwarded. For example, two tenants may both define a route for 10.0.0.0/24, where the first tenant's route leads to a server in its virtual network and the second tenant's leads to its office in London. If the second tenant adds a route 10.0.0.5/32 for a printer in that office, packets of the first tenant must be unaffected.

This is usually achieved by keeping a separate routing table, called a virtual routing and forwarding (VRF) instance, for each tenant [9]. The router identifies each packet's tenant, for example from the interface it arrived on or from a tag carried in the packet, and performs LPM in that tenant's VRF.

[Figure 2.2: one device with several VRFs; packets tagged with their tenant]

Because tenants draw their addresses from the same private ranges, their tables often contain the same prefixes, and storing them separately duplicates this data. This adds to the memory and cache pressure described above. In the routing tables of one cloud provider, only about 10% of prefixes were distinct across tenants [10].

## 2.3 Multi-tenant settings

We focus on three settings in which multi-tenant lookup arises: software cloud gateways, provider-edge routers for L3VPNs, and host virtual switches. All three forward traffic for many tenants from a single device, but they cover a wide range of workloads. Cloud gateways serve up to a million tenants, most with very small VRFs. Provider-edge routers serve tens to hundreds of VPN customers, each with VRFs of thousands of routes. Host virtual switches serve tens to about a hundred tenants on each server of a data centre. For each setting, we describe the system, why it runs or could run in software, and its workload: the number of tenants, the size and similarity of their tables, and how often routes change and are looked up.

### 2.3.1 Software cloud gateways

Large public clouds let tenants rent virtual private clouds (VPCs), which are isolated virtual networks of virtual machines. Traffic that leaves a VPC to the internet, to cloud services such as storage, to the tenant's own offices, or to another VPC, passes through a cloud gateway [11]. The gateway therefore holds the complete routing state of every tenant [11]. Such gateways are commonly built in software on ordinary x86 servers. Alibaba's gateway is built on DPDK, a framework that lets packet-processing software bypass the operating system kernel and run directly on the server's cores [11], [12], and Google's runs entirely in software [13]. Some providers offload part of this traffic to programmable switches [11], but we focus on gateways implemented in software.

Within the cloud, tenant traffic is carried using Virtual Extensible LAN (VXLAN), which tags each packet with a VXLAN network identifier (VNI) [14]. The gateway reads the VNI to determine which tenant the packet belongs to, then performs LPM on the destination address in that tenant's VRF [11].

A large cloud region has up to a million tenants [11], with between 177K and 1.16M routing entries in total across the regions measured by QuarkTable [15]. Most tenants' tables are very small: 80% have fewer than 10 routes, fewer than 5% have more than 100, and the largest has about 1,500 [15]. The tables are also highly similar. Almost 80% of tenants draw their addresses from 10.0.0.0/8 or 172.16.0.0/12 [15], and only about 10% of prefixes are distinct across tenants [10]. Prefix lengths range from /16 to /32, peaking at /16, /20 and /24 [15].

On the read side, a single x86 core processes about 1 million packets per second, and a whole server tens of millions [11]. Traffic is heavily skewed: about 5% of entries carry 95% of traffic [11], so a structure that keeps the busiest entries in cache could benefit considerably. The tables are large relative to the cache. Server processors provide only a few megabytes of last-level cache per core; the Xeon used by Arashloo et al., for example, has 45 MB shared by 18 cores [7]. A single table of two million routes in VPP (Vector Packet Processing), an open-source software router, already takes about 47 MB [16], and keeping a separate table for each of a million tenants would need far more. Updates arrive at about 1,200 per second on average across a region (over 100 million per day) [17], but often in bursts, for example when a large tenant joins or adds many routes at once [11]. The resulting workload combines a very large number of tenants with small, highly similar tables, traffic concentrated on a few entries, and updates that are modest on average but bursty.

### 2.3.2 L3VPN provider-edge routers

Here, tenants are customers of a VPN provider who want to connect their sites, such as offices or data centres, to each other. Each site has a customer edge (CE) router, which connects to a provider edge (PE) router maintained by the VPN provider. The PE forwards the customer's traffic across the provider's network using MPLS to the customer's other sites [9]. Each PE serves several customers and keeps one VRF per customer. That VRF holds all of the customer's VPN routes, so that any site can reach any other site [18]. The size of a VRF grows with the number of sites the customer has, not with the traffic at this PE. The PE learns each customer's routes from the customer's CE router, either through static configuration or a routing protocol such as BGP [9], and exchanges them with the other PEs using multiprotocol BGP [9].

PEs have traditionally been hardware routers, whose forwarding tables sit in limited on-chip memory. Kim et al. describe this memory as "a key bottleneck in provisioning new customers" [18]. PEs can also be implemented in software, such as Juniper's vMX, which runs on x86 servers using DPDK [19]. In this report we focus on PEs implemented in software.

A PE may serve from tens to hundreds of tenants; vMX, for example, supports up to 250 VPN instances on its highest licence tier, and up to 2 million routes (8 million at 10 Gbps and above) on its Advance tier [19]. Each tenant may have thousands of routes. In AT&T's network, the largest VPN installs more than 20,000 routes in each of its VRFs [18]. For example, a chain of 7,200 stores connected by a single VPN requires at least 14,400 routes in every VRF of that VPN [18]. At the extreme, a VRF may contain the Internet routes themselves [9], and some providers carry the full Internet routing table inside a VRF in this way [20], which today holds about one million IPv4 prefixes [21]. Tenants' tables are less similar than in the cloud setting, since each VPN's routes lead to its own sites [22], but customers still reuse private address ranges, so some overlap remains. To our knowledge, no published data describes the distribution of prefix lengths in VPN tables.

On the read side, Arashloo et al. built a cloud provider's VPN edge whose per-customer lookups run in software on a server, forwarding about 40 Gbps on 10 cores [7]. When their lookup table grew to 16 million entries, larger than the processor's 45 MB L3 cache, it needed 10 cores instead of 6 for the same packet rate [7]. Traffic is concentrated on a small part of each table: 80% of VRFs use only about 10% of their prefixes, and these active sets stay stable over weeks [18]. Updates are driven by BGP. No public data on their frequency in VPNs exists, but Internet routing tables, which are larger and more volatile, receive up to about 160 updates per second [23]. This suggests an upper bound for most VRFs and is the actual rate for a VRF carrying the Internet table. Compared with cloud gateways, PEs serve fewer tenants with larger and less similar tables; traffic remains concentrated on a small part of each table, and updates are driven by BGP.

### 2.3.3 Host virtual switches

A host virtual switch (vSwitch) is a software component running on every server in a cloud data centre [24], [13]. It typically acts as the tunnel endpoint for the server's virtual machines, encapsulating their packets, for example with VXLAN, before they enter the physical network and decapsulating packets on arrival. VXLAN was originally designed to carry layer-2 traffic between machines [14], but in clouds the vSwitch also routes packets at layer 3 on behalf of each tenant [24]. This leaves the physical network unaware of tenants; multi-tenancy is handled entirely by the vSwitches.

When a vSwitch receives a packet from a tenant VM, it encapsulates it with the VM's VNI and sends it across the physical network as an ordinary packet. To do so, it must know the physical address of the server hosting the destination, so it keeps an exact-match table mapping (VNI, private address) pairs to server addresses [24], [11]. This table alone is not enough, as each virtual network also has a set of routes in its VRF describing where ranges of destination addresses should go. A virtual network might, for example, send 10.1.0.0/16 to cloud storage, 10.2.0.0/16 to its VPN endpoint, and 10.3.0.0/16 to other VMs in the same network. The vSwitch therefore first performs LPM in the tenant's VRF to decide what to do with the packet, and only then, if the destination is another VM, uses the exact-match table to find its server [24]. As a result, it must hold a VRF for every virtual network with a VM on the server. Providers differ in how much of this state they keep on hosts: Google installs only part of it, sending other traffic through dedicated gateways [13], while Azure's vSwitch holds routing tables directly [24]. We assume the latter, so each VRF on a host has the same contents as at the cloud gateway.

This may sound similar to the cloud gateway, but the vSwitch operates under different constraints. Above all, it shares the server's cache and memory with the tenants' VMs. Co-located VMs already compete for the shared last-level cache [25], and the vSwitch adds to this contention. Memory also has a direct cost: DRAM can account for half of a server's cost [26], and memory used by the vSwitch cannot be sold to tenants. Google, for example, caps its host dataplane's memory and plans to account for it when placing VMs [13]. In addition, VMs are created and destroyed frequently, with most living only minutes [27], and a server may host only one or a few VMs from each virtual network. The vSwitch must therefore add and remove whole VRFs as VMs arrive and leave, which can be costly for structures with a large fixed size per table.

Current server processors have up to 192 cores [28], and most cloud VMs need only one to four cores [27]. A single server can therefore host roughly 50–100 VMs, and its vSwitch may hold tens to about a hundred VRFs, at most one per VM. Since the VRFs have the same contents as at the gateway, most hold fewer than 10 routes [15]. On the read side, the vSwitch processes all traffic of the server's VMs, often on a small, dedicated share of the CPU. Google reserves a single physical core for its dataplane, which processes over 3 million packets per second within a budget of 300 ns per packet [13]. Updates take two forms. Route changes within a VRF arrive from the cloud's control plane, as at the gateway, but a host sees only those for the virtual networks it serves. VRFs themselves are added and removed as VMs arrive and leave. No published figures give the rate per host, but with VM lifetimes of minutes [27], this happens regularly on a busy server. In summary, a vSwitch serves a modest number of tenants with very small, likely overlapping tables, but must add and remove VRFs dynamically and has a direct financial incentive to minimise memory use.

### 2.3.4 Comparison

[Table 2.1: Workload characteristics of the three settings — table as in your draft]

Table 2.1 summarises the workloads of the three settings. Cloud gateways combine a very large number of tenants with small, highly similar tables and traffic concentrated on a few entries. PEs serve far fewer tenants, but with much larger tables, and although customers still reuse private address ranges, their tables are likely less similar, since each VPN's routes lead to its own sites. Host virtual switches hold the same kind of small tables as cloud gateways, but for only tens of tenants, and under tighter constraints: they share cache and memory with the tenants' VMs, memory they use cannot be sold, and whole VRFs must be added and removed as VMs come and go. Despite these differences, all three share the features that motivate this project: a single device holds many tables at once, and the tables change while traffic flows through them. Where it has been measured, lookups also concentrate on a small part of each table.

## 2.4 Concurrent updates and lock-free lookups

To increase the throughput of forwarding in software, routers process packets on multiple threads, each typically running on its own core [29]. This allows the same machine to handle many packets at the same time, increasing throughput and utilisation. Nevertheless, concurrency has its own drawbacks once there are concurrent updates and reads of the same shared state, in our case the forwarding table. We have to ensure that even while updates are in progress, readers can still fetch correct information from the data structure, and that update processing does not slow them down. This matters especially in forwarding, where read throughput is critical and reads vastly outnumber updates. This guarantee should hold even if threads stall for an arbitrary time. We also have to ensure atomic visibility of updates: readers should see the state of the data structure only before or after an update, never while it is in progress.

In practice, updates to forwarding tables are serialised, so that at most one writer modifies a table at a time, while many threads perform lookups concurrently. In the Linux kernel, a single global lock (RTNL) is used for all changes to network configuration, while readers access the shared state under RCU without taking it [30]. DPDK's LPM library allows lookups from many threads in parallel, but requires that additions and deletions are not performed by multiple threads at once [31]. Google's Andromeda goes further: a control thread builds an updated copy of the flow table and then points the packet-processing threads to it, so these threads never modify the table and no locks are needed [13].

How updates can affect reads is characterised by three progress guarantees, as defined by Herlihy and Shavit [32]. Under a blocking guarantee, a stalled thread can stop the progress of others, so a read might not be able to complete until an update has finished. Under a lock-free guarantee, at least one thread is always guaranteed to make progress, so a complete stall of the whole program is impossible. Under a wait-free guarantee, every operation finishes in a bounded number of its own steps, so regardless of what other threads are doing, each thread completes its operation [33].

A blocking guarantee is the easiest to implement, and the usual choice for most applications. However, blocking does not fit our requirements, as a stalled writer could prevent all other threads from reading, reducing throughput and slowing down readers. Preferably, readers would be wait-free, so that they are independent of the writer, and the writer would be wait-free too, never waiting for readers. Which guarantee readers achieve depends on the chosen memory reclamation method, but regardless of which one is chosen, neither will block.

Giving up blocking makes memory management harder. With locks, a thread that frees memory has exclusive access to it, so it is certain that no other thread is reading it. Without them, even if the writer decides that part of the data structure should be deleted, it cannot reclaim that memory immediately, as readers might still be accessing it. Section 2.5 describes how such memory can be reclaimed safely.

## 2.5 Memory reclamation methods

Concurrent memory reclamation methods split freeing memory into two steps: retirement and reclamation. First, memory is retired: it is unlinked from the data structure, so new lookups can no longer reach it, but lookups that started earlier may still be using it. Then it is reclaimed, once the method can guarantee that no thread can still be accessing it, at which point it is safe to free.

Common methods for this are reference counting, quiescent-state-based reclamation (QSBR), epoch-based reclamation (EBR) and hazard pointers (HP) [34]. Reference counting keeps, in each node, a count of the readers currently using it. It is simple, but inefficient, as every read must write to shared memory. QSBR, the approach behind read-copy-update (RCU) in the Linux kernel, waits until every reader has passed a point at which it holds no references, guaranteeing that none can still access retired memory [35]. EBR maintains a global epoch, which readers announce when they start an operation, and memory retired in an epoch is freed once no reader can still be in that epoch [36]. With hazard pointers, each reader publishes the pointers it is about to access in shared slots, and a node is freed only when no thread has published a hazard pointer to it [37].

EBR and QSBR are similar in concept: both wait for all threads to pass through some state before deciding that memory is safe to reclaim. This is why both add very little overhead to readers, but can leave an unbounded amount of unreclaimed memory if a reader stalls [34]. HP bounds unreclaimed memory, but adds a cost to every read [34], [37]. The two approaches also give readers different guarantees. Under EBR, a lookup only announces the epoch and then traverses a structure of bounded depth, so it is wait-free. Under HP, a lookup must re-check each pointer after publishing it and retry if it has changed, so it is lock-free [37]. In both cases, the writer is wait-free, as it never waits for readers. Because EBR and QSBR are so similar, implementing both would add little, so we consider only EBR and HP, which sit at opposite ends of this trade-off.

## References

[1] M. Á. Ruiz-Sánchez, E. W. Biersack, and W. Dabbous, "Survey and taxonomy of IP address lookup algorithms," *IEEE Network*, vol. 15, no. 2, pp. 8–23, 2001.

[2] V. Srinivasan and G. Varghese, "Fast address lookups using controlled prefix expansion," *ACM Transactions on Computer Systems*, vol. 17, no. 1, pp. 1–40, 1999.

[3] D. R. Morrison, "PATRICIA—Practical algorithm to retrieve information coded in alphanumeric," *Journal of the ACM*, vol. 15, no. 4, pp. 514–534, 1968.

[4] P. Gupta, S. Lin, and N. McKeown, "Routing lookups in hardware at memory access speeds," in *Proc. IEEE INFOCOM*, 1998, pp. 1240–1247.

[5] U. Drepper, "What every programmer should know about memory," Red Hat, Inc., Nov. 2007. [Online]. Available: https://people.redhat.com/drepper/cpumemory.pdf

[6] M. Fleming and J. Brouer, "A deep dive into BPF LPM trie performance and optimization," *The Cloudflare Blog*, Oct. 21, 2025. [Online]. Available: https://blog.cloudflare.com/a-deep-dive-into-bpf-lpm-trie-performance-and-optimization/

[7] M. T. Arashloo, P. Shirshov, R. Gandhi, G. Lu, L. Yuan, and J. Rexford, "A scalable VPN gateway for multi-tenant cloud services," *ACM SIGCOMM Computer Communication Review*, vol. 48, no. 1, pp. 50–55, Jan. 2018.

[8] Y. Rekhter, B. Moskowitz, D. Karrenberg, G. J. de Groot, and E. Lear, "Address allocation for private internets," RFC 1918 (BCP 5), Feb. 1996.

[9] E. Rosen and Y. Rekhter, "BGP/MPLS IP virtual private networks (VPNs)," RFC 4364, Feb. 2006.

[10] K. Xu, D. Jiang, Y. Li, X. Wang, D. Zhang, and G. Xie, "MaP: Increasing node capacity of programmable cloud gateways," *Computer Networks*, vol. 251, Art. no. 110590, 2024.

[11] T. Pan, N. Yu, C. Jia, J. Pi, L. Xu, Y. Qiao, Z. Li, K. Liu, J. Lu, J. Lu, E. Song, J. Zhang, T. Huang, and S. Zhu, "Sailfish: Accelerating cloud-scale multi-tenant multi-service gateways with programmable switches," in *Proc. ACM SIGCOMM*, 2021.

[12] DPDK Project, "Data Plane Development Kit." [Online]. Available: https://www.dpdk.org

[13] M. Dalton et al., "Andromeda: Performance, isolation, and velocity at scale in cloud network virtualization," in *Proc. 15th USENIX Symposium on Networked Systems Design and Implementation (NSDI)*, 2018, pp. 373–387.

[14] M. Mahalingam, D. Dutt, K. Duda, P. Agarwal, L. Kreeger, T. Sridhar, M. Bursell, and C. Wright, "Virtual eXtensible Local Area Network (VXLAN): A framework for overlaying virtualized layer 2 networks over layer 3 networks," RFC 7348, Aug. 2014.

[15] J. Lu, H. Zhao, Y. Qi, Y. Feng, X. Guo, S. Li, E. Song, X. Wei, B. Lyu, R. Wen, and S. Zhu, "QuarkTable: Building compact forwarding tables for programmable switches on public clouds," in *Proc. 8th Asia-Pacific Workshop on Networking (APNet)*, 2024.

[16] FD.io, "Scale," *The Vector Packet Processor (VPP) v24.02 documentation*. [Online]. Available: https://s3-docs.fd.io/vpp/24.02/developer/corefeatures/fib/scale.html

[17] C. Wei, X. Li, Y. Yang, X. Jiang, T. Xu, B. Yang, T. Wu, C. Xu, Y. Lv, H. Gao, Z. Zhang, Z. Chen, Z. Wang, Z. Zhang, S. Zhu, and W. Chen, "Achelous: Enabling programmability, elasticity, and reliability in hyperscale cloud networks," in *Proc. ACM SIGCOMM*, 2023, pp. 769–782.

[18] C. Kim, A. Gerber, C. Lund, D. Pei, and S. Sen, "Scalable VPN routing via relaying," in *Proc. ACM SIGMETRICS*, 2008, pp. 61–72.

[19] M. Dinham, M. Katti, and V. Reddy, *Day One: vMX Up and Running*, 2nd ed. Juniper Networks Books, 2021.

[20] M. Tinka, "[c-nsp] Internet in VRF," cisco-nsp mailing list, May 1, 2015. [Online]. Available: https://puck.nether.net/pipermail/cisco-nsp/2015-May/099383.html

[21] G. Huston, "A day in the life of BGP," *The ISP Column*, Jun. 2025. [Online]. Available: https://ispcol.potaroo.net/2025-06/daybgp.html

[22] L. Luo, G. Xie, S. Uhlig, L. Mathy, K. Salamatian, and Y. Xie, "Towards TCAM-based scalable virtual routers," in *Proc. 8th ACM International Conference on Emerging Networking Experiments and Technologies (CoNEXT)*, 2012, pp. 73–84.

[23] Y. Li, D. Zhang, K. Huang, D. He, and W. Long, "A memory-efficient parallel routing lookup model with fast updates," *Computer Communications*, vol. 38, pp. 60–71, 2014.

[24] D. Firestone, "VFP: A virtual switch platform for host SDN in the public cloud," in *Proc. 14th USENIX Symposium on Networked Systems Design and Implementation (NSDI)*, 2017, pp. 315–328.

[25] X. Hu, Y. Cao, N. Guan, Y. Wu, X. Qiu, S. Dai, B. Luo, S. Cheng, F. Qiu, Y. Shen, and J. Wu, "Cacheman: A comprehensive last-level cache management system for multi-tenant clouds," in *Proc. ACM SIGPLAN Symposium on Principles and Practice of Parallel Programming (PPoPP)*, 2026.

[26] H. Li, D. S. Berger, S. Novakovic, L. Hsu, D. Ernst, P. Zardoshti, M. Shah, S. Rajadnya, S. Lee, I. Agarwal, M. D. Hill, M. Fontoura, and R. Bianchini, "Pond: CXL-based memory pooling systems for cloud platforms," in *Proc. ACM International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS)*, 2023.

[27] O. Hadary, L. Marshall, I. Menache, A. Pan, E. E. Greeff, D. Dion, S. Dorminey, S. Joshi, Y. Chen, M. Russinovich, and T. Moscibroda, "Protean: VM allocation service at scale," in *Proc. 14th USENIX Symposium on Operating Systems Design and Implementation (OSDI)*, 2020, pp. 845–861.

[28] AMD, "AMD EPYC 9965," product specifications, 2024. [Online]. Available: https://www.amd.com/en/products/processors/server/epyc/9005-series/amd-epyc-9965.html

[29] FD.io, "Multi-threading in VPP," *The Vector Packet Processor (VPP) v24.06 documentation*. [Online]. Available: https://s3-docs.fd.io/vpp/24.06/developer/corearchitecture/multi_thread.html

[30] The Linux Kernel, `include/linux/rtnetlink.h` (source file; comment: "RTNL is used as a global lock for all changes to network configuration"). [Online]. Available: https://android.googlesource.com/kernel/common/+/cb0a3edf8d00/include/linux/rtnetlink.h

[31] DPDK Project, "Thread safety of DPDK functions," in *DPDK Programmer's Guide*, v17.11. [Online]. Available: https://dpdk.readthedocs.io/en/v17.11/prog_guide/thread_safety_dpdk_functions.html

[32] M. Herlihy and N. Shavit, *The Art of Multiprocessor Programming*. Morgan Kaufmann, 2008.

[33] M. Herlihy, "Wait-free synchronization," *ACM Transactions on Programming Languages and Systems*, vol. 13, no. 1, pp. 124–149, 1991.

[34] T. E. Hart, P. E. McKenney, A. D. Brown, and J. Walpole, "Performance of memory reclamation for lockless synchronization," *Journal of Parallel and Distributed Computing*, vol. 67, no. 12, pp. 1270–1285, 2007.

[35] P. E. McKenney and J. D. Slingwine, "Read-copy update: Using execution history to solve concurrency problems," in *Proc. Parallel and Distributed Computing and Systems (PDCS)*, Las Vegas, NV, USA, Oct. 1998, pp. 509–518.

[36] K. Fraser, "Practical lock-freedom," Ph.D. dissertation, University of Cambridge, 2004. Published as Technical Report UCAM-CL-TR-579.

[37] M. M. Michael, "Hazard pointers: Safe memory reclamation for lock-free objects," *IEEE Transactions on Parallel and Distributed Systems*, vol. 15, no. 6, pp. 491–504, 2004.
