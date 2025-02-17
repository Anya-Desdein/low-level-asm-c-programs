include makefile
all: test5

override test3: test2 force
	@nroff -e readme | awk "NF" > readme.tmp && mv readme.tmp readme

override test4: test3 force
	@echo -e "Which file will be used to make in order:\nGNUmakefile > makefile > Makefile\n"
force: 
