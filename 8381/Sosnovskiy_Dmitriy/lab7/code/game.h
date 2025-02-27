#pragma once
#include <map>
#include "field.h"
#include "base.h"
#include "snapshot.h"
#include "rules.h"


class IGame {
public:
    virtual unsigned int getCurrentPlayer() = 0;
    virtual unsigned int getTotalPlayersAmount() = 0;
    virtual unsigned int getRowsAmount() = 0;
    virtual unsigned int getColumnsAmount() = 0;

    virtual void addBase(unsigned int row, unsigned int column, unsigned int possessorId) = 0;

    virtual void addUnit(unsigned int row, unsigned int column, UnitType unitType, unsigned int possessorId) = 0;

    virtual void moveUnit(unsigned int rowFrom, unsigned int columnFrom,
                  unsigned int rowTo, unsigned int columnTo, unsigned int actorId) = 0;

    virtual void attack(unsigned int rowFrow, unsigned int columnFrom,
                unsigned int rowTo, unsigned int columnTo, unsigned int actorId) = 0;

    virtual QString getBaseInfo(unsigned int possessorId) = 0;

    virtual CreatureType getCreatureType(unsigned int row, unsigned int column) = 0;

    virtual UnitType getUnitType(unsigned int row, unsigned int column) = 0;

    virtual NeutralObjectType getNeutralObjectType(unsigned int row, unsigned int column) = 0;

    virtual LandscapeType getLandscapeType(unsigned int row, unsigned int column) = 0;

    virtual GameSnapshot* createSnapshot() = 0;

    virtual bool isWinner() = 0;

    virtual int getWinner() = 0;
};



class PlayerState{
private:
    unsigned int playerNumber;
    PlayerState* nextPlayer;
public:
    PlayerState(unsigned int playerNumber){
        this->playerNumber = playerNumber;
    }

    unsigned int getPlayerNumber() { return playerNumber; }

    PlayerState* setNext(PlayerState* next){
        nextPlayer = next;

        return next;
    }

    PlayerState* next() {
        return nextPlayer;
    }
};



template <class RuleTypeClass>
class Game : public IGame{

private:
    static Game<RuleTypeClass>* instance;

    Field* field;

    std::map<unsigned int, Base*> bases;

    std::vector<unsigned int> playersAlive;

    unsigned int totalPlayersAmount;

    PlayerState* currentPlayerState = nullptr;

    unsigned int currentPlayer;

    IRule* rule;


    bool isPlayerAlive(unsigned int playerNumber)
    {
        return bases[playerNumber]->isDemolished()? false: true;
    }

    void switchCurrentPlayer()
    {
        /*
        for(int i = 0; i < playersAlive.size(); i++)
        {
            if(isPlayerAlive(playersAlive[i]) == false)
            {
                playersAlive.erase(playersAlive.begin() + i);
                i--;
            }
        }

        do{
            currentPlayer = (currentPlayer + 1) % totalPlayersAmount;
        } while (isPlayerAlive(currentPlayer) == false);
        */

        for(int i = 0; i < playersAlive.size(); i++)
        {
            if(isPlayerAlive(playersAlive[i]) == false)
            {
                playersAlive.erase(playersAlive.begin() + i);
                i--;
            }
        }

        do {
            currentPlayerState = currentPlayerState->next();
        } while (isPlayerAlive(currentPlayerState->getPlayerNumber()) == false);

        currentPlayer = currentPlayerState->getPlayerNumber();
    }


    void setPlayerStates()
    {
        if(rule->getPlayersOrder() == USUAL)
        {
            currentPlayerState = new PlayerState(currentPlayer);
            PlayerState* tmp = currentPlayerState;

            int i = currentPlayer + 1;
            do{
                tmp = tmp->setNext(new PlayerState(i % totalPlayersAmount));
                i++;
            } while( i % totalPlayersAmount != currentPlayer);

            tmp->setNext(currentPlayerState);
        }
        else if(rule->getPlayersOrder() == REVERSIVE)
        {
            currentPlayerState = new PlayerState(currentPlayer);

            PlayerState* tmp = currentPlayerState;
            int nextPlayer = currentPlayer;
            do{
                nextPlayer -= 1;
                if(nextPlayer < 0) nextPlayer = totalPlayersAmount - 1;
                if(nextPlayer == currentPlayer) break;
                tmp = tmp->setNext(new PlayerState(nextPlayer));
            } while (nextPlayer != currentPlayer);

            tmp->setNext(currentPlayerState);
        }

    }

