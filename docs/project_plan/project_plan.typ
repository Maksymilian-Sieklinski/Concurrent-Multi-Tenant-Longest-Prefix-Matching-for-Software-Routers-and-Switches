#set document(title: "Project Plan", author: "Maksymilian Siekliński")
#set page(paper: "a4", margin: 1in)
#set text(font: "New Computer Modern", size: 11pt, lang: "en")
#set par(justify: true)
#set heading(numbering: "1")
#show heading: set block(above: 1.6em, below: 0.9em)
#show heading.where(level: 1): set text(size: 14.4pt)
#set list(spacing: 0.6em)
#set enum(spacing: 0.6em)

#align(center, text(size: 17.28pt, weight: "bold")[Project Plan])

*Project Title:* Concurrent Multi-Tenant Longest-Prefix Matching for Software Routers and Switches \
*Name:* Maksymilian Siekliński \
*Supervisor:* Stefano Vissicchio

= Aims & Objectives

*Aim:* To improve the efficiency (memory use, lookup performance and update cost) of
software routers and switches in multi-tenant environments, with a focus on concurrent lookups and
updates. In particular, we aim to determine how the number of tenants and the similarity of their
forwarding tables affect these three metrics under concurrent updates.

*Objectives:*
+ Review existing data structures for multi-tenant packet forwarding, as well as safe memory
  reclamation methods for concurrent data structures.
+ Identify realistic multi-tenant settings for software forwarding and characterise their
  workloads using published data.
+ Implement and evaluate a simple baseline, using a separate data structure for each tenant,
  and state-of-the-art forwarding data structures from the literature, as reference points for
  comparison.
+ Extend the most promising of these structures to support concurrent lookups and updates
  where they do not already, and develop new ones where worthwhile.
+ Integrate safe memory reclamation methods with these forwarding data structures.
+ Evaluate which combinations of forwarding data structure and memory reclamation method suit
  each setting, under different workloads and update patterns.

= Expected outcomes/deliverables

- A literature review of multi-tenant forwarding data structures and safe memory reclamation
  techniques.
- A characterisation of the target settings, and a workload generator based on published
  statistics.
- A design specification and documented C++ implementations of the data structures and
  reclamation methods.
- A testing and evaluation strategy, with results comparing combinations of data structures
  and reclamation methods across settings.

#pagebreak()
= Work Plan

Dates are completion milestones: each activity is finished by the date given.

- *Throughout the whole project:* Write the report alongside implementation and
  evaluation.
- *Mid-October:* Literature review of multi-tenant forwarding data structures and
  memory reclamation methods.
- *End of October:* Identify three real-life settings for the project, together with a
  characterisation of their workloads.
- *Mid-November:* Implement simple baseline, together with the workload generator and
  the correctness and performance measurement harnesses.
- *Mid-December:* Implement state-of-the-art data structures and explore new shared
  designs; compare them with the baseline in a single-threaded environment.
- *Mid-January:* Extend the promising data structures to support
  concurrent lookups and updates, with a first memory reclamation method, and document their
  design.
- *Mid-January:* Write the interim report with the current results.
- *Mid-February:* Implement a second memory reclamation method.
- *Mid-March:* Evaluate all combinations of structures and reclamation methods on
  synthetic workloads based on published statistics.
- *Mid-April:* Complete the full draft.
