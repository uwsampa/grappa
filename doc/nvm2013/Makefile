# main latex file and bib file should have same prefix
MAIN      = paper
#TEXFILES  = paper.tex header.tex abstract.tex intro.tex relatedwork.tex overview.tex tasks.tex memory.tex communication.tex method.tex evaluation.tex
TEXFILES  = paper.tex header.tex
BIBFILES  = paper.bib
FIGURES   = #figs/*pdf
TARGET    = $(MAIN).pdf
LATEX     = pdflatex
BIBTEX    = bibtex
FIG2PDF   = fig2dev -L pdf

default: $(TARGET)

$(TARGET): $(TEXFILES) $(FIGURES) $(BIBFILES) 
	$(LATEX) $(MAIN)
	if grep "bibdata" $(MAIN).aux; then $(BIBTEX) $(MAIN); fi
	$(LATEX) $(MAIN)
	$(LATEX) $(MAIN)

	while grep "Rerun" $(MAIN).log; \
	  do $(LATEX) $(MAIN); \
	done

view: $(TARGET)
	open $(TARGET) &
clean:
	@$(RM) *~ *.aux *.log *.bbl *.blg $(MAIN).pdf 
