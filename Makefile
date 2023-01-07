
server: main.cpp locker.o http_conn.o
	g++ main.cpp http_conn.o locker.o -o server -pthread


http_conn: http_conn.cpp locker.o
	g++ -c http_conn.cpp locker.o -o http_conn.o -pthread

locker:locker.cpp
	g++ -c locker.cpp -o locker.o -pthread
	
	
clean:
	rm -rf locker.o http_conn.o server