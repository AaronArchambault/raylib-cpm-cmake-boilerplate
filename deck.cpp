#include "deck.h"
#include <map>
#include <raylib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <random>

using namespace std;

Player::Player(bool ai, const std::string& playerName) : isAI(ai), name(playerName) {}

bool Player::canPlay(const Card& topCard) const {
    for (const auto& card : hand) {
        if (card.matches(topCard)) {
            return true;
        }
    }
    return false;
}

int Player::chooseOptimalCard(const Card& topCard, int opponentHandSize) {
    if (!isAI) {
        return -1;
    }
    //find all the playable cards
    vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }
    if (playableIndices.empty()) {
        return -1;
    }

    //score each playable card using the linear programming
    int bestIndex = -1;
    double bestScore = -1.0;

    for (int idx : playableIndices) {
        CardScore score = LPOptimizer::calcCard(hand[idx], topCard,
            hand.size(), opponentHandSize);
        //apply the optimization
        if (score.strategicValue > bestScore) {
            bestScore = score.strategicValue;
            bestIndex = idx;
        }
    }
    return bestIndex;
}

int Player::chooseOptimalCardMultiTurn(const Card& topCard, int opponentHandSize, int turnsToAnalyze) {
    if (!isAI) {
        return -1;
    }
    vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }
    if (playableIndices.empty()) {
        return -1;
    }
    int bestIndex = -1;
    double bestScore = -1.0;
    for (int idx : playableIndices) {
        double multiTurnValues = 0.0;

        for (int turn = 0; turn < turnsToAnalyze; turn++) {
            CardScore score = LPOptimizer::calcCard(hand[idx], topCard,
                hand.size() - turn,
                opponentHandSize);

            double discountFactor = pow(0.9, turn);
            multiTurnValues += score.strategicValue * discountFactor;
        }
        if (multiTurnValues > bestScore) {
            bestScore = multiTurnValues;
            bestIndex = idx;
        }
    }
    return bestIndex;
}


Card Player::playCard(int index) {
    Card played = hand[index];
    hand.erase(hand.begin() + index);
    return played;
}

void Player::addCard(const Card& card) {
    hand.push_back(card);
}

int Player::getHandSize() const {
    return hand.size();
}

const std::vector<Card>& Player::getHand() const {
    return hand;
}

cardColor Player::chooseBestColor(const Card& topCard) const {
    map<cardColor, int> colorCount;
    for (const auto& card : hand) {
        if (card.color != WILDS) {
            colorCount[card.color]++;
        }
    }
    //choose the color with the most cards
    cardColor bestColor = REDS;
    int maxCount = 0;
    for (const auto& pair : colorCount) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            bestColor = pair.first;
        }
    }
    return bestColor;

}

//helps to get the value of each card
CardScore LPOptimizer::calcCard(const Card& card, const Card& topCard, int handSize, int opponentHandSize) {
    CardScore score;

    //gets the attacking value/getting rid of cards
    score.attackingValue = calcAttackingValue(card, handSize);

    //gets the defensive value of the cards that are good to keep
    score.defendingValue = calcDefendingValue(card, opponentHandSize);

    //it gets the combined strategic value of the card
    score.strategicValue = 0.6 * score.attackingValue +
        0.4 * score.defendingValue;

    return score;
}

//private:
double LPOptimizer::calcAttackingValue(const Card& card, int handSize) {
    double value = 0.0;

    //the base values for different card types
    switch (card.type) {
    case WILD:
        value = 0.5;
        break;
    case WILD_DRAW_FOUR:
        value = 0.7;
        break;
    case DRAW_TWO:
        value = 0.4;
        break;
    case REVERSE:
        value = 0.35;
        break;
    case SKIP:
        value = 0.35;
        break;
    default:
        value = 0.2 + (card.type * 0.01);
        break;
    }
    //adjust based on hand size
    if (handSize <= 3) {
        value *= 1.5;
    }
    else if (handSize > 7) {
        if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR) {
            value *= 1.3;
        }
    }
    return value;
}

