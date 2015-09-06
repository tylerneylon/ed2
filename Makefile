# Makefile for ed2
#

# Intermediate target lists.
obj = $(addprefix out/,array.o list.o map.o memprofile.o global.o subst.o)

# Variables for build settings.
includes = -I.
ifeq ($(shell uname -s), Darwin)
	cflags = $(includes) -std=c99
else
	cflags = $(includes) -std=c99 -D _BSD_SOURCE -D _GNU_SOURCE
endif
cc = cc $(cflags)

# Primary targets.
all: $(obj) ed2

ed2: ed2.c $(obj)
	$(cc) ed2.c -o ed2 -lreadline $(obj)

clean:
	rm -rf out

# Internal rules; meant to only be used indirectly by the above rules.

out:
	mkdir -p out

out/global.o : global.c global.h | out
	$(cc) -o $@ -c $<

out/subst.o : subst.c subst.h | out
	$(cc) -o $@ -c $<

out/%.o : cstructs/%.c cstructs/%.h | out
	$(cc) -o $@ -c $<

# Listing this special-name rule prevents the deletion of intermediate files.
.SECONDARY:

