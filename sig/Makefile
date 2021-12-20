all: c r

c:
	gcc -Wall sig.c -o sig

r:
	./sig test1.txt

test: c
	./sig test.txt > test2.txt
	diff test2.txt test.txt
