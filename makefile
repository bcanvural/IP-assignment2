CC=gcc
ALL = client serv1

build: client serv1

client: client.o
	$(CC) -o client client.o

serv1: serv1.o
	$(CC) -o serv1 serv1.o 


clean:
	$(RM) *.o $(ALL)