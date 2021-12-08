all:    c e
c:
	g++ -Wall msg.cpp -o msg

e:
	./msg ${ARG}

num:
	g++ -Wall nums.cpp -o nums

differ:
	./msg ${ARG} > test1.txt
	./nums ${ARG} > test2.txt
	diff test1.txt test2.txt

test:	c num differ