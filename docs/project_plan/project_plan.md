# Project Plan

**Project Title:** Concurrent Multi-Tenant Longest-Prefix Matching for Software Routers and Switches
**Name:** Maksymilian Siekliński
**Supervisor:** Stefano Vissicchio

## 1 Aims & Objectives

**Aim:** To improve the efficiency (memory use, lookup performance and update cost) of software routers and switches in multi-tenant environments, with a focus on concurrent reads and updates.

**Objectives:**

1. Review existing data structures for multi-tenant packet forwarding, as well as safe memory reclamation methods for concurrent data structures.
2. Identify realistic settings for multi-tenant software routing and characterise their workloads using published data.
3. Implement and evaluate a per-tenant baseline and state-of-the-art multi-tenant forwarding structures, as a reference for comparison.
4. Adapt existing multi-tenant forwarding structures to support concurrent lookups and updates, and where promising develop new ones.
5. Integrate safe memory reclamation methods with these structures.
6. Evaluate which combinations of data structure and memory reclamation method suit each setting, under different workloads and update patterns.

## 2 Expected outcomes/deliverables

- A literature survey of multi-tenant forwarding structures and safe memory reclamation techniques.
- A characterisation of the target settings, and a workload generator based on published statistics.
- A design specification and documented C++ implementations of the data structures and reclamation methods.
- A testing and evaluation strategy, with results comparing combinations of data structures and reclamation methods across settings.

## 3 Work Plan

- **Mid-October:** Complete the overview of multi-tenant forwarding structures and memory reclamation methods.
- **End of October:** Identify three real-life settings for the project, together with a characterisation of their workloads.
- **Mid-November:** Implement the per-tenant baseline, together with the workload generator and the correctness and performance measurement harnesses.
- **Early December:** Implement existing compression techniques and explore new shared designs; compare them with the baseline in a single-threaded environment.
- **Late December to early January:** Implement a first memory reclamation method for the promising structures and compare their performance.
- **Mid-January:** Write the interim report with the current results.
- **End of January:** Implement a second memory reclamation method.
- **End of February:** Evaluate all combinations of structures and reclamation methods on synthetic workloads based on published statistics.
- **Early March:** Start report writing.
