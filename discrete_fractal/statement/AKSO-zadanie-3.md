## Dyskretne fraktale

Zaimplementuj w asemblerze program `discrete_fractal`, który generuje fraktale
składające się ze znaków ASCII i który uruchamiany jest poleceniem

`./discrete_fractal n`

Parametrem programu jest liczba iteracji z zakresu od 0 do $2^{32}-1$ zapisana
przy podstawie 10.

Symbolem nazywamy znak o kodzie ASCII od 33 do 126.

Program czyta dane ze standardowego wejścia. Pierwsza wczytana linia zawiera
składający się z symboli napis początkowy (być może pusty). Jeśli dane wejściowe
zawierają kolejne linie, to zawierają one reguły zastępowania. Reguła
zastępowania składa się z zastępowanego symbolu i z zastępującego go ciągu
symboli (być może pustego). Dla każdego symbolu występuje co najwyżej jedna
reguła zastępowania. Każda linia danych wejściowych kończy się znakiem nowej
linii.

Program wykonuje `n` iteracji modyfikowania napisu, zaczynając od napisu
początkowego. W każdej iteracji program zastępuje w napisie wszystkie symbole,
dla których podano regułę zastępowania.
Program wypisuje końcowy napis na standardowe wyjście.
Po wypisaniu końcowego napisu program wypisuje znak nowej linii.

## Przykład użycia

Program wywołany z parametrem 4 i danymi

```text
A
AAB
BA
```

powinien wypisać

```text
ABAABABA
```

## Wymagania formalne

Program nie powinien mieć żadnych sztucznych ograniczeń na długość
przetwarzanego napisu. Jedynym ograniczeniem jest rozmiar dostępnej pamięci.
Do czytanie ze standardowego wejścia, pisania na standardowe wyjście
i zarządzania pamięcią należy użyć funkcji systemowych Linuksa.

## Obsługa błędów i kod zakończenia programu

Program powinien sprawdzać liczbę przekazanych mu parametrów i poprawność
przekazanego mu parametru. Program powinien sprawdzać poprawność danych
wejściowych. Program powinien sprawdzać poprawność wykonania funkcji systemowych
(z wyjątkiem `sys_exit`).

Program sygnalizuje poprawne zakończenie kodem `0`.
Program sygnalizuje błąd, kończąc się kodem `1`.
W każdym przypadku program powinien jawnie zwolnić przydzieloną pamięć.

## Rozwiązanie

Jako rozwiązanie należy wstawić w Moodle plik o nazwie `discrete_fractal.asm`.
Rozwiązanie będzie kompilowane i linkowane poleceniami:

```bash
nasm -f elf64 -w+all -w+error -w-unknown-warning -w-reloc-rel -o discrete_fractal.o discrete_fractal.asm
ld -pie -I /lib64/ld-linux-x86-64.so.2 --fatal-warnings -o discrete_fractal discrete_fractal.o
```

Rozwiązanie musi się kompilować i działać na maszynie students i w laboratorium
komputerowym.

## Ocenianie

Ocena składa się z dwóch części.

1. Zgodność rozwiązania ze specyfikacją będzie oceniania za pomocą testów
   automatycznych, za które dostaje się maksymalnie 7 punktów. Przede wszystkim
   będzie oceniana poprawność wyniku. W tym zadaniu priorytetem jest szybkość
   działania programu, ale oceniane będą też rozmiary sekcji i wykorzystanie
   pamięci. Za błędną nazwę pliku źródłowego odejmiemy jeden punkt.

2. Za formatowanie i jakość kodu dostaje się maksymalnie 3 punkty. Tradycyjne
   formatowanie programów w asemblerze polega na rozpoczynaniu etykiet od pierwszej
   kolumny, a mnemoników rozkazów, ich parametrów i komentarzy do nich od wybranej
   ustalonej kolumny. Nie stosuje się innych wcięć. Taki format mają przykłady
   pokazywane na zajęciach. Kod powinien być dobrze skomentowany, co oznacza między
   innymi, że każdy blok kodu powinien być opatrzony informacją, co robi. Należy
   opisać przeznaczenie rejestrów. Komentarza wymagają wszystkie kluczowe lub
   nietrywialne linie kodu. W przypadku asemblera nie jest przesadą komentowanie
   prawie każdej linii kodu, ale należy unikać komentarzy opisujących to, co widać.

Zastrzegamy sobie uzależnienie wystawienia oceny od osobistego wyjaśnienia
prowadzącemu zajęcia szczegółów działania kodu.

https://en.wikipedia.org/wiki/L-system
