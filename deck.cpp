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
#include <algorithm>
#include <limits>

using namespace std;

// ------ PLAYER ------------ PLAYER ------------ PLAYER ------------ PLAYER ------------ PLAYER ------------ PLAYER ------

//it creates a new player with the AI flag and the player name
Player::Player(bool ai, const std::string& playerName) : isAI(ai), name(playerName) {}

//it checks if the player has any cards that can be played on the top card
bool Player::canPlay(const Card& topCard) const {
    //it loops through all the cards in the players hand
    for (const auto& card : hand) {
        //it checks if the card matches the top card by color or value
        if (card.matches(topCard)) {
            return true;
        }
    }
    return false; //it returns false if no playable cards are found
}

//it updates the opponent model based on what the opponent played or drew
void Player::updateOpponentModel(const Card& playedCard, bool opponentDrew) {
    opponentModel.totalTurnsObserved++;

    if (opponentDrew) {
        //it increments turns without playing
        opponentModel.turnsWithoutPlaying++;

        //it assumes they avoided all colors if they drew
        for (int c = REDS; c <= YELLOWS; c++) {
            opponentModel.colorsAvoided[static_cast<cardColor>(c)]++;
        }
    } else {
        //it resets the counter since they played
        opponentModel.turnsWithoutPlaying = 0;

        //it records the color they played
        if (playedCard.color != WILDS) {
            opponentModel.colorsPlayed[playedCard.color]++;
        }
    }
}

//it calculates the utility value of a card based on the game state for the AI to use
double LPOptimizer::getCardUtility(const Card& card, int handSize, int opponentHandSize) {
    double utility = 0.0;

    //it assigns base utility values to each card type for the AI to know which cards are better
    switch (card.type) {
        case WILD_DRAW_FOUR:
            utility = 10.0; //it is the most powerful card so it gets the highest value
            break;
        case WILD:
            utility = 8.0; //it is very useful for changing colors
            break;
        case DRAW_TWO:
            utility = 7.0; //it is a strong offensive card
            break;
        case SKIP:
            utility = 6.0; //it is a tactical card that skips the opponent
            break;
        case REVERSE:
            utility = 6.0; //it is a tactical card that changes direction
            break;
        default: //it is for the number cards 0-9
            //it gets the utility based on the cards value so higher numbers are slightly better
            utility = 2.0 + card.type;
            break;
    }

    //it adjusts the utility when the AI is close to winning with 2 or fewer cards
    if (handSize <= 2) {
        utility += 5.0; //it boosts all cards because the AI just wants to get rid of cards
    }

    //it adjusts the utility when the opponent is close to winning with 2 or fewer cards
    if (opponentHandSize <= 2) {
        //it prioritizes cards that can disrupt the opponent from winning
        if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR ||
            card.type == SKIP) {
            utility += 8.0; //it gives a significant boost to defensive cards
        }
    }

    //it makes wild cards more valuable when the AI has many cards because they provide flexibility
    if (card.isWild() && handSize > 5) {
        utility += 3.0; //it tells the AI to hold onto wilds when it has options
    }

    return utility;
}

//it calculates how versatile a card is based on how many situations it can be played in
double LPOptimizer::getCardVersatility(const Card& card, const std::vector<Card>& hand) {
    double versatility = 0.0;

    //it makes wild cards extremely versatile since they can always be played
    if (card.isWild()) {
        return 10.0;
    }

    //it counts how many other cards in hand share this cards color
    int sameColorCount = 0;
    int sameValueCount = 0;

    for (const auto& otherCard : hand) {
        if (otherCard.color == card.color && otherCard.color != WILDS) {
            sameColorCount++;
        }
        if (otherCard.type == card.type) {
            sameValueCount++;
        }
    }

    //it gives higher versatility to cards that connect well with the hand
    versatility = 2.0 + sameColorCount * 0.5 + sameValueCount * 0.3;

    return versatility;
}

//it calculates the probability that the opponent can block this play
double LPOptimizer::getBlockingProbability(const Card& cardToPlay, const OpponentModel& model) {
    //it estimates how likely opponent has a matching card

    if (cardToPlay.isWild()) {
        return 0.0; //it cant be blocked since wild changes color
    }

    //it gets the probability opponent has this color
    double colorProb = model.getProbabilityHasColor(cardToPlay.color);

    //it adjusts based on how many turns they havent played
    if (model.turnsWithoutPlaying > 2) {
        colorProb *= 0.5; //it reduces probability if theyve been drawing
    }

    return colorProb;
}

