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
	``` shell
	make
	```
4. Nyní je program připraven na spuštění.

## Spuštění serveru

1. Přepněte se do privilegovaného režimu.
	``` shell
	su
	```
2. Použijte jednu ze tří možností spuštění programu:
	``` shell
	./popser -h
	./popser -r
	./popser [-a PATH] [-d PATH] [-p PORT] [-c] [-r]
	```

### Popis přepínačů

`-a` cesta k souboru s přihlašovacími údaji<br />
`-d` cesta do složky s emaily (Maildir)<br />
`-p` číslo portu, na kterém poběží server<br />
`-h` volitelný parametr, program vypíše nápovědu a ukončí se<br />
`-r` volitelný parametr, <br />
`-c` volitelný parametr, při zadání server akceptuje autentizační metodu, která přenáší heslo v 		nešifrované podobě<br />

## Autor

* **Jan Kotas** - [kotasjn](https://github.com/kotasjn)

## Licence

Tento projekt je licencován licencí MIT. Tu si můžete prohlédnout zde: [LICENSE.md](LICENSE.md) file for details
