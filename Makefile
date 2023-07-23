
exec: ./src/main.c ./src/dns.c
	gcc -g -Wall -Werror ./src/main.c ./src/dns.c -o exec

clean:
	rm exec