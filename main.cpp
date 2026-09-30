/**
 * Verification harness for the Ayoayo assignment.
 *
 * It replays the two worked examples from the specification and checks that
 * the program produces the exact output documented there, then plays a full
 * game to a natural finish to confirm the ending rule and that no seed is
 * ever lost or created.
 */

#include "Ayoayo.h"

#include <algorithm>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

/**
 * playGame prints the "player N take another turn" notice the specification
 * requires. Bulk games below would otherwise bury the check results, so their
 * output is muted while the checks run.
 */
class MutedOutput {
public:
    MutedOutput() : saved_(std::cout.rdbuf(sink_.rdbuf())) {}
    ~MutedOutput() { std::cout.rdbuf(saved_); }

    MutedOutput(const MutedOutput&) = delete;
    MutedOutput& operator=(const MutedOutput&) = delete;

private:
    std::stringstream sink_;
    std::streambuf* saved_;
};

void checkEqual(const std::string& label, const std::string& actual, const std::string& expected) {
    if (actual == expected) {
        std::cout << "PASS  " << label << "\n";
    } else {
        ++failures;
        std::cout << "FAIL  " << label << "\n";
        std::cout << "        expected: " << expected << "\n";
        std::cout << "        actual:   " << actual << "\n";
    }
}

void banner(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

/** Example 1 from the specification. */
void exampleOne() {
    banner("Example 1");
    Ayoayo game;
    game.createPlayer("Jensen");
    game.createPlayer("Brian");

    checkEqual("playGame(1, 3)", toString(game.playGame(1, 3)),
               "[4, 4, 0, 5, 5, 5, 1, 4, 4, 4, 4, 4, 4, 0]");
    game.playGame(1, 1);
    game.playGame(2, 3);
    game.playGame(2, 4);
    game.playGame(1, 2);
    game.playGame(2, 2);
    game.playGame(1, 1);

    game.printBoard();
    checkEqual("returnWinner()", game.returnWinner(), "Game has not ended");
}

/** Example 2 from the specification: player 1 empties their own side. */
void exampleTwo() {
    banner("Example 2");
    Ayoayo game;
    game.createPlayer("Jensen");
    game.createPlayer("Brian");

    for (int pit = 1; pit <= 6; ++pit) {
        game.playGame(1, pit);
    }
    game.printBoard();
    checkEqual("returnWinner()", game.returnWinner(), "Winner is player 2: Brian");
}

/** Error handling required by the specification. */
void errorCases() {
    banner("Error handling");
    Ayoayo game;
    game.createPlayer("Jensen");
    game.createPlayer("Brian");

    checkEqual("pit index 0", toString(game.playGame(1, 0)), "Invalid number for pit index");
    checkEqual("pit index 7", toString(game.playGame(1, 7)), "Invalid number for pit index");
    checkEqual("pit index -1", toString(game.playGame(1, -1)), "Invalid number for pit index");
    checkEqual("player index 3", toString(game.playGame(3, 1)), "Invalid number for player index");

    // Emptying pit 6 leaves it empty, so playing it again must change nothing.
    game.playGame(1, 6);
    checkEqual("empty pit is a no-op", toString(game.playGame(1, 6)),
               "[4, 4, 4, 4, 4, 0, 1, 5, 5, 5, 4, 4, 4, 0]");

    Ayoayo finished;
    finished.createPlayer("Jensen");
    finished.createPlayer("Brian");
    for (int pit = 1; pit <= 6; ++pit) {
        finished.playGame(1, pit);
    }
    checkEqual("move after the end", toString(finished.playGame(1, 1)), "Game is ended");
    checkEqual("winner after the end", finished.returnWinner(), "Winner is player 2: Brian");
}

/**
 * Plays alternating legal moves until the game ends, asserting after every
 * move that all 48 seeds are still on the board, and finally that both sides
 * are empty.
 */
void fullGame() {
    banner("Full game to a natural finish");
    int moves = 0;
    std::vector<int> state;
    {
        Ayoayo game;
        game.createPlayer("Jensen");
        game.createPlayer("Brian");
        MutedOutput mute;

        int player = 1;
        while (game.returnWinner() == "Game has not ended" && moves < 500) {
            state = game.snapshot();
            int total = 0;
            for (int value : state) {
                total += value;
            }
            if (total != 48) {
                ++failures;
                std::cout << "FAIL  seed conservation broken: " << total << " after " << moves << " moves\n";
                return;
            }
            const int base = player == 1 ? 0 : 7;
            int chosen = 0;
            for (int pit = 1; pit <= 6; ++pit) {
                if (state[base + pit - 1] > 0) {
                    chosen = pit;
                    break;
                }
            }
            if (chosen == 0) {
                break;
            }
            game.playGame(player, chosen);
            player = 3 - player;
            ++moves;
        }
        state = game.snapshot();
    }

    int onBoard = 0;
    for (int i = 0; i < 6; ++i) {
        onBoard += state[i] + state[7 + i];
    }
    std::cout << (moves < 500 ? "PASS  " : "FAIL  ") << "game finished in " << moves << " moves, "
              << "stores " << state[6] << ":" << state[13] << ", seeds left in pits " << onBoard << "\n";
    if (moves >= 500 || onBoard != 0) {
        ++failures;
    }
}

/**
 * Plays many random legal games, asserting after every move that all 48 seeds
 * are accounted for, that each game reaches a natural end, and that the final
 * stores add up to 48 with no seeds left in any pit.
 */
void stressTest() {
    banner("Randomised stress test");
    std::mt19937 rng(20260930u);
    int completed = 0;
    int longest = 0;
    std::string problem;

    {
        MutedOutput mute;
        for (int gameIndex = 0; gameIndex < 500; ++gameIndex) {
            Ayoayo game;
            game.createPlayer("Jensen");
            game.createPlayer("Brian");

            int player = 1 + static_cast<int>(rng() % 2);
            int moves = 0;
            bool aborted = false;
            while (moves < 2000) {
                const std::vector<int> state = game.snapshot();
                int total = 0;
                for (int value : state) {
                    total += value;
                }
                if (total != 48) {
                    problem = "seed conservation broken: " + std::to_string(total) + " in game " +
                              std::to_string(gameIndex) + " after " + std::to_string(moves) + " moves";
                    aborted = true;
                    break;
                }
                if (game.returnWinner() != "Game has not ended") {
                    break;
                }
                const int base = player == 1 ? 0 : 7;
                std::vector<int> options;
                for (int pit = 1; pit <= 6; ++pit) {
                    if (state[base + pit - 1] > 0) {
                        options.push_back(pit);
                    }
                }
                if (options.empty()) {
                    break;
                }
                game.playGame(player, options[rng() % options.size()]);
                player = 3 - player;
                ++moves;
            }

            const std::vector<int> state = game.snapshot();
            int inPits = 0;
            for (int i = 0; i < 6; ++i) {
                inPits += state[i] + state[7 + i];
            }
            const std::string winner = game.returnWinner();
            const bool validEnd = winner == "It's a tie" || winner.rfind("Winner is player", 0) == 0;
            if (!aborted && (moves >= 2000 || inPits != 0 || state[6] + state[13] != 48 || !validEnd)) {
                std::ostringstream out;
                out << "game " << gameIndex << " ended badly after " << moves << " moves: pits " << inPits
                    << ", stores " << state[6] << ":" << state[13] << ", " << winner;
                problem = out.str();
                break;
            }
            if (aborted) {
                break;
            }
            ++completed;
            longest = std::max(longest, moves);
        }
    }

    if (problem.empty()) {
        std::cout << "PASS  " << completed << " random games finished cleanly (longest " << longest << " moves)\n";
    } else {
        ++failures;
        std::cout << "FAIL  " << problem << "\n";
    }
}

}  // namespace

int main() {
    exampleOne();
    exampleTwo();
    errorCases();
    fullGame();
    stressTest();

    std::cout << "\n" << (failures == 0 ? "ALL CHECKS PASSED" : "FAILURES: " + std::to_string(failures))
              << "\n";
    return failures == 0 ? 0 : 1;
}
