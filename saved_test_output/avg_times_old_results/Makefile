PGFPLOTSDIR="/usr/home/user/projectscratch/pgfplots/pgfplots//:"

all: clean parse octave pdf

octave:
	octave plotall.octave

pdf:
	TEXINPUTS=$(PGFPLOTSDIR) pdflatex file.tex && \
	TEXINPUTS=$(PGFPLOTSDIR) pdflatex file.tex && \
	TEXINPUTS=$(PGFPLOTSDIR) pdflatex file.tex

clean:
	rm -f file.aux file.log file.pdf plot_data* octout* plotall.png plotall.ps octave.core

parse:
	./parse_run_output.sh
