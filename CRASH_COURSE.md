# AYOA YO CRASH COURSE
## Everything about this project, so you can defend it in a viva

Read this once end to end. After that, skim section 10 (the Q&A) — that is
where the questions will actually come from.

---

## 1. WHAT THE PROJECT IS

The brief asked for a text-based version of **Ayoayo**, the board game better
known as **Kalah** or **Mancala**. Two players, six pits each, one store each,
48 seeds total. You win by collecting the most seeds in your store.

The brief was written for Java (it mentions `Ayoayo.java` and shows Java code),
but it explicitly allowed C++, and C++ is what this submission uses. Same class
names, same method names, same output strings.

**Files shipped:**

| File | Lines | Role |
| --- | --- | --- |
| `Ayoayo.h` | 142 | `Player` and `Ayoayo` declarations. All data members private. |
| `Ayoayo.cpp` | 257 | All the logic. |
| `main.cpp` | 267 | Verification harness (the "tests"). |
| `README.md` | — | Build instructions, API table, rules summary. |
| `reflection.txt` | — | Required deliverable: initial + final reflection. |
| `.gitignore` | — | Excludes `.obj` / `.exe` build output from git. |

Build output (`ayoayo.exe`, `*.obj`) is on disk but deliberately not committed.

---

## 2. THE ONE THING THAT MATTERS MOST: HOW I FIGURED OUT THE BOARD

**If you understand this, you understand the project.** This is also the most
likely thing an examiner will probe, because it is the only genuinely hard part.

### The problem

The brief says seeds are sown "to the right" and captured from "the pit on the
opposite side", but it never defines:

- what "to the right" means for **player 2** (right from whose point of view?)
- how the two rows of pits are **numbered** on the physical board
- whether "opposite" means the **same index** or the **mirrored index**

The figure in the brief would have shown this. The text I was given did not
include it. So the layout had to be **derived from the two worked examples** in
the brief, which print exact board positions.

### The answer

The board is one **ring of 14 slots**:

```
slot:   0    1    2    3    4    5    6       7    8    9   10   11   12      13
     P1p1  P1p2  P1p3  P1p4  P1p5  P1p6  STORE1  P2p1  P2p2  P2p3  P2p4  P2p5  P2p6  STORE2
                                                                      ^ back round to slot 0
```

Three findings, each forced by the examples:

**(a) Both players sow forwards through the same ring.**
Player 1 starts at their pit and walks to higher slot numbers. Player 2 also
walks to higher slot numbers. Each player's *own* store is simply the one that
follows their own pit 6: slot 6 for player 1, slot 13 for player 2.

Why this is right: in example 1, `playGame(2, 3)` gives player 2 an extra turn,
which only happens if their 4 seeds land in slots 10, 11, 12, 13 — that is,
pits 4, 5, 6 and their own store.

**(b) "Opposite" is the MIRRORED index: `7 - i`.**
Your pit 1 faces the opponent's pit 6, your pit 6 faces their pit 1.

Why this is right: the last move of example 1 captures. I traced it both ways:

- same-index capture  → player 1's store ends at **8**
- mirrored-index capture → player 1's store ends at **10**

The brief documents `store: 10`. Only the mirrored reading matches. That single
number is what pinned the whole layout down.

**(c) A seed SKIPS the opponent's store; it does not get counted into it.**

Why this is right, and it is the subtlest rule in the whole game: look at the
final move of example 2. Player 1 sows 8 seeds from pit 6, and the 8th seed
would land in slot 13, which is **player 2's store**.

