#include "deck.h"
#include <glpk.h>
#include <map>
#include <raylib.h>
#include <string>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <random>
#include <queue>

using namespace std;

// ------ PLAYER ------------ PLAYER ------------ PLAYER ------------ PLAYER ------------ PLAYER ------------ PLAYER ------

Player::Player(bool ai, const std::string& playerName) : isAI(ai), name(playerName) {}

bool Player::canPlay(const Card& topCard) const {
    for (const auto& card : hand) {
        if (card.matches(topCard)) {
            return true;
        }
    }
    return false;
}

double LPOptimizer::getCardUtility(const Card& card, int handSize, int opponentHandSize) {
    double utility = 0.0;

    // Base utility by card type
    switch (card.type) {
        case WILD_DRAW_FOUR:
            utility = 10.0;
            break;
        case WILD:
            utility = 8.0;
            break;
        case DRAW_TWO:
            utility = 7.0;
            break;
        case SKIP:
            utility = 6.0;
            break;
        case REVERSE:
            utility = 6.0;
            break;
        default: //it is the value for the number cards
            utility = 2.0 + card.type;  //it sets the untitly vlaue of the card by getting the 0-9 and then gets values 2-11 accordinly
            break;
    }

    //it adjusts the utility based on the games state so that the ai can be better
    if (handSize <= 2) //it tests the games state of the players handsize and if it is grater than 2
    {
        //if hte handsize is 2 or less then it tell the ai that the it is close to winning prefer any card to get rid of
        utility += 5.0;
    }

    if (opponentHandSize <= 2) {
        //it checks if the opponent is close to winning and if it is then the ai prefers action cards to try and stop the player
        if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR ||
            card.type == SKIP) {
            utility += 8.0;
            }
    }

    //it tells the ai that wilds are more valuable when it has fewer matching cards so that the wilds hold more value to the ai
    if (card.isWild() && handSize > 5) {
        utility += 3.0;
    }

    return utility;
}

int LPOptimizer::solveLPForBestCard(const std::vector<Card>& hand, const Card& topCard, int handSize, int opponentHandSize)
{
    //it makes it so that the ai finds all playable cards in thier hand
    std::vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }

    if (playableIndices.empty())
    {
        return -1; //it returns -1 if the ai has no playable cards so that it knows that it has to draw
    }

    if (playableIndices.size() == 1) {
        return playableIndices[0];//it is what happens when the AI has only one option to do so that it does that option
    }

    //it is the set up for the Linear Programming problems
    glp_prob *lp;
    lp = glp_create_prob();
    glp_set_prob_name(lp, "UNO_Card_Selection");
    glp_set_obj_dir(lp, GLP_MAX);//it calculates the cards/objects maximize utility

    int numPlayable = playableIndices.size();

    //it adds one binary variable for each playable card and x[i] = 1 if it plays a card i, 0 otherwise
    glp_add_cols(lp, numPlayable);

    for (int i = 0; i < numPlayable; i++) {
        char var_name[50];
        snprintf(var_name, sizeof(var_name), "play_card_%d", i);
        glp_set_col_name(lp, i + 1, var_name);
        glp_set_col_kind(lp, i + 1, GLP_BV);// it is the binarys variable of 0 or 1

        //it sets the objective coefficient or the utility of playing this card for the AI so that they can better pick a card to play
        int cardIdx = playableIndices[i];
        double utility = getCardUtility(hand[cardIdx], handSize, opponentHandSize);
        glp_set_obj_coef(lp, i + 1, utility);
    }

    //it adds the constraint, that it can only play exactly 1 card
    glp_add_rows(lp, 1);
    glp_set_row_name(lp, 1, "play_one_card");
    glp_set_row_bnds(lp, 1, GLP_FX, 1.0, 1.0);// it makes it so that the sum must equal 1 to play

    //it is the set up of the constraint coefficients
    int *indices = new int[numPlayable + 1];//it sets the 1-indexed for GLPK to use it and work
    double *values = new double[numPlayable + 1];

    for (int i = 0; i < numPlayable; i++) {
        indices[i + 1] = i + 1;
        values[i + 1] = 1.0;//it makes it so that each variable contributes 1 to the sum
    }

    glp_set_mat_row(lp, 1, numPlayable, indices, values);

    glp_term_out(GLP_OFF);//it turns off solver messages that is included with glpk
    glp_simplex(lp, NULL);
    glp_intopt(lp, NULL);//it solves it as integer program

    //it gets the solution or in other words the best card to play
    int bestCardIndex = -1;
    for (int i = 0; i < numPlayable; i++) {
        double value = glp_mip_col_val(lp, i + 1);
        if (value > 0.5) { //it checks if this card was selected and that should be exactly 1
            bestCardIndex = playableIndices[i];
            break;
        }
    }

    //it is the clean up for the memory so that there is no memory leak
    delete[] indices;
    delete[] values;
    glp_delete_prob(lp);

    return bestCardIndex;
}

int Player::chooseOptimalCard(const Card& topCard, int opponentHandSize) {
    if (!isAI) {
        return -1;
    }
    //it finds all the playable cards
    vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }
    if (playableIndices.empty()) {
        return -1;
    }

    //it scores each playable card using the linear programming
    int bestIndex = -1;
    double bestScore = -1.0;

    for (int idx : playableIndices) {
        CardScore score = LPOptimizer::calcCard(hand[idx], topCard,
            hand.size(), opponentHandSize);
        //it applys the optimization
        if (score.strategicValue > bestScore) {
            bestScore = score.strategicValue;
            bestIndex = idx;
        }
    }
    return bestIndex;
}