//it evaluates a sequence of cards to see how good it would be to play them in order
double LPOptimizer::evaluateSequence(const std::vector<Card>& hand,
                                      const std::vector<int>& sequence,
                                      const Card& topCard, int opponentHandSize,
                                      const OpponentModel& opponentModel) {
    if (sequence.empty()) return -1000.0;

    double totalUtility = 0.0;
    Card currentTop = topCard;
    int remainingHandSize = hand.size();

    //it simulates playing each card in the sequence
    for (size_t i = 0; i < sequence.size(); i++) {
        int cardIdx = sequence[i];
        const Card& card = hand[cardIdx];

        //it checks if this card can actually be played
        if (!card.matches(currentTop)) {
            return -1000.0; //it penalizes invalid sequences heavily
        }

        //it gets the base utility of playing this card
        double cardUtil = getCardUtility(card, remainingHandSize, opponentHandSize);

        //it adds versatility bonus for early cards in sequence
        double versatility = getCardVersatility(card, hand);
        double versatilityBonus = versatility * (1.0 / (i + 1)); //it decreases over turns

        //it calculates blocking probability
        double blockProb = getBlockingProbability(card, opponentModel);
        double blockPenalty = blockProb * 2.0; //it penalizes cards that might get blocked

        //it adds point value consideration - play high value cards early
        double pointValue = card.getPointValue();
        double pointBonus = pointValue * 0.1 * (sequence.size() - i); //it rewards playing high cards first

        //it combines all factors
        totalUtility += cardUtil + versatilityBonus - blockPenalty + pointBonus;

        //it updates state for next iteration
        currentTop = card;
        remainingHandSize--;
    }

    //it adds bonus for reducing hand size
    totalUtility += (sequence.size() * 5.0);

    return totalUtility;
}

//it creates a plan for the next N turns using multi-turn planning
TurnPlan LPOptimizer::planNextTurns(const std::vector<Card>& hand, const Card& topCard,
                                     int opponentHandSize, const OpponentModel& opponentModel,
                                     int numTurns) {
    TurnPlan bestPlan;
    bestPlan.expectedUtility = -std::numeric_limits<double>::infinity();

    //it finds all playable cards for the first turn
    std::vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }

    if (playableIndices.empty()) {
        bestPlan.expectedHandSize = hand.size() + 1; //it will draw a card
        return bestPlan;
    }

    //it limits turns to plan based on hand size
    numTurns = min(numTurns, (int)hand.size());
    numTurns = min(numTurns, 3); //it caps at 3 turns to keep it fast

    //it tries different sequences of cards
    if (numTurns == 1) {
        //it just picks the best single card
        for (int idx : playableIndices) {
            std::vector<int> seq = {idx};
            double utility = evaluateSequence(hand, seq, topCard, opponentHandSize, opponentModel);

            if (utility > bestPlan.expectedUtility) {
                bestPlan.cardSequence = seq;
                bestPlan.expectedUtility = utility;
                bestPlan.expectedHandSize = hand.size() - 1;
            }
        }
    } else if (numTurns == 2) {
        //it tries all pairs of cards
        for (int idx1 : playableIndices) {
            Card firstCard = hand[idx1];

            //it finds cards that could be played after the first card
            for (int idx2 = 0; idx2 < hand.size(); idx2++) {
                if (idx2 == idx1) continue;

                if (hand[idx2].matches(firstCard)) {
                    std::vector<int> seq = {idx1, idx2};
                    double utility = evaluateSequence(hand, seq, topCard, opponentHandSize, opponentModel);

                    if (utility > bestPlan.expectedUtility) {
                        bestPlan.cardSequence = seq;
                        bestPlan.expectedUtility = utility;
                        bestPlan.expectedHandSize = hand.size() - 2;
                    }
                }
            }

            //it also considers just playing the first card
            std::vector<int> seq = {idx1};
            double utility = evaluateSequence(hand, seq, topCard, opponentHandSize, opponentModel);

            if (utility > bestPlan.expectedUtility) {
                bestPlan.cardSequence = seq;
                bestPlan.expectedUtility = utility;
                bestPlan.expectedHandSize = hand.size() - 1;
            }
        }
    } else {
        //it tries triple sequences for 3 turns
        for (int idx1 : playableIndices) {
            Card firstCard = hand[idx1];

            for (int idx2 = 0; idx2 < hand.size(); idx2++) {
                if (idx2 == idx1) continue;
                if (!hand[idx2].matches(firstCard)) continue;

                Card secondCard = hand[idx2];

                for (int idx3 = 0; idx3 < hand.size(); idx3++) {
                    if (idx3 == idx1 || idx3 == idx2) continue;
                    if (!hand[idx3].matches(secondCard)) continue;

                    std::vector<int> seq = {idx1, idx2, idx3};
                    double utility = evaluateSequence(hand, seq, topCard, opponentHandSize, opponentModel);

                    if (utility > bestPlan.expectedUtility) {
                        bestPlan.cardSequence = seq;
                        bestPlan.expectedUtility = utility;
                        bestPlan.expectedHandSize = hand.size() - 3;
                    }
                }
            }
        }
    }

    return bestPlan;
}