- If the seed is **banked** into player 2's store → final scores are 5 : 43
- If the seed is **skipped** and continues to slot 0 (player 1's pit 1) →
  player 1's pit 1 was empty, so it triggers a **capture** of player 2's pit 6,
  giving final scores of **12 : 36**

The brief documents `store: 12` and `store: 36`. Only the skip rule produces it.
The brief's own words support this too: *"Only put seeds in your own store, not
your opponent's store."*

### The general principle to say out loud

> The brief's prose and its worked examples only reconcile under one
> interpretation of the geometry. I derived that interpretation from the
> examples, then confirmed the prose agrees with it. Both examples are
> reproduced byte-for-byte.

---

## 3. HOW THE CODE IS STRUCTURED

### 3.1 The 14-slot ring is the single source of truth

This is the key design decision. Rather than juggling `pits1_`, `pits2_`,
`store1_`, `store2_` separately, **every move is computed on one 14-integer
buffer**:

```cpp
std::array<int, RING_SIZE> board = loadBoard();   // read the real board
... sow, capture ...
saveBoard(board);                                 // write it back
return snapshot();                                // report it
```

`loadBoard()` packs the four members into the ring; `saveBoard()` unpacks them.

**Why it matters:** my first version sowed into a *local copy of a different
structure* while the real members were updated through helper functions. The
two representations drifted apart and seeds were being destroyed silently (bug
1 and bug 2 below). One buffer, one loop, one write-back makes that class of
bug impossible.

### 3.2 Member variables (all private, as required)

```cpp
std::vector<Player> players_;              // creation order defines seat 1, 2
std::array<int, 6>   pits1_;                // player 1's six pits
std::array<int, 6>   pits2_;                // player 2's six pits
int                 store1_;               // player 1's store
int                 store2_;               // player 2's store
```

Note what is **absent**: no `gameOver_` boolean. I wrote one, then deleted it —
see bug 4. It duplicated `isGameOver()` and duplicated state drifts.

### 3.3 The `Player` class

The brief said "You can define the player class by yourself. It will be your own
design," so this is minimal and immutable:

```cpp
class Player {
public:
    Player(const std::string& name, int index);
    const std::string& getName() const;    // read-only
    int getIndex() const;                  // read-only
private:
    std::string name_;
    int index_;
};
```

**Design justification:** a player's name and seat are assigned once by
`createPlayer` and can never change during a game, so there are **no setters at
all**. Only `const` accessors. A player's identity is immutable, which is
enforced by the compiler rather than by convention.

### 3.4 `createPlayer` — how seats are assigned

```cpp
Player Ayoayo::createPlayer(const std::string& name) {
    const int index = static_cast<int>(players_.size()) + 1;
    players_.push_back(Player(name, index));
    return players_.back();
}
```

The brief's example calls `createPlayer("Jensen")` then `createPlayer("Brian")`
and then refers to them as player 1 and player 2. So **creation order = seat
number**. `players_[0]` is seat 1. Returning `players_.back()` by value matches
the brief's `Player player1 = game.createPlayer("Jensen");`.

---

## 4. EVERY RULE AND WHERE IT LIVES

`playGame` is the heart. Here is its structure, in order:

```cpp
PlayResult Ayoayo::playGame(int playerIndex, int pitIndex) {
    // (1) input validation - three early returns
    if (playerIndex < 1 || playerIndex > 2) return "Invalid number for player index";
    if (pitIndex < 1 || pitIndex > PIT_COUNT) return "Invalid number for pit index";
    if (isGameOver())                          return "Game is ended";

    // (2) load the board into the 14-slot ring
    std::array<int, RING_SIZE> board = loadBoard();

    // (3) work out which slots are "mine"
    const int ownStoreSlot     = playerIndex == 1 ? STORE1_SLOT : STORE2_SLOT;
    const int opponentStoreSlot= playerIndex == 1 ? STORE2_SLOT : STORE1_SLOT;
    const int ownSlot          = playerIndex == 1 ? pitIndex - 1 : P2_PIT1_SLOT + pitIndex - 1;

    // (4) lift every seed out of the chosen pit
    const int hand = board[ownSlot];
    board[ownSlot] = 0;

    // (5) sow them one at a time
    int lastSlot = ownSlot;
    for (int seed = 1; seed <= hand; ++seed) {
        do { lastSlot = (lastSlot + 1) % RING_SIZE; }   // <-- the skip rule
        while (lastSlot == opponentStoreSlot);

        const bool wasEmptyOwnPit =
            seed == hand && isOwnPit(lastSlot, playerIndex) && board[lastSlot] == 0;
        ++board[lastSlot];

        if (wasEmptyOwnPit) { /* special rule 2: capture */ }
    }

    // (6) special rule 1: extra turn
    if (hand > 0 && lastSlot == ownStoreSlot)
        std::cout << "player " << playerIndex << " take another turn\n";

    // (7) commit, then check for the end of the game
    saveBoard(board);
    finishGame();
    return snapshot();
}
```

### The two special rules, precisely

**Rule 1 — extra turn.** Fires when the last seed lands in **your own store**.
Note it is a `print`, not a state change: the brief requires the message
`"player 1 take another turn"`, and separately says the grader may call
`playGame` for the same player repeatedly. So the class **announces** the extra
turn but does **not** track whose turn it is. A capture does *not* grant an
extra turn — example 2 has a capture on the last move and prints no extra-turn
line for it.

**Rule 2 — capture.** Four conditions must all hold:
1. it is the **last** seed of the move (`seed == hand`),
2. it lands in **one of your own** pits (`isOwnPit`),
3. that pit was **empty before** the seed landed (`board[lastSlot] == 0`,
   checked *before* the `++`),
4. the opposite pit actually **has seeds** (`board[oppositeSlot] > 0`).

If condition 4 fails there is nothing to capture and the seed simply stays in
the pit. On a capture, the store gains the opposite pit's seeds **plus the one
seed just placed** — hence `+= board[oppositeSlot] + 1` — and both pits are
zeroed.

The opposite slot: your pit `i` → opponent's pit `7 - i`.

```cpp
const int landedPit    = pitIndexOfSlot(lastSlot);
const int oppositeSlot = ringSlot(3 - playerIndex, oppositePit(landedPit));
```

`3 - playerIndex` flips 1↔2. `oppositePit` returns `PIT_COUNT + 1 - pitIndex`.

### The ending rule

```cpp
bool Ayoayo::isGameOver() const {
    const int inPits1 = pits1_[0] + ... + pits1_[5];
    const int inPits2 = pits2_[0] + ... + pits2_[5];
    return inPits1 == 0 || inPits2 == 0;
}
```

The rules say the game ends when one player's **pits** are empty. The store is
deliberately **excluded** from that sum — excluding it was bug 4.

`finishGame()` then sweeps both sides' pits into the matching stores. It sweeps
both rather than just the winner because the losing side is already zero; this
keeps the code branch-free.

`finishGame()` is called after **every** move, not conditionally, and it checks
`isGameOver()` itself before doing anything.

### `returnWinner`

```cpp
if (!isGameOver())                     return "Game has not ended";
if (store1_ == store2_)                return "It's a tie";
return "Winner is player " << (store1_ > store2_ ? 1 : 2) << ": " << name;
```

Exact strings from the brief: `"Winner is player 1: Brian"`, `"It's a tie"`,
`"Game has not ended"`. It derives the state on demand rather than caching it.

---

## 5. THE API DESIGN DECISION YOU WILL BE ASKED ABOUT

**The problem:** the brief requires `playGame` to return a 14-integer list on a
normal move, but a *string* like `"Invalid number for pit index"` on bad input.
One function, two return types.

**The options I considered:**

1. Return `std::string` always — but then the list is a string, and a caller
   parsing it has to re-parse.
2. Return `std::vector<int>` always and print errors instead — but the brief
   says *return* them, and this loses the exact error text as a value.
3. **`std::variant<std::vector<int>, std::string>`** — what I did.
4. Throw exceptions for errors — a big behavioural change the brief does not
   want, since it specifies returned messages.

**What I shipped (option 3):**

```cpp
using PlayResult = std::variant<std::vector<int>, std::string>;
PlayResult playGame(int playerIndex, int pitIndex);
std::string toString(const PlayResult& result);
```

**How to justify it:** it is type-safe, so a caller *cannot* accidentally treat
an error message as a board position, and it still prints byte-identically to
the brief via `toString`. This was your explicit instruction too — when asked
about the return type, you chose "best practices" over "match the spec
literally", and `std::variant` is the idiomatic C++17 answer. The trade-off to
admit honestly: a caller must unwrap it, which is slightly more work than
`std::cout << result` would have been.

The 14 integers are always in this order:

```
[P1 pit1..pit6, store1, P2 pit1..pit6, store2]
```

which is exactly the order the brief uses in both examples.

### Other small decisions

- **All members private, all accessors `const`** — the brief requires private
  data members, and the `const` correctness throughout means `printBoard`,
  `returnWinner` and `snapshot` cannot accidentally mutate the game.
- **`PIT_COUNT` / `INITIAL_SEEDS_PER_PIT` are `static constexpr`** so the
  constants live in one place and can be used as array bounds.
- **Helper predicates** (`isOwnPit`, `ringSlot`, `pitIndexOfSlot`) are `static`
  and private, because they are pure functions of slot numbers.
- **`printBoard` uses the compact single-spaced layout.** The brief's sample is
  double-spaced, which I judged to be PDF line-wrapping rather than intended
  output. Flag this if asked — it is a judgement call, not a fact.

---

## 6. THE FOUR BUGS — YOUR BEST VIVA MATERIAL

An examiner asking "did you hit any problems?" is really asking "do you
understand your own code?" These four are the proof that you do. Know the
symptom, the cause and the lesson for each.

### Bug 1 — sowing into a local copy
**Symptom:** the returned board was correct but the stored state never changed.
Playing the same move twice gave the same result.
**Cause:** the sow loop mutated a local ring built by a `ringFrom()` helper,
while the real members were updated separately by `pits()` / `store()`. Two
representations of the same board, updated by different code.
**Fix:** `loadBoard()` / `saveBoard()` — one buffer, one loop, one write-back.
**Lesson:** when a method mutates state, make the mutation the primary
operation and *derive* the return value from it. Never compute a result
somewhere else and hope the two agree.

### Bug 2 — capturing without the "was empty" condition
**Symptom:** a game finished with **47** seeds instead of 48.
**Cause:** the capture fired whenever the last seed landed in an own pit, but the
rule requires that pit to have been **empty** beforehand.
**Fix:** compute `wasEmptyOwnPit` *before* incrementing the slot.
**Lesson:** this is what the "48 seeds always" assertion in the stress test was
for. A missing-seeds bug is invisible in a printed board unless you assert the
total.

### Bug 3 — the store classified as a pit
**Symptom:** again 48 → 47 seeds.
**Cause:** `isOwnPit(6, 1)` returned `true`, because the test was
`slot <= STORE1_SLOT` and slot 6 **is** `STORE1_SLOT`. So a store landing
triggered a phantom capture: it added a seed to the store and then zeroed the
same slot, destroying one.
**Fix:**
```cpp
if (slot == STORE1_SLOT || slot == STORE2_SLOT) return false;   // stores are not pits
return slot <= STORE1_SLOT ? playerIndex == 1 : playerIndex == 2;
```
**Lesson:** "is this a pit or a store?" deserved one small, obviously-correct,
tested predicate instead of being re-derived at each call site. Note the
off-by-one: `<=` should have been `<`.

### Bug 4 — the ending condition included the store
**Symptom:** example 2 returned `"Game has not ended"` on a visibly finished
board.
**Cause:** `isGameOver()` summed `store + pits`, so a player with an empty side
but a full store never triggered the end.
**Fix:** sum **only** the six pits. The rules say "pits are completely empty".
**Lesson:** be suspicious whenever a threshold in the code is **broader** than
the threshold in the rules. This is the clearest example in the project of
encoding the rule more loosely than the rule was stated.

### The theme worth saying out loud

> All four bugs were a disagreement between two places that both believed they
> owned the truth: a local copy versus the real board, a rule versus its
> wording, a boundary comparison, a cached flag versus a derived one. Every fix
> that mattered removed the duplication rather than patching a symptom — and
> the "48 seeds" invariant is what turned silent corruption into a visible
> failure.

---

## 7. HOW I VERIFIED IT

`main.cpp` is the test suite. It exits **0** on success, **1** on any failure,
so it doubles as a CI check. Five groups:

1. **Example 1** — the brief's 7-move sequence. Asserts the exact returned list
   from the first move, then the final board and winner string.
2. **Example 2** — the brief's 6-move sequence. Asserts the final board
   (`store: 12` / `store: 36`) and the winner.
3. **Error paths** — pit index `0`, `7`, `-1`; player index `3`; an empty pit;
   and a move attempted after the game ended.
4. **One full game** to a natural finish, checking stores total 48 and no pit
   holds seeds.
5. **500 randomised games** (seeded `mt19937` for reproducibility), asserting
   after **every single move** that all 48 seeds are on the board, and at the
   end that no pit holds seeds and the stores sum to 48.

Current output: `ALL CHECKS PASSED`, longest random game 77 moves.

**Why the stress test earns its place:** the two worked examples only cover a
handful of code paths. They never exercise a long sow that wraps past a store
in a real game. 500 random games cover those, and the per-move conservation
assertion is what actually caught bugs 2 and 3.

**One honesty point:** the randomised play alternates players and *ignores* the
extra-turn notice, so it is not a faithful Kalah simulation — it is a fuzz test
for state integrity. Its job is conservation and termination, not strategy.

---

## 8. THE GIT AND SUBMISSION WORK

- The directory was empty and **not a git repo**, so I ran `git init`.
- Committed the five source files; added `.gitignore` so `*.obj` and `*.exe`
  never get committed.
- Created the GitHub repo `mfalme0/takeaway-EAeye`, added it as `origin`.
- Tagged the submission **`Ayayol`** (the brief's spelling) as an **annotated**
  tag, then moved it after the README commit so it points at the final tree.
- You asked for both a public repo and a `main` branch, so: renamed `master` →
  `main`, set it as the default branch, deleted the old remote `master`, and
  flipped visibility to public.
- Local and remote verified in sync at the same commit; `git ls-remote` confirms
  the branch and the tag are on GitHub.

**If asked about the tag name:** the brief spells it `Ayayol` in the submission
instructions. I used it exactly as written rather than "correcting" it to
`Ayoayo`, because the brief specifies it literally.

---

## 9. HONEST LIMITATIONS

Do not be caught out by these; they are judgement calls, and saying so is a
strength.

- **The geometry is inferred, not given.** If your grader's board figure
  numbered the top row the other way, the mirrored-index capture would differ.
  The evidence from the examples points firmly to what I implemented, and both
  examples match exactly, but this is the single assumption most worth stating
  aloud.
- **`printBoard` spacing is a judgement call** — I dropped the PDF's blank
  lines.
- **No interactive input loop.** The brief's example drives the class
  programmatically; the assignment asks for text I/O, which the class provides
  via `printBoard` and the returned strings. There is no `std::cin` menu loop.
- **Turn order is not enforced**, deliberately, because the brief says the
  grader may call the same player repeatedly.
- **`std::cout` in a game method is a slight smell.** `playGame` printing is
  required by the brief, but in ordinary design a method would return a "you go
  again" flag instead. Adopting the brief's contract here was deliberate.

---

## 10. LIKELY QUESTIONS, WITH ANSWERS

**Q. Why is the capture `7 - i` and not `i`?**
Because the two rows are numbered in opposite directions on the board, so your
pit 1 sits across from the opponent's pit 6. I verified it from example 1: the
same-index reading gives player 1 a store of 8, the mirrored reading gives 10,
and the brief documents 10.

**Q. Walk me through `playGame(1, 3)` on a fresh board.**
Player 1 lifts all 4 seeds from slot 2. They land in slots 3, 4, 5, 6 — pits 4,
5, 6 and the store. Last seed in own store → print `"player 1 take another
turn"`. Result `[4, 4, 0, 5, 5, 5, 1, 4, 4, 4, 4, 4, 4, 0]`, which is the
brief's value exactly.

**Q. What does the `do/while` in the sow loop do?**
It is the skip rule. `lastSlot` advances one slot, and while that slot is the
**opponent's** store it advances again, so the seed is dropped in the next pit
instead. A `while` alone would not work, because the very first advance could
land on the opponent's store; `do/while` guarantees at least one step.

**Q. Why is `wasEmptyOwnPit` computed before `++board[lastSlot]`?**
Because the rule asks whether the pit was empty *before* the seed arrived. After
the increment it is never empty, so the check would always be false. Same
pattern as `hand` being read before the pit is zeroed.

**Q. When does the game end?**
When either player's six **pits** total zero. The store is excluded. Then
`finishGame` sweeps the remaining pit seeds into the other player's store.

**Q. Why does `finishGame` sweep both players' pits?**
Because when the game ends one side is already zero, so sweeping both is
equivalent to sweeping the winner's and avoids a branch.

**Q. What happens if someone picks an empty pit?**
`hand` is 0, the loop body never runs, `lastSlot` stays as the origin slot which
is not the store, so no extra turn is printed, the board is saved unchanged and
the current snapshot is returned. Legal but inert. The brief only defines errors
for an out-of-range index, so I did not invent an error for this.

**Q. How do you know you never lose a seed?**
The stress test asserts the total is exactly 48 after every move of 500 random
games, and 48 again in the two final stores. This check found two real bugs that
printed perfectly plausible boards.

**Q. Why `std::variant` instead of just returning a string?**
Because the brief needs two genuinely different return shapes. A variant keeps
them type-safe — a caller cannot treat `"Game is ended"` as a board — while
`toString` reproduces the brief's exact printed output.

**Q. Is there anything you would change?**
Yes: factor the 14-slot ring into a tiny `Board` struct so the slot arithmetic
is named rather than numbered; move the "take another turn" notice behind a
callback or return flag so the game logic has no `cout` in it; and add
capture/skip micro-tests so each rule is pinned by its own test rather than
relying on the two large examples to cover them.

**Q. Why did you not use a 2D array for the board?**
It would not reflect the game's topology. A Kalah board is a **ring**, not a
grid; the ring makes the sow a modular increment and the wrap-around automatic.
A 2D array would have needed a hand-rolled index-translation table — which is
exactly the duplication that produced bugs 1 and 2.

---

## 11. FIVE-SECOND SUMMARY

- Kalah/Mancala in C++, 48 seeds, text-only, all members private.
- Board modelled as one **14-slot ring**; both players sow forwards; the
  opponent's store is **skipped**; captures take the **mirrored** pit `7 - i`.
- Layout was **derived from the brief's two worked examples**, because the prose
  never defines it. Both examples reproduce byte-for-byte.
- `playGame` returns `std::variant<std::vector<int>, std::string>` — a board
  snapshot or a diagnostic message.
- Four real bugs found and fixed, all caused by duplicated sources of truth; the
  strongest fix replaced the duplication rather than patching a symptom.
- Verified by replaying both examples plus **500 randomised games** asserting
  48-seed conservation after every move. All checks pass.
- Pushed to `github.com/mfalme0/takeaway-EAeye`, branch `main`, tagged `Ayayol`.