int Player::chooseOptimalCardMultiTurn(const Card& topCard, int opponentHandSize, int turnsToAnalyze) const {
    if (!isAI)
    {
        return -1;
    }

    //it uses the Linear Programming solver
    return LPOptimizer::solveLPForBestCard(hand, topCard, hand.size(), opponentHandSize);
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
    //it chooses the color with the most cards
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

// performs a radix sort on this player's hand
// sorts primarily by color, then by number
void Player::sortHand()
{
    //std::cout << "Debug: Sorting the hand" << std::endl;

    vector<queue<Card>> sortQueues;
    sortQueues.resize(15, queue<Card>()); // create the queues

    for (int sortByCol = 0; sortByCol < 2; sortByCol++)
    {
        for (Card current : hand) // put all cards into their appropriate queues
        {
            if (sortByCol)
                sortQueues[current.color].push(current);
            else
                sortQueues[current.type].push(current);
        }

        // clear out the hand
        hand.clear();

        for (queue<Card> &bucket : sortQueues) // put the sorted cards back into the hand
        {
            while (!bucket.empty())
            {
                hand.push_back(bucket.front());
                bucket.pop();
            }
        }
    }
    

    //with help from https://visualgo.net/en/sorting
    // and https://en.wikipedia.org/wiki/Radix_sort
}

// ---- LINEAR PROG -------- LINEAR PROG -------- LINEAR PROG -------- LINEAR PROG -------- LINEAR PROG -------- LINEAR PROG ----

//it helps to get the value of each card
//it keeps the old calcCard for compatibility, but mark it as legacy
CardScore LPOptimizer::calcCard(const Card& card, const Card& topCard, int handSize, int opponentHandSize) {
    CardScore score;

    //it is the legacy scoring that is uses for reference
    score.attackingValue = calcAttackingValue(card, handSize);
    score.defendingValue = calcDefendingValue(card, opponentHandSize);
    score.strategicValue = 0.6 * score.attackingValue + 0.4 * score.defendingValue;

    //it is the LP-based value
    score.lpOptimalValue = getCardUtility(card, handSize, opponentHandSize);

    return score;
}

double LPOptimizer::calcAttackingValue(const Card& card, int handSize) {
    double value = 0.0;

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

    if (card.type == WILD || card.type == WILD_DRAW_FOUR) {
        value = 0.8;
    }
    else if (card.type == DRAW_TWO) {
        value = 0.6;
    }
    else {
        value = 0.2;
    }

    if (opponentHandSize <= 2) {
        if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR) {
            value *= 1.5;
        }
    }
    return value;
}

// ------- DECK -------------- DECK -------------- DECK -------------- DECK -------------- DECK -------------- DECK -------

Deck::Deck()
{
    rng = std::mt19937(std::random_device{}());
}

void Deck::initinialize() {
    cards.clear();

    //it is the adding of the cards 0-9 for each of the colors
    for (int color = REDS; color <= YELLOWS; color++) {
        //one 0 per color
        cards.push_back({ static_cast<cardColor>(color), ZERO });

        //it adds two of each of the numbers per color
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

    //it adds the wild cards to th game
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
    std::uniform_int_distribution<int> dist(0, 14);
    drawn.type = cardValue(dist(rng));
    if (drawn.isWild())
        drawn.color = WILDS;
    else
    {
        uniform_int_distribution<int> colDist(0, 3);
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

// ------- GAME -------------- GAME -------------- GAME -------------- GAME -------------- GAME -------------- GAME -------

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

    //it creates player
    for (int i = 0; i < numPlayers - numAI; i++) {
        players.push_back(Player(false, "Player" + to_string(i + 1)));
    }
    for (int i = 0; i < numAI; i++) {
        players.push_back(Player(true, "AI" + to_string(i + 1)));
    }

    //it deals 7 cards to each player
    for (int i = 0; i < 7; i++) {
        for (auto& player : players) {
            player.addCard(deck.draw());
        }
    }
    //it sort each player's hand
    for (auto& player : players) {
        player.sortHand();
    }

    //it draw the initial top card
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
            player.sortHand();
            drawStack = 0;
            nextPlayer();
        }
        else {
            player.addCard(deck.draw());
            player.sortHand();
            const Card& drawnCard = player.getHand().back();
            if (!drawnCard.matches(topCard)) {
                nextPlayer();
            }
        }
        return;
    }

    //it is for the playing the cards
    if (cardIndex < 0 || cardIndex >= player.getHandSize()) {
        return;
    }
    const Card& cardToPlay = player.getHand()[cardIndex];

    if (!cardToPlay.matches(topCard)) {
        return;
    }

    //it checks the draw stack before allowing the card to be played
    if (drawStack > 0) {
        if (cardToPlay.type != DRAW_TWO && cardToPlay.type != WILD_DRAW_FOUR) {
            return;  //it returns to not let the card to be played
        }
    }

    Card played = player.playCard(cardIndex);
    discardPile.addCard(topCard);
    topCard = played;

    //it checks for the winner
    if (player.getHandSize() == 0) {
        winner = currentPlayer;
        state = GAME_OVER; //changes the state of the game to game over
        return;
    }

    //it is the handling of the action cards
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

   
    if (played.isWild()) {
        if (player.getISAI()) {
            topCard.colorChange(player.chooseBestColor(topCard));
            nextPlayer(); //it is the AIs auto-chooses and moves to next player
        }
        else {
            state = WAITING_FOR_COLOR_CHOICE;
      
            return;
        }
    }
    else {
        //it is for non-wild cards, move to the next player
        nextPlayer();
    }
}

void Game::nextPlayer() {
    if (clockwise) {
        currentPlayer = (currentPlayer + 1) % players.size();
    }
    else {
        currentPlayer = (currentPlayer - 1 + players.size()) % players.size();
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