//it uses advanced linear programming with multi-turn planning and opponent modeling
int LPOptimizer::solveLPMultiTurn(const std::vector<Card>& hand, const Card& topCard,
                                   int handSize, int opponentHandSize,
                                   const OpponentModel& opponentModel, int turnsAhead) {
    //it creates a multi-turn plan
    TurnPlan plan = planNextTurns(hand, topCard, opponentHandSize, opponentModel, turnsAhead);

    //it returns the first card in the best sequence
    if (!plan.cardSequence.empty()) {
        return plan.cardSequence[0];
    }

    return -1; //it means draw a card
}

//it uses linear programming to determine the optimal card to play for the AI
int LPOptimizer::solveLPForBestCard(const std::vector<Card>& hand, const Card& topCard, int handSize, int opponentHandSize)
{
    //it finds all the cards that can legally be played
    std::vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }

    //it returns -1 if no valid moves are available so the AI must draw a card
    if (playableIndices.empty()) {
        return -1;
    }

    //it returns the only option if there is only one playable card
    if (playableIndices.size() == 1) {
        return playableIndices[0];
    }

    //it sets up the linear programming problem using GLPK
    glp_prob *lp;
    lp = glp_create_prob(); //it creates a new LP problem instance
    glp_set_prob_name(lp, "UNO_Card_Selection"); //it names the problem for debugging
    glp_set_obj_dir(lp, GLP_MAX); //it tells the solver to maximize utility

    int numPlayable = playableIndices.size();

    //it creates binary decision variables one for each playable card
    //each variable x[i] will be either 0 for dont play or 1 for play
    glp_add_cols(lp, numPlayable);

    for (int i = 0; i < numPlayable; i++) {
        //it sets up the variable name for debugging purposes
        char var_name[50];
        snprintf(var_name, sizeof(var_name), "play_card_%d", i);
        glp_set_col_name(lp, i + 1, var_name);

        //it makes this a binary variable so it can only be 0 or 1
        glp_set_col_kind(lp, i + 1, GLP_BV);

        //it sets the objective coefficient which is how much utility this card provides
        int cardIdx = playableIndices[i];
        double utility = getCardUtility(hand[cardIdx], handSize, opponentHandSize);
        glp_set_obj_coef(lp, i + 1, utility);
    }

    //it adds the constraint that the AI must play exactly one card
    glp_add_rows(lp, 1); //it creates one constraint row
    glp_set_row_name(lp, 1, "play_one_card");
    //it makes the sum equal exactly 1.0 so only one card is played
    glp_set_row_bnds(lp, 1, GLP_FX, 1.0, 1.0);

    //it sets up the constraint coefficients for the linear program
    //these arrays are 1-indexed as required by GLPK
    int *indices = new int[numPlayable + 1];
    double *values = new double[numPlayable + 1];

    for (int i = 0; i < numPlayable; i++) {
        indices[i + 1] = i + 1; //it sets the variable index
        values[i + 1] = 1.0;     //it makes each variable contribute 1 to the sum
    }

    //it installs the constraint so the sum of all variables equals 1
    glp_set_mat_row(lp, 1, numPlayable, indices, values);

    //it solves the optimization problem to find the best card
    glp_term_out(GLP_OFF); //it suppresses solver output messages
    glp_simplex(lp, NULL); //it first solves as continuous LP
    glp_intopt(lp, NULL);  //it then solves as integer program with binary values

    //it extracts the solution to find which card was selected
    int bestCardIndex = -1;
    for (int i = 0; i < numPlayable; i++) {
        double value = glp_mip_col_val(lp, i + 1); //it gets the variable value
        if (value > 0.5) { //it should be exactly 1 but uses 0.5 for safety
            bestCardIndex = playableIndices[i];
            break; //it found the selected card
        }
    }

    //it cleans up the memory to prevent memory leaks
    delete[] indices;
    delete[] values;
    glp_delete_prob(lp);

    return bestCardIndex;
}

