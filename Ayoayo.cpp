#include "Ayoayo.h"

#include <iostream>
#include <sstream>

namespace {

const int RING_SIZE = 14;
const int STORE1_SLOT = Ayoayo::PIT_COUNT;                     // 6
const int P2_PIT1_SLOT = Ayoayo::PIT_COUNT + 1;                // 7
const int STORE2_SLOT = 2 * Ayoayo::PIT_COUNT + 1;            // 13

std::string formatPits(const std::array<int, Ayoayo::PIT_COUNT>& pits) {
    std::ostringstream out;
    out << "[";
    for (int i = 0; i < Ayoayo::PIT_COUNT; ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << pits[i];
    }
    out << "]";
    return out.str();
}

}  // namespace

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------

Player::Player(const std::string& name, int index) : name_(name), index_(index) {}

const std::string& Player::getName() const {
    return name_;
}

int Player::getIndex() const {
    return index_;
}

// ---------------------------------------------------------------------------
// PlayResult rendering
// ---------------------------------------------------------------------------

std::string toString(const PlayResult& result) {
    if (const std::string* message = std::get_if<std::string>(&result)) {
        return *message;
    }
    const std::vector<int>& state = std::get<std::vector<int>>(result);
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < state.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << state[i];
    }
    out << "]";
    return out.str();
}

// ---------------------------------------------------------------------------
// Ayoayo
// ---------------------------------------------------------------------------

Ayoayo::Ayoayo() {
    pits1_.fill(INITIAL_SEEDS_PER_PIT);
    pits2_.fill(INITIAL_SEEDS_PER_PIT);
    store1_ = 0;
    store2_ = 0;
}

Player Ayoayo::createPlayer(const std::string& name) {
    const int index = static_cast<int>(players_.size()) + 1;
    players_.push_back(Player(name, index));
    return players_.back();
}

int Ayoayo::ringSlot(int playerIndex, int pitIndex) {
    return playerIndex == 1 ? pitIndex - 1 : P2_PIT1_SLOT + pitIndex - 1;
}

int Ayoayo::pitIndexOfSlot(int slot) {
    return slot <= STORE1_SLOT ? slot + 1 : slot - P2_PIT1_SLOT + 1;
}

bool Ayoayo::isOwnPit(int slot, int playerIndex) {
    if (slot == STORE1_SLOT || slot == STORE2_SLOT) {
        return false;
    }
    return slot <= STORE1_SLOT ? playerIndex == 1 : playerIndex == 2;
}

int Ayoayo::oppositePit(int pitIndex) {
    return PIT_COUNT + 1 - pitIndex;
}

std::array<int, RING_SIZE> Ayoayo::loadBoard() const {
    std::array<int, RING_SIZE> board{};
    for (int i = 0; i < PIT_COUNT; ++i) {
        board[i] = pits1_[i];
    }
    board[STORE1_SLOT] = store1_;
    for (int i = 0; i < PIT_COUNT; ++i) {
        board[P2_PIT1_SLOT + i] = pits2_[i];
    }
    board[STORE2_SLOT] = store2_;
    return board;
}

void Ayoayo::saveBoard(const std::array<int, RING_SIZE>& board) {
    for (int i = 0; i < PIT_COUNT; ++i) {
        pits1_[i] = board[i];
    }
    store1_ = board[STORE1_SLOT];
    for (int i = 0; i < PIT_COUNT; ++i) {
        pits2_[i] = board[P2_PIT1_SLOT + i];
    }
    store2_ = board[STORE2_SLOT];
}

std::array<int, Ayoayo::PIT_COUNT>& Ayoayo::pits(int playerIndex) {
    return playerIndex == 1 ? pits1_ : pits2_;
}

const std::array<int, Ayoayo::PIT_COUNT>& Ayoayo::pits(int playerIndex) const {
    return playerIndex == 1 ? pits1_ : pits2_;
}

int& Ayoayo::store(int playerIndex) {
    return playerIndex == 1 ? store1_ : store2_;
}

int Ayoayo::store(int playerIndex) const {
    return playerIndex == 1 ? store1_ : store2_;
}

std::string Ayoayo::playerName(int playerIndex) const {
    const std::size_t seat = static_cast<std::size_t>(playerIndex - 1);
    if (seat >= players_.size()) {
        return "player " + std::to_string(playerIndex);
    }
    return players_[seat].getName();
}

