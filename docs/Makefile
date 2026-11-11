# Build every document:     make
# Live-rebuild one on save: make watch DOC=BackGround_Section/chapter2
#
# Hide #todo notes:          make -B FINAL=1
#
# --root . lets documents import from shared/ (the template and bibliography).

TYPST := typst
DOCS := $(basename $(shell find . -name '*.typ' -not -path './shared/*'))
SHARED := $(wildcard shared/*)
FLAGS := --root . $(if $(FINAL),--input final=true)

.PHONY: all watch clean

all: $(DOCS:=.pdf)

%.pdf: %.typ $(SHARED)
	$(TYPST) compile $(FLAGS) $< $@

watch:
	@test -n "$(DOC)" || { echo "usage: make watch DOC=path/to/file (without .typ)"; exit 1; }
	$(TYPST) watch $(FLAGS) $(DOC).typ $(DOC).pdf

clean:
	rm -f $(DOCS:=.pdf)