//it lets the AI choose the best card to play using strategic evaluation
int Player::chooseOptimalCard(const Card& topCard, int opponentHandSize) {
    //it only works for AI players
    if (!isAI) {
        return -1;
    }

    //it finds all the cards that can legally be played
    vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }

    //it returns -1 if there are no playable cards
    if (playableIndices.empty()) {
        return -1;
    }

    //it evaluates each playable card and chooses the best one
    int bestIndex = -1;
    double bestScore = -1.0;

    for (int idx : playableIndices) {
        //it calculates the strategic value of this card
        CardScore score = LPOptimizer::calcCard(hand[idx], topCard,
            hand.size(), opponentHandSize);

        //it keeps track of the highest scoring card
        if (score.strategicValue > bestScore) {
            bestScore = score.strategicValue;
            bestIndex = idx;
        }
    }
    return bestIndex;
}

//it uses multi-turn planning to choose the optimal card with the full LP solver
int Player::chooseOptimalCardMultiTurn(const Card& topCard, int opponentHandSize, int turnsToAnalyze) const {
    //it only works for AI players
    if (!isAI) {
        return -1;
    }

    //it delegates to the full linear programming solver for deeper analysis
    return LPOptimizer::solveLPForBestCard(hand, topCard, hand.size(), opponentHandSize);
}

//it uses advanced LP with multi-turn planning and opponent modeling
int Player::chooseOptimalCardAdvanced(const Card& topCard, int opponentHandSize, int turnsAhead) const {
    //it only works for AI players
    if (!isAI) {
        return -1;
    }

    //it uses the advanced multi-turn LP solver with opponent modeling
    return LPOptimizer::solveLPMultiTurn(hand, topCard, hand.size(),
                                          opponentHandSize, opponentModel, turnsAhead);
}

//it removes and returns a card from the players hand
Card Player::playCard(int index) {
    Card played = hand[index];
    hand.erase(hand.begin() + index); //it removes the card from the hand
    return played;
}

//it adds a card to the players hand when they draw
void Player::addCard(const Card& card) {
    hand.push_back(card);
}

//it returns the number of cards currently in the players hand
int Player::getHandSize() const {
    return hand.size();
}

//it returns a reference to the players hand for viewing
const std::vector<Card>& Player::getHand() const {
    return hand;
}

//it lets the AI choose the best color when playing a wild card
cardColor Player::chooseBestColor(const Card& topCard) const {
    //it counts how many cards of each color the AI has
    map<cardColor, int> colorCount;
    for (const auto& card : hand) {
        if (card.color != WILDS) { //it doesnt count wild cards
            colorCount[card.color]++;
        }
    }

    //it finds the color with the most cards
    cardColor bestColor = REDS; //it defaults to red if no cards
    int maxCount = 0;
    for (const auto& pair : colorCount) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            bestColor = pair.first;
        }
    }
    return bestColor;
}

//performs a radix sort on this player's hand
//sorts primarily by color, then by number
void Player::sortHand()
{
    //it creates 15 queues for sorting the cards into buckets
    vector<queue<Card>> sortQueues;
    sortQueues.resize(15, queue<Card>());

    //it performs two passes first by type then by color
    for (int sortByCol = 0; sortByCol < 2; sortByCol++)
    {
        //it distributes cards into appropriate buckets
        for (Card current : hand)
        {
            if (sortByCol)
                sortQueues[current.color].push(current); //it sorts by color
            else
                sortQueues[current.type].push(current);  //it sorts by type
        }

        //it clears the hand to rebuild it in sorted order
        hand.clear();

        //it collects cards back from buckets in order
        for (queue<Card> &bucket : sortQueues)
        {
            while (!bucket.empty())
            {
                hand.push_back(bucket.front());
                bucket.pop();
            }
        }
    }

    //with help from https://visualgo.net/en/sorting
    //and https://en.wikipedia.org/wiki/Radix_sort
}

// ---- LINEAR PROG -------- LINEAR PROG -------- LINEAR PROG -------- LINEAR PROG -------- LINEAR PROG -------- LINEAR PROG ----

