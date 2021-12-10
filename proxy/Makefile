all: c r

pr:
	g++ -Wall proxy.cpp -ggdb3 -c

lib:
	g++ -Wall MyLib.cpp -ggdb3 -c

b:
	g++ proxy.o MyLib.o -o proxy

test: pr lib b
	./proxy 5 test1.txt > test2.txt
	diff test1.txt test2.txt

t2: pr lib b
	./proxy 1 test.txt

r:
	./proxy ${NUM} ${FILE}
