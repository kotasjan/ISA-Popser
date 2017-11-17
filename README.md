# Popser

Popser je studentský projekt do předmětu ISA pro rok 2017. Cílem tohoto projektu bylo vytvořit funkční POP3 server, který bude vycházet z oficiálního protokolu RFC 1939 + doplněný o požadavky od zadavatele projektu.

## Úvodem

Následující instrukce vám poslouží k instalaci a spuštění projektu na lokálním PC.

### Požadavky

1. Libovolná linuxová distribuce s podporou překladu pomocí gcc.
2. Aplikaci je nutno spouštět jako privilegovaný uživatel (root).

### Instalace

1. Naklonujte si repozitář na své lokální úložiště.
2. Pomocí terminálu vstupte do složky s projektem.
3. Spusťte překlad projektu do binární podoby pomocí "make".
	make
4. Nyní je program připraven na spuštění.

## Spuštění serveru

1. Přepněte se do privilegovaného režimu.
	su
2. Použijte jednu ze tří možností spuštění programu:
	./popser -h
	./popser -r
	./popser [-a PATH] [-d PATH] [-p PORT] [-c] [-r]

### Popis přepínačů

-a	cesta k souboru s přihlašovacími údaji
-d	cesta do složky s emaily (Maildir)
-p	číslo portu, na kterém poběží server
-h	volitelný parametr, program vypíše nápovědu a ukončí se
-r	volitelný parametr, 
-c	volitelný parametr, při zadání server akceptuje autentizační metodu, která přenáší heslo v 		nešifrované podobě

## Autor

* **Jan Kotas** - *Initial work* - [kotasjn](https://github.com/kotasjn)

## Licence

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* Hat tip to anyone who's code was used
* Inspiration
* etc
