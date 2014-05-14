PGFPLOTSDIR="/usr/home/user/projectscratch/pgfplots/pgfplots//:"

all: clean parse octave pdf

octave:
	octave plotall.octave

pdf:
	TEXINPUTS=$(PGFPLOTSDIR) pdflatex -shell-escape file.tex && \
	TEXINPUTS=$(PGFPLOTSDIR) pdflatex -shell-escape file.tex && \
	TEXINPUTS=$(PGFPLOTSDIR) pdflatex -shell-escape file.tex

clean:
	rm -f file.aux file.log file.pdf plot_data* octout* plotall.png plotall.ps octave.core file-figure* file.auxlock

parse:
	./parse_run_output.sh
