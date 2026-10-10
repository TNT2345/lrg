lrg: src/main.c src/reader.c src/args.c src/match.c src/reader.h src/args.h src/match.h
	gcc -std=gnu11 -Wall -g src/main.c src/reader.c src/args.c src/match.c -o lrg
clean:
	rm -f lrg