all : G25manage

G25manage: G25manage.c
	gcc -Wall -lusb -g3 -o G25manage G25manage.c

G25manage-static: G25manage.c
	gcc -Wall -o G25manage G25manage.c /usr/lib/libusb.a

clean:
	rm -rf G25manage
