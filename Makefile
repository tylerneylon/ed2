# Makefile for ed2
#

all: ed2

ed2: ed2.c
	cc ed2.c -o ed2 -lreadline
