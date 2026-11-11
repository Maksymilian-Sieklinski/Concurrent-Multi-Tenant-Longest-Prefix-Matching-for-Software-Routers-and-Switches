# Concurrent Multi-Tenant Longest-Prefix Matching for Software Routers and Switches

This project aims to improve the efficiency (memory use, lookup performance and update cost) of software routers and switches that forward traffic for many tenants at once, such as cloud gateways, L3VPN provider-edge routers and host virtual switches. It implements and evaluates a simple per-tenant baseline and state-of-the-art forwarding data structures, extends the most promising ones to support concurrent lookups and updates, and integrates them with safe memory reclamation methods (epoch-based reclamation and hazard pointers). The evaluation studies how the number of tenants and the similarity of their forwarding tables affect these metrics under concurrent updates.

The `docs/` folder contains the current project documents.

## Building the documents

The documents are written in [Typst](https://typst.app). Each `.typ` file is the source; the PDF next to it is generated from it.

```sh
cd docs
make                                          # build every PDF
make watch DOC=BackGround_Section/chapter2    # rebuild one PDF on every save
```

- `docs/shared/report.typ` is the shared chapter style (headings, page header, figure/table numbering, `figure-placeholder`).
- `docs/shared/refs.bib` is the single bibliography; cite with `@key` and finish a chapter with `#references()`.
- For live preview in VS Code, install the **Tinymist** extension and open the repo (or `docs/`) as the workspace.