    Game()
    {
        this->rule = new RuleTypeClass();

        this->totalPlayersAmount = rule->getPlayersAmount();

        for(unsigned int i = 0; i < totalPlayersAmount; i++){
            playersAlive.push_back(i);
        }

        if(rule->getPlayersOrder() == USUAL)
            currentPlayer = 0;
        else currentPlayer = totalPlayersAmount - 1;

        setPlayerStates();

        field = new Field(rule->getFieldRowsAmount(),
                          rule->getFieldColumnsAmount(),
                          rule->getMaximumEntitiesAmount());

        field->setLandscapes(rule->getLandscapes());

        field->setNeutralObjects(rule->getNeutralObjects());

        std::map<unsigned int, CoordsInfo*> rulesBases = rule->getBases();

        for(int i = 0; i < totalPlayersAmount; i++)
        {
            this->addBase(rulesBases[i]->row, rulesBases[i]->column, i);
        }
    }

    Game(GameSnapshot* snapshot)
    {
        this->rule = new RuleTypeClass();
        this->totalPlayersAmount = rule->getPlayersAmount();
        this->currentPlayer = snapshot->currentPlayer;
        this->playersAlive = snapshot->playersAlive;


        if(currentPlayer >= totalPlayersAmount || currentPlayer < 0)
            throw new Exception(202, "Wrong file format", "There is an error in the save file in the data of CURRENT_PLAYER.");
        if(playersAlive.size() == 0)
             throw new Exception(205, "Wrong file format", "There is an error in the save file in the data of PLAYERS_ALIVE");
        for(auto i : playersAlive)
            if(i >= totalPlayersAmount || i < 0)
                 throw new Exception(207, "Wrong file format", "There is an error in the save file in the data of PLAYERS_AIVE");

        field = new Field(snapshot->fieldSnapshot);

        for(unsigned int i = 0; i < rule->getPlayersAmount(); i++)
        {
            addBase(snapshot->bases[i]->coordinates->row,
                    snapshot->bases[i]->coordinates->column,
                    snapshot->bases[i]->possessorId);

            field->getCell(snapshot->bases[i]->coordinates->row,
                           snapshot->bases[i]->coordinates->column)
                    ->getCreature()->setHealth(snapshot->bases[i]->healthAmount);
        }



        setPlayerStates();

        for(unsigned i = 0; i < snapshot->fieldSnapshot->units.size(); i++)
        {
            bases[snapshot->fieldSnapshot->units[i]->possessorId]->createUnit(
                        snapshot->fieldSnapshot->units[i]->type,
                        snapshot->fieldSnapshot->units[i]->coordinates->row,
                        snapshot->fieldSnapshot->units[i]->coordinates->column);

            field->getCell(snapshot->fieldSnapshot->units[i]->coordinates->row,
                           snapshot->fieldSnapshot->units[i]->coordinates->column)
                    ->getCreature()->setHealth(snapshot->fieldSnapshot->units[i]->healthAmount);
        }
    }

public:

    static Game<RuleTypeClass>* getInstance(bool isNewGame){
        if(instance == nullptr)
            instance = new Game<RuleTypeClass>();
        else{
            if(isNewGame) {
                delete instance;
                instance = new Game<RuleTypeClass>();
            }
        }
        return instance;
    }

    static Game* getInstance(GameSnapshot* snapshot, bool isNewGame)
    {
        Game<RuleTypeClass>* controlGameState = new Game<RuleTypeClass>(snapshot);

        if(instance == nullptr)
            instance = controlGameState;
        else{
            if(isNewGame) {
                delete instance;
                instance = controlGameState;
            }
        }
        return instance;
    }