double LPOptimizer::calcDefendingValue(const Card& card, int opponentHandSize) {
    double value = 0.0;

    //gets/decides if it should save the card
    if (card.type == WILD || card.type == WILD_DRAW_FOUR) {
        value = 0.8;
    }
    else if (card.type == DRAW_TWO) {
        value = 0.6;
    }
    else {
        value = 0.2;
    }

    //test if the enemy is close to winning
    if (opponentHandSize <= 2) {
        if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR) {
            value *= 1.5;
        }
    }
    return value;
}

Deck::Deck()
{
    rng = std::mt19937(std::random_device{}());
}

void Deck::initinialize() {
    cards.clear();

    //adding cards 0-9 for each of the colors
    for (int color = REDS; color <= YELLOWS; color++) {
        //one 0 per color
        cards.push_back({ static_cast<cardColor>(color), ZERO });

        //two of each of the numbers per color
        for (int num = ONE; num <= NINE; num++) {
            cards.push_back({ static_cast<cardColor>(color), static_cast<cardValue>(num) });
            cards.push_back({ static_cast<cardColor>(color), static_cast<cardValue>(num) });
        }

        //it adds two of each of the cards to the deck
        cards.push_back({ static_cast<cardColor>(color), SKIP });
        cards.push_back({ static_cast<cardColor>(color), SKIP });
        cards.push_back({ static_cast<cardColor>(color), REVERSE });
        cards.push_back({ static_cast<cardColor>(color), REVERSE });
        cards.push_back({ static_cast<cardColor>(color), DRAW_TWO });
        cards.push_back({ static_cast<cardColor>(color), DRAW_TWO });
    }

    //adds the wild cards to th game
    for (int i = 0; i < 4; i++) {
        cards.push_back({ WILDS, WILD });
        cards.push_back({ WILDS, WILD_DRAW_FOUR });
    }

    shuffle();
}

void Deck::shuffle() {
    std::shuffle(std::begin(cards), std::end(cards), rng);
}

Card Deck::draw() {
    Card drawn;
    std::uniform_int_distribution dist(0, 14);
    drawn.type = cardValue(dist(rng));
    if (drawn.isWild())
        drawn.color = WILDS;
    else
    {
        uniform_int_distribution colDist(0, 3);
        drawn.color = cardColor(colDist(rng));
    }
    return drawn;
}

bool Deck::isEmpty() const {
    return cards.empty();
}

int Deck::size() const {
    return cards.size();
}

void Deck::addCard(const Card& card) {
    cards.push_back(card);
}

Game::Game() : currentPlayer(0), clockwise(true), drawStack(0), state(GAME_MENU), winner(-1) {}

void Game::initialize(int numPlayers, int numAI) {
    players.clear();
    deck.initinialize();
    discardPile = Deck();
    currentPlayer = 0;
    clockwise = true;
    drawStack = 0;
    state = GAME_PLAYING;
    winner = -1;

    //creates player
    for (int i = 0; i < numPlayers - numAI; i++) {
        players.push_back(Player(false, "Player" + to_string(i + 1)));
    }
    for (int i = 0; i < numAI; i++) {
        players.push_back(Player(true, "AI" + to_string(i + 1)));
    }

    //deals 7 cards to each player
    for (int i = 0; i < 7; i++) {
        for (auto& player : players) {
            player.addCard(deck.draw());
        }
    }

    //draw the intatl top card
    do {
        topCard = deck.draw();
    } while (topCard.isWild() || topCard.isActionCard());
}

