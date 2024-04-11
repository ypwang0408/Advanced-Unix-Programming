all:
	g++ -o logger logger.cpp
	gcc -shared -fPIC -o logger.so logger_so.c -ldl
clean:
	rm -f logger logger.so