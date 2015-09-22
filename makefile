CC=gcc
ALL = client serv1 serv2

build: client serv1 serv2

client: client.o
	$(CC) -o client client.o

serv1: serv1.o
	$(CC) -o serv1 serv1.o 

serv2: serv2.o
	$(CC) -o serv2 serv2.o 


clean:
	$(RM) *.o $(ALL)