    unsigned int getCurrentPlayer() override { return currentPlayer; }
    unsigned int getTotalPlayersAmount() override { return totalPlayersAmount; }
    unsigned int getRowsAmount() override { return field->getRowsAmount(); }
    unsigned int getColumnsAmount() override { return field->getColumnsAmount(); }

    // exception here?
    void addBase(unsigned int row, unsigned int column, unsigned int possessorId) override {
        if(bases.count(possessorId) == 1)
             throw new Exception(274, "Impossible base creating", "Base of this possessor exists already. Check the correctness of BASES in the save file.");

        Base* base = new Base(field, row, column, possessorId);
        bases[possessorId] = base;
    }

    void addUnit(unsigned int row, unsigned int column, UnitType unitType, unsigned int possessorId) override {
        if(bases.count(possessorId) == 0)
             throw new Exception(282, "Impossible unit creating", "This possessor doesn't exist. This way this unit couldn't be created. Check the save file. ");
        bases[possessorId]->createUnit(unitType, row, column);
        switchCurrentPlayer();
    }

    bool isWinner() override { return playersAlive.size() == rule->getWinState() ? true : false; }
    int getWinner() override {
        if(isWinner()) return playersAlive[0];
        else return -1;
    }

    void moveUnit(unsigned int rowFrom, unsigned int columnFrom,
                  unsigned int rowTo, unsigned int columnTo, unsigned int actorId) override
    {
        field->moveUnit(rowFrom, columnFrom, rowTo, columnTo, actorId);
        switchCurrentPlayer();
    }

    void attack(unsigned int rowFrow, unsigned int columnFrom,
                unsigned int rowTo, unsigned int columnTo, unsigned int actorId) override
    {
        field->commitFight(rowFrow, columnFrom, rowTo, columnTo, actorId);
        switchCurrentPlayer();
    }

    QString getBaseInfo(unsigned int possessorId) override
    {
        return bases[possessorId]->getInfo();
    }

    CreatureType getCreatureType(unsigned int row, unsigned int column) override {
        if(field->getCell(row, column)->hasCreature())
            return field->getCell(row, column)->getCreature()->getCreatureType();
        else {
             throw new Exception(320, "Cell emptyness", "No creature is placed in the " + QString::number(row) + " " + QString::number(column));
        }
    }

    UnitType getUnitType(unsigned int row, unsigned int column) override
    {
        if(field->getCell(row, column)->hasCreature() &&
                field->getCell(row, column)->getCreature()->getCreatureType() == UNIT){
                return dynamic_cast<IUnit*>(field->getCell(row, column)->getCreature())->getType();
        }
        else throw new Exception(331, "Cell emptyness", "No unit is placed in the " + QString::number(row) + " " + QString::number(column));
    }

    NeutralObjectType getNeutralObjectType(unsigned int row, unsigned int column) override
    {
        if(field->getCell(row, column)->hasNeutralObject())
            return field->getCell(row, column)->getNeutralObject()->getType();
        else throw new Exception(338, "Cell emptyness", "No neutral object is placed in the " + QString::number(row) + " " + QString::number(column));
    }

    LandscapeType getLandscapeType(unsigned int row, unsigned int column) override {
        return field->getCell(row, column)->getLandscape()->getType();
    }

    GameSnapshot* createSnapshot() override {
        FieldSnapshot* fieldSnapshot = field->createSnapshot();

        std::vector<BaseSnapshot*> basesSnapshots;

        for(unsigned int i = 0; i < bases.size(); i++)
        {
            basesSnapshots.push_back(bases[i]->createSnapshot());
        }

        GameSnapshot* gameSnapshot = new GameSnapshot(rule->getRuleType(), fieldSnapshot, basesSnapshots, currentPlayer, playersAlive);

        return gameSnapshot;
    }
};

template <class RuleTypeClass>
Game<RuleTypeClass>* Game<RuleTypeClass>::instance = nullptr;
