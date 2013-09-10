# main latex file and bib file should have same prefix
MAIN      = paper
#TEXFILES  = paper.tex header.tex abstract.tex intro.tex relatedwork.tex overview.tex tasks.tex memory.tex communication.tex method.tex evaluation.tex
TEXFILES  = $(wildcard *.tex)
BIBFILES  = paper.bib generals.bib
FIGURES   = figs/*pdf
TARGET    = $(MAIN).pdf
LATEX     = pdflatex
BIBTEX    = bibtex
FIG2PDF   = fig2dev -L pdf
INCLUDEPATH = ".//:asplos14-template//:$$TEXINPUTS"

default: $(TARGET)


$(TARGET): $(TEXFILES) $(FIGURES) $(BIBFILES) 
	(export TEXINPUTS=$(INCLUDEPATH); $(LATEX) $(MAIN))
	if grep "bibdata" $(MAIN).aux; then $(BIBTEX) $(MAIN); fi
	(export TEXINPUTS=$(INCLUDEPATH); $(LATEX) $(MAIN))
	(export TEXINPUTS=$(INCLUDEPATH); $(LATEX) $(MAIN))

	while grep "^[^P].*Rerun" $(MAIN).log; \
	  do (export TEXINPUTS=$(INCLUDEPATH); $(LATEX) $(MAIN)) \
	done

view: $(TARGET)
	open $(TARGET) &
clean:
	@$(RM) *~ *.aux *.log *.bbl *.blg $(MAIN).pdf 
