Integrctrl: integ.c libintegrctrl.c
	gcc -w -o Integrctrl integ.c libintegrctrl.c -lcrypto

integ.o: integ.c
	gcc -c integ.c

Integrctrl.o: libintegrctrl.c
	gcc -c libintegrctrl.c

.PHONY: clean
clean:
	rm Integrctrl *.o