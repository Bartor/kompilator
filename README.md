# Kompilator

Zadanie zaliczeniowe laboratorium kursu Języki Formalne i Techniki Translacji prowadzonego w semestrze zimowym 2019/2020 przez dr. Maćka Gębalę.

W celu uruchomienia:

```bash
make
```
a następinie
```bash
./compiler [plik wejściowy] [plik wyjściowy - bez podania mamy "a.out"]
```

**DISCLAIMER**: program zaprezentowany w tym repozytorium jest moim *pierwszym* większym zderzeniem z C++ i prezentuje absolutnie *żałosny* jego poziom, z wyciekami pamięci, 
brzydkimi patternami i nieprofesjonalnym podejściem do strukturyzowania programu na każdym kroku. Wynikało to głównie z faktu, że głównym założeniem kompilatora było działać i 
robić to skutecznie, a nie pięknie, a C++ był wyborem pragmatycznym jako najlepiej współpracujący z dostępnymi narzędziami.

> Kiedyś przepiszę to na Jflexa i eksperymentalnego javowego Bisona z całą resztą w Kotlinie

~ ja, ciągle

## Wstęp

Jest to kompilator prostego języka imperatywnego napisany w C++ przy użyciu leksera [flex](https://github.com/westes/flex) oraz generatora paserów [bison](https://www.gnu.org/software/bison/). Pełna specyfikacja języka znajduje się w [pliku](./specs.pdf), ale można go podsumować przez wspierane funkcjonalności:
- zmienne liczbowe oraz tablicowe liczb deklarowane w początkowej sekcji seklaracji,
- sekcje warunkowe, pętle `for` oraz `while`,
- przypisywanie wyników obliczeń (mnożenia, dzielenia, dodawania, odejmowania, modulo) do zmiennych,
- przyjmowanie wejścia z klawiatury i wyświetlanie wyjścia na ekranie

Zarówno język, jak i docelowy assembler zostały zaprojektowane przez doktora Gębalę prowadzącego przedmiot; stąd docelowy assembler także objęty jest ograniczeniami:
- nieskończona taśma pamięci operacyjnej niezależnej od pamięci kodu programu
- brak rejestrów
- wykonywanie większości operacji przy użyciu tylko pierwszej komórki pamięci
- brak instrukcji mnożących, dzielących, modulo

Kod assemblera wykonywany jest na załączonej przez doktora [maszynie wirtualnej](./vm).

## Struktura programu

Kompilator podzielony jest na trzy zasadnicze części:
- [front](./front) - odpowiedzialną za interakcję z kodem w programie wejściowym
- [middle](./middle) - odpowiedzialną za interakcję z AST wygenerowanym przez kod z poprzedniej części
- [back](./back) - odpowiedzialną za interakcję z kodem opartym na klasach opisujących instrukcje ASM

Plikiem łączącym wszystkie części w działającą całość jest [main.cpp](./main.cpp) z entrypointem programu.

### Front

Najważniejszymi plikami tej części są [compiler.l](./front/compiler.l) i [compiler.y](./front/compiler.y), czyli kolejno plik wejściowy leksera oraz generatora parserów. 
Przy kompilacji najpierw używany jest bison w celu wygenerowania pliku nagłówkowego oraz implementacyjnego z generatorem parserów o zdefiniowanej przez nas gramatyce, a 
następnie uruchamiany jest flex na swoim pliku z dołączonym tymże nagłówkiem, który dzieli wejście zdefiniowane jeszcze w mainie na tokeny.

Bison nastepnie dopasowuje tokeny do odpowiadających im produkcji, które zwracają obiekty tworzące AST (abstrakcyjne drzewo składni) zdefiniowane w [ast](./front/ast).
Każda z części (nazywanych `node`'ami) przechowuje jedynie informacje; nie trzyma żadnych referencji na tablicę symboli (nie istniejącą jeszcze na tym etapie), jedynie
stringowe nazwy referowanych zmiennych. Takie podejście pozwala na pracę na utworzonym drzewie bez obaw o niszczenie wskaźników oraz referencji w procesie, gdzyż kopiowanie
staje się bardzo łatwe (więcej o tym w części poświęcowej `ASTOptimizer`).

Dodatkowo w procesie zbierane są stałe liczbowe do tablicy `constants`; ich wygenerowanie na początku programu przed użyciem jest jedną z prostszych optymalizacji, jakie
możemy wykonać.

Tak przygotowany kod jest następnie przekazywany jako obiekt klasy `Program` do sekcji `middle`.

### Middle

Middle jest sekcją złożoną z kilku podsekcji, w tym dwóch optymalizacyjnych.

#### 1. ASTOptimizer

Jest to optymalizator pracujący na drzewie abstrakcyjnym; nie jest świadomy niepoprawności programu, nie wie, co oznaczają przetwarzane symbole w ogólności, a jedynie
zna konkretne przypadki, które może transformować w inne i właśnie na tej transformacji oparta jest cała idea. Optymalizator ma dwie funkcje umożliwiające mu przemieszczanie
się po wszystkich `node`'ach programu i wywoływanie na nich funkcji zwanych `Callback`ami, które dostarczamy jako parametr każdego wywołania `traverse`. Funkcje te zwracają
oryginalny `node` lub jego zmodyfikowaną wersję (przez zastąpienie jej czymś skonstruowanym samemu lub użycia specjalnych metod `copy(Callback replacer)` zdefiniowanych w każdym
z `node`'ów z [ast.h](./front/ast.h)). W ten sposób optymalizowane są kolejne części drzewa, umożliwiając dowolne wymienianie jego odpowiednich kawałków. Przykładami takich optymalizacji
są chociażby rozwijanie pętli czy podmiana wyrażeń stałych.

#### 2. AbstractAssembler

Ogromna, monolityczna i omnipotentna klas potrafiąca zamieniać AST na ASM. W wielkim skrócie rozróżnia ona różne `node`'y AST i wie, jakie instrukcje assemblera dla nich wygenerować.
Nie ma sensu zagłębiać się w szczegóły implementacyjne rozwiązywania adresów zmiennych, wartościowania wyrażeń czy dostępów tablicowych, ale warto wspomnieć o kilku istotnych częściach:
- [ScopedVariables](./middle/abstract_assembler/ScopedVariables.h) - klasa tworząca stos zmiennych w czasie kompilacji, jej ważnym elementem jest klasa wspomagająca [ResolableAddress](./middle/abstract_assembler/ResolvableAddress.h),
która umożliwia oddelegowanie adresów w pamięci operacyjnej do referencji na instancje tej klasy zamiast używania konkretnych wartości liczbowych; jest to abstrakcja umożliwiająca realnie
manipulowanie pamięcią z wysoką abstrakcją bez potrzeby martwienia się o jej układ. Klasa `ScopedVariables` sama dba o nienachodzenie na siebie w pamięci tworzonych zmiennych oraz
zwalnianiu tego miejsca po zakończeniu ich życia.
- [Constants](./middle/abstract_assembler/Constants.h) - klasa umożliwiająca zapytania o adresy wygenerowanych i zapisanych w pamięciu na początku programu stałych liczbowych używanych
nastepnie jedynie przez czytanie z ich adresów, a także dodawanie nowych stałych w przypadku odnalezienia możliwości optymalizacyjnych w trakcie kompilacji (np. odnajdując wyrażenie 
`a ASSIGN x TIMES 512;` nie ma sensu wykonywać standardowego algorytmu mnożenia, a lepiej dodać do stałych liczbowych `9` i wykonać `LOAD x; SHIFT [adres_9]; STORE a`, to samo dotyczy 
kilku innych przypadków). Ważnym aspektem jest klasa [Constant](./middle/abstract_assembler/Constant.h), która wie, w jaki sposób wygenerować efektywnie stałą liczbową korzystając z
dostępnych instrukcji assemblera (w trakcie pisania nie jest to nadal najwydajniejszy algortym, bo nie bierze pod uwagę już wygenerowanych stałych)

Kod wygenerowany przez `AbstractAssembler` to obiekt klasy `InstructionList` (należącej do części już assmeblerowej, końcowej), który następnie jest przekazywany do fazy trzeciej.

#### 3. PeepholeOptimizer

Klasa ta skupia się na prostych optymalizacjach, które wykonywane są już na samych instrukcjach assemblera. Większość z wykonywanych operacji jest dosyć bezpieczna i prosta programistycznie
ze względu na *wygodną* postać obiektową instrukcji (więcej w kolejnej sekcji). Wykonywane tu optymalizacje obejmują np. usuwanie `LOAD x` z pary `STORE x; LOAD x`, jeżeli żaden skok
w programie nań nie wskazuje.

### Back

Ta sekcja obejmuje głównie definicje instrukcji assemblerowych jako klasy, co umożliwia im dosyć dużą elastyczność względem traktowania ich jako chociażby po prostu stringi. Wiele ze
zdefiniowanych tu instrukcji przyjmuje jakiś adres (chociażby `LOAD 10`, 10 jest w tym wypadku adresem w pamięci), więc przyjmują one referencję na `ResolvableAddress` zamiast liczbę,
umożliwiając poprzestawianie chociażby stałych w pamięci bez potrzeby podmiany adresów w instrukcjach. Analogicznie, instrukcje skoków przyjmują wskaźnik na funkcję, do której mają
skoczyć, zamiast liczbę oznaczająca numer linii. Umożliwia to dowolne dodawanie i usuwanie instrukcji pomiędzy nimi bez potrzeby aktualizowania jakichkolwiek pól w skokach.

Z uwzględnieniem powyższych, proces generowania kodu z adresami liczbowymi wygląda nastepująco:
- instrukcje tworzone są z referencjami na obiekty
- instrukcje układane są w poprawnej kolejności na liście instrukcji
- odpowiedni kod nadaje każdej z instrukcji kolejne numery linii programu
- na każdej instrukcji wołana jest metoda `toAssemblyCode`, która pobiera adres z klasy `ResolvableAddres` w przypadku referencji na pamięć lub nadany krok wcześniej numer linii instrukcji
będącej celem skoku w przypadku skoków

W ten sposób można nadać instrukcjom wszelkie potrzebne wartości wielokrotnie; po zmodyfikowaniu oryginalnej listy (na podstawie której generowany jest assembler) starczy wykonać
dwa ostatnie kroki z powyższej listy ponownie, aby wszystkie adresy znowu były poprawne.

Istnieją tu także specjalne pseudo-instrukcje `Stub` nie kompilujące się i posiadające zawsze numer linii instrukcji następującej po nich. Są one dodawane zawsze na sam koniec bloku
instrukcji (tj. instancji klasy [InstructionList](./back/asm/InstructionList.h)), aby skoki do końca jakiegoś bloku wykonywały się zawsze właśnie tam, nawet jeżeli ostatnia prawdziwa
instrukcja w tym bloku zostanie usunięta lub przeniesiona w jakimś innym procesie optymalizacji. Instrukcje `Stub` istnieją na liście aż do samego końca, są nadawane im poprawne adresy
(tj. adresy instrukcji następujących po nich), wskazuje na nie wiele skoków, a dopiero podczas samego zapisywania/wypisywania gotowego kodu są pomijane.