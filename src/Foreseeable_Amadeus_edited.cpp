#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <random>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#include "helpers.h"
#include "pacman.h"

int start_time;

namespace MCTS
{
    typedef Direction Move;

    struct Board
    {
       public:
        int height, width;
        int remain;
        int generatorCount;
        int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT;
        GridStaticType fieldStatic[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        GridContentType fieldContent[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        int generatorTurnLeft;  // 多少回合后产生豆子
        int aliveCount;         // 有多少玩家存活
        int smallFruitCount;
        int turnID;
        int currentPlayer;
        vector<double> score;
        FieldProp generators[MAX_GENERATOR_COUNT];  // 有哪些豆子产生器
        Player players[MAX_PLAYER_COUNT];           // 有哪些玩家

        Direction actions[MAX_PLAYER_COUNT];

        vector<Move> getMoves()
        {
            vector<Move> moves;
            for (Direction d = up; d < 4; ++d)
                if (ActionValid(currentPlayer, d)) moves.push_back(d);
            if (moves.size() == 0) moves.push_back(stay);
            return moves;
        }

        inline bool ActionValid(int playerID, Direction &dir) const
        {
            if (dir == stay) return true;
            const Player &p = players[playerID];
            const GridStaticType &s = fieldStatic[p.row][p.col];
            return dir >= -1 && dir < 4 && !(s & direction2OpposingWall[dir]);
        }

        void makeMove(Move m)
        {
            if (remain == 0)
            {
                remain = MAX_PLAYER_COUNT;
                NextTurn();
            }
            else
            {
                actions[currentPlayer++] = m;
                if (currentPlayer == MAX_PLAYER_COUNT) currentPlayer = 0;
                remain--;
            }
        }

        int getCurrentPlayer()
        {
            return currentPlayer;
        }

        int getQuantityOfPlayers()
        {
            return 4;
        }

        bool gameOver()
        {
            return turnID >= 100 || aliveCount <= 1;
        }

        vector<double> getScore()
        {
            int rank[4] = {0, 1, 2, 3};
            for (int i = 3; i >= 0; i--)
                for (int j = 0; j < i; j++)
                    if (players[rank[j]].strength >
                        players[rank[j + 1]].strength)
                        std::swap(rank[j], rank[j + 1]);

            // assert(remain == 4);
            double sum = 0;
            score.resize(MAX_PLAYER_COUNT);
            score[0] = players[0].strength;
            sum += score[0];
            score[1] = players[1].strength;
            sum += score[1];
            score[2] = players[2].strength;
            sum += score[2];
            score[3] = players[3].strength;
            sum += score[3];
            for (int i = 0; i < MAX_PLAYER_COUNT; i++) score[i] /= sum;
            int playerrank[4];
            playerrank[rank[0]] = 0;
            for (int i = 1; i < 4; i++)
                if (players[rank[i]].strength == players[rank[i - 1]].strength)
                    playerrank[rank[i]] = playerrank[rank[i - 1]];
                else
                    playerrank[rank[i]] = playerrank[rank[i - 1]] + 1;
            for (int i = 0; i < 4; i++) score[i] += playerrank[i] / 10.0;
            return score;
        }

        bool NextTurn()
        {
            int _, i, j;
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &p = players[_];
                if (!p.dead)
                {
                    Direction &action = actions[_];
                    if (action == stay) continue;
                    if (!ActionValid(_, action))
                    {
                        fieldContent[p.row][p.col] &= ~playerID2Mask[_];
                        p.strength = 0;
                        p.dead = true;
                        aliveCount--;
                    }
                    else
                    {
                        GridContentType target =
                            fieldContent[(p.row + dy[action] + height) % height]
                                        [(p.col + dx[action] + width) % width];
                        if (target & playerMask)
                            for (i = 0; i < MAX_PLAYER_COUNT; i++)
                                if (target & playerID2Mask[i] &&
                                    players[i].strength > p.strength)
                                    action = stay;
                    }
                }
            }
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead) continue;

                if (actions[_] == stay) continue;

                // 移动
                fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                _p.row = (_p.row + dy[actions[_]] + height) % height;
                _p.col = (_p.col + dx[actions[_]] + width) % width;
                fieldContent[_p.row][_p.col] |= playerID2Mask[_];
            }

            // 2. 玩家互殴
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead) continue;
                int player, containedCount = 0;
                int containedPlayers[MAX_PLAYER_COUNT];
                for (player = 0; player < MAX_PLAYER_COUNT; player++)
                    if (fieldContent[_p.row][_p.col] & playerID2Mask[player])
                        containedPlayers[containedCount++] = player;

                if (containedCount > 1)
                {
                    // NAIVE
                    for (i = 0; i < containedCount; i++)
                        for (j = 0; j < containedCount - i - 1; j++)
                            if (players[containedPlayers[j]].strength <
                                players[containedPlayers[j + 1]].strength)
                                swap(containedPlayers[j],
                                     containedPlayers[j + 1]);

                    int begin;
                    for (begin = 1; begin < containedCount; begin++)
                        if (players[containedPlayers[begin - 1]].strength >
                            players[containedPlayers[begin]].strength)
                            break;

                    // 这些玩家将会被杀死
                    int lootedStrength = 0;
                    for (i = begin; i < containedCount; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];

                        // 从格子上移走
                        fieldContent[p.row][p.col] &= ~playerID2Mask[id];
                        p.dead = true;
                        int drop = p.strength / 2;
                        lootedStrength += drop;
                        p.strength -= drop;
                        aliveCount--;
                    }

                    // 分配给其他玩家
                    int inc = lootedStrength / begin;
                    for (i = 0; i < begin; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];
                        p.strength += inc;
                    }
                }
            }

            // 3. 产生豆子
            if (--generatorTurnLeft == 0)
            {
                generatorTurnLeft = GENERATOR_INTERVAL;
                // NewFruits &fruits = newFruits[newFruitsCount++];
                // fruits.newFruitCount = 0;
                for (i = 0; i < generatorCount; i++)
                    for (Direction d = up; d < 8; ++d)
                    {
                        // 取余，穿过场地边界
                        int r = (generators[i].row + dy[d] + height) % height,
                            c = (generators[i].col + dx[d] + width) % width;
                        if (fieldStatic[r][c] & generator ||
                            fieldContent[r][c] & (smallFruit | largeFruit))
                            continue;
                        fieldContent[r][c] |= smallFruit;
                        // fruits.newFruits[fruits.newFruitCount].row = r;
                        // fruits.newFruits[fruits.newFruitCount++].col = c;
                        smallFruitCount++;
                    }
            }

            // 4. 吃掉豆子
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead) continue;

                GridContentType &content = fieldContent[_p.row][_p.col];

                // 只有在格子上只有自己的时候才能吃掉豆子
                if (content & playerMask & ~playerID2Mask[_]) continue;

                if (content & smallFruit)
                {
                    content &= ~smallFruit;
                    _p.strength++;
                    smallFruitCount--;
                }
                else if (content & largeFruit)
                {
                    content &= ~largeFruit;
                    if (_p.powerUpLeft == 0)
                    {
                        _p.strength += LARGE_FRUIT_ENHANCEMENT;
                    }
                    _p.powerUpLeft += LARGE_FRUIT_DURATION;
                }
            }

            // 5. 大豆回合减少
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead) continue;
                if (_p.powerUpLeft > 0 && --_p.powerUpLeft == 0)
                {
                    _p.strength -= LARGE_FRUIT_ENHANCEMENT;
                }
            }
            ++turnID;
            if (aliveCount <= 1)
            {
                for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
                    if (!players[_].dead)
                    {
                        players[_].strength += smallFruitCount;
                    }
                return false;
            }
            if (turnID >= 100) return false;
            return true;
        }

        Board(GameField &gameField, int _currentPlayer)
        {
            height = gameField.height;
            width = gameField.width;
            generatorCount = gameField.generatorCount;
            GENERATOR_INTERVAL = gameField.GENERATOR_INTERVAL;
            LARGE_FRUIT_DURATION = gameField.LARGE_FRUIT_DURATION;
            LARGE_FRUIT_ENHANCEMENT = gameField.LARGE_FRUIT_ENHANCEMENT;
            score.resize(MAX_PLAYER_COUNT);
            memcpy(fieldStatic, gameField.fieldStatic, sizeof(fieldStatic));
            memcpy(fieldContent, gameField.fieldContent, sizeof(fieldContent));
            generatorTurnLeft = gameField.generatorTurnLeft;
            aliveCount = gameField.aliveCount;
            smallFruitCount = gameField.smallFruitCount;
            turnID = gameField.turnID;
            memcpy(generators, gameField.generators, sizeof(generators));
            memcpy(players, gameField.players, sizeof(players));
            memcpy(actions, gameField.actions, sizeof(actions));
            currentPlayer = _currentPlayer;
            remain = MAX_PLAYER_COUNT;
        }
    };

    struct Node
    {
        double games;
        Move move;
        bool unvisitedChildren_constructed;
        vector<shared_ptr<Node>> unvisitedChildren;
        vector<shared_ptr<Node>> children;
        shared_ptr<Node> parent;
        int player;
        vector<double> score;
        int depth;

        /**
         * This creates the root node
         *
         * @param b
         */
        Node(Board b)
        {
            depth = 0;
            games = 0;
            parent = nullptr;
            children.clear();
            player = b.getCurrentPlayer();
            score.resize(b.getQuantityOfPlayers());
            unvisitedChildren_constructed = false;
        }

        /**
         * This creates non-root nodes
         *
         * @param b
         * @param m
         * @param prnt
         */
        Node(Board b, Move m, shared_ptr<Node> prnt)
        {
            games = 0;
            children.clear();
            parent = prnt;
            depth = parent->depth + 1;
            move = m;
            b.makeMove(m);
            player = b.getCurrentPlayer();
            score.resize(b.getQuantityOfPlayers());
            unvisitedChildren_constructed = false;
        }

        /**
         * Return the upper confidence bound of this state
         *
         * @param c
         *            typically sqrt(2). Increase to emphasize exploration.
         * Decrease
         *            to incr. exploitation
         * @param t
         * @return
         */
        double upperConfidenceBound(double c)
        {
            return score[parent->player] / games +
                   c * sqrt(log(parent->games + 1) / games);
        }

        /**
         * Update the tree with the new score.
         * @param scr
         */
        void backPropagateScore(vector<double> scr)
        {
            this->games++;
            for (int i = 0; i < scr.size(); i++) this->score[i] += scr[i];
            if (parent != nullptr) parent->backPropagateScore(scr);
        }
    };

