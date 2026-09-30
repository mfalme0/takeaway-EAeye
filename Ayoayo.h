#ifndef AYOAYO_H
#define AYOAYO_H

#include <array>
#include <string>
#include <variant>
#include <vector>

/**
 * One of the two competitors in an Ayoayo (Kalah / Mancala) game.
 *
 * The class deliberately exposes read-only accessors only: the name and the
 * seat (1 or 2) of a player are assigned by Ayoayo::createPlayer and must not
 * change for the lifetime of the game.
 */
class Player {
public:
    Player(const std::string& name, int index);

    const std::string& getName() const;
    int getIndex() const;

private:
    std::string name_;
    int index_;
};

/**
 * Result of a single Ayoayo::playGame call.
 *
 * Either the current board snapshot (14 integers, see snapshot()) or a
 * diagnostic message such as "Invalid number for pit index".
 */
using PlayResult = std::variant<std::vector<int>, std::string>;

/**
 * Renders a PlayResult the way the assignment specification expects it to be
 * printed: a normalised 14-integer list such as "[4, 4, 0, ..., 0]", or the
 * diagnostic message it carries.
 */
std::string toString(const PlayResult& result);

/**
 * A single Ayoayo (Kalah) game: two players, six pits and one store each,
 * 48 seeds in total.
 *
 * Board layout, in sowing order, is one ring of 14 slots:
 *
 *     P1 pit1 .. P1 pit6 -> store1 -> P2 pit1 .. P2 pit6 -> store2 -> (P1 pit1)
 *
 * Both players sow forward through that ring and always skip the opponent's
 * store. The pit "opposite" a player's own pit i is the opponent's pit 7 - i.
 */
class Ayoayo {
public:
    static constexpr int PIT_COUNT = 6;
    static constexpr int INITIAL_SEEDS_PER_PIT = 4;

    Ayoayo();

    /**
     * Registers a player. The first player created takes seat 1, the second
     * takes seat 2, and so on.
     */
    Player createPlayer(const std::string& name);

    /**
     * Prints both players' store and pits, e.g.
     *
     *     player1:
     *     store: 10
     *     [0, 0, 2, 7, 7, 6]
     *     player2:
     *     store: 2
     *     [5, 0, 1, 1, 0, 7]
     */
    void printBoard() const;

    /**
     * Returns
     *   - "Winner is player <n>: <name>" when the game is over and the stores
     *     differ,
     *   - "It's a tie" when the game is over and the stores are equal,
     *   - "Game has not ended" otherwise.
     */
    std::string returnWinner() const;

    /**
     * Plays one move: the given player lifts every seed from pitIndex and sows
     * them one by one to the right, honouring both special rules (extra turn on
     * a store landing, capture on a landing in a previously empty own pit) and
     * then the ending condition.
     *
     * Returns the resulting board snapshot, or a diagnostic string when the
     * move cannot be played:
     *   - "Invalid number for pit index"  (pit index outside 1..6)
     *   - "Invalid number for player index" (player index outside 1..2)
     *   - "Game is ended"
     * A move from an empty pit is legal but changes nothing.
     */
    PlayResult playGame(int playerIndex, int pitIndex);

    /**
     * Current board as 14 integers:
     * [P1 pit1..pit6, store1, P2 pit1..pit6, store2].
     */
    std::vector<int> snapshot() const;

private:
    std::vector<Player> players_;
    std::array<int, PIT_COUNT> pits1_;
    std::array<int, PIT_COUNT> pits2_;
    int store1_;
    int store2_;

    /** Ring slot of a player's own pit 1..6, i.e. seat 1 -> 0..5, seat 2 -> 7..12. */
    static int ringSlot(int playerIndex, int pitIndex);
    /**
     * Pit number 1..6 held by a ring slot. Only meaningful for pit slots, i.e.
     * slots 0..5 and 7..12; callers must rule out the two store slots first.
     */
    static int pitIndexOfSlot(int slot);
    /** Whether a ring slot is a pit (not a store) belonging to playerIndex. */
    static bool isOwnPit(int slot, int playerIndex);
    /** Pit number 1..6 opposite to the given one on the other side: 7 - pit. */
    static int oppositePit(int pitIndex);
    /** The whole board as one 14-slot ring: 6 pits, store, 6 pits, store. */
    std::array<int, 14> loadBoard() const;
    /** Writes a 14-slot ring back into the pit and store members. */
    void saveBoard(const std::array<int, 14>& board);
    std::array<int, PIT_COUNT>& pits(int playerIndex);
    const std::array<int, PIT_COUNT>& pits(int playerIndex) const;
    int& store(int playerIndex);
    int store(int playerIndex) const;
    /** Player's name, falling back to "player N" if createPlayer was never called for that seat. */
    std::string playerName(int playerIndex) const;
    bool isGameOver() const;
    /** Applies the ending rule: the seeds left on the board go to the winner's store. */
    void finishGame();
};

#endif // AYOAYO_H
