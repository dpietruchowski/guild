# Guild

Framework w C++ do budowania zespołów agentów opartych o binarkę `claude`.
Zamysł i kierunek: [doc/idea.md](doc/idea.md).

## Zasady

- **Nie dodawaj `Co-Authored-By` ani żadnej innej atrybucji do commitów i PR-ów.**
- **Nie pushuj**, dopóki nie poproszę.
- **Nie ruszaj mojego środowiska.** Testowe uruchomienia nie mogą pisać do
  `~/.claude/` ani do prawdziwych sesji — przekieruj `HOME`/`XDG_*` do `tmp/`.

## Komentarze

Zero komentarzy w plikach `.cpp`, `.h` i `.qml`.

- Zakaz obejmuje każdą formę: `//`, `/* */`, bloki nagłówkowe nad plikiem, klasą,
  funkcją i propercją, `TODO`/`FIXME`, separatory sekcji, notatki o obejściach
  bugów i o wersjach bibliotek.
- Nie ma wyjątku „to jest *dlaczego*, a nie *co*". Nie ma wyjątku dla opisu API
  ani dla wiedzy, której nie widać z kodu.
- Powód nieoczywistej decyzji zapisz w commit message albo w `doc/` — nigdy w
  pliku źródłowym.
- Komentarze zastane w repo nie są wzorem stylu. Nie naśladuj ich gęstości w
  nowym kodzie.
- Z fragmentów, które i tak zmieniasz, komentarze usuwaj.

## Commity

- Conventional Commits, po angielsku, tryb rozkazujący, subject małą literą.
- **Stage'uj pliki jawnie, po nazwie.** Nigdy `git add -A` ani `git add .`.
- Jeden commit = jedna zmiana. Nie doklejaj przy okazji niezwiązanych poprawek.

## Scratch

Wszystko jednorazowe idzie do `tmp/` w katalogu repo: skrypty pomocnicze, logi,
wyjście z gdb, katalogi testowych uruchomień. Jest w `.gitignore`.

Nie używaj `/tmp` ani scratchpada sesji — `tmp/` trzyma robocze pliki obok kodu
i przeżywa między sesjami.

## Język

Rozmowa po polsku. Kod, nazwy, commity, `doc/` i README po angielsku.

## Format

Przed commitem uruchom target `format` (clang-format).
