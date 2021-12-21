all: c r

c:
	g++ proxy.c -o proxy

r:
	./proxy 10 test.txt

test:
	./proxy 10 /bin/bash > check
	diff /bin/bash check
	./proxy 10 input.txt > check.txt
	diff input.txt check.txt
