# This Makefile is to be overwritten by makefile

ifeq ("$(V)", "1")
	Q:=
else
	Q:=@
endif

.PHONY: test1 test2 test3 test4 test5

all: test5

test1:
	$(Q)echo -e "---------Memory:---------" > readme
	$(Q)free -h >> readme
	$(Q)echo -e "----------Disk:----------" >> readme
	$(Q)df -h >> readme

test2: test1 
	$(Q)sed -e 's/1/666/g' -e 's/M/W/g' -e 's/run/makefile magic/g' -e 's/B/p/I2' -e 's/0/-1/g' readme > readme.tmp && mv readme.tmp readme

test3: test2
	$(Q)echo "\n\n--Replace Me--\n\n" >> readme

test4: test3
	$(Q)rev readme > readme.tmp && mv readme.tmp readme
	$(Q)tac readme > readme.tmp && mv readme.tmp readme

test5: test4
	$(Q)cat readme | lolcat	


clean:
	$(Q)rm -f readme
