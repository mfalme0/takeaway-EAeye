# Ayoayo (Kalah / Mancala)

A text-based implementation of the Ayoayo board game, written in C++ for the
assignment described in the brief. Two players, six pits and one store each,
48 seeds in total. No GUI: all input and output is plain text.

## Files

| File | Purpose |
| --- | --- |
| `Ayoayo.h` | `Player` and `Ayoayo` class declarations. All data members are private. |
| `Ayoayo.cpp` | Implementation: sowing, the two special rules, the ending rule, winner reporting. |
| `main.cpp` | Verification harness. Replays both worked examples from the brief, checks the error paths, and plays 500 randomised games. |
| `reflection.txt` | Initial and final reflection (required deliverable). |

## Build and run

Requires a C++17 compiler. On Windows with Visual Studio:

```bat
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /nologo /EHsc /W4 /std:c++17 Ayoayo.cpp main.cpp /Fe:ayoayo.exe
ayoayo.exe
```

Or on a machine with `g++` or `clang++`:

```sh
g++ -std=c++17 -Wall -Wextra Ayoayo.cpp main.cpp -o ayoayo
./ayoayo
```

The program exits with status 0 when every check passes and 1 otherwise, so it
can be used directly as a test.

## API

```cpp
Ayoayo game;
game.createPlayer("Jensen");   // seat 1
game.createPlayer("Brian");    // seat 2

game.playGame(1, 3);           // player 1 lifts pit 3 and sows the seeds
game.printBoard();
game.returnWinner();
```

| Method | Behaviour |
| --- | --- |
| `Player createPlayer(const std::string& name)` | Registers a player. The first created takes seat 1, the second seat 2. Returns the player. |
| `void printBoard() const` | Prints both stores and both pit lists in the required format. |
| `std::string returnWinner() const` | `"Winner is player <n>: <name>"`, `"It's a tie"`, or `"Game has not ended"`. |
| `PlayResult playGame(int playerIndex, int pitIndex)` | Plays one move and returns the resulting board. |
| `std::vector<int> snapshot() const` | The current board as 14 integers. |

`playGame` prints `"player <n> take another turn"` when a move ends in the
player's own store, as the brief requires.

### `PlayResult`

The brief requires `playGame` to return a 14-integer list on success and a
message such as `"Invalid number for pit index"` on failure. To keep both cases
type-safe, the return type is:

```cpp
using PlayResult = std::variant<std::vector<int>, std::string>;
```

Use the free helper `toString(result)` to print either case exactly as the
brief expects:

```cpp
std::cout << toString(game.playGame(1, 3)) << "\n";
// [4, 4, 0, 5, 5, 5, 1, 4, 4, 4, 4, 4, 4, 0]
```

The returned list is always
`[P1 pit1..pit6, store1, P2 pit1..pit6, store2]`.

### Error and edge-case behaviour

| Situation | Result |
| --- | --- |
| Pit index `<= 0` or `> 6` | `"Invalid number for pit index"` |
| Player index not 1 or 2 | `"Invalid number for player index"` |
| Any move after the game has ended | `"Game is ended"` |
| Move from an empty pit | Legal but inert: returns the unchanged board |

Turn order is deliberately **not** enforced, because the brief states the
grader may call the same player several times in a row to reach a position
faster.

## Rules as implemented

The brief does not state the board geometry, so it was derived from the two
worked examples in the brief. Both examples are reproduced exactly by the
following rules:

- The board is one ring of 14 slots in sowing order:
  `P1 pit1..pit6 -> store1 -> P2 pit1..pit6 -> store2 -> back to P1 pit1`.
- Both players sow **forwards** through that ring. Each player's own store is
  the one following their own pit 6.
- A seed that reaches the **opponent's store is skipped** and continues to the
  next pit. Seeds are never banked for the opponent.
- **Special rule 1:** if the last seed of a move lands in your own store, the
  game prints `"player <n> take another turn"`.
- **Special rule 2:** if the last seed lands in one of your own pits *and that
  pit was empty*, you capture the seeds in the opposite pit plus the seed you
  just placed. The opposite pit is the **mirrored** index, `7 - i`: your pit 1
  faces the opponent's pit 6. If the opposite pit is empty there is nothing to
  capture and the seed stays where it landed.
- **Ending rule:** the game ends as soon as one player's six pits are all empty
  (their store is irrelevant). The seeds still in the pits go to the other
  player's store, and `returnWinner` then reports the result.

## Verification

`main.cpp` runs five groups of checks:

1. **Example 1** from the brief, asserting the exact returned list and the
   final printed board and winner string.
2. **Example 2** from the brief, asserting the exact final board and winner.
3. **Error handling** for invalid pit index, invalid player index, an empty pit,
   and a move after the end.
4. **One full game** played to a natural finish, checking that the final stores
   total 48 and no pit holds seeds.
5. **500 randomised games**, asserting after *every* move that all 48 seeds are
   still on the board, and at the end of each game that no pit holds seeds and
   the two stores sum to 48.

Expected output:

```
=== Example 1 ===
player 1 take another turn
PASS  playGame(1, 3)
player 2 take another turn
player1:
store: 10
[0, 0, 2, 7, 7, 6]
player2:
store: 2
[5, 0, 1, 1, 0, 7]
PASS  returnWinner()

=== Example 2 ===
player 1 take another turn
player1:
store: 12
[0, 0, 0, 0, 0, 0]
player2:
store: 36
[0, 0, 0, 0, 0, 0]
PASS  returnWinner()

=== Error handling ===
PASS  pit index 0
PASS  pit index 7
PASS  pit index -1
PASS  player index 3
PASS  empty pit is a no-op
player 1 take another turn
PASS  move after the end
PASS  winner after the end

=== Full game to a natural finish ===
PASS  game finished in 11 moves, stores 13:35, seeds left in pits 0

=== Randomised stress test ===
PASS  500 random games finished cleanly (longest 77 moves)

ALL CHECKS PASSED
```

The seed-conservation check is the one that matters most: it caught two real
bugs that produced plausible-looking boards while silently losing a seed.

## Notes

- The brief names the file `Ayoayo.java` because the reference solution is
  Java. C++ was chosen for this submission, so the same class and method names
  live in `Ayoayo.h` / `Ayoayo.cpp`. The observable behaviour matches the brief,
  including the literal output strings.
- `Ayoayo.h` / `Ayoayo.cpp` are the only files needed to use the class; drop
  `main.cpp` if an external harness supplies its own `main`.