//it helps to get the value of each card
//it keeps the old calcCard for compatibility but marks it as legacy
CardScore LPOptimizer::calcCard(const Card& card, const Card& topCard, int handSize, int opponentHandSize) {
    CardScore score;

    //it is the legacy scoring that is used for reference
    score.attackingValue = calcAttackingValue(card, handSize);
    score.defendingValue = calcDefendingValue(card, opponentHandSize);
    score.strategicValue = 0.6 * score.attackingValue + 0.4 * score.defendingValue;

    //it is the LP-based value
    score.lpOptimalValue = getCardUtility(card, handSize, opponentHandSize);

    return score;
}

//it calculates how good a card is for attacking and advancing toward victory
double LPOptimizer::calcAttackingValue(const Card& card, int handSize) {
    double value = 0.0;

    //it assigns base values for each card type
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

    //it modifies the value based on game state
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

//it calculates how good a card is for defending and disrupting the opponent
double LPOptimizer::calcDefendingValue(const Card& card, int opponentHandSize) {
    double value = 0.0;

    //it makes wild cards excellent for defense
    if (card.type == WILD || card.type == WILD_DRAW_FOUR) {
        value = 0.8;
    }
    else if (card.type == DRAW_TWO) {
        value = 0.6;
    }
    else {
        value = 0.2;
    }

    //it boosts value if opponent is close to winning
    if (opponentHandSize <= 2) {
        if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR) {
            value *= 1.5;
        }
    }
    return value;
}

// ------- DECK -------------- DECK -------------- DECK -------------- DECK -------------- DECK -------------- DECK -------

//it initializes the random number generator for shuffling
Deck::Deck()
{
    rng = std::mt19937(std::random_device{}());
}

//it creates a complete standard UNO deck with all 108 cards
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

//it randomizes the order of all cards in the deck
void Deck::shuffle() {
    std::shuffle(std::begin(cards), std::end(cards), rng);
}

//it draws a random card when the deck is conceptually infinite
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

//it returns true if there are no cards left in the deck
bool Deck::isEmpty() const {
    return cards.empty();
}

//it returns the number of cards currently in the deck
int Deck::size() const {
    return cards.size();
}

//it adds a card to the deck typically from the discard pile
void Deck::addCard(const Card& card) {
    cards.push_back(card);
}

// ------- GAME -------------- GAME -------------- GAME -------------- GAME -------------- GAME -------------- GAME -------

//it initializes a new game with default values
Game::Game() : currentPlayer(0), clockwise(true), drawStack(0), state(GAME_MENU), winner(-1) {}

//it sets up a new game with the specified number of players
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

//it executes a turn for the current player
void Game::playTurn(int cardIndex) {
    Player& player = players[currentPlayer];

    //it gets the next player index for opponent modeling
    int nextPlayerIdx = clockwise ?
        (currentPlayer + 1) % players.size() :
        (currentPlayer - 1 + players.size()) % players.size();

    //if they are drawing a card
    if (cardIndex == -1) {
        //it updates opponent model that this player drew instead of playing
        if (player.getISAI() && players.size() > 1) {
            players[nextPlayerIdx].updateOpponentModel(topCard, true);
        }

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

    //it updates opponent model that this player played this card
    if (player.getISAI() && players.size() > 1) {
        players[nextPlayerIdx].updateOpponentModel(played, false);
    }

    discardPile.addCard(topCard);
    topCard = played;
    lastPlayedCard = played;

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

//it advances to the next player based on current direction
void Game::nextPlayer() {
    if (clockwise) {
        currentPlayer = (currentPlayer + 1) % players.size();
    }
    else {
        currentPlayer = (currentPlayer - 1 + players.size()) % players.size();
    }
}

//it reverses the direction of play
void Game::reverseDirection() {
    clockwise = !clockwise;
    if (players.size() == 2) {
        nextPlayer();
    }
}

//it skips the current player
void Game::skipPlayer() {
    nextPlayer();
}

//it forces a player to draw multiple cards
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

//it checks if any player has won the game
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

//it handles the color selection after a wild card is played
void Game::chooseColorForWild(cardColor color) {
    topCard.colorChange(color);
    state = GAME_PLAYING;

    if (topCard.type == WILD_DRAW_FOUR || topCard.type == DRAW_TWO) {

    }
    nextPlayer();
}