void Game::playTurn(int cardIndex) {
    Player& player = players[currentPlayer];

    //if they are drawing a card
    if (cardIndex == -1) {
        if (drawStack > 0) {
            drawCards(currentPlayer, drawStack);
            drawStack = 0;
            nextPlayer();
        }
        else {
            player.addCard(deck.draw());

            //check if the card can play
            const Card& drawnCard = player.getHand().back();
            if (!drawnCard.matches(topCard)) {
                nextPlayer();
            }
        }
        return;
    }
    //playing the cards
    if (cardIndex < 0 || cardIndex >= player.getHandSize()) {
        return;
    }
    const Card& cardToPlay = player.getHand()[cardIndex];

    if (!cardToPlay.matches(topCard)) {
        return;
    }

    if (drawStack > 0) {
        if (cardToPlay.type != DRAW_TWO && cardToPlay.type != WILD_DRAW_FOUR) {
            drawCards(currentPlayer, drawStack);
            drawStack = 0;
            nextPlayer();
            return;
        }
    }
    Card played = player.playCard(cardIndex);
    discardPile.addCard(topCard);
    topCard = played;

    //handles the wild cards
    if (played.isWild()) {
        if (player.getISAI()) {
            topCard.colorChange(player.chooseBestColor());
        }
        else {
            state = WAITING_FOR_COLOR_CHOICE;
            return;
        }
    }

    //handeling the action cards
    switch (played.type) {
    case SKIP:
        nextPlayer();
        break;
    case REVERSE:
        reverseDirection();
        break;
    case DRAW_TWO:
        drawStack += 2;
        break;
    case WILD_DRAW_FOUR:
        drawStack += 4;
        break;
    default:
        break;
    }

    //check for the winner
    if (player.getHandSize() == 0) {
        winner = currentPlayer;
        state = GAME_OVER;
        return;
    }
    nextPlayer();
}

void Game::nextPlayer() {
    if (clockwise) {
        currentPlayer = (currentPlayer + 1) % players.size();
    }
    else {
        currentPlayer = (currentPlayer - 1 + players.size()) % players.size();
    }

    //AI turn
    if (state == GAME_PLAYING && players[currentPlayer].getISAI()) {
        int nextPlayerIndex = clockwise ?
            (currentPlayer + 1) % players.size() : (currentPlayer - 1 + players.size()) % players.size();
        int opponentHandSize = players[nextPlayerIndex].getHandSize();
        int cardToPlay = players[currentPlayer].chooseOptimalCardMultiTurn(topCard, opponentHandSize);

        if (cardToPlay != -1) {
            playTurn(-1);
        }
        else {
            playTurn(cardToPlay);
        }
    }
}

void Game::reverseDirection() {
    clockwise = !clockwise;
    if (players.size() == 2) {
        nextPlayer();
    }
}

void Game::skipPlayer() {
    nextPlayer();
}

void Game::drawCards(int playerIndex, int count) {
    for (int i = 0; i < count; i++) {
        if (deck.isEmpty()) {
            while (discardPile.size() > 0) {
                deck.addCard(discardPile.draw());
            }
            deck.shuffle();
        }
        players[playerIndex].addCard(deck.draw());
    }
}

bool Game::checkWinner() {
    for (size_t i = 0; i < players.size(); i++) {
        if (players[i].getHandSize() == 0) {
            winner = i;
            state = GAME_OVER;
            return true;
        }
    }
    return false;
}

void Game::chooseColorForWild(cardColor color) {
    topCard.colorChange(color);
    state = GAME_PLAYING;

    if (topCard.type == WILD_DRAW_FOUR || topCard.type == DRAW_TWO) {

    }
    nextPlayer();
}

Color getCardColor(cardColor color) {
    switch (color) {
    case REDS: return RED;
    case BLUES: return BLUE;
    case GREENS: return GREEN;
    case YELLOWS: return YELLOW;
    case WILDS: return BLACK;
    default: return WHITE;
    }
}

string getCardValueString(cardValue type) {
    switch (type) {
    case ZERO: return "0";
    case ONE: return "1";
    case TWO: return "2";
    case THREE: return "3";
    case FOUR: return "4";
    case FIVE: return "5";
    case SIX: return "6";
    case SEVEN: return "7";
    case EIGHT: return "8";
    case NINE: return "9";
    case SKIP: return "SKIP";
    case DRAW_TWO: return "+2";
    case REVERSE: return "REVERSE";
    case WILD_DRAW_FOUR: return "+4";
    case WILD: return "Wild";
    default: return "?";

    }
}