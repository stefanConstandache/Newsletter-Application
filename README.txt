
OBS: Am pornit implementarea temei de la scheletul din laboratorul 8.


~~~~~Subscriber

	- Se primeste o comanda de la stdin pe care o parcurg folosind strtok. Despart fiecare
		paramentru si il pun in structura care urmeaza sa fie trimisa la server ca
		mesaj.

	- Daca comanda primita de la stdin nu corespunde cu comenzile acceptate atunci se
		afiseaza un mesaj corespunzator, dupa care se inchide programul.

	- Daca mesajul a fost trimis cu succes afisez mesajul corespunzator.

OBS: Cu toate ca am pastrez in structura SF-ul din comanda acesta nu este folosit la
	implementare, deoarece nu am implementat cazul pentru SF = 1.

~~~~~Server

	- Pentru implementare am folosit 2 vectori: 
		- un vector pentru retinerea informatiilor clientilor dupa file descriptor

		- un vector pentru retinerea topicurilor impreuna cu file descriptorii
			clientilor care sunt abonati la topicul respectiv

	- Fac "bind-urile" pentru socketii clientului tcp si clientului udp.

	- In ciclul "while" parcurg toti file descriptorii pana la valoare file descriptorului
		maxim.

	- Daca s-a primit o comanda pe server si aceasta e comanda "exit", atunci se iese din
		ciclul "while" si se inchid toti sochetii care au fost folositi, altfel nu se
		intampla nimic.

	- Daca se cere o noua conexiune tcp atunci adaug noul socket la multimea dde file
		dedscriptori, retin id-ul clientului in vectorul de clienti dupa care afisez
		un mesaj care arata ca s-a conectat un client.

	- Daca un subscriber a trimis o comanda de abonare la un topic:
		- Daca exista topicul atunci se adauga file dedscriptorul clientului la
			vectorul cu topicuri.

		- Daca nu exista atunci se creeaza si se adauga topicul respectiv impreuna cu
			file descriptorul clientului.

		- Se adauga topicul si la clientul respectiv din vectorul de clienti.
		
	- Daca un subscriber a trimis o comanda de dezabonare de la un topic:
		- Verific daca clientul este abonat la topic si in cazul in care este atunci
			il sterg din vectorul de clienti si ii sterg file descriptorul din
			vectorul de topicuri asociate file descriptorilor.

	- Daca un client se deconecteaza atunci afisez un mesaj prin care arat ca s-a
		deconectat dupa care updatez file descriptorul maxim.

	- Daca s-a primit un mesaj de la un client udp atunci:
		- transform mesajul primit de la clientul udp pentru a putea fi citit de
			clientul tcp.

		- pun ip-ul si portul clientului udp in mesajul pe care vreau sa il transmit
			clientilor tcp abonati sa topicul respectiv.
		
		- Transmit mesajul clientilor tcp(subscriberilor) care sunt abonati la
			 topicul respectiv.
