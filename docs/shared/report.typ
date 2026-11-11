// Shared style for report chapters (background, interim report, final report).
//
// Usage, from a file in a subfolder of docs/:
//   #import "../shared/report.typ": *
//   #show: report.with(title: "Background and Related Work")
//   ...
//   #references()
//
// Importing from a sibling folder needs the Typst root to be docs/ (or above),
// so compile with `--root .` from docs/ (the Makefile does this).

#let accent = rgb(25, 55, 110)
#let link-colour = rgb("#006795") // close to LaTeX's dvipsnames MidnightBlue

// Dashed box standing in for a figure that has not been drawn yet.
#let figure-placeholder(description) = align(center, rect(
  width: 80%,
  height: 5cm,
  radius: 4pt,
  stroke: (paint: accent.lighten(40%), thickness: 0.8pt, dash: "dashed"),
  align(center + horizon, text(fill: accent.lighten(20%), style: "italic")[
    Figure to be added: \ #description
  ]),
))

// Notes to self: #todo[add a citation for the 67 ns figure]
// Shown in red while drafting. Compile with `--input final=true`
// (`make -B FINAL=1`) and they disappear, so none leak into a submission.
#let draft = sys.inputs.at("final", default: "false") != "true"

#let todo(body) = if draft {
  [#metadata(body) <todo>]
  text(fill: red, style: "italic")[[TODO: #body]]
}

// Clickable list of every #todo in the document, with page numbers.
// Put #todo-list() at the top or bottom of a file while drafting.
#let todo-list() = if draft {
  context {
    let notes = query(<todo>)
    if notes.len() == 0 { return }
    block(stroke: 0.6pt + red, inset: 8pt, radius: 4pt, width: 100%, {
      text(fill: red, weight: "bold")[Outstanding TODOs (#notes.len())]
      for n in notes {
        let loc = n.location()
        [\ #link(loc)[p. #counter(page).at(loc).first()]: #n.value]
      }
    })
  }
}

// booktabs-style rules for tables.
#let toprule = table.hline(stroke: 0.08em)
#let midrule = table.hline(stroke: 0.05em)
#let bottomrule = table.hline(stroke: 0.08em)

#let report(title: none, body) = {
  set document(title: title)
  set page(
    paper: "a4",
    margin: 2.5cm,
    header: context {
      // No header on pages where a chapter starts; page number goes in the footer.
      let here-page = here().page()
      let starts = query(heading.where(level: 1)).filter(h => (
        h.location().page() == here-page
      ))
      if starts.len() > 0 { return }
      let before = query(heading.where(level: 1).before(here()))
      if before.len() == 0 { return }
      let ch = before.last()
      set text(size: 9pt)
      grid(
        columns: (1fr, auto),
        emph[Chapter #counter(heading).at(ch.location()).first(). #ch.body],
        counter(page).display(),
      )
      v(-0.6em)
      line(length: 100%, stroke: 0.4pt)
    },
    footer: context {
      let here-page = here().page()
      let starts = query(heading.where(level: 1)).filter(h => (
        h.location().page() == here-page
      ))
      if starts.len() > 0 {
        align(center, text(size: 9pt, counter(page).display()))
      }
    },
  )

  set text(font: "New Computer Modern", size: 11pt, lang: "en")
  set par(justify: true, leading: 0.7em, spacing: 1.5em)
  show raw: set text(size: 1.1em) // raw shrinks to 0.8em by default; bring it closer to body size

  // Headings: chapter (level 1), section (level 2), subsection (level 3).
  set heading(numbering: "1.1")
  show heading: set text(fill: accent)
  show heading.where(level: 1): set heading(supplement: [Chapter])
  show heading.where(level: 1): it => {
    pagebreak(weak: true)
    counter(figure.where(kind: image)).update(0)
    counter(figure.where(kind: table)).update(0)
    set text(weight: "bold")
    block(below: 28pt, {
      if it.numbering != none {
        text(size: 48pt, counter(heading).display("1"))
        v(6pt, weak: true)
      }
      text(size: 24.88pt, it.body)
      v(4pt, weak: true)
      line(length: 100%, stroke: 0.8pt + accent)
    })
  }
  show heading.where(level: 2): set text(size: 14.4pt)
  show heading.where(level: 2): set block(above: 1.6em, below: 0.9em)
  show heading.where(level: 3): set text(size: 12pt)
  show heading.where(level: 3): set block(above: 1.3em, below: 0.8em)

  // Figures and tables are numbered per chapter (2.1, 2.2, ...).
  set figure(numbering: n => {
    let ch = counter(heading).get().first()
    numbering("1.1", ch, n)
  })
  show figure.where(kind: table): set figure.caption(position: top)
  set figure.caption(separator: [. ])
  show figure.caption: it => pad(x: 1cm, text(size: 0.9em, {
    text(
      weight: "bold",
      fill: accent,
      it.supplement + [ ] + context it.counter.display(it.numbering),
    )
    it.separator
    it.body
  }))

  set table(stroke: none, align: left, inset: (x: 6pt, y: 4pt))
  show table: set par(justify: false)

  show link: set text(fill: link-colour)
  show ref: set text(fill: link-colour)
  show cite: set text(fill: link-colour)

  body
}

// Bibliography (shared/refs.bib) typeset as an unnumbered section of the
// current chapter. Only cited entries are listed.
#let references() = {
  heading(level: 2, numbering: none)[References]
  set text(size: 0.9em)
  set par(justify: false)
  bibliography("refs.bib", title: none, style: "ieee")
}
