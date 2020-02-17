CC=gcc

CFLAGS= -std=gnu99 -Wall -pedantic -g

ALL: APP APP2 APP3 APP4 APP5 APPSEND APPRECV CLIP clean

CLIP: clipboard_main.o
	$(CC) $(CFLAGS) -pthread -o clip clipboard_main.o

APP: app.o
	$(CC) $(CFLAGS) -o app app_teste.o

APP2: app2.o
	$(CC) $(CFLAGS) -o app2 app_teste2.o

APP3: app3.o
	$(CC) $(CFLAGS) -o app3 app_teste3.o

APP4: app4.o
	$(CC) $(CFLAGS) -o app4 app_teste4.o

APP5: app5.o
	$(CC) $(CFLAGS) -o app5 app_teste5.o

APPSEND: appsend.o
	$(CC) $(CFLAGS) -o appsend app_teste_send.o

APPRECV: apprecv.o
	$(CC) $(CFLAGS) -o apprecv app_teste_recv.o

app.o:	app_teste.c library.c
	$(CC) -c $(CFLAGS) app_teste.c

app2.o:	app_teste2.c library.c
	$(CC) -c $(CFLAGS) app_teste2.c

app3.o:	app_teste3.c library.c
	$(CC) -c $(CFLAGS) app_teste3.c

app4.o:	app_teste4.c library.c
	$(CC) -c $(CFLAGS) app_teste4.c

app5.o:	app_teste5.c library.c
	$(CC) -c $(CFLAGS) app_teste5.c

appsend.o:	app_teste_send.c library.c
	$(CC) -c $(CFLAGS) app_teste_send.c

apprecv.o:	app_teste_recv.c library.c
	$(CC) -c $(CFLAGS) app_teste_recv.c

clipboard.o: clipboard_main.c
	$(CC) -c $(CFLAGS) -pthread clipboard_main.c

clean::
	rm -f *.o core a.out APP*~
