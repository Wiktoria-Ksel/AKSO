## Ciąg arytmetyczny

Mając dane wyrazy $A_0$ i $A_1$ pewnego ciągu arytmetycznego, chcemy policzyć
jego wyraz $A_k$, gdzie $k$ jest liczbą całkowitą (dopuszczamy indeksy ujemne).

## Polecenie

Zaimplementuj w asemblerze wołaną z języka C funkcję o następującej deklaracji:

`int128_t arithmetic_sequence(uint64_t const *A0, uint64_t const *A1, uint64_t *Ak, size_t n, int64_t k);`

gdzie:

```c
typedef struct {
  uint64_t lo;
  int64_t  hi;
} int128_t;
```

Parametry `A0` i `A1` są wskaźnikami na binarną reprezentację odpowiednio liczb
$A_0$ i $A_1$. Parametr `Ak` jest wskaźnikiem na miejsce w pamięci, gdzie należy
umieścić binarną reprezentację `64n` młodszych bitów liczby $A_k$. Wynikiem
funkcji jest struktura zawierająca `128` starszych bitów reprezentacji liczby
$A_k$. Liczby $A_0$, $A_1$ i $A_k$ są reprezentowane w kodzie uzupełnieniowym do
dwójki, w porządku cienkokońcówkowym. Parametr `n` zawiera liczbę słów typu
`uint64_t` wskazywanych przez `A0`, `A1` i `Ak`. O wartości `n` wolno założyć,
że jest dodatnia. Parametr `k` zawiera wartość indeksu $k$.

## Oddawanie rozwiązania

Jako rozwiązanie należy wstawić w Moodle plik o nazwie `arithmetic_sequence.asm`.

## Kompilowanie

Rozwiązanie będzie kompilowane poleceniem:

```bash
nasm -f elf64 -w+all -w+error -o arithmetic_sequence.o arithmetic_sequence.asm
```

Rozwiązanie musi się kompilować i działać w laboratorium komputerowym.

## Przykłady użycia

Przykłady użycia znajdują się w niżej załączonych plikach
`arithmetic_sequence_example.c` i `arithmetic_sequence_example.cpp`.
Kompiluje i łączy się je z rozwiązaniem poleceniami:

```bash
gcc -c -Wall -Wextra -std=c23 -O2 -o arithmetic_sequence_example_c.o arithmetic_sequence_example.c
gcc -z noexecstack -o arithmetic_sequence_example_c arithmetic_sequence_example_c.o arithmetic_sequence.o
```

```bash
g++ -c -Wall -Wextra -std=c++23 -O2 -o arithmetic_sequence_example_cpp.o arithmetic_sequence_example.cpp
g++ -z noexecstack -o arithmetic_sequence_example_cpp arithmetic_sequence_example_cpp.o arithmetic_sequence.o -lgmp
```

## Ocenianie

Ocena składa się z dwóch części.

1. Zgodność rozwiązania ze specyfikacją będzie oceniania za pomocą testów
   automatycznych, za które dostaje się maksymalnie 7 punktów. Oprócz poprawności
   wyniku sprawdzane będą przestrzeganie reguł ABI, poprawność odwołań do pamięci
   i zajętość pamięci, w szczególności rozmiar kodu i użycie stosu. Oceniana będzie
   szybkość działania funkcji. Za błędną nazwę pliku odejmiemy jeden punkt.
   Rozwiązanie niekompilujące się, nielinkujące się lub naruszające ochronę pamięci
   dostanie 0 punktów.

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

## Załączniki
