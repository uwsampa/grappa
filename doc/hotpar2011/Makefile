latex=pdflatex --halt-on-error
bibtex=bibtex

softxmt-hotpar2011.pdf:

%.pdf: %.tex %.bib Makefile evaluation.tex sigplanconf.cls
	$(latex) $<
	$(bibtex) $(subst .tex,, $<)
	$(latex) $<
	$(latex) $<
