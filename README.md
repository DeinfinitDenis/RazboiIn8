# Război în 8

Joc C++17 cu OpenGL și GLFW pe Windows și Linux, implementat în `main.cpp`, după regulile de bază din capitolul 10 al cărții **Olimpiada Jocurilor Raționale**, Nicolae Oprișiu.

## Compilare

Necesită CMake 3.20+, Ninja și un compilator C++17.

Compilările sunt separate: `build/windows/` pentru Windows și `build/linux/` pentru Linux. Rulează fiecare preset pe sistemul corespunzător; alegerea presetului nu instalează un compilator pentru alt sistem.

### Windows (MSYS2 UCRT64)

Instalează [MSYS2](https://www.msys2.org/) și deschide **MSYS2 UCRT64**. Actualizează sistemul:

```sh
pacman -Syu
```

Dacă actualizarea cere închiderea terminalului, redeschide UCRT64 și repetă comanda. Instalează compilatorul, CMake și bibliotecile:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-glfw
```

În terminalul UCRT64, intră în folderul proiectului și folosește un folder nou de compilare:

```sh
cd /c/Users/YourName/Projects/RazboiIn8
cmake --preset windows
cmake --build --preset windows
./build/windows/RazboiIn8.exe
ctest --preset windows
```

Rulează jocul din terminalul UCRT64 pentru acces la DLL-urile GLFW și dependențele lor. Pentru pornire din Explorer, adaugă `C:\msys64\ucrt64\bin` la `PATH` (adaptează calea instalării). Executabilul singur nu include aceste DLL-uri.

Ambele platforme folosesc GLFW pentru ferestre și intrare, OpenGL pentru desen și un font bitmap inclus pentru text românesc și englez. Nu este necesară instalarea unui font sau a unei biblioteci pentru text. Fontul inclus și licența sa se află în `font_data.h` și `FONT_LICENSE.txt`.

Configurarea CMake pentru MSYS2 urmează [documentația MSYS2](https://www.msys2.org/docs/cmake/).

### Linux (Debian/Ubuntu)

Instalează dependențele:

```sh
sudo apt install cmake ninja-build build-essential pkg-config libglfw3-dev libgl1-mesa-dev
```

Compilează, pornește jocul și rulează testele:

```sh
cmake --preset linux
cmake --build --preset linux
./build/linux/RazboiIn8
ctest --preset linux
```

## Control

- Click pe piesa proprie, apoi pe o destinație marcată verde.
La pornire apare meniul principal: **Jucător vs Jucător**, **Jucător vs Calculator**, **Setări** și **Ieșire**. Calculatorul joacă negru. Din **Setări** poți alege **Română** sau **English**; schimbarea se aplică imediat întregii interfețe. Limba este păstrată pe durata sesiunii.

În timpul partidei, butoanele **Meniu principal** și **Setări** permit navigarea fără pierderea progresului. **Continuă partida** reia jocul din meniu. Alegerea unui mod începe o partidă nouă.

- **C**: revine la meniul principal.
- **U**: anulează o mutare; împotriva calculatorului revine la tura omului.
- **R**: partidă nouă în modul curent.
- **H**: afișează sau ascunde regulile și pune calculatorul în pauză.
- **Esc**: închide regulile, revine din setări sau din partidă; în meniul principal închide jocul.

## Regulile folosite

Fiecare jucător începe cu opt piese pe pătratele de aceeași culoare din primele două rânduri. Albul începe. Se mută o singură piesă cu un pas pe o diagonală liberă, în orice sens, fără sărituri. O piesă este blocată dacă nu are niciun pătrat diagonal vecin liber, indiferent de culoarea pieselor care o înconjoară.

Implementarea adoptă recomandarea din carte de a permite patru mutări de dezvoltare fiecărui jucător. Eliminările devin active imediat după a opta mutare individuală totală. Toate piesele blocate sunt eliminate automat și simultan după fiecare mutare, inclusiv cele proprii. Aceasta automatizează observarea și ridicarea prizonierilor descrise în carte.

Partida se încheie după 100 de mutări pentru fiecare jucător, când o armată dispare sau când jucătorul la mutare nu are mutări legale. Scorul este numărul de piese rămase; câștigă scorul mai mare, iar egalitatea înseamnă remiză. Convențiile opționale din carte privind penalizarea zidurilor și ridicarea manuală a prizonierilor nu sunt implementate.

Calculatorul analizează trei mutări individuale folosind minimax cu tăiere alfa-beta, favorizând avantajul material și mobilitatea.

Sursa: [cartea, capitolul 10, paginile 246–249](https://ro.scribd.com/document/464940512/Nicolae-Oprisiu-Olimpiada-Jocurilor-Rationale-pdf).
