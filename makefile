CC=gcc
ALL = client serv1 serv2 serv3 talk

build: client serv1 serv2 serv3 talk

client: client.o
	$(CC) -o client client.o

serv1: serv1.o
	$(CC) -o serv1 serv1.o

serv2: serv2.o
	$(CC) -o serv2 serv2.o

serv3: serv3.o
	$(CC) -o serv3 serv3.o

talk: talk.o
	$(CC) -o talk talk.o

clean:
	$(RM) *.o $(ALL)
