include Makefile
all: test5

test3: test2 force
	@fmt -w 20 -c readme | head -10 > readme.tmp && mv readme.tmp readme
force: 
