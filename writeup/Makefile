#No-so-good Makefile for latex on Linux
FILE=cs598mcc_paper

all:
	pdflatex ${FILE}
	bibtex ${FILE}
	pdflatex ${FILE}
	pdflatex ${FILE}
clean:
	rm ${FILE}.blg
	rm ${FILE}.bbl
	rm ${FILE}.aux
	rm *.log
	rm *.out
