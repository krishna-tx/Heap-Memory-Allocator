run: interactive.c myalloc.c myalloc.h 
	gcc interactive.c  myalloc.c -o interactive && ./interactive

clean:
	rm -rf interactive