std::vector<int> Ayoayo::snapshot() const {
    std::vector<int> state;
    state.reserve(RING_SIZE);
    for (int i = 0; i < PIT_COUNT; ++i) {
        state.push_back(pits1_[i]);
    }
    state.push_back(store1_);
    for (int i = 0; i < PIT_COUNT; ++i) {
        state.push_back(pits2_[i]);
    }
    state.push_back(store2_);
    return state;
}

bool Ayoayo::isGameOver() const {
    // The rules end the game when one player's six pits are empty; the seeds
    // already banked in that player's store do not matter.
    const int inPits1 = pits1_[0] + pits1_[1] + pits1_[2] + pits1_[3] + pits1_[4] + pits1_[5];
    const int inPits2 = pits2_[0] + pits2_[1] + pits2_[2] + pits2_[3] + pits2_[4] + pits2_[5];
    return inPits1 == 0 || inPits2 == 0;
}

void Ayoayo::finishGame() {
    if (!isGameOver()) {
        return;
    }
    for (int who = 1; who <= 2; ++who) {
        for (int i = 0; i < PIT_COUNT; ++i) {
            store(who) += pits(who)[i];
            pits(who)[i] = 0;
        }
    }
}

void Ayoayo::printBoard() const {
    for (int who = 1; who <= 2; ++who) {
        std::cout << "player" << who << ":\n";
        std::cout << "store: " << store(who) << "\n";
        std::cout << formatPits(pits(who)) << "\n";
    }
}

std::string Ayoayo::returnWinner() const {
    if (!isGameOver()) {
        return "Game has not ended";
    }
    const int winner = store1_ == store2_ ? 0 : (store1_ > store2_ ? 1 : 2);
    if (winner == 0) {
        return "It's a tie";
    }
    std::ostringstream out;
    out << "Winner is player " << winner << ": " << playerName(winner);
    return out.str();
}

PlayResult Ayoayo::playGame(int playerIndex, int pitIndex) {
    if (playerIndex < 1 || playerIndex > 2) {
        return std::string("Invalid number for player index");
    }
    if (pitIndex < 1 || pitIndex > PIT_COUNT) {
        return std::string("Invalid number for pit index");
    }
    if (isGameOver()) {
        return std::string("Game is ended");
    }

    // Slot 0..5 are player 1's pits, 6 is player 1's store, 7..12 are player 2's
    // pits and 13 is player 2's store. Both players sow forwards through this
    // one ring, so a slot number identifies a pit or a store unambiguously.
    std::array<int, RING_SIZE> board = loadBoard();

    const int ownStoreSlot = playerIndex == 1 ? STORE1_SLOT : STORE2_SLOT;
    const int opponentStoreSlot = playerIndex == 1 ? STORE2_SLOT : STORE1_SLOT;
    const int ownSlot = playerIndex == 1 ? pitIndex - 1 : P2_PIT1_SLOT + pitIndex - 1;
    const int hand = board[ownSlot];
    board[ownSlot] = 0;

    int lastSlot = ownSlot;
    for (int seed = 1; seed <= hand; ++seed) {
        // Advance to the next slot, skipping the opponent's store: seeds are
        // never dropped into it, they travel past it to the next pit.
        do {
            lastSlot = (lastSlot + 1) % RING_SIZE;
        } while (lastSlot == opponentStoreSlot);

        const bool wasEmptyOwnPit = seed == hand && isOwnPit(lastSlot, playerIndex) && board[lastSlot] == 0;
        ++board[lastSlot];

        // Special rule 2: the last seed landed in one of our own pits and that
        // pit was empty beforehand, so we take the seeds sitting in the pit
        // opposite to it, together with the seed we just dropped.
        if (wasEmptyOwnPit) {
            const int landedPit = pitIndexOfSlot(lastSlot);
            const int oppositeSlot = ringSlot(3 - playerIndex, oppositePit(landedPit));
            if (board[oppositeSlot] > 0) {
                board[ownStoreSlot] += board[oppositeSlot] + 1;
                board[oppositeSlot] = 0;
                board[lastSlot] = 0;
            }
        }
    }

    // Special rule 1: the last seed landed in our own store, so we move again.
    if (hand > 0 && lastSlot == ownStoreSlot) {
        std::cout << "player " << playerIndex << " take another turn\n";
    }

    saveBoard(board);
    finishGame();
    return snapshot();
}