#define nextInt(x) RandBetween(0, x)

    class MCTS
    {
       public:
        shared_ptr<Node> rootNode;
        double explorationConstant = sqrt(2.0);
        bool trackTime;  // display thinking time used
        int timeLimit;   // Set a time limit per move.

        /**
         * Run a UCT-MCTS simulation for a number of iterations.
         *
         * @param s
         * @param runs
         * @return
         */
        Move runMCTS(Board s, int runs)
        {
            rootNode = shared_ptr<Node>(new Node(s));

            // long startTime = System.nanoTime();
            int now_time;

            for (int i = 0;; i++)
            {
                select(s, rootNode);
                now_time = clock();
                // EDITED 0.96
                if ((double)(now_time - start_time) / CLOCKS_PER_SEC > 0.1)
                    break;
            }

            // long endTime = System.nanoTime();
            // if (this.trackTime)
            // System.out.println("Thinking time per move in milliseconds: " +
            // (endTime - startTime) / 1000000);

            return finalSelect(rootNode);
        }

        /**
         * This represents the select stage, or default policy, of the
         * algorithm.
         * Traverse down to the bottom of the tree using the selection strategy
         * until you find an unexpanded child node. Expand it. Run a random
         * playout.
         * Backpropagate results of the playout.
         *
         * @param node
         *            Node from which to start selection
         */
        void select(Board brd, shared_ptr<Node> node)
        {
            shared_ptr<Node> currentNode = node;
            Board &currentBoard = brd;
            while (true)
            {
                // Break procedure if end of tree
                if (currentBoard.gameOver())
                {
                    currentNode->backPropagateScore(currentBoard.getScore());
                    return;
                }
                if (!currentNode->unvisitedChildren_constructed)
                {
                    // currentNode.initializeUnexplored(currentBoard);
                    vector<Move> legalMoves = currentBoard.getMoves();
                    currentNode->unvisitedChildren.resize(legalMoves.size());
                    for (int i = 0; i < legalMoves.size(); i++)
                    {
                        shared_ptr<Node> tempState(
                            new Node(currentBoard, legalMoves[i], currentNode));
                        currentNode->unvisitedChildren[i] = tempState;
                    }
                    currentNode->unvisitedChildren_constructed = true;
                }

                if (!currentNode->unvisitedChildren.empty())
                {
                    // it picks a move at random from list of unvisited children
                    int sel = nextInt(currentNode->unvisitedChildren.size());
                    shared_ptr<Node> temp = currentNode->unvisitedChildren[sel];
                    currentNode->unvisitedChildren.erase(
                        currentNode->unvisitedChildren.begin() + sel);
                    currentNode->children.push_back(temp);
                    brd.makeMove(temp->move);
                    playout(temp, brd);
                    return;
                }
                else
                {
                    double bestValue = -1e9;
                    shared_ptr<Node> bestChild = nullptr;
                    double tempBest;
                    vector<shared_ptr<Node>> bestNodes;
                    for (auto s : currentNode->children)
                    {
                        // Pruned is only ever true if a branch has been pruned
                        // from the tree and that can only happen if bounds
                        // propagation mode is enabled.
                        tempBest = s->upperConfidenceBound(explorationConstant);
                        if (tempBest > bestValue)
                        {
                            bestNodes.clear();
                            bestNodes.push_back(s);
                            bestChild = s;
                            bestValue = tempBest;
                        }
                        else if (tempBest == bestValue)
                        {
                            // If we found an equal node
                            bestNodes.push_back(s);
                        }
                    }
                    if (currentNode == rootNode && bestChild == nullptr) return;
                    if (bestNodes.size() == 0)
                    {
                        for (auto s : currentNode->children)
                        {
                            tempBest =
                                s->upperConfidenceBound(explorationConstant);
                            if (tempBest > bestValue)
                            {
                                bestNodes.clear();
                                bestNodes.push_back(s);
                                bestChild = s;
                                bestValue = tempBest;
                            }
                            else if (tempBest == bestValue)
                            {
                                // If we found an equal node
                                bestNodes.push_back(s);
                            }
                        }
                    }
                    shared_ptr<Node> finalNode =
                        bestNodes[nextInt(bestNodes.size())];
                    currentNode = finalNode;
                    currentBoard.makeMove(finalNode->move);
                }
            }
        }

        /**
         * This is the final step of the algorithm, to pick the best move to
         * actually make.
         *
         * @param n
         *            this is the node whose children are considered
         * @return the best Move the algorithm can find
         */
        Move finalSelect(shared_ptr<Node> n)
        {
            double bestValue = -1e9;
            shared_ptr<Node> bestChild = nullptr;
            double tempBest;
            vector<shared_ptr<Node>> bestNodes;
            for (auto s : n->children)
            {
                tempBest = s->games;
                if (tempBest > bestValue)
                {
                    bestNodes.clear();
                    bestNodes.push_back(s);
                    bestValue = tempBest;
                }
                else if (tempBest == bestValue)
                {
                    // If we found an equal node
                    bestNodes.push_back(s);
                }
            }
            shared_ptr<Node> finalNode = bestNodes[nextInt(bestNodes.size())];
            return finalNode->move;
        }

        void playout(shared_ptr<Node> state, Board brd)
        {
            vector<Move> moves;
            Move mv;
            while (true)
            {
                if (brd.gameOver())
                {
                    state->backPropagateScore(brd.getScore());
                    return;
                }
                moves = brd.getMoves();
                mv = moves[nextInt(moves.size())];
                brd.makeMove(mv);
            }
        }

        /**
         * Sets the exploration constant for the algorithm. You will need to
         * find
         * the optimal value through testing. This can have a big impact on
         * performance. Default value is sqrt(2)
         *
         * @param exp
         */
        void setExplorationConstant(double exp = sqrt(2.0))
        {
            explorationConstant = exp;
        }
    };

    Move run(int myID, GameField &gameField, int it, double exp = sqrt(2.0))
    {
        MCTS player;
        player.setExplorationConstant(exp);
        Board brd(gameField, myID);
        Move m = player.runMCTS(brd, it);
        return m;
    }

    Direction solve(GameField &gameField, int playerID)
    {
        return run(playerID, gameField, 3000);
    }
}

using namespace MCTS;

int main()
{
    GameField gameField;
    string data, globalData;

    start_time = clock();
    int myID = gameField.ReadInput("input.txt", data, globalData);

    if (gameField.players[myID].dead)
    {
        gameField.WriteOutput((Direction)(-1), "DEAD", data, globalData);
        return 0;
    }

    auto maxD = solve(gameField, myID);

    gameField.DebugPrint();
    gameField.WriteOutput(maxD, "", data, globalData);
    return 0;
}
