// Copyright 2017 Chris Coxe.
// 
// ZZZKBot is distributed under the terms of the GNU Lesser General
// Public License (LGPL) version 3.
//
// This file is part of ZZZKBot.
// 
// ZZZKBot is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// ZZZKBot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with ZZZKBot.  If not, see <http://www.gnu.org/licenses/>.
// 
// This file was created by copying then modifying file
// ExampleAIModule/Source/ExampleAIModule.cpp
// of BWAPI version 4.1.2
// (https://github.com/bwapi/bwapi/releases/tag/v4.1.2 which is
// distributed under the terms of the GNU Lesser General Public License
// (LGPL) version 3), then updating it to be compatible with a later version
// of BWAPI.

#include "ZZZKBotAIModule.h"
#include <iostream>
#include <limits>
#include <fstream>
#include <iomanip>
#include <cstdlib>

using namespace BWAPI;
using namespace Filter;

ZZZKBotAIModule::ZZZKBotAIModule()
  : Broodwar(BWAPIClient)
{
  ;
}

void ZZZKBotAIModule::onStart()
{
    // Print the map name.
    // BWAPI returns std::string when retrieving a string, don't forget to add .c_str() when printing!
    Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;

    // Enable the UserInput flag, which allows us to control the bot and type messages.
    // COMMENT-OUT THIS STATEMENT FOR COMPETITIONS/LADDERS! Only use it while debugging.
    //Broodwar->enableFlag(Flag::UserInput);

    // Uncomment the following line and the bot will know about everything through the fog of war (cheat).
    //Broodwar->enableFlag(Flag::CompleteMapInformation);

    // Set the command optimization level so that common commands can be grouped
    // and reduce the bot's APM (Actions Per Minute).
    //Broodwar->setCommandOptimizationLevel(2);
    Broodwar->setCommandOptimizationLevel(1);

    // Speedups (including disabling the GUI) for automated play.
    //Broodwar->setLocalSpeed(0);
    //Broodwar->setFrameSkip(16);   // Not needed if using setGUI(false).
    //Broodwar->setGUI(false);

    // Check if this is a replay
    if (Broodwar->isReplay())
    {
        // Announce the players in the replay
        Broodwar << "The following players are in this replay:" << std::endl;

        // Iterate all the players in the game using a std:: iterator
        Playerset players = Broodwar->getPlayers();
        for (auto p : players)
        {
            // Only print the player if they are not an observer
            if (!p->isObserver())
            {
                Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;
            }
        }
    }
    else // if this is not a replay
    {
        // Retrieve you and your enemy's races. enemy() will just return the first enemy.
        // If you wish to deal with multiple enemies then you must use enemies().
        if (Broodwar->enemy()) // First make sure there is an enemy
        {
            Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;
        }
    }
}

void ZZZKBotAIModule::onEnd(bool isWinner)
{
    if (Broodwar->isReplay())
    {
        return;
    }

    // I don't know if it's possible, but I am adding this check just in case it is needed if
    // it's possible that onEnd() might be called multiple times for whatever reason, e.g.
    // perhaps it could happen while the game is paused/unpaused? Better safe than sorry
    // because we do not want to spam the output file while the game is paused.
    static int frameCountLastCalled = -1;
    if (frameCountLastCalled != -1)
    {
        return;
    }
    frameCountLastCalled = Broodwar->getFrameCount();

    if (enemyPlayerID >= 0)
    {
        std::ostringstream oss;
    
        oss << startOfUpdateSentinel << delim;
    
        // Update type.
        oss << onEndUpdateSignifier << delim;
    
        oss << Broodwar->getFrameCount() << delim;
        oss << Broodwar->elapsedTime() << delim;
    
        // Get time now.
        // Note: localtime_s is not portable but w/e.
        std::time_t timer = std::time(nullptr);
        oss << (timer - timerAtGameStart) << delim;
        struct tm buf;
        errno_t errNo = localtime_s(&buf, &timer);
        if (errNo == 0)
        {
            oss << std::put_time(&buf, "%Y-%m-%d");
        }
        oss << delim;
        if (errNo == 0)
        {
            oss << std::put_time(&buf, "%H:%M:%S");
        }
        oss << delim;
    
        oss << isWinner << delim;
    
        oss << endOfUpdateSentinel << delim;
    
        oss << endOfLineSentinel;
    
        // Block to restrict scope of variables.
        {
            // Append to the file for the enemy in the write folder.
            std::ofstream enemyWriteFileOFS(enemyWriteFilePath, std::ios_base::out | std::ios_base::app);
            if (enemyWriteFileOFS)
            {
                enemyWriteFileOFS << oss.str();
                enemyWriteFileOFS.flush();
            }
        }
    }
}

void ZZZKBotAIModule::onFrame()
{
    // DISABLE THIS LOGIC FOR COMPETITIONS/LADDERS! Only use it while training.
    // Feature for speeding up the number of games per hour during training by intentionally
    // leaving the game if a game is taking too long to finish in terms of real world seconds.
    // Useful to avoid wasting time against bots that sometimes get into a state where they
    // are almost always using the full allowable milliseconds per frame, e.g. because they
    // are using the maximal allowed CPU time during combat simulations for battles, or are
    // bugged (slow) searching for a valid place to place a building, etc.
    // The downside is that it will be recorded in the game results as a loss, which affects
    // the strategy parameter learning logic.
    /*
    // Get time now.
    // Note: localtime_s is not portable but w/e.
    std::time_t timer = std::time(nullptr);
    if (timer - timerAtGameStart > 500)
    {
        Broodwar->leaveGame();
        return;
    }
    */

    // Called once every game frame unless the game is paused in which case it may be called
    // continually.

    // Display the game frame rate as text in the upper left area of the screen.
    Broodwar->drawTextScreen(200, 0,  "FPS: %d", Broodwar->getFPS() );
    Broodwar->drawTextScreen(200, 20, "Average FPS: %f", Broodwar->getAverageFPS() );

    // Return if the game is a replay or paused. Note that whenever the game is paused, what
    // happens is as follows: suppose the frame count it becomes paused is 1000. onFrame()
    // will be called (probably multiple times) with getFrameCount() returning 1000 and
    // isPaused() returning true. Then when it becomes unpaused, onFrame() will be called
    // exactly once with getFrameCount() returning 1000 and isPaused() returning false. Then
    // execution continues. When onFrame() is called again, getFrameCount() will be a
    // higher frame count than 1000. So the following logic is correct, i.e. we will not
    // accidentally skip a frame by always returning when isPaused() is true.
    if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
    {
        return;
    }

    // Unit client info indices.
    // The argument of getClientInfo() is the index.
    // The arguments of setClientInfo() are the value then the index.
    // Note: querying for a value that doesn't exist will return 0,
    // so you can't distinguish value 0 from undefined, so be careful about
    // storing zeroed values.
    // Let's use value 1 to mean it is (i.e. is currently, or was when we check it in later frames)
    // carrying minerals and 0 means not.
    const int wasJustCarryingMineralsInd = 1;
    const int wasJustCarryingMineralsDefaultVal = 0;
    const int wasJustCarryingMineralsTrueVal = 1;
    const int frameLastReturnedMineralsInd = 2;
    const int frameLastChangedPosInd = 3;
    const int frameLastAttackingInd = 4;
    const int frameLastAttackFrameInd = 5;
    const int frameLastStartingAttackInd = 6;
    const int frameLastStoppedInd = 7;
    const int posXInd = 8;
    const int posYInd = 9;
    const int scoutingTargetStartLocXInd = 10;
    const int scoutingTargetStartLocYInd = 11;
    const int scoutingTargetPosXInd = 12;
    const int scoutingTargetPosYInd = 13;
    const int lastGroundWeaponCooldownInd = 14;
    const int lastAirWeaponCooldownInd = 15;
    const int lastPeakGroundWeaponCooldownInd = 16;
    const int lastPeakAirWeaponCooldownInd = 17;
    const int lastPeakGroundWeaponCooldownFrameInd = 18;
    const int lastPeakAirWeaponCooldownFrameInd = 19;

    static std::set<BWAPI::TilePosition> enemyStartLocs;
    static std::set<BWAPI::TilePosition> possibleOverlordScoutLocs;
    // TODO: this bot is currently only designed to support 1v1 games without other players unless they
    // haven't had any buildings and don't currently have any buildings, i.e. these types of players are
    // not currently dealt with properly:
    // allies that are still playing,
    // allies that have left,
    // players that were enemies but have left (i.e. are now neutral not enemies),
    // other neutral players that have neutral buildings.
    static bool isARemainingEnemyZerg = false;
    static bool isARemainingEnemyTerran = false;
    static bool isARemainingEnemyProtoss = false;
    static bool isARemainingEnemyRandomRace = false;
    for (const BWAPI::Player& p : Broodwar->enemies())
    {
        if (p->getRace() == BWAPI::Races::Zerg)
        {
            isARemainingEnemyZerg = true;
            isARemainingEnemyRandomRace = false;
        }
        else if (p->getRace() == BWAPI::Races::Terran)
        {
            isARemainingEnemyTerran = true;
            isARemainingEnemyRandomRace = false;
        }
        else if (p->getRace() == BWAPI::Races::Protoss)
        {
            isARemainingEnemyProtoss = true;
            isARemainingEnemyRandomRace = false;
        }
        else
        {
            isARemainingEnemyRandomRace = true;
        }

        const BWAPI::TilePosition enemyStartLoc = p->getStartLocation();
        if (enemyStartLoc != BWAPI::TilePositions::Unknown && enemyStartLoc != BWAPI::TilePositions::None)
        {
            if (enemyStartLocs.empty())
            {
                possibleOverlordScoutLocs.clear();
            }

            possibleOverlordScoutLocs.insert(enemyStartLoc);
            enemyStartLocs.insert(enemyStartLoc);
        }
    }

    static std::set<BWAPI::TilePosition> startLocs;
    static BWAPI::TilePosition myStartLoc = BWAPI::TilePositions::Unknown;
    static BWAPI::Position myStartRoughPos = BWAPI::Positions::Unknown;
    static std::set<BWAPI::TilePosition> otherStartLocs;
    static std::set<BWAPI::TilePosition> scoutedOtherStartLocs;
    static std::set<BWAPI::TilePosition> unscoutedOtherStartLocs;

    // Converts a specified BWAPI::TilePosition and building type into a BWAPI::Position that would be roughly at the
    // centre of the building if it is built at the specified BWAPI::TilePosition.
    auto getRoughPos =
        [](const BWAPI::TilePosition loc, const BWAPI::UnitType ut)
        {
            return Position(Position(loc) + Position((ut.tileWidth() * BWAPI::TILEPOSITION_SCALE) / 2, (ut.tileHeight() * BWAPI::TILEPOSITION_SCALE) / 2));
        };

    if (myStartLoc == BWAPI::TilePositions::Unknown)
    {
        const BWAPI::TilePosition loc = Broodwar->self()->getStartLocation();
        if (loc != BWAPI::TilePositions::None && loc != BWAPI::TilePositions::Unknown)
        {
            myStartLoc = loc;
            myStartRoughPos = getRoughPos(loc, BWAPI::UnitTypes::Special_Start_Location);
        }
    }

    for (const BWAPI::TilePosition loc : Broodwar->getStartLocations())
    {
        if (loc != BWAPI::TilePositions::None && loc != BWAPI::TilePositions::Unknown)
        {
            startLocs.insert(loc);
    
            if (loc != myStartLoc)
            {
                const std::pair<std::set<BWAPI::TilePosition>::iterator, bool> ret = otherStartLocs.insert(loc);
    
                if (ret.second)
                {
                    unscoutedOtherStartLocs.insert(*ret.first);
                    if (enemyStartLocs.empty())
                    {
                        possibleOverlordScoutLocs.insert(*ret.first);
                    }
                }
            }
        }
    }

    static bool isMapPlasma_v_1_0 = false;

    // Each element of the set is a tile position where creep should be on frame zero for that start location.
    struct InitialCreepLocsSet { std::set<BWAPI::TilePosition> val; };
    // The key is the starting location (mine and possible enemy start locations).
    struct InitialCreepLocsMap { std::map<const BWAPI::TilePosition, InitialCreepLocsSet> val; };

    static InitialCreepLocsMap initialCreepLocsMap;

    if (initialCreepLocsMap.val.empty())
    {
        const std::string mapHash = Broodwar->mapHash();

        auto& initialCreepLocsMapAuto = initialCreepLocsMap;
        auto addToCreepLocsMap =
            [&initialCreepLocsMapAuto](const BWAPI::TilePosition startLoc, const std::vector<int> creepLocXY)
            {
                for (unsigned int i = 0; i < creepLocXY.size(); i += 2)
                {
                    // Size safety check.
                    if (i + 1 < creepLocXY.size())
                    {
                        initialCreepLocsMapAuto.val[startLoc].val.insert(BWAPI::TilePosition(creepLocXY[i], creepLocXY[i + 1]));
                    }
                }

                return;
            };
    
        // FYI: here is a handy Cygwin command to parse the creep_data.txt file(s) ready for copy-and-pasting into this source code:
        // cat creep_data*.txt | sort -u | sed 's/^\([^ ]* [^ ]* [^ ]* [^ ]* [^ ]*\) .*/\1/' | awk -F' ' '{possibleComma = ""; if ($1" "$2" "$3 in a) possibleComma = ","; a[$1" "$2" "$3] = a[$1" "$2" "$3]""possibleComma""$4","$5};END{for(i in a) { split(i,iArr," "); print iArr[1]": addToCreepLocsMap(BWAPI::TilePosition("iArr[2]", "iArr[3]"), std::vector<int> { "a[i]" } );"} }' | sort

        // Icarus map version 1.0 (map name converted to plain text contains "Icarus 1.0")
        if (mapHash == "0409ca0d7fe0c7f4083a70996a8f28f664d2fe37")
        {
            addToCreepLocsMap(BWAPI::TilePosition(116, 47), std::vector<int> { 108,47,108,48,108,49,109,45,109,46,109,47,109,48,109,49,109,50,109,51,110,44,110,45,110,46,110,47,110,48,110,49,110,50,110,51,110,52,111,44,111,45,111,46,111,47,111,48,111,49,111,50,111,51,111,52,112,43,112,44,112,45,112,46,112,47,112,48,112,49,112,50,112,51,112,52,112,53,113,43,113,44,113,45,113,46,113,47,113,48,113,49,113,50,113,51,113,52,113,53,114,43,114,44,114,45,114,46,114,47,114,48,114,49,114,50,114,51,114,52,114,53,115,42,115,43,115,44,115,45,115,46,115,47,115,48,115,49,115,50,115,51,115,52,115,53,115,54,116,44,116,45,116,46,116,47,116,48,116,49,116,50,116,51,116,52,116,53,116,54,117,44,117,45,117,46,117,47,117,48,117,49,117,50,117,51,117,52,117,53,117,54,118,44,118,45,118,46,118,47,118,48,118,49,118,50,118,51,118,52,118,53,118,54,119,44,119,45,119,46,119,47,119,48,119,49,119,50,119,51,119,52,119,53,119,54,120,42,120,43,120,44,120,45,120,46,120,47,120,48,120,49,120,50,120,51,120,52,120,53,120,54,121,43,121,44,121,45,121,46,121,47,121,48,121,49,121,50,121,51,121,52,121,53,122,43,122,44,122,45,122,46,122,47,122,48,122,49,122,50,122,51,122,52,122,53,123,43,123,45,123,47,123,50,123,53 } );
            addToCreepLocsMap(BWAPI::TilePosition(43, 8), std::vector<int> { 39,11,39,14,39,4,39,6,39,8,40,10,40,11,40,12,40,13,40,14,40,4,40,5,40,6,40,7,40,8,40,9,41,10,41,11,41,12,41,13,41,14,41,4,41,5,41,6,41,7,41,8,41,9,42,10,42,11,42,12,42,13,42,14,42,15,42,3,42,4,42,5,42,6,42,7,42,8,42,9,43,10,43,11,43,12,43,13,43,14,43,15,43,5,43,6,43,7,43,8,43,9,44,10,44,11,44,12,44,13,44,14,44,15,44,5,44,6,44,7,44,8,44,9,45,10,45,11,45,12,45,13,45,14,45,15,45,5,45,6,45,7,45,8,45,9,46,10,46,11,46,12,46,13,46,14,46,15,46,5,46,6,46,7,46,8,46,9,47,10,47,11,47,12,47,13,47,14,47,15,47,3,47,4,47,5,47,6,47,7,47,8,47,9,48,10,48,11,48,12,48,13,48,14,48,4,48,5,48,6,48,7,48,8,48,9,49,10,49,11,49,12,49,13,49,14,49,4,49,5,49,6,49,7,49,8,49,9,50,10,50,11,50,12,50,13,50,14,50,4,50,5,50,6,50,7,50,8,50,9,51,10,51,11,51,12,51,13,51,5,51,6,51,7,51,8,51,9,52,10,52,11,52,12,52,13,52,5,52,6,52,7,52,8,52,9,53,10,53,11,53,12,53,6,53,7,53,8,53,9,54,10,54,8,54,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 77), std::vector<int> { 10,74,10,75,10,76,10,77,10,78,10,79,10,80,10,81,10,82,10,83,10,84,11,74,11,75,11,76,11,77,11,78,11,79,11,80,11,81,11,82,11,83,11,84,12,72,12,73,12,74,12,75,12,76,12,77,12,78,12,79,12,80,12,81,12,82,12,83,12,84,13,73,13,74,13,75,13,76,13,77,13,78,13,79,13,80,13,81,13,82,13,83,14,73,14,74,14,75,14,76,14,77,14,78,14,79,14,80,14,81,14,82,14,83,15,73,15,74,15,75,15,76,15,77,15,78,15,79,15,80,15,81,15,82,15,83,16,74,16,75,16,76,16,77,16,78,16,79,16,80,16,81,16,82,17,74,17,75,17,76,17,77,17,78,17,79,17,80,17,81,17,82,18,75,18,76,18,77,18,78,18,79,18,80,18,81,19,77,19,78,19,79,4,73,4,75,4,77,4,78,4,80,4,83,5,73,5,74,5,75,5,76,5,77,5,78,5,79,5,80,5,81,5,82,5,83,6,73,6,74,6,75,6,76,6,77,6,78,6,79,6,80,6,81,6,82,6,83,7,72,7,73,7,74,7,75,7,76,7,77,7,78,7,79,7,80,7,81,7,82,7,83,7,84,8,74,8,75,8,76,8,77,8,78,8,79,8,80,8,81,8,82,8,83,8,84,9,74,9,75,9,76,9,77,9,78,9,79,9,80,9,81,9,82,9,83,9,84 } );
            addToCreepLocsMap(BWAPI::TilePosition(81, 118), std::vector<int> { 73,118,73,119,73,120,74,116,74,117,74,118,74,119,74,120,74,121,74,122,75,115,75,116,75,117,75,118,75,119,75,120,75,121,75,122,75,123,76,115,76,116,76,117,76,118,76,119,76,120,76,121,76,122,76,123,77,114,77,115,77,116,77,117,77,118,77,119,77,120,77,121,77,122,77,123,77,124,78,114,78,115,78,116,78,117,78,118,78,119,78,120,78,121,78,122,78,123,78,124,79,114,79,115,79,116,79,117,79,118,79,119,79,120,79,121,79,122,79,123,79,124,80,113,80,114,80,115,80,116,80,117,80,118,80,119,80,120,80,121,80,122,80,123,80,124,80,125,81,115,81,116,81,117,81,118,81,119,81,120,81,121,81,122,81,123,81,124,81,125,82,115,82,116,82,117,82,118,82,119,82,120,82,121,82,122,82,123,82,124,82,125,83,115,83,116,83,117,83,118,83,119,83,120,83,121,83,122,83,123,83,124,83,125,84,115,84,116,84,117,84,118,84,119,84,120,84,121,84,122,84,123,84,124,84,125,85,113,85,114,85,115,85,116,85,117,85,118,85,119,85,120,85,121,85,122,85,123,85,124,85,125,86,114,86,115,86,116,86,117,86,118,86,119,86,120,86,121,86,122,86,123,86,124,87,114,87,115,87,116,87,117,87,118,87,119,87,120,87,121,87,122,87,123,87,124,88,114,88,116,88,119,88,122,88,124 } );
        }
        // Match Point map version 1.3 (map name converted to plain text contains "MatchPoint 1.3")
        else if (mapHash == "0a41f144c6134a2204f3d47d57cf2afcd8430841")
        {
            addToCreepLocsMap(BWAPI::TilePosition(100, 14), std::vector<int> { 100,11,100,12,100,13,100,14,100,15,100,16,100,17,100,18,100,19,100,20,100,21,101,11,101,12,101,13,101,14,101,15,101,16,101,17,101,18,101,19,101,20,101,21,102,11,102,12,102,13,102,14,102,15,102,16,102,17,102,18,102,19,102,20,102,21,103,11,103,12,103,13,103,14,103,15,103,16,103,17,103,18,103,19,103,20,103,21,104,10,104,11,104,12,104,13,104,14,104,15,104,16,104,17,104,18,104,19,104,20,104,21,104,9,105,10,105,11,105,12,105,13,105,14,105,15,105,16,105,17,105,18,105,19,105,20,106,11,106,12,106,13,106,14,106,15,106,16,106,17,106,18,106,19,107,12,107,14,107,15,107,17,108,12,108,17,109,11,109,12,109,13,109,16,109,17,109,18,109,19,110,12,110,13,110,14,110,15,110,16,110,17,110,18,111,14,111,15,111,16,92,14,92,15,92,16,93,12,93,13,93,14,93,15,93,16,93,17,93,18,94,11,94,12,94,13,94,14,94,15,94,16,94,17,94,18,94,19,95,11,95,12,95,13,95,14,95,15,95,16,95,17,95,18,95,19,96,10,96,11,96,12,96,13,96,14,96,15,96,16,96,17,96,18,96,19,96,20,97,10,97,11,97,12,97,13,97,14,97,15,97,16,97,17,97,18,97,19,97,20,98,10,98,11,98,12,98,13,98,14,98,15,98,16,98,17,98,18,98,19,98,20,99,10,99,11,99,12,99,13,99,14,99,15,99,16,99,17,99,18,99,19,99,20,99,21,99,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 112), std::vector<int> { 10,109,10,110,10,111,10,112,10,113,10,114,10,115,10,116,10,117,10,118,10,119,11,109,11,110,11,111,11,112,11,113,11,114,11,115,11,116,11,117,11,118,11,119,12,107,12,108,12,109,12,110,12,111,12,112,12,113,12,114,12,115,12,116,12,117,12,118,12,119,13,108,13,109,13,110,13,111,13,112,13,113,13,114,13,115,13,116,13,117,13,118,14,108,14,109,14,110,14,111,14,112,14,113,14,114,14,115,14,116,14,117,14,118,15,108,15,109,15,110,15,111,15,112,15,113,15,114,15,115,15,116,15,117,15,118,16,109,16,110,16,111,16,112,16,113,16,114,16,115,16,116,16,117,17,109,17,110,17,111,17,112,17,113,17,114,17,115,17,116,17,117,18,110,18,111,18,112,18,113,18,114,18,115,18,116,19,112,19,113,19,114,4,108,4,110,4,112,4,113,4,115,4,118,5,108,5,109,5,110,5,111,5,112,5,113,5,114,5,115,5,116,5,117,5,118,6,108,6,109,6,110,6,111,6,112,6,113,6,114,6,115,6,116,6,117,6,118,7,107,7,108,7,109,7,110,7,111,7,112,7,113,7,114,7,115,7,116,7,117,7,118,7,119,8,109,8,110,8,111,8,112,8,113,8,114,8,115,8,116,8,117,8,118,8,119,9,109,9,110,9,111,9,112,9,113,9,114,9,115,9,116,9,117,9,118,9,119 } );
        }
        // Neo Aztec map version 2.1 (map name converted to plain text contains "Neo Aztec2.1")
        else if (mapHash == "19f00ba3a407e3f13fb60bdd2845d8ca2765cf10")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 100), std::vector<int> { 109,100,109,101,109,102,110,100,110,101,110,102,110,103,110,104,110,98,110,99,111,100,111,101,111,102,111,103,111,104,111,105,111,97,111,98,111,99,112,100,112,101,112,102,112,103,112,104,112,105,112,97,112,98,112,99,113,100,113,101,113,102,113,103,113,104,113,105,113,106,113,96,113,97,113,98,113,99,114,100,114,101,114,102,114,103,114,104,114,105,114,106,114,96,114,97,114,98,114,99,115,100,115,101,115,102,115,103,115,104,115,105,115,106,115,96,115,97,115,98,115,99,116,100,116,101,116,102,116,103,116,104,116,105,116,106,116,107,116,95,116,96,116,97,116,98,116,99,117,100,117,101,117,102,117,103,117,104,117,105,117,106,117,107,117,97,117,98,117,99,118,100,118,101,118,102,118,103,118,104,118,105,118,106,118,107,118,97,118,98,118,99,119,100,119,101,119,102,119,103,119,104,119,105,119,106,119,107,119,97,119,98,119,99,120,100,120,101,120,102,120,103,120,104,120,105,120,106,120,107,120,97,120,98,120,99,121,100,121,101,121,102,121,103,121,104,121,105,121,106,121,107,121,95,121,96,121,97,121,98,121,99,122,100,122,101,122,102,122,103,122,104,122,105,122,106,122,96,122,97,122,98,122,99,123,100,123,101,123,102,123,103,123,104,123,105,123,97,123,98,123,99,124,100,124,102,124,103,124,99,125,102,125,99,126,101,126,102,126,104,126,105,126,97,126,98,126,99,127,100,127,101,127,102,127,103,127,104,127,98,127,99 } );
            addToCreepLocsMap(BWAPI::TilePosition(68, 6), std::vector<int> { 60,6,60,7,60,8,61,10,61,4,61,5,61,6,61,7,61,8,61,9,62,10,62,11,62,3,62,4,62,6,62,7,62,8,63,11,64,11,64,12,64,5,64,9,65,10,65,11,65,12,65,3,65,4,65,5,65,6,65,7,65,8,65,9,66,10,66,11,66,12,66,2,66,3,66,4,66,5,66,6,66,7,66,8,66,9,67,1,67,10,67,11,67,12,67,13,67,2,67,3,67,4,67,5,67,6,67,7,67,8,67,9,68,10,68,11,68,12,68,13,68,3,68,4,68,5,68,6,68,7,68,8,68,9,69,10,69,11,69,12,69,13,69,3,69,4,69,5,69,6,69,7,69,8,69,9,70,10,70,11,70,12,70,13,70,3,70,4,70,5,70,6,70,7,70,8,70,9,71,10,71,11,71,12,71,13,71,3,71,4,71,5,71,6,71,7,71,8,71,9,72,1,72,10,72,11,72,12,72,13,72,2,72,3,72,4,72,5,72,6,72,7,72,8,72,9,73,10,73,11,73,12,73,2,73,3,73,4,73,5,73,6,73,7,73,8,73,9,74,10,74,11,74,12,74,2,74,3,74,4,74,5,74,6,74,7,74,8,74,9,75,10,75,11,75,12,75,2,75,3,75,4,75,5,75,6,75,7,75,8,75,9,76,10,76,11,76,3,76,4,76,5,76,6,76,7,76,8,76,9,77,10,77,11,77,3,77,4,77,5,77,6,77,7,77,8,77,9,78,10,78,4,78,5,78,6,78,7,78,8,78,9,79,6,79,7,79,8 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 82), std::vector<int> { 0,80,0,81,0,82,0,83,0,84,0,85,0,86,1,79,1,80,1,81,1,83,1,84,1,86,1,87,10,79,10,80,10,81,10,82,10,83,10,84,10,85,10,86,10,87,10,88,10,89,11,77,11,78,11,79,11,80,11,81,11,82,11,83,11,84,11,85,11,86,11,87,11,88,11,89,12,78,12,79,12,80,12,81,12,82,12,83,12,84,12,85,12,86,12,87,12,88,13,78,13,79,13,80,13,81,13,82,13,83,13,84,13,85,13,86,13,87,13,88,14,78,14,79,14,80,14,81,14,82,14,83,14,84,14,85,14,86,14,87,14,88,15,79,15,80,15,81,15,82,15,83,15,84,15,85,15,86,15,87,16,79,16,80,16,81,16,82,16,83,16,84,16,85,16,86,16,87,17,80,17,81,17,82,17,83,17,84,17,85,17,86,18,82,18,83,18,84,2,79,3,78,3,79,3,82,3,85,4,78,4,79,4,80,4,81,4,82,4,83,4,84,4,85,4,86,4,87,5,78,5,79,5,80,5,81,5,82,5,83,5,84,5,85,5,86,5,87,5,88,6,77,6,78,6,79,6,80,6,81,6,82,6,83,6,84,6,85,6,86,6,87,6,88,6,89,7,79,7,80,7,81,7,82,7,83,7,84,7,85,7,86,7,87,7,88,7,89,8,79,8,80,8,81,8,82,8,83,8,84,8,85,8,86,8,87,8,88,8,89,9,79,9,80,9,81,9,82,9,83,9,84,9,85,9,86,9,87,9,88,9,89 } );
        }
        // Andromeda map version 1.0 (map name converted to plain text contains "Andromeda 1.0")
        else if (mapHash == "1e983eb6bcfa02ef7d75bd572cb59ad3aab49285")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 119), std::vector<int> { 109,119,109,120,109,121,110,117,110,118,110,119,110,120,110,121,110,122,110,123,111,116,111,117,111,118,111,119,111,120,111,121,111,122,111,123,111,124,112,116,112,117,112,118,112,119,112,120,112,121,112,122,112,123,112,124,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,113,124,113,125,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,114,124,114,125,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,115,124,115,125,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,116,125,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,117,125,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,118,125,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,119,125,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,120,125,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,121,124,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,122,123,122,124,123,115,123,116,123,117,123,118,123,119,123,120,123,121,123,122,123,123,123,124,124,115,124,116,124,118,124,119,124,121,124,122,125,116,125,122,126,116,126,117,126,120,126,122,126,123,126,124,127,117,127,118,127,119,127,120,127,121,127,122,127,123 } );
            addToCreepLocsMap(BWAPI::TilePosition(117, 7), std::vector<int> { 109,7,109,8,109,9,110,10,110,11,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,12,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,2,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,4,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,4,119,5,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,2,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,3,123,4,123,5,123,6,123,7,123,8,123,9,124,10,124,3,124,5,124,7,124,8,125,10,126,10,126,11,126,12,126,4,126,6,126,9,127,10,127,11,127,5,127,6,127,7,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 118), std::vector<int> { 0,116,0,117,0,118,0,119,0,120,0,121,0,122,1,115,1,116,1,119,1,121,1,123,10,115,10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,10,124,10,125,11,113,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,11,124,11,125,12,114,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,12,123,12,124,13,114,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,13,123,13,124,14,114,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,14,123,14,124,15,115,15,116,15,117,15,118,15,119,15,120,15,121,15,122,15,123,16,115,16,116,16,117,16,118,16,119,16,120,16,121,16,122,16,123,17,116,17,117,17,118,17,119,17,120,17,121,17,122,18,118,18,119,18,120,2,119,3,114,3,117,3,118,3,119,3,120,3,122,4,114,4,115,4,116,4,117,4,118,4,119,4,120,4,121,4,122,4,123,5,114,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,5,123,5,124,6,113,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,6,124,6,125,7,115,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,7,124,7,125,8,115,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,8,124,8,125,9,115,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123,9,124,9,125 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 6), std::vector<int> { 0,10,0,4,0,5,0,6,0,7,0,8,0,9,1,10,1,11,1,3,1,4,1,5,1,7,1,8,10,10,10,11,10,12,10,13,10,3,10,4,10,5,10,6,10,7,10,8,10,9,11,1,11,10,11,11,11,12,11,13,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,2,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,2,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,2,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,3,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,3,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,4,17,5,17,6,17,7,17,8,17,9,18,6,18,7,18,8,2,4,3,2,3,4,3,6,3,9,4,10,4,11,4,2,4,3,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,2,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,1,6,10,6,11,6,12,6,13,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,3,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,3,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,3,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // Luna the Final map version 2.3 (map name converted to plain text contains "Luna the Final 2.3")
        else if (mapHash == "33527b4ce7662f83485575c4b1fcad5d737dfcf1")
        {
            addToCreepLocsMap(BWAPI::TilePosition(116, 107), std::vector<int> { 108,107,108,108,108,109,109,105,109,106,109,107,109,108,109,109,109,110,109,111,110,104,110,105,110,106,110,107,110,108,110,109,110,110,110,111,110,112,111,104,111,105,111,106,111,107,111,108,111,109,111,110,111,111,111,112,112,103,112,104,112,105,112,106,112,107,112,108,112,109,112,110,112,111,112,112,112,113,113,103,113,104,113,105,113,106,113,107,113,108,113,109,113,110,113,111,113,112,113,113,114,103,114,104,114,105,114,106,114,107,114,108,114,109,114,110,114,111,114,112,114,113,115,102,115,103,115,104,115,105,115,106,115,107,115,108,115,109,115,110,115,111,115,112,115,113,115,114,116,104,116,105,116,106,116,107,116,108,116,109,116,110,116,111,116,112,116,113,116,114,117,104,117,105,117,106,117,107,117,108,117,109,117,110,117,111,117,112,117,113,117,114,118,104,118,105,118,106,118,107,118,108,118,109,118,110,118,111,118,112,118,113,118,114,119,104,119,105,119,106,119,107,119,108,119,109,119,110,119,111,119,112,119,113,119,114,120,102,120,103,120,104,120,105,120,106,120,107,120,108,120,109,120,110,120,111,120,112,120,113,120,114,121,103,121,104,121,105,121,106,121,107,121,108,121,109,121,110,121,111,121,112,121,113,122,103,122,104,122,105,122,106,122,107,122,108,122,109,122,110,122,111,122,112,123,103,123,106,123,107,123,110,123,111,124,111,125,104,125,105,125,108,125,109,125,111,125,112,126,105,126,106,126,107,126,108,126,109,126,110,126,111,127,107,127,108,127,109 } );
            addToCreepLocsMap(BWAPI::TilePosition(116, 31), std::vector<int> { 108,31,108,32,108,33,109,29,109,30,109,31,109,32,109,33,109,34,109,35,110,28,110,29,110,30,110,31,110,32,110,33,110,34,110,35,110,36,111,28,111,29,111,30,111,31,111,32,111,33,111,34,111,35,111,36,112,27,112,28,112,29,112,30,112,31,112,32,112,33,112,34,112,35,112,36,112,37,113,27,113,28,113,29,113,30,113,31,113,32,113,33,113,34,113,35,113,36,113,37,114,27,114,28,114,29,114,30,114,31,114,32,114,33,114,34,114,35,114,36,114,37,115,26,115,27,115,28,115,29,115,30,115,31,115,32,115,33,115,34,115,35,115,36,115,37,115,38,116,28,116,29,116,30,116,31,116,32,116,33,116,34,116,35,116,36,116,37,116,38,117,28,117,29,117,30,117,31,117,32,117,33,117,34,117,35,117,36,117,37,117,38,118,28,118,29,118,30,118,31,118,32,118,33,118,34,118,35,118,36,118,37,118,38,119,28,119,29,119,30,119,31,119,32,119,33,119,34,119,35,119,36,119,37,119,38,120,26,120,27,120,28,120,29,120,30,120,31,120,32,120,33,120,34,120,35,120,36,120,37,120,38,121,27,121,28,121,29,121,30,121,31,121,32,121,33,121,34,121,35,121,36,121,37,122,27,122,28,122,29,122,30,122,31,122,32,122,33,122,34,122,35,122,36,123,27,123,30,123,32,123,33,123,35,124,35,125,28,125,29,125,31,125,34,125,35,125,36,126,29,126,30,126,31,126,32,126,33,126,34,126,35,127,31,127,32,127,33 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 9), std::vector<int> { 0,10,0,11,0,9,1,10,1,11,1,12,1,13,1,7,1,8,1,9,10,10,10,11,10,12,10,13,10,14,10,15,10,16,10,6,10,7,10,8,10,9,11,10,11,11,11,12,11,13,11,14,11,15,11,16,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,13,12,14,12,15,12,16,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,13,13,14,13,15,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,13,14,14,14,15,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,12,15,13,15,14,15,15,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,12,16,13,16,14,16,6,16,7,16,8,16,9,17,10,17,11,17,12,17,13,17,14,17,6,17,7,17,8,17,9,18,10,18,11,18,12,18,13,18,7,18,8,18,9,19,10,19,11,19,9,2,11,2,14,2,6,2,7,2,8,3,6,3,7,4,10,4,12,4,13,4,5,4,6,4,7,4,9,5,10,5,11,5,12,5,13,5,14,5,5,5,6,5,7,5,8,5,9,6,10,6,11,6,12,6,13,6,14,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,14,7,16,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,14,8,15,8,16,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,14,9,15,9,16,9,6,9,7,9,8,9,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 98), std::vector<int> { 0,100,0,98,0,99,1,100,1,101,1,102,1,96,1,97,1,98,1,99,10,100,10,101,10,102,10,103,10,104,10,105,10,95,10,96,10,97,10,98,10,99,11,100,11,101,11,102,11,103,11,104,11,105,11,95,11,96,11,97,11,98,11,99,12,100,12,101,12,102,12,103,12,104,12,105,12,93,12,94,12,95,12,96,12,97,12,98,12,99,13,100,13,101,13,102,13,103,13,104,13,94,13,95,13,96,13,97,13,98,13,99,14,100,14,101,14,102,14,103,14,104,14,94,14,95,14,96,14,97,14,98,14,99,15,100,15,101,15,102,15,103,15,104,15,94,15,95,15,96,15,97,15,98,15,99,16,100,16,101,16,102,16,103,16,95,16,96,16,97,16,98,16,99,17,100,17,101,17,102,17,103,17,95,17,96,17,97,17,98,17,99,18,100,18,101,18,102,18,96,18,97,18,98,18,99,19,100,19,98,19,99,2,102,2,103,2,95,2,96,2,97,2,99,3,96,4,100,4,101,4,104,4,96,4,98,5,100,5,101,5,102,5,103,5,104,5,95,5,96,5,97,5,98,5,99,6,100,6,101,6,102,6,103,6,104,6,94,6,95,6,96,6,97,6,98,6,99,7,100,7,101,7,102,7,103,7,104,7,105,7,93,7,94,7,95,7,96,7,97,7,98,7,99,8,100,8,101,8,102,8,103,8,104,8,105,8,95,8,96,8,97,8,98,8,99,9,100,9,101,9,102,9,103,9,104,9,105,9,95,9,96,9,97,9,98,9,99 } );
        }
        // Great Barrier Reef map version 1.0 (map name converted to plain text contains "Great Barrier Reef 1.0")
        else if (mapHash == "3506e6d942f9721dc99495a141f41c5555e8eab5")
        {
            addToCreepLocsMap(BWAPI::TilePosition(116, 86), std::vector<int> { 108,86,108,87,108,88,109,84,109,85,109,86,109,87,109,88,109,89,109,90,110,83,110,84,110,85,110,86,110,87,110,88,110,89,110,90,110,91,111,83,111,84,111,85,111,86,111,87,111,88,111,89,111,90,111,91,112,82,112,83,112,84,112,85,112,86,112,87,112,88,112,89,112,90,112,91,112,92,113,82,113,83,113,84,113,85,113,86,113,87,113,88,113,89,113,90,113,91,113,92,114,82,114,83,114,84,114,85,114,86,114,87,114,88,114,89,114,90,114,91,114,92,115,81,115,82,115,83,115,84,115,85,115,86,115,87,115,88,115,89,115,90,115,91,115,92,115,93,116,83,116,84,116,85,116,86,116,87,116,88,116,89,116,90,116,91,116,92,116,93,117,83,117,84,117,85,117,86,117,87,117,88,117,89,117,90,117,91,117,92,117,93,118,83,118,84,118,85,118,86,118,87,118,88,118,89,118,90,118,91,118,92,118,93,119,83,119,84,119,85,119,86,119,87,119,88,119,89,119,90,119,91,119,92,119,93,120,81,120,82,120,83,120,84,120,85,120,86,120,87,120,88,120,89,120,90,120,91,120,92,120,93,121,82,121,83,121,84,121,85,121,86,121,87,121,88,121,89,121,90,121,91,121,92,122,82,122,83,122,84,122,85,122,86,122,87,122,88,122,89,122,90,122,91,122,92,123,82,123,85,123,88,123,89,123,92 } );
            addToCreepLocsMap(BWAPI::TilePosition(61, 6), std::vector<int> { 53,6,53,7,53,8,54,10,54,4,54,5,54,8,54,9,55,10,55,11,55,3,55,4,55,5,55,8,55,9,56,10,56,11,56,3,56,4,56,5,56,8,56,9,57,10,57,11,57,12,57,2,57,3,57,4,57,5,57,8,57,9,58,10,58,11,58,12,58,2,58,3,58,4,58,5,58,6,58,7,58,8,58,9,59,10,59,11,59,12,59,2,59,3,59,4,59,5,59,6,59,7,59,8,59,9,60,1,60,10,60,11,60,12,60,13,60,2,60,3,60,4,60,5,60,6,60,7,60,8,60,9,61,1,61,10,61,11,61,12,61,13,61,2,61,3,61,4,61,5,61,6,61,7,61,8,61,9,62,1,62,10,62,11,62,12,62,13,62,3,62,4,62,5,62,6,62,7,62,8,62,9,63,10,63,11,63,12,63,13,63,3,63,4,63,5,63,6,63,7,63,8,63,9,64,10,64,11,64,12,64,13,64,2,64,3,64,4,64,5,64,6,64,7,64,8,64,9,65,1,65,10,65,11,65,12,65,13,65,3,65,4,65,5,65,6,65,7,65,8,65,9,66,10,66,11,66,12,66,3,66,4,66,5,66,6,66,7,66,8,66,9,67,10,67,11,67,12,67,3,67,4,67,5,67,6,67,7,67,8,67,9,68,10,68,11,68,12,68,3,68,5,68,7,68,9,69,10,69,11,69,3,69,9,70,10,70,11,70,3,70,4,70,6,70,8,70,9,71,10,71,4,71,5,71,6,71,7,71,8,71,9,72,6,72,7,72,8 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 90), std::vector<int> { 0,90,0,91,0,92,1,88,1,89,1,90,1,91,1,92,1,93,1,94,10,87,10,88,10,89,10,90,10,91,10,92,10,93,10,94,10,95,10,96,10,97,11,87,11,88,11,89,11,90,11,91,11,92,11,93,11,94,11,95,11,96,11,97,12,85,12,86,12,87,12,88,12,89,12,90,12,91,12,92,12,93,12,94,12,95,12,96,12,97,13,86,13,87,13,88,13,89,13,90,13,91,13,92,13,93,13,94,13,95,13,96,14,86,14,87,14,88,14,89,14,90,14,91,14,92,14,93,14,94,14,95,14,96,15,86,15,87,15,88,15,89,15,90,15,91,15,92,15,93,15,94,15,95,15,96,16,87,16,88,16,89,16,90,16,91,16,92,16,93,16,94,16,95,17,87,17,88,17,89,17,90,17,91,17,92,17,93,17,94,17,95,18,88,18,89,18,90,18,91,18,92,18,93,18,94,19,90,19,91,19,92,2,87,2,88,2,89,2,91,2,93,2,94,2,95,3,88,3,94,4,86,4,88,4,90,4,92,4,94,5,86,5,87,5,88,5,89,5,90,5,91,5,92,5,93,5,94,5,95,6,86,6,87,6,88,6,89,6,90,6,91,6,92,6,93,6,94,6,95,7,85,7,86,7,87,7,88,7,89,7,90,7,91,7,92,7,93,7,94,7,95,7,97,8,87,8,88,8,89,8,90,8,91,8,92,8,93,8,94,8,95,8,96,8,97,9,87,9,88,9,89,9,90,9,91,9,92,9,93,9,94,9,95,9,96,9,97 } );
        }
        // Arcadia II map version 2.02 (map name converted to plain text contains "Arcadia II")
        else if (mapHash == "442e456721c94fd085ecd10230542960d57928d9")
        {
            addToCreepLocsMap(BWAPI::TilePosition(116, 117), std::vector<int> { 108,117,108,118,108,119,109,115,109,116,109,117,109,120,109,121,110,114,110,115,110,116,110,117,110,120,110,121,110,122,111,114,111,115,111,116,111,117,111,120,111,121,111,122,112,113,112,114,112,115,112,116,112,117,112,120,112,121,112,122,112,123,113,113,113,114,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,114,113,114,114,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,115,112,115,113,115,114,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,124,116,112,116,113,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,124,117,112,117,113,117,114,117,115,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,124,118,112,118,113,118,114,118,115,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,119,112,119,113,119,114,119,115,119,116,119,117,119,118,119,119,119,120,119,121,119,122,120,112,120,113,120,114,120,115,120,116,120,117,120,118,120,119,120,120,120,121,120,122,121,113,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,122,113,122,114,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,123,113,123,114,123,115,123,116,123,117,123,118,123,119,123,120,124,114,124,115,124,116,124,117,124,118,124,119,124,120,125,114,125,115,125,116,125,117,125,118,125,119,125,120,125,121,125,122,126,115,126,116,126,117,126,118,126,119,126,120,126,121,127,117,127,118,127,119 } );
            addToCreepLocsMap(BWAPI::TilePosition(116, 6), std::vector<int> { 108,6,108,7,108,8,109,10,109,4,109,5,109,6,109,7,109,8,109,9,110,10,110,11,110,3,110,4,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,3,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,2,112,3,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,2,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,2,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,3,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,3,118,4,118,5,118,6,118,7,118,8,118,9,119,1,119,10,119,11,119,12,119,13,119,2,119,3,119,4,119,5,119,6,119,7,119,8,119,9,120,1,120,10,120,11,120,12,120,13,120,3,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,5,123,6,123,7,124,10,124,11,124,6,125,10,125,11,125,3,125,4,125,6,125,8,125,9,126,10,126,4,126,5,126,6,126,7,126,8,126,9,127,6,127,7,127,8 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 117), std::vector<int> { 0,117,0,118,0,119,1,115,1,116,1,117,1,120,1,121,10,112,10,113,10,114,10,115,10,116,10,117,10,118,10,119,10,120,10,121,10,122,11,112,11,113,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,12,112,12,113,12,114,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,13,113,13,114,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,14,113,14,114,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,14,123,15,113,15,114,15,115,15,116,15,117,15,118,15,119,15,120,15,121,15,122,15,123,16,114,16,115,16,116,16,117,16,118,16,119,16,120,16,121,16,122,17,114,17,115,17,116,17,117,17,118,17,119,17,120,17,121,17,122,18,115,18,116,18,117,18,118,18,119,18,120,18,121,19,117,19,118,19,119,2,114,2,115,2,116,2,117,2,120,2,121,2,122,3,114,3,115,3,116,3,117,3,120,4,113,4,114,4,115,4,116,4,117,4,120,5,113,5,114,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,6,113,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,7,112,7,113,7,114,7,115,7,116,7,117,7,118,7,119,7,120,7,121,7,122,8,112,8,113,8,114,8,115,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,9,112,9,113,9,114,9,115,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 6), std::vector<int> { 0,6,0,7,0,8,1,10,1,4,1,5,1,6,1,7,1,8,1,9,10,10,10,11,10,12,10,13,10,3,10,4,10,5,10,6,10,7,10,8,10,9,11,10,11,11,11,12,11,13,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,13,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,2,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,2,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,12,15,2,15,3,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,3,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,11,17,3,17,4,17,5,17,6,17,7,17,8,17,9,18,10,18,4,18,5,18,6,18,7,18,8,18,9,19,6,19,7,19,8,2,10,2,11,2,3,2,4,2,5,2,6,2,9,3,10,3,11,3,4,4,10,4,11,4,12,4,4,4,7,4,8,5,10,5,11,5,12,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,10,6,11,6,12,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,1,7,10,7,11,7,12,7,13,7,3,7,4,7,5,7,6,7,7,7,8,7,9,8,1,8,10,8,11,8,12,8,13,8,2,8,3,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,3,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // Circuit Breakers map version 1.0 (map name converted to plain text contains "Circuit Breakers 1.0")
        else if (mapHash == "450a792de0e544b51af5de578061cb8a2f020f32")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 118), std::vector<int> { 109,118,109,119,109,120,110,116,110,117,110,118,110,119,110,120,110,121,110,122,111,115,111,116,111,117,111,118,111,119,111,120,111,121,111,122,111,123,112,115,112,116,112,117,112,118,112,119,112,120,112,121,112,122,112,123,113,114,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,113,124,114,114,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,114,124,115,114,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,115,124,116,113,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,116,125,117,115,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,117,125,118,115,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,118,125,119,115,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,119,125,120,115,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,120,125,121,113,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,121,124,121,125,122,114,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,122,123,122,124,123,114,123,115,123,116,123,117,123,118,123,119,123,120,123,121,123,122,123,123,124,114,124,116,124,118,124,121,125,116,126,115,126,116,126,117,126,119,126,120,126,122,126,123,127,116,127,117,127,118,127,119,127,120,127,121,127,122 } );
            addToCreepLocsMap(BWAPI::TilePosition(117, 9), std::vector<int> { 109,10,109,11,109,9,110,10,110,11,110,12,110,13,110,7,110,8,110,9,111,10,111,11,111,12,111,13,111,14,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,13,112,14,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,14,113,15,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,14,114,15,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,14,115,15,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,15,116,16,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,15,117,16,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,15,118,16,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,15,119,16,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,15,120,16,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,15,121,16,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,14,122,15,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,13,123,14,123,5,123,6,123,7,123,8,123,9,124,11,124,13,124,5,124,7,124,8,125,7,126,10,126,12,126,14,126,6,126,7,126,9,127,10,127,11,127,12,127,13,127,7,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 118), std::vector<int> { 0,116,0,117,0,118,0,119,0,120,0,121,0,122,1,115,1,116,1,117,1,118,1,119,1,120,1,122,1,123,10,115,10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,10,124,10,125,11,113,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,11,124,11,125,12,114,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,12,123,12,124,13,114,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,13,123,13,124,14,114,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,14,123,14,124,15,115,15,116,15,117,15,118,15,119,15,120,15,121,15,122,15,123,16,115,16,116,16,117,16,118,16,119,16,120,16,121,16,122,16,123,17,116,17,117,17,118,17,119,17,120,17,121,17,122,18,118,18,119,18,120,2,115,2,116,2,118,3,114,3,115,3,116,3,118,3,121,4,114,4,115,4,116,4,117,4,118,4,119,4,120,4,121,4,122,4,123,5,114,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,5,123,6,113,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,7,115,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,7,124,8,115,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,8,124,8,125,9,115,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123,9,124,9,125 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 9), std::vector<int> { 0,10,0,11,0,12,0,13,0,7,0,8,0,9,1,11,1,13,1,14,1,6,1,7,1,8,10,10,10,11,10,12,10,13,10,14,10,15,10,16,10,6,10,7,10,8,10,9,11,10,11,11,11,12,11,13,11,14,11,15,11,16,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,13,12,14,12,15,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,13,13,14,13,15,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,13,14,14,14,15,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,12,15,13,15,14,15,6,15,7,15,8,15,9,16,10,16,11,16,12,16,13,16,14,16,6,16,7,16,8,16,9,17,10,17,11,17,12,17,13,17,7,17,8,17,9,18,10,18,11,18,9,2,6,2,7,3,10,3,12,3,5,3,6,3,7,3,9,4,10,4,11,4,12,4,13,4,14,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,13,5,14,5,5,5,6,5,7,5,8,5,9,6,10,6,11,6,12,6,13,6,14,6,16,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,14,7,15,7,16,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,14,8,15,8,16,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,14,9,15,9,16,9,6,9,7,9,8,9,9 } );
        }
        // Destination map version 1.1 (map name converted to plain text contains "Destination 1.1")
        else if (mapHash == "4e24f217d2fe4dbfa6799bc57f74d8dc939d425b")
        {
            addToCreepLocsMap(BWAPI::TilePosition(31, 7), std::vector<int> { 23,7,23,8,23,9,24,10,24,11,24,5,24,6,24,7,24,8,24,9,25,10,25,11,25,12,25,4,25,5,25,6,25,9,26,10,26,5,27,10,27,5,27,7,27,8,28,10,28,11,28,12,28,4,28,5,28,6,28,7,28,8,28,9,29,10,29,11,29,12,29,13,29,3,29,4,29,5,29,6,29,7,29,8,29,9,30,10,30,11,30,12,30,13,30,14,30,2,30,3,30,4,30,5,30,6,30,7,30,8,30,9,31,10,31,11,31,12,31,13,31,14,31,4,31,5,31,6,31,7,31,8,31,9,32,10,32,11,32,12,32,13,32,14,32,4,32,5,32,6,32,7,32,8,32,9,33,10,33,11,33,12,33,13,33,14,33,4,33,5,33,6,33,7,33,8,33,9,34,10,34,11,34,12,34,13,34,14,34,4,34,5,34,6,34,7,34,8,34,9,35,10,35,11,35,12,35,13,35,14,35,2,35,3,35,4,35,5,35,6,35,7,35,8,35,9,36,10,36,11,36,12,36,13,36,3,36,4,36,5,36,6,36,7,36,8,36,9,37,10,37,11,37,12,37,13,37,3,37,4,37,5,37,6,37,7,37,8,37,9,38,10,38,11,38,12,38,13,38,3,38,4,38,5,38,6,38,7,38,8,38,9,39,10,39,11,39,12,39,4,39,5,39,6,39,7,39,8,39,9,40,10,40,11,40,12,40,4,40,5,40,6,40,7,40,8,40,9,41,10,41,11,41,5,41,6,41,7,41,8,41,9,42,7,42,8,42,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(64, 118), std::vector<int> { 56,118,56,119,56,120,57,116,57,117,57,118,57,119,57,120,57,121,57,122,58,115,58,116,58,117,58,118,58,119,58,120,58,121,58,122,58,123,59,115,59,116,59,117,59,118,59,119,59,120,59,121,59,122,59,123,60,114,60,115,60,116,60,117,60,118,60,119,60,120,60,121,60,122,60,123,60,124,61,114,61,115,61,116,61,117,61,118,61,119,61,120,61,121,61,122,61,123,61,124,62,114,62,115,62,116,62,117,62,118,62,119,62,120,62,121,62,122,62,123,62,124,63,113,63,114,63,115,63,116,63,117,63,118,63,119,63,120,63,121,63,122,63,123,63,124,63,125,64,115,64,116,64,117,64,118,64,119,64,120,64,121,64,122,64,123,64,124,64,125,65,115,65,116,65,117,65,118,65,119,65,120,65,121,65,122,65,123,65,124,65,125,66,115,66,116,66,117,66,118,66,119,66,120,66,121,66,122,66,123,66,124,66,125,67,115,67,116,67,117,67,118,67,119,67,120,67,121,67,122,67,123,67,124,67,125,68,113,68,114,68,115,68,116,68,117,68,118,68,119,68,120,68,121,68,122,68,123,68,124,68,125,69,114,69,115,69,116,69,117,69,118,69,119,69,120,69,121,69,122,69,123,69,124,70,114,70,115,70,116,70,117,70,118,70,119,70,120,70,121,70,122,70,123,71,114,71,116,71,118,71,119,71,121,72,116,73,115,73,116,73,117,73,120,73,122,73,123,74,116,74,117,74,118,74,119,74,120,74,121,74,122,75,118,75,119,75,120 } );
        }
        // Fighting Spirit map version 1.3 (map name is not in English but says "1.3" at the end)
        // This variant is used in CIG. Note that this is different to the iCCup variant
        else if (mapHash == "5731c103687826de48ba3cc7d6e37e2537b0e902")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 117), std::vector<int> { 109,117,109,118,109,119,110,115,110,116,110,117,110,118,110,119,110,120,110,121,111,114,111,115,111,116,111,117,111,118,111,119,111,120,111,121,111,122,112,114,112,115,112,116,112,117,112,118,112,119,112,120,112,121,112,122,113,113,113,114,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,114,113,114,114,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,115,113,115,114,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,116,112,116,113,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,117,114,117,115,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,118,114,118,115,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,119,114,119,115,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,120,114,120,115,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,121,112,121,113,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,121,124,122,113,122,114,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,122,123,123,113,123,114,123,115,123,116,123,117,123,118,123,119,123,120,123,121,123,122,124,113,124,114,124,116,124,118,124,120,125,114,126,114,126,115,126,117,126,119,126,121,126,122,127,115,127,116,127,117,127,118,127,119,127,120,127,121 } );
            addToCreepLocsMap(BWAPI::TilePosition(117, 7), std::vector<int> { 109,7,109,8,109,9,110,10,110,11,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,12,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,2,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,4,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,4,119,5,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,2,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,3,123,4,123,5,123,6,123,7,123,8,123,9,124,10,124,3,124,4,124,6,124,8,125,4,126,11,126,12,126,4,126,5,126,7,126,9,127,10,127,11,127,5,127,6,127,7,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 116), std::vector<int> { 0,114,0,115,0,116,0,117,0,118,0,119,0,120,1,113,1,114,1,116,1,118,1,120,1,121,10,113,10,114,10,115,10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,11,111,11,112,11,113,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,12,112,12,113,12,114,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,13,112,13,113,13,114,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,14,112,14,113,14,114,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,15,113,15,114,15,115,15,116,15,117,15,118,15,119,15,120,15,121,16,113,16,114,16,115,16,116,16,117,16,118,16,119,16,120,16,121,17,114,17,115,17,116,17,117,17,118,17,119,17,120,18,116,18,117,18,118,2,113,3,112,3,113,3,115,3,117,3,119,4,112,4,113,4,114,4,115,4,116,4,117,4,118,4,119,4,120,4,121,5,112,5,113,5,114,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,6,111,6,112,6,113,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,7,113,7,114,7,115,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,8,113,8,114,8,115,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,9,113,9,114,9,115,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 6), std::vector<int> { 0,10,0,4,0,5,0,6,0,7,0,8,0,9,1,10,1,11,1,3,1,4,1,6,1,8,10,10,10,11,10,12,10,13,10,3,10,4,10,5,10,6,10,7,10,8,10,9,11,1,11,10,11,11,11,12,11,13,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,2,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,2,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,2,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,3,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,3,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,4,17,5,17,6,17,7,17,8,17,9,18,6,18,7,18,8,2,3,3,2,3,3,3,5,3,7,3,9,4,10,4,11,4,2,4,3,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,2,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,1,6,10,6,11,6,12,6,13,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,3,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,3,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,3,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // Hitchhiker map version 1.1 (map name converted to plain text contains "Hitchhiker1.1")
        else if (mapHash == "69a3b6a5a3d4120e47408defd3ca44c954997948")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 79), std::vector<int> { 109,79,109,80,109,81,110,77,110,78,110,79,110,80,110,81,110,82,110,83,111,76,111,77,111,78,111,79,111,80,111,81,111,82,111,83,111,84,112,76,112,77,112,78,112,79,112,80,112,81,112,82,112,83,112,84,113,75,113,76,113,77,113,78,113,79,113,80,113,81,113,82,113,83,113,84,113,85,114,75,114,76,114,77,114,78,114,79,114,80,114,81,114,82,114,83,114,84,114,85,115,75,115,76,115,77,115,78,115,79,115,80,115,81,115,82,115,83,115,84,115,85,116,74,116,75,116,76,116,77,116,78,116,79,116,80,116,81,116,82,116,83,116,84,116,85,116,86,117,76,117,77,117,78,117,79,117,80,117,81,117,82,117,83,117,84,117,85,117,86,118,76,118,77,118,78,118,79,118,80,118,81,118,82,118,83,118,84,118,85,118,86,119,76,119,77,119,78,119,79,119,80,119,81,119,82,119,83,119,84,119,85,119,86,120,76,120,77,120,78,120,79,120,80,120,81,120,82,120,83,120,84,120,85,120,86,121,74,121,75,121,76,121,77,121,78,121,79,121,80,121,81,121,82,121,83,121,84,121,85,121,86,122,75,122,76,122,77,122,78,122,79,122,80,122,81,122,82,122,83,122,84,122,85,123,75,123,76,123,77,123,78,123,79,123,80,123,81,123,82,123,83,123,84,124,75,124,76,124,78,124,79,124,82,125,76,126,76,126,77,126,80,126,81,126,83,126,84,127,77,127,78,127,79,127,80,127,81,127,82,127,83 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 6), std::vector<int> { 0,10,0,4,0,5,0,6,0,7,0,8,0,9,1,10,1,11,1,3,1,4,1,6,1,7,1,8,10,10,10,11,10,12,10,13,10,3,10,4,10,5,10,6,10,7,10,8,10,9,11,1,11,10,11,11,11,12,11,13,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,2,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,2,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,2,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,3,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,3,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,4,17,5,17,6,17,7,17,8,17,9,18,6,18,7,18,8,2,7,3,2,3,5,3,7,3,9,4,10,4,11,4,2,4,3,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,2,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,1,6,10,6,11,6,12,6,13,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,3,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,3,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,3,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // Plasma map version 1.0 (map name converted to plain text contains "Plasma 1.0")
        else if (mapHash == "6f5295624a7e3887470f3f2e14727b1411321a67")
        {
            isMapPlasma_v_1_0 = true;
            
            addToCreepLocsMap(BWAPI::TilePosition(14, 110), std::vector<int> { 10,106,10,107,10,108,10,109,10,111,10,112,10,114,11,106,11,107,11,108,11,109,11,110,11,111,11,112,11,113,11,114,11,115,12,106,12,107,12,108,12,109,12,110,12,111,12,112,12,113,12,114,12,115,13,105,13,106,13,107,13,108,13,109,13,110,13,111,13,112,13,113,13,114,13,115,14,107,14,108,14,109,14,110,14,111,14,112,14,113,14,114,14,115,14,116,15,107,15,108,15,109,15,110,15,111,15,112,15,113,15,114,15,115,15,117,16,107,16,108,16,109,16,110,16,111,16,112,16,113,16,114,16,115,16,117,17,107,17,108,17,109,17,110,17,111,17,112,17,113,17,114,17,115,17,116,17,117,18,105,18,106,18,107,18,108,18,109,18,110,18,111,18,112,18,113,18,114,18,115,18,116,18,117,19,106,19,107,19,108,19,109,19,110,19,111,19,112,19,113,19,114,19,115,19,116,20,106,20,107,20,108,20,109,20,110,20,111,20,112,20,113,20,114,20,115,20,116,21,106,21,107,21,108,21,109,21,110,21,111,21,112,21,113,21,114,21,115,21,116,22,107,22,108,22,109,22,110,22,111,22,112,22,113,22,114,22,115,23,107,23,108,23,109,23,110,23,111,23,112,23,113,23,114,23,115,24,108,24,109,24,110,24,111,24,112,24,113,24,114,25,110,25,111,25,112,6,110,6,111,6,112,7,108,7,109,7,110,7,111,7,112,7,113,7,114,8,107,8,108,8,109,8,110,8,113,8,114,8,115,9,107,9,108,9,109,9,114 } );
            addToCreepLocsMap(BWAPI::TilePosition(14, 14), std::vector<int> { 10,11,10,13,10,15,10,17,10,18,11,11,11,12,11,13,11,14,11,15,11,16,11,17,11,18,11,19,12,10,12,11,12,12,12,13,12,14,12,15,12,16,12,17,12,18,12,19,12,20,13,10,13,11,13,12,13,13,13,14,13,15,13,16,13,17,13,18,13,19,13,20,13,21,13,9,14,11,14,12,14,13,14,14,14,15,14,16,14,17,14,18,14,19,14,20,14,21,15,11,15,12,15,13,15,14,15,15,15,16,15,17,15,18,15,19,15,20,15,21,16,11,16,12,16,13,16,14,16,15,16,16,16,17,16,18,16,19,16,20,16,21,17,11,17,12,17,13,17,14,17,15,17,16,17,17,17,18,17,19,17,20,17,21,18,10,18,11,18,12,18,13,18,14,18,15,18,16,18,17,18,18,18,19,18,20,18,21,18,9,19,10,19,11,19,12,19,13,19,14,19,15,19,16,19,17,19,18,19,19,19,20,20,10,20,11,20,12,20,13,20,14,20,15,20,16,20,17,20,18,20,19,20,20,21,10,21,11,21,12,21,13,21,14,21,15,21,16,21,17,21,18,21,19,21,20,22,11,22,12,22,13,22,14,22,15,22,16,22,17,22,18,22,19,23,11,23,12,23,13,23,14,23,15,23,16,23,17,23,18,23,19,24,12,24,13,24,14,24,15,24,16,24,17,24,18,25,14,25,15,25,16,6,14,6,15,6,16,7,12,7,13,7,14,7,15,7,16,7,17,7,18,8,11,8,12,8,14,8,16,8,17,8,19,9,11,9,17 } );
            addToCreepLocsMap(BWAPI::TilePosition(77, 63), std::vector<int> { 70,61,70,62,70,63,70,64,70,65,70,66,70,67,71,60,71,61,71,62,71,63,71,64,71,65,71,66,71,67,71,68,72,60,72,61,72,62,72,63,72,64,72,65,72,66,72,67,72,68,73,59,73,60,73,61,73,62,73,63,73,64,73,65,73,66,73,67,73,68,73,69,74,59,74,60,74,61,74,62,74,63,74,64,74,65,74,66,74,67,74,68,74,69,75,59,75,60,75,61,75,62,75,63,75,64,75,65,75,66,75,67,75,68,75,69,76,58,76,59,76,60,76,61,76,62,76,63,76,64,76,65,76,66,76,67,76,68,76,69,76,70,77,60,77,61,77,62,77,63,77,64,77,65,77,66,77,67,77,68,77,69,77,70,78,60,78,61,78,62,78,63,78,64,78,65,78,66,78,67,78,68,78,69,78,70,79,60,79,61,79,62,79,63,79,64,79,65,79,66,79,67,79,68,79,69,79,70,80,60,80,61,80,62,80,63,80,64,80,65,80,66,80,67,80,68,80,69,80,70,81,58,81,59,81,60,81,61,81,62,81,63,81,64,81,65,81,66,81,67,81,68,81,69,81,70,82,59,82,60,82,61,82,62,82,63,82,64,82,65,82,66,82,67,82,68,82,69,83,59,83,60,83,61,83,62,83,63,83,64,83,65,83,66,83,67,83,68,84,59,84,61,84,63,84,65,84,66,85,66,86,60,86,62,86,64,86,66,86,67,86,68,87,61,87,62,87,63,87,64,87,65,87,66,87,67,88,63,88,64,88,65 } );
        }
        // Heartbreak Ridge map version 1.1 (map name is not in English but says "1.1" at the end)
        else if (mapHash == "6f8da3c3cc8d08d9cf882700efa049280aedca8c")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 56), std::vector<int> { 109,56,109,57,109,58,110,54,110,55,110,56,110,57,110,58,110,59,110,60,111,53,111,54,111,55,111,56,111,57,111,58,111,59,111,60,111,61,112,53,112,54,112,55,112,56,112,57,112,58,112,59,112,60,112,61,113,52,113,53,113,54,113,55,113,56,113,57,113,58,113,59,113,60,113,61,113,62,114,52,114,53,114,54,114,55,114,56,114,57,114,58,114,59,114,60,114,61,114,62,115,52,115,53,115,54,115,55,115,56,115,57,115,58,115,59,115,60,115,61,115,62,116,51,116,52,116,53,116,54,116,55,116,56,116,57,116,58,116,59,116,60,116,61,116,62,116,63,117,53,117,54,117,55,117,56,117,57,117,58,117,59,117,60,117,61,117,62,117,63,118,53,118,54,118,55,118,56,118,57,118,58,118,59,118,60,118,61,118,62,118,63,119,53,119,54,119,55,119,56,119,57,119,58,119,59,119,60,119,61,119,62,119,63,120,53,120,54,120,55,120,56,120,57,120,58,120,59,120,60,120,61,120,62,120,63,121,51,121,52,121,53,121,54,121,55,121,56,121,57,121,58,121,59,121,60,121,61,121,62,121,63,122,52,122,53,122,54,122,55,122,56,122,57,122,58,122,59,122,60,122,61,122,62,123,53,123,54,123,55,123,56,123,57,123,58,123,59,123,60,123,61,123,62,124,54,124,55,124,57,124,58,124,60,124,62,125,54,126,53,126,54,126,56,126,59,126,61,127,54,127,55,127,56,127,57,127,58,127,59,127,60 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 37), std::vector<int> { 0,35,0,36,0,37,0,38,0,39,0,40,0,41,1,34,1,35,1,37,1,40,1,42,10,34,10,35,10,36,10,37,10,38,10,39,10,40,10,41,10,42,10,43,10,44,11,32,11,33,11,34,11,35,11,36,11,37,11,38,11,39,11,40,11,41,11,42,11,43,11,44,12,33,12,34,12,35,12,36,12,37,12,38,12,39,12,40,12,41,12,42,12,43,13,33,13,34,13,35,13,36,13,37,13,38,13,39,13,40,13,41,13,42,13,43,14,33,14,34,14,35,14,36,14,37,14,38,14,39,14,40,14,41,14,42,14,43,15,34,15,35,15,36,15,37,15,38,15,39,15,40,15,41,15,42,16,34,16,35,16,36,16,37,16,38,16,39,16,40,16,41,16,42,17,35,17,36,17,37,17,38,17,39,17,40,17,41,18,37,18,38,18,39,2,35,3,35,3,36,3,38,3,39,3,41,3,43,4,34,4,35,4,36,4,37,4,38,4,39,4,40,4,41,4,42,4,43,5,33,5,34,5,35,5,36,5,37,5,38,5,39,5,40,5,41,5,42,5,43,6,32,6,33,6,34,6,35,6,36,6,37,6,38,6,39,6,40,6,41,6,42,6,43,6,44,7,34,7,35,7,36,7,37,7,38,7,39,7,40,7,41,7,42,7,43,7,44,8,34,8,35,8,36,8,37,8,38,8,39,8,40,8,41,8,42,8,43,8,44,9,34,9,35,9,36,9,37,9,38,9,39,9,40,9,41,9,42,9,43,9,44 } );
        }
        // Alchemist map version 1.0 (map name converted to plain text contains "Alchemist 1.0")
        else if (mapHash == "8000dc6116e405ab878c14bb0f0cde8efa4d640c")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 51), std::vector<int> { 109,51,109,52,109,53,110,49,110,50,110,51,110,52,110,53,110,54,110,55,111,48,111,49,111,50,111,51,111,52,111,53,111,54,111,55,111,56,112,48,112,49,112,50,112,51,112,52,112,53,112,54,112,55,112,56,113,47,113,48,113,49,113,50,113,51,113,52,113,53,113,54,113,55,113,56,113,57,114,47,114,48,114,49,114,50,114,51,114,52,114,53,114,54,114,55,114,56,114,57,115,47,115,48,115,49,115,50,115,51,115,52,115,53,115,54,115,55,115,56,115,57,116,46,116,47,116,48,116,49,116,50,116,51,116,52,116,53,116,54,116,55,116,56,116,57,116,58,117,48,117,49,117,50,117,51,117,52,117,53,117,54,117,55,117,56,117,57,117,58,118,48,118,49,118,50,118,51,118,52,118,53,118,54,118,55,118,56,118,57,118,58,119,48,119,49,119,50,119,51,119,52,119,53,119,54,119,55,119,56,119,57,119,58,120,48,120,49,120,50,120,51,120,52,120,53,120,54,120,55,120,56,120,57,120,58,121,46,121,47,121,48,121,49,121,50,121,51,121,52,121,53,121,54,121,55,121,56,121,58,122,47,122,48,122,49,122,50,122,51,122,52,122,53,122,54,122,55,122,56,123,47,123,48,123,49,123,50,123,51,123,52,123,53,123,54,123,55,123,56,124,47,124,48,124,50,124,51,124,53,124,54,124,55,125,48,125,54,126,48,126,49,126,52,126,54,126,56,127,49,127,50,127,51,127,52,127,53,127,54,127,55 } );
            addToCreepLocsMap(BWAPI::TilePosition(43, 118), std::vector<int> { 35,118,35,119,35,120,36,116,36,117,36,120,36,121,36,122,37,115,37,116,37,117,37,120,37,121,37,122,37,123,38,115,38,116,38,117,38,120,38,121,38,122,38,123,39,114,39,115,39,116,39,117,39,120,39,121,39,122,39,123,39,124,40,114,40,115,40,116,40,117,40,118,40,119,40,120,40,121,40,122,40,123,40,124,41,114,41,115,41,116,41,117,41,118,41,119,41,120,41,121,41,122,41,123,42,113,42,114,42,115,42,116,42,117,42,118,42,119,42,120,42,121,42,122,42,123,42,125,43,113,43,114,43,115,43,116,43,117,43,118,43,119,43,120,43,121,43,122,43,123,43,125,44,113,44,114,44,115,44,116,44,117,44,118,44,119,44,120,44,121,44,122,44,123,44,125,45,113,45,114,45,115,45,116,45,117,45,118,45,119,45,120,45,121,45,122,45,123,45,124,46,113,46,114,46,115,46,116,46,117,46,118,46,119,46,120,46,121,46,122,46,123,47,113,47,114,47,115,47,116,47,117,47,118,47,119,47,120,47,121,47,122,47,123,48,114,48,115,48,116,48,117,48,118,48,119,48,120,48,121,48,122,48,123,48,124,49,114,49,115,49,116,49,117,49,118,49,119,49,120,49,121,49,122,49,123,50,114,50,115,50,116,50,117,50,118,50,119,50,120,50,122,51,115,51,116,51,117,51,118,51,119,51,122,52,115,52,116,52,117,52,118,52,119,52,121,52,122,52,123,53,116,53,117,53,118,53,119,53,120,53,121,53,122,54,118,54,119,54,120 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 7), std::vector<int> { 0,7,0,8,0,9,1,10,1,11,1,5,1,6,1,7,1,8,1,9,10,10,10,11,10,12,10,13,10,14,10,4,10,5,10,6,10,7,10,8,10,9,11,10,11,11,11,12,11,13,11,14,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,13,12,14,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,13,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,13,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,12,15,13,15,3,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,12,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,11,17,12,17,4,17,5,17,6,17,7,17,8,17,9,18,10,18,11,18,5,18,6,18,7,18,8,18,9,19,7,19,8,19,9,2,11,2,12,2,4,2,5,2,7,2,9,3,11,3,7,4,10,4,11,4,3,4,6,4,7,4,8,5,10,5,11,5,12,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,10,6,11,6,12,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,14,7,2,7,3,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,14,8,2,8,3,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,14,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // The Fortress map version 1.1 (map name converted to plain text contains "The Fortress 1.1")
        else if (mapHash == "83320e505f35c65324e93510ce2eafbaa71c9aa1")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 54), std::vector<int> { 109,54,109,55,109,56,110,52,110,53,110,54,110,55,110,56,110,57,110,58,111,51,111,52,111,53,111,54,111,55,111,56,111,57,111,58,111,59,112,51,112,52,112,53,112,54,112,55,112,56,112,57,112,58,112,59,113,50,113,51,113,52,113,53,113,54,113,55,113,56,113,57,113,58,113,59,113,60,114,50,114,51,114,52,114,53,114,54,114,55,114,56,114,57,114,58,114,59,114,60,115,50,115,51,115,52,115,53,115,54,115,55,115,56,115,57,115,58,115,59,115,60,116,49,116,50,116,51,116,52,116,53,116,54,116,55,116,56,116,57,116,58,116,59,116,60,116,61,117,51,117,52,117,53,117,54,117,55,117,56,117,57,117,58,117,59,117,60,117,61,118,51,118,52,118,53,118,54,118,55,118,56,118,57,118,58,118,59,118,60,118,61,119,51,119,52,119,53,119,54,119,55,119,56,119,57,119,58,119,59,119,60,119,61,120,51,120,52,120,53,120,54,120,55,120,56,120,57,120,58,120,59,120,60,120,61,121,49,121,50,121,51,121,52,121,53,121,54,121,55,121,56,121,57,121,58,121,59,121,60,121,61,122,50,122,51,122,52,122,53,122,54,122,55,122,56,122,57,122,58,122,59,122,60,123,50,123,51,123,52,123,53,123,54,123,55,123,56,123,57,123,58,123,59,124,50,124,51,124,53,124,55,124,57,125,51,126,51,126,52,126,54,126,56,126,58,126,59,127,52,127,53,127,54,127,55,127,56,127,57,127,58 } );
            addToCreepLocsMap(BWAPI::TilePosition(49, 7), std::vector<int> { 41,7,41,8,41,9,42,10,42,11,42,5,42,6,42,7,42,8,42,9,43,11,43,12,43,4,43,5,43,6,43,7,43,9,44,6,45,10,45,3,45,6,45,8,46,10,46,11,46,12,46,3,46,4,46,5,46,6,46,7,46,8,46,9,47,10,47,11,47,12,47,13,47,3,47,4,47,5,47,6,47,7,47,8,47,9,48,10,48,11,48,12,48,13,48,14,48,2,48,3,48,4,48,5,48,6,48,7,48,8,48,9,49,10,49,11,49,12,49,13,49,14,49,4,49,5,49,6,49,7,49,8,49,9,50,10,50,11,50,12,50,13,50,14,50,4,50,5,50,6,50,7,50,8,50,9,51,10,51,11,51,12,51,13,51,14,51,4,51,5,51,6,51,7,51,8,51,9,52,10,52,11,52,12,52,13,52,14,52,4,52,5,52,6,52,7,52,8,52,9,53,10,53,11,53,12,53,13,53,14,53,2,53,3,53,4,53,5,53,6,53,7,53,8,53,9,54,10,54,11,54,12,54,13,54,3,54,4,54,5,54,6,54,7,54,8,54,9,55,10,55,11,55,12,55,13,55,3,55,4,55,5,55,6,55,7,55,8,55,9,56,10,56,11,56,12,56,13,56,3,56,4,56,5,56,6,56,7,56,8,56,9,57,10,57,11,57,12,57,4,57,5,57,6,57,7,57,8,57,9,58,10,58,11,58,12,58,4,58,5,58,6,58,7,58,8,58,9,59,10,59,11,59,5,59,6,59,7,59,8,59,9,60,7,60,8,60,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 74), std::vector<int> { 0,72,0,73,0,74,0,75,0,76,0,77,0,78,1,71,1,72,1,73,1,74,1,76,1,78,1,79,10,71,10,72,10,73,10,74,10,75,10,76,10,77,10,78,10,79,10,80,10,81,11,69,11,70,11,71,11,72,11,73,11,74,11,75,11,76,11,77,11,78,11,79,11,80,11,81,12,70,12,71,12,72,12,73,12,74,12,75,12,76,12,77,12,78,12,79,12,80,13,70,13,71,13,72,13,73,13,74,13,75,13,76,13,77,13,78,13,79,13,80,14,70,14,71,14,72,14,73,14,74,14,75,14,76,14,77,14,78,14,79,14,80,15,71,15,72,15,73,15,74,15,75,15,76,15,77,15,78,15,79,16,71,16,72,16,73,16,74,16,75,16,76,16,77,16,78,16,79,17,72,17,73,17,74,17,75,17,76,17,77,17,78,18,74,18,75,18,76,2,71,2,73,3,70,3,71,3,73,3,75,3,77,4,70,4,71,4,72,4,73,4,74,4,75,4,76,4,77,4,78,4,79,5,70,5,71,5,72,5,73,5,74,5,75,5,76,5,77,5,78,5,79,6,69,6,70,6,71,6,72,6,73,6,74,6,75,6,76,6,77,6,78,6,79,6,81,7,71,7,72,7,73,7,74,7,75,7,76,7,77,7,78,7,79,7,80,7,81,8,71,8,72,8,73,8,74,8,75,8,76,8,77,8,78,8,79,8,80,8,81,9,71,9,72,9,73,9,74,9,75,9,76,9,77,9,78,9,79,9,80,9,81 } );
            addToCreepLocsMap(BWAPI::TilePosition(77, 119), std::vector<int> { 69,119,69,120,69,121,70,117,70,118,70,119,70,120,70,121,70,122,70,123,71,116,71,117,71,118,71,119,71,120,71,121,71,122,71,123,71,124,72,116,72,117,72,118,72,119,72,120,72,121,72,122,72,123,72,124,73,115,73,116,73,117,73,118,73,119,73,120,73,121,73,122,73,123,73,124,73,125,74,115,74,116,74,117,74,118,74,119,74,120,74,121,74,122,74,123,74,124,74,125,75,115,75,116,75,117,75,118,75,119,75,120,75,121,75,122,75,123,75,124,75,125,76,114,76,115,76,116,76,117,76,118,76,119,76,120,76,121,76,122,76,123,76,124,76,125,77,116,77,117,77,118,77,119,77,120,77,121,77,122,77,123,77,124,77,125,78,116,78,117,78,118,78,119,78,120,78,121,78,122,78,123,78,124,78,125,79,116,79,117,79,118,79,119,79,120,79,121,79,122,79,123,79,124,79,125,80,116,80,117,80,118,80,119,80,120,80,121,80,122,80,123,80,124,80,125,81,114,81,115,81,116,81,117,81,118,81,119,81,120,81,121,81,122,81,123,81,124,81,125,82,115,82,116,82,117,82,118,82,119,82,120,82,121,82,122,82,123,82,124,82,125,83,115,83,116,83,117,83,118,83,119,83,120,83,121,83,122,83,123,83,124,84,115,84,116,84,118,84,121,84,123,85,116,86,116,86,117,86,119,86,120,86,122,86,124,87,117,87,118,87,119,87,120,87,121,87,122,87,123,88,119,88,120,88,121 } );
        }
        // Python map version 1.3 (map name converted to plain text contains "Python 1.3")
        // This version is used in CIG
        else if (mapHash == "86afe0f744865befb15f65d47865f9216edc37e5")
        {
            addToCreepLocsMap(BWAPI::TilePosition(116, 40), std::vector<int> { 108,40,108,41,108,42,109,38,109,39,109,40,109,41,109,42,109,43,109,44,110,37,110,38,110,39,110,40,110,41,110,42,110,43,110,44,110,45,111,37,111,38,111,39,111,40,111,41,111,42,111,43,111,44,111,45,112,36,112,37,112,38,112,39,112,40,112,41,112,42,112,43,112,44,112,45,112,46,113,36,113,37,113,38,113,39,113,40,113,41,113,42,113,43,113,44,113,45,113,46,114,36,114,37,114,38,114,39,114,40,114,41,114,42,114,43,114,44,114,45,114,46,115,35,115,36,115,37,115,38,115,39,115,40,115,41,115,42,115,43,115,44,115,45,115,46,115,47,116,37,116,38,116,39,116,40,116,41,116,42,116,43,116,44,116,45,116,46,116,47,117,37,117,38,117,39,117,40,117,41,117,42,117,43,117,44,117,45,117,46,117,47,118,37,118,38,118,39,118,40,118,41,118,42,118,43,118,44,118,45,118,46,118,47,119,37,119,38,119,39,119,40,119,41,119,42,119,43,119,44,119,45,119,46,119,47,120,35,120,36,120,37,120,38,120,39,120,40,120,41,120,42,120,43,120,44,120,45,120,46,120,47,121,36,121,37,121,38,121,39,121,40,121,41,121,42,121,43,121,44,121,45,121,46,122,36,122,37,122,38,122,39,122,40,122,41,122,42,122,43,122,44,122,45,123,36,123,39,123,41,123,42,123,43,124,42,125,37,125,38,125,40,125,42,125,44,125,45,126,38,126,39,126,40,126,41,126,42,126,43,126,44,127,40,127,41,127,42 } );
            addToCreepLocsMap(BWAPI::TilePosition(42, 119), std::vector<int> { 34,119,34,120,34,121,35,117,35,118,35,119,35,120,35,121,35,122,35,123,36,116,36,118,36,120,36,122,36,123,36,124,37,123,38,115,38,117,38,119,38,121,38,123,39,115,39,116,39,117,39,118,39,119,39,120,39,121,39,122,39,123,39,124,40,115,40,116,40,117,40,118,40,119,40,120,40,121,40,122,40,123,40,124,40,125,41,114,41,115,41,116,41,117,41,118,41,119,41,120,41,121,41,122,41,123,41,124,41,125,42,116,42,117,42,118,42,119,42,120,42,121,42,122,42,123,42,124,42,125,43,116,43,117,43,118,43,119,43,120,43,121,43,122,43,123,43,124,43,125,44,116,44,117,44,118,44,119,44,120,44,121,44,122,44,123,44,124,44,125,45,116,45,117,45,118,45,119,45,120,45,121,45,122,45,123,45,124,45,125,46,114,46,115,46,116,46,117,46,118,46,119,46,120,46,121,46,122,46,123,46,124,46,125,47,115,47,116,47,117,47,118,47,119,47,120,47,121,47,122,47,123,47,124,47,125,48,115,48,116,48,117,48,118,48,119,48,120,48,121,48,122,48,123,48,124,48,125,49,115,49,116,49,117,49,118,49,119,49,120,49,121,49,122,49,123,49,124,49,125,50,116,50,117,50,118,50,119,50,120,50,121,50,122,50,123,50,124,51,116,51,117,51,118,51,119,51,120,51,121,51,122,51,123,51,124,52,117,52,118,52,119,52,120,52,121,52,122,52,123,53,119,53,120,53,121 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 85), std::vector<int> { 0,85,0,86,0,87,1,83,1,84,1,85,1,86,1,87,1,88,1,89,10,82,10,83,10,84,10,85,10,86,10,87,10,88,10,89,10,90,10,91,10,92,11,82,11,83,11,84,11,85,11,86,11,87,11,88,11,89,11,90,11,91,11,92,12,80,12,81,12,82,12,83,12,84,12,85,12,86,12,87,12,88,12,89,12,90,12,91,12,92,13,81,13,82,13,83,13,84,13,85,13,86,13,87,13,88,13,89,13,90,13,91,14,81,14,82,14,83,14,84,14,85,14,86,14,87,14,88,14,89,14,90,14,91,15,81,15,82,15,83,15,84,15,85,15,86,15,87,15,88,15,89,15,90,15,91,16,82,16,83,16,84,16,85,16,86,16,87,16,88,16,89,16,90,17,82,17,83,17,84,17,85,17,86,17,87,17,88,17,89,17,90,18,83,18,84,18,85,18,86,18,87,18,88,18,89,19,85,19,86,19,87,2,82,2,85,2,87,2,89,2,90,3,90,4,83,4,84,4,86,4,88,4,90,4,91,5,82,5,83,5,84,5,85,5,86,5,87,5,88,5,89,5,90,5,91,6,81,6,82,6,83,6,84,6,85,6,86,6,87,6,88,6,89,6,90,6,91,7,80,7,81,7,82,7,83,7,84,7,85,7,86,7,87,7,88,7,89,7,90,7,91,7,92,8,82,8,83,8,84,8,85,8,86,8,87,8,88,8,89,8,90,8,91,8,92,9,82,9,83,9,84,9,85,9,86,9,87,9,88,9,89,9,90,9,91,9,92 } );
            addToCreepLocsMap(BWAPI::TilePosition(83, 6), std::vector<int> { 75,6,75,7,75,8,76,10,76,4,76,5,76,6,76,7,76,8,76,9,77,10,77,11,77,3,77,4,77,5,77,6,77,7,77,8,77,9,78,10,78,11,78,3,78,4,78,5,78,6,78,7,78,8,78,9,79,10,79,11,79,12,79,2,79,3,79,4,79,5,79,6,79,7,79,8,79,9,80,10,80,11,80,12,80,2,80,3,80,4,80,5,80,6,80,7,80,8,80,9,81,10,81,11,81,12,81,2,81,3,81,4,81,5,81,6,81,7,81,8,81,9,82,1,82,10,82,11,82,12,82,13,82,2,82,3,82,4,82,5,82,6,82,7,82,8,82,9,83,10,83,11,83,12,83,13,83,3,83,4,83,5,83,6,83,7,83,8,83,9,84,10,84,11,84,12,84,13,84,3,84,4,84,5,84,6,84,7,84,8,84,9,85,10,85,11,85,12,85,13,85,3,85,4,85,5,85,6,85,7,85,8,85,9,86,10,86,11,86,12,86,13,86,3,86,4,86,5,86,6,86,7,86,8,86,9,87,1,87,10,87,11,87,12,87,13,87,2,87,3,87,4,87,5,87,6,87,7,87,8,87,9,88,10,88,11,88,12,88,2,88,3,88,4,88,5,88,6,88,7,88,8,88,9,89,10,89,11,89,3,89,4,89,5,89,6,89,7,89,8,89,9,90,10,90,4,90,5,90,6,90,8,91,10,91,5,92,10,92,11,92,3,92,5,92,7,92,9,93,10,93,4,93,5,93,6,93,7,93,8,93,9,94,6,94,7,94,8 } );
        }
        // Electric Circuit map unknown version (map name converted to plain text contains "Electric Circuit")
        else if (mapHash == "9505d618c63a0959f0c0bfe21c253a2ea6e58d26")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 119), std::vector<int> { 109,119,109,120,109,121,110,117,110,118,110,119,110,120,110,121,110,122,110,123,111,116,111,117,111,118,111,119,111,120,111,121,111,122,111,123,111,124,112,116,112,117,112,118,112,119,112,120,112,121,112,122,112,123,112,124,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,113,124,113,125,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,114,124,114,125,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,115,124,115,125,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,116,125,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,117,125,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,118,125,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,119,125,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,120,125,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,121,124,121,125,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,122,123,122,124,122,125,123,116,123,117,123,118,123,119,123,120,123,121,123,122,123,123,123,124,124,118,124,119,124,121,124,123,125,118,126,116,126,117,126,118,126,120,126,122,126,124,127,117,127,118,127,119,127,120,127,121,127,122,127,123 } );
            addToCreepLocsMap(BWAPI::TilePosition(117, 7), std::vector<int> { 109,8,109,9,110,10,110,11,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,12,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,2,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,4,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,4,119,5,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,2,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,13,123,4,123,5,123,6,123,7,123,8,123,9,124,11,124,13,124,6,124,7,124,9,125,6,126,10,126,12,126,4,126,5,126,6,126,8,127,10,127,11,127,5,127,6,127,7,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 119), std::vector<int> { 0,117,0,118,0,119,0,120,0,121,0,122,0,123,1,116,1,117,1,118,1,120,1,122,1,124,10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,10,124,10,125,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,11,124,11,125,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,12,123,12,124,12,125,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,13,123,13,124,13,125,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,14,123,14,124,14,125,15,116,15,117,15,118,15,119,15,120,15,121,15,122,15,123,15,124,16,116,16,117,16,118,16,119,16,120,16,121,16,122,16,123,16,124,17,117,17,118,17,119,17,120,17,121,17,122,17,123,2,118,3,118,3,119,3,121,3,123,4,116,4,117,4,118,4,119,4,120,4,121,4,122,4,123,4,124,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,5,123,5,124,5,125,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,6,124,6,125,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,7,124,7,125,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,8,124,8,125,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123,9,124,9,125 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 7), std::vector<int> { 0,10,0,11,0,5,0,6,0,7,0,8,0,9,1,10,1,12,1,4,1,5,1,6,1,8,10,10,10,11,10,12,10,13,10,14,10,4,10,5,10,6,10,7,10,8,10,9,11,10,11,11,11,12,11,13,11,14,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,13,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,13,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,13,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,12,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,12,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,11,17,5,17,6,17,7,17,8,17,9,18,7,18,8,18,9,2,6,3,11,3,13,3,6,3,7,3,9,4,10,4,11,4,12,4,13,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,13,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,10,6,11,6,12,6,13,6,14,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,14,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,14,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,14,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // Roadrunner map version 1.2 (map name converted to plain text contains "Roadrunner_SE 1.2")
        else if (mapHash == "9a4498a896b28d115129624f1c05322f48188fe0")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 35), std::vector<int> { 109,35,109,36,109,37,110,33,110,34,110,35,110,36,110,37,110,38,110,39,111,32,111,33,111,34,111,35,111,36,111,37,111,38,111,39,111,40,112,32,112,33,112,34,112,35,112,36,112,37,112,38,112,39,112,40,113,31,113,32,113,33,113,34,113,35,113,36,113,37,113,38,113,39,113,40,113,41,114,31,114,32,114,33,114,34,114,35,114,36,114,37,114,38,114,39,114,40,114,41,115,31,115,32,115,33,115,34,115,35,115,36,115,37,115,38,115,39,115,40,115,41,116,30,116,31,116,32,116,33,116,34,116,35,116,36,116,37,116,38,116,39,116,40,116,41,116,42,117,32,117,33,117,34,117,35,117,36,117,37,117,38,117,39,117,40,117,41,117,42,118,32,118,33,118,34,118,35,118,36,118,37,118,38,118,39,118,40,118,41,118,42,119,32,119,33,119,34,119,35,119,36,119,37,119,38,119,39,119,40,119,41,119,42,120,32,120,33,120,34,120,35,120,36,120,37,120,38,120,39,120,40,120,41,120,42,121,30,121,31,121,32,121,33,121,34,121,35,121,36,121,37,121,38,121,39,121,40,121,41,121,42,122,31,122,32,122,33,122,34,122,35,122,36,122,37,122,38,122,39,122,40,122,41,123,31,123,32,123,33,123,34,123,35,123,36,123,37,123,38,123,39,123,40,124,31,124,33,124,35,124,37,124,38,125,38,126,32,126,34,126,36,126,38,126,39,126,40,127,33,127,34,127,35,127,36,127,37,127,38,127,39 } );
            addToCreepLocsMap(BWAPI::TilePosition(27, 6), std::vector<int> { 19,6,19,7,19,8,20,10,20,4,20,5,20,6,20,7,20,8,20,9,21,11,21,3,21,5,21,7,21,8,21,9,22,9,23,10,23,12,23,4,23,6,23,9,24,10,24,11,24,12,24,3,24,4,24,5,24,6,24,7,24,8,24,9,25,10,25,11,25,12,25,2,25,3,25,4,25,5,25,6,25,7,25,8,25,9,26,1,26,10,26,11,26,12,26,13,26,2,26,3,26,4,26,5,26,6,26,7,26,8,26,9,27,10,27,11,27,12,27,13,27,3,27,4,27,5,27,6,27,7,27,8,27,9,28,10,28,11,28,12,28,13,28,3,28,4,28,5,28,6,28,7,28,8,28,9,29,10,29,11,29,12,29,13,29,3,29,4,29,5,29,6,29,7,29,8,29,9,30,10,30,11,30,12,30,13,30,3,30,4,30,5,30,6,30,7,30,8,30,9,31,1,31,10,31,11,31,12,31,13,31,2,31,3,31,4,31,5,31,6,31,7,31,8,31,9,32,10,32,11,32,12,32,2,32,3,32,4,32,5,32,6,32,7,32,8,32,9,33,10,33,11,33,12,33,2,33,3,33,4,33,5,33,6,33,7,33,8,33,9,34,10,34,11,34,12,34,2,34,3,34,4,34,5,34,6,34,7,34,8,34,9,35,10,35,11,35,3,35,4,35,5,35,6,35,7,35,8,35,9,36,10,36,11,36,3,36,4,36,5,36,6,36,7,36,8,36,9,37,10,37,4,37,5,37,6,37,7,37,8,37,9,38,6,38,7,38,8 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 90), std::vector<int> { 0,88,0,89,0,90,0,91,0,92,0,93,0,94,1,87,1,89,1,91,1,93,1,94,1,95,10,87,10,88,10,89,10,90,10,91,10,92,10,93,10,94,10,95,10,96,10,97,11,85,11,86,11,87,11,88,11,89,11,90,11,91,11,92,11,93,11,94,11,95,11,96,11,97,12,86,12,87,12,88,12,89,12,90,12,91,12,92,12,93,12,94,12,95,12,96,13,86,13,87,13,88,13,89,13,90,13,91,13,92,13,93,13,94,13,95,13,96,14,86,14,87,14,88,14,89,14,90,14,91,14,92,14,93,14,94,14,95,14,96,15,87,15,88,15,89,15,90,15,91,15,92,15,93,15,94,15,95,16,87,16,88,16,89,16,90,16,91,16,92,16,93,16,94,16,95,17,88,17,89,17,90,17,91,17,92,17,93,17,94,18,90,18,91,18,92,2,93,3,86,3,88,3,90,3,92,3,93,4,86,4,87,4,88,4,89,4,90,4,91,4,92,4,93,4,94,4,95,5,86,5,87,5,88,5,89,5,90,5,91,5,92,5,93,5,94,5,95,5,96,6,85,6,86,6,87,6,88,6,89,6,90,6,91,6,92,6,93,6,94,6,95,6,96,6,97,7,87,7,88,7,89,7,90,7,91,7,92,7,93,7,94,7,95,7,96,7,97,8,87,8,88,8,89,8,90,8,91,8,92,8,93,8,94,8,95,8,96,8,97,9,87,9,88,9,89,9,90,9,91,9,92,9,93,9,94,9,95,9,96,9,97 } );
            addToCreepLocsMap(BWAPI::TilePosition(98, 119), std::vector<int> { 100,116,100,117,100,118,100,119,100,120,100,121,100,122,100,125,101,116,101,117,101,118,101,119,101,120,101,121,101,122,101,125,102,114,102,115,102,116,102,117,102,118,102,119,102,120,102,121,102,122,102,123,102,124,103,115,103,116,103,117,103,118,103,119,103,120,103,121,103,122,103,123,103,124,104,115,104,116,104,117,104,118,104,119,104,120,104,121,104,122,104,123,104,124,105,115,105,116,105,118,105,120,105,122,105,123,106,116,106,122,107,116,107,117,107,119,107,121,107,122,107,124,108,117,108,118,108,119,108,120,108,121,108,122,108,123,109,119,109,120,109,121,90,119,90,120,90,121,91,117,91,118,91,119,91,120,91,121,91,122,91,123,92,116,92,117,92,118,92,119,92,120,92,121,92,122,92,123,92,124,93,116,93,117,93,118,93,119,93,120,93,121,93,122,93,123,93,124,94,115,94,116,94,117,94,118,94,119,94,120,94,121,94,122,94,123,94,124,94,125,95,115,95,116,95,117,95,118,95,119,95,120,95,121,95,122,95,123,95,124,95,125,96,115,96,116,96,117,96,118,96,119,96,120,96,121,96,122,96,123,96,124,96,125,97,114,97,115,97,116,97,117,97,118,97,119,97,120,97,121,97,122,97,123,97,124,97,125,98,116,98,117,98,118,98,119,98,120,98,121,98,122,98,123,98,124,98,125,99,116,99,117,99,118,99,119,99,120,99,121,99,122,99,123,99,124,99,125 } );
        }
        // Tau Cross map version 1.1 (map name converted to plain text contains "Tau Cross 1.1")
        else if (mapHash == "9bfc271360fa5bab3707a29e1326b84d0ff58911")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 9), std::vector<int> { 109,10,109,11,109,9,110,10,110,11,110,12,110,13,110,7,110,8,110,9,111,10,111,11,111,12,111,13,111,14,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,13,112,14,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,14,113,15,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,14,114,15,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,14,115,15,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,15,116,16,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,15,117,16,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,15,118,16,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,15,119,16,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,15,120,16,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,15,121,16,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,14,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,13,123,14,123,5,123,6,123,7,123,8,123,9,124,10,124,11,124,13,124,15,124,5,124,8,124,9,125,10,126,10,126,12,126,14,126,6,126,7,127,10,127,11,127,12,127,13,127,7,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 44), std::vector<int> { 0,42,0,43,0,44,0,45,0,46,0,47,0,48,1,41,1,43,1,46,1,48,1,49,10,41,10,42,10,43,10,44,10,45,10,46,10,47,10,48,10,49,10,50,10,51,11,39,11,40,11,41,11,42,11,43,11,44,11,45,11,46,11,47,11,48,11,49,11,50,11,51,12,40,12,41,12,42,12,43,12,44,12,45,12,46,12,47,12,48,12,49,12,50,13,40,13,41,13,42,13,43,13,44,13,45,13,46,13,47,13,48,13,49,13,50,14,40,14,41,14,42,14,43,14,44,14,45,14,46,14,47,14,48,14,49,14,50,15,41,15,42,15,43,15,44,15,45,15,46,15,47,15,48,15,49,16,41,16,42,16,43,16,44,16,45,16,46,16,47,16,48,16,49,17,42,17,43,17,44,17,45,17,46,17,47,17,48,18,44,18,45,18,46,2,43,2,49,3,40,3,42,3,43,3,44,3,45,3,47,3,49,4,40,4,41,4,42,4,43,4,44,4,45,4,46,4,47,4,48,4,49,5,40,5,41,5,42,5,43,5,44,5,45,5,46,5,47,5,48,5,49,6,39,6,40,6,41,6,42,6,43,6,44,6,45,6,46,6,47,6,48,6,49,6,51,7,41,7,42,7,43,7,44,7,45,7,46,7,47,7,48,7,49,7,50,7,51,8,41,8,42,8,43,8,44,8,45,8,46,8,47,8,48,8,49,8,50,8,51,9,41,9,42,9,43,9,44,9,45,9,46,9,47,9,48,9,49,9,50,9,51 } );
            addToCreepLocsMap(BWAPI::TilePosition(93, 118), std::vector<int> { 100,114,100,116,100,118,100,120,100,121,101,116,101,121,102,115,102,116,102,117,102,119,102,121,102,122,102,123,103,116,103,117,103,118,103,119,103,120,103,121,103,122,104,118,104,119,104,120,85,118,85,119,85,120,86,116,86,117,86,120,86,121,86,122,87,115,87,116,87,117,87,120,87,121,87,122,87,123,88,115,88,116,88,117,88,120,88,121,88,122,88,123,89,114,89,115,89,116,89,117,89,120,89,121,89,122,89,123,89,124,90,114,90,115,90,116,90,117,90,118,90,119,90,120,90,121,90,122,90,123,90,124,91,114,91,115,91,116,91,117,91,118,91,119,91,120,91,121,91,122,91,123,91,124,92,113,92,114,92,115,92,116,92,117,92,118,92,119,92,120,92,121,92,122,92,123,92,124,92,125,93,113,93,114,93,115,93,116,93,117,93,118,93,119,93,120,93,121,93,122,93,123,93,124,93,125,94,113,94,114,94,115,94,116,94,117,94,118,94,119,94,120,94,121,94,122,94,123,94,124,94,125,95,113,95,114,95,115,95,116,95,117,95,118,95,119,95,120,95,121,95,122,95,123,95,124,95,125,96,113,96,114,96,115,96,116,96,117,96,118,96,119,96,120,96,121,96,122,96,123,96,124,96,125,97,113,97,114,97,115,97,116,97,117,97,118,97,119,97,120,97,121,97,122,97,123,97,125,98,114,98,115,98,116,98,117,98,118,98,119,98,120,98,121,98,122,98,123,99,114,99,115,99,116,99,117,99,118,99,119,99,120,99,121,99,122,99,123 } );
        }
        // Neo Sniper Ridge map version 2.0 (map name is not in English but says "2.0" at the end)
        else if (mapHash == "9e9e6a3372251ac7b0acabcf5d041fbf0b755fdb")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 117), std::vector<int> { 109,117,109,118,109,119,110,115,110,116,110,117,110,118,110,119,110,120,110,121,111,114,111,115,111,116,111,117,111,118,111,119,111,120,111,121,111,122,112,114,112,115,112,116,112,117,112,118,112,119,112,120,112,121,112,122,113,113,113,114,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,114,113,114,114,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,115,113,115,114,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,116,112,116,113,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,117,114,117,115,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,118,114,118,115,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,119,114,119,115,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,120,114,120,115,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,121,112,121,113,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,121,124,122,113,122,114,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,122,123,123,113,123,114,123,115,123,116,123,117,123,118,123,119,123,120,123,121,123,122,124,113,124,114,124,116,124,118,124,119,124,121,125,114,126,114,126,115,126,117,126,120,126,122,127,115,127,116,127,117,127,118,127,119,127,120,127,121 } );
            addToCreepLocsMap(BWAPI::TilePosition(117, 7), std::vector<int> { 109,7,109,8,109,9,110,10,110,11,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,12,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,2,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,4,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,4,119,5,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,2,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,3,123,4,123,5,123,6,123,7,123,8,123,9,124,11,124,3,124,4,124,6,124,8,124,9,125,4,126,10,126,12,126,4,126,5,126,7,127,10,127,11,127,5,127,6,127,7,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 117), std::vector<int> { 0,115,0,116,0,117,0,118,0,119,0,120,0,121,1,114,1,115,1,117,1,120,1,122,10,114,10,115,10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,10,124,11,112,11,113,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,11,124,12,113,12,114,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,12,123,13,113,13,114,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,13,123,14,113,14,114,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,14,123,15,114,15,115,15,116,15,117,15,118,15,119,15,120,15,121,15,122,16,114,16,115,16,116,16,117,16,118,16,119,16,120,16,121,16,122,17,115,17,116,17,117,17,118,17,119,17,120,17,121,18,117,18,118,18,119,2,114,3,113,3,114,3,116,3,118,3,119,3,121,4,113,4,114,4,115,4,116,4,117,4,118,4,119,4,120,4,121,4,122,5,113,5,114,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,5,123,6,112,6,113,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,6,124,7,114,7,115,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,7,124,8,114,8,115,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,8,124,9,114,9,115,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123,9,124 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 7), std::vector<int> { 0,10,0,11,0,5,0,6,0,7,0,8,0,9,1,10,1,12,1,4,1,5,1,7,10,10,10,11,10,12,10,13,10,14,10,4,10,5,10,6,10,7,10,8,10,9,11,10,11,11,11,12,11,13,11,14,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,13,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,13,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,13,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,12,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,12,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,11,17,5,17,6,17,7,17,8,17,9,18,7,18,8,18,9,2,4,3,11,3,3,3,4,3,6,3,8,3,9,4,10,4,11,4,12,4,3,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,13,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,10,6,11,6,12,6,13,6,14,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,14,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,14,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,14,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // Empire Of The Sun map version 1.0 (map name is not in English but says "1.0" at the end)
        else if (mapHash == "a220d93efdf05a439b83546a579953c63c863ca7")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 119), std::vector<int> { 109,119,109,120,109,121,110,117,110,118,110,119,110,120,110,121,110,122,110,123,111,116,111,117,111,118,111,119,111,120,111,121,111,122,111,123,111,124,112,116,112,117,112,118,112,119,112,120,112,121,112,122,112,123,112,124,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,113,124,113,125,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,114,124,114,125,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,115,124,115,125,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,116,125,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,117,125,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,118,125,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,119,125,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,120,125,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,121,124,121,125,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,122,123,122,124,122,125,123,115,123,116,123,117,123,118,123,119,123,120,123,121,123,122,123,123,123,124,124,115,124,118,124,121,124,122 } );
            addToCreepLocsMap(BWAPI::TilePosition(117, 6), std::vector<int> { 109,6,109,7,109,8,110,10,110,4,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,3,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,3,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,2,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,2,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,2,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,1,116,10,116,11,116,12,116,13,116,2,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,3,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,3,118,4,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,3,119,4,119,5,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,3,120,4,120,5,120,6,120,7,120,8,120,9,121,1,121,10,121,11,121,12,121,13,121,2,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,2,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,2,123,3,123,4,123,5,123,6,123,7,123,8,123,9,124,12,124,2,124,5,124,6,124,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 119), std::vector<int> { 10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,10,124,10,125,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,11,124,11,125,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,12,123,12,124,12,125,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,13,123,13,124,13,125,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,14,123,14,124,14,125,15,116,15,117,15,118,15,119,15,120,15,121,15,122,15,123,15,124,16,116,16,117,16,118,16,119,16,120,16,121,16,122,16,123,16,124,17,117,17,118,17,119,17,120,17,121,17,122,17,123,18,119,18,120,18,121,3,115,3,118,3,120,3,121,4,115,4,116,4,117,4,118,4,119,4,120,4,121,4,122,4,123,4,124,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,5,123,5,124,5,125,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,6,124,6,125,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,7,124,7,125,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,8,124,8,125,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123,9,124,9,125 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 6), std::vector<int> { 10,10,10,11,10,12,10,13,10,3,10,4,10,5,10,6,10,7,10,8,10,9,11,1,11,10,11,11,11,12,11,13,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,2,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,2,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,2,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,3,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,3,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,4,17,5,17,6,17,7,17,8,17,9,18,6,18,7,18,8,3,12,3,2,3,5,3,7,3,8,3,9,4,10,4,11,4,12,4,2,4,3,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,2,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,1,6,10,6,11,6,12,6,13,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,3,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,3,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,3,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // Blue Storm map version 1.2 (map name converted to plain text contains "Blue Storm 1.2")
        else if (mapHash == "aab66dbf9c85f85c47c219277e1e36181fe5f9fc")
        {
            addToCreepLocsMap(BWAPI::TilePosition(116, 8), std::vector<int> { 108,10,108,8,108,9,109,10,109,11,109,12,109,6,109,7,109,8,109,9,110,10,110,11,110,12,110,13,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,12,111,13,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,13,112,14,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,14,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,14,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,14,115,15,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,15,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,15,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,15,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,15,119,5,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,15,120,3,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,6,123,7,123,8,124,11,124,7,125,11,125,12,125,13,125,5,125,7,125,9,126,10,126,11,126,12,126,6,126,7,126,8,126,9,127,10,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 85), std::vector<int> { 0,85,0,86,0,87,1,83,1,84,1,85,1,86,1,87,1,88,1,89,10,82,10,83,10,84,10,85,10,86,10,87,10,88,10,89,10,90,10,91,10,92,11,82,11,83,11,84,11,85,11,86,11,87,11,88,11,89,11,90,11,91,11,92,12,80,12,81,12,82,12,83,12,84,12,85,12,86,12,87,12,88,12,89,12,90,12,91,12,92,13,81,13,82,13,83,13,84,13,85,13,86,13,87,13,88,13,89,13,90,13,91,14,81,14,82,14,83,14,84,14,85,14,86,14,87,14,88,14,89,14,90,14,91,15,81,15,82,15,83,15,84,15,85,15,86,15,87,15,88,15,89,15,90,15,91,16,82,16,83,16,84,16,85,16,86,16,87,16,88,16,89,16,90,17,82,17,83,17,84,17,85,17,86,17,87,17,88,17,89,17,90,18,83,18,84,18,85,18,86,18,87,18,88,18,89,19,85,19,86,19,87,2,82,2,83,2,85,2,87,2,88,2,89,2,90,3,83,3,88,4,83,4,84,4,86,4,88,5,82,5,83,5,84,5,85,5,86,5,87,5,88,5,89,5,90,6,81,6,82,6,83,6,84,6,85,6,86,6,87,6,88,6,89,6,90,6,91,7,80,7,81,7,82,7,83,7,84,7,85,7,86,7,87,7,88,7,89,7,90,7,91,7,92,8,82,8,83,8,84,8,85,8,86,8,87,8,88,8,89,8,90,8,91,8,92,9,82,9,83,9,84,9,85,9,86,9,87,9,88,9,89,9,90,9,91,9,92 } );
        }
        // Benzene map version 1.1 (map name converted to plain text contains "Benzene1.1")
        else if (mapHash == "af618ea3ed8a8926ca7b17619eebcb9126f0d8b1")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 13), std::vector<int> { 109,13,109,14,109,15,110,11,110,12,110,13,110,14,110,15,110,16,110,17,111,10,111,11,111,12,111,13,111,14,111,15,111,16,111,17,111,18,112,10,112,11,112,12,112,13,112,14,112,15,112,16,112,17,112,18,113,10,113,11,113,12,113,13,113,14,113,15,113,16,113,17,113,18,113,19,113,9,114,10,114,11,114,12,114,13,114,14,114,15,114,16,114,17,114,18,114,19,114,9,115,10,115,11,115,12,115,13,115,14,115,15,115,16,115,17,115,18,115,19,115,9,116,10,116,11,116,12,116,13,116,14,116,15,116,16,116,17,116,18,116,19,116,20,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,15,117,16,117,17,117,18,117,19,117,20,118,10,118,11,118,12,118,13,118,14,118,15,118,16,118,17,118,18,118,19,118,20,119,10,119,11,119,12,119,13,119,14,119,15,119,16,119,17,119,18,119,19,119,20,120,10,120,11,120,12,120,13,120,14,120,15,120,16,120,17,120,18,120,19,120,20,121,10,121,11,121,12,121,13,121,14,121,15,121,16,121,17,121,18,121,19,121,20,121,8,121,9,122,10,122,11,122,12,122,13,122,14,122,15,122,16,122,17,122,18,122,19,122,9,123,10,123,11,123,12,123,13,123,14,123,15,123,16,123,17,123,18,123,19,124,11,124,13,124,16,124,19,125,11,126,10,126,11,126,12,126,14,126,15,126,17,126,18,127,11,127,12,127,13,127,14,127,15,127,16,127,17 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 96), std::vector<int> { 0,100,0,94,0,95,0,96,0,97,0,98,0,99,1,100,1,101,1,93,1,94,1,95,1,97,1,98,10,100,10,101,10,102,10,103,10,93,10,94,10,95,10,96,10,97,10,98,10,99,11,100,11,101,11,102,11,103,11,91,11,92,11,93,11,94,11,95,11,96,11,97,11,98,11,99,12,100,12,101,12,102,12,92,12,93,12,94,12,95,12,96,12,97,12,98,12,99,13,100,13,101,13,102,13,92,13,93,13,94,13,95,13,96,13,97,13,98,13,99,14,100,14,101,14,102,14,92,14,93,14,94,14,95,14,96,14,97,14,98,14,99,15,100,15,101,15,93,15,94,15,95,15,96,15,97,15,98,15,99,16,100,16,101,16,93,16,94,16,95,16,96,16,97,16,98,16,99,17,100,17,94,17,95,17,96,17,97,17,98,17,99,18,96,18,97,18,98,2,94,3,102,3,94,3,96,3,99,4,100,4,101,4,102,4,93,4,94,4,95,4,96,4,97,4,98,4,99,5,100,5,101,5,102,5,92,5,93,5,94,5,95,5,96,5,97,5,98,5,99,6,100,6,101,6,102,6,103,6,91,6,92,6,93,6,94,6,95,6,96,6,97,6,98,6,99,7,100,7,101,7,102,7,103,7,93,7,94,7,95,7,96,7,97,7,98,7,99,8,100,8,101,8,102,8,103,8,93,8,94,8,95,8,96,8,97,8,98,8,99,9,100,9,101,9,102,9,103,9,93,9,94,9,95,9,96,9,97,9,98,9,99 } );
        }
        // Pathfinder map version 1.0 (map name converted to plain text contains "Pathfinder 1.0")
        else if (mapHash == "b10e73a252d5c693f19829871a01043f0277fd58")
        {
            addToCreepLocsMap(BWAPI::TilePosition(31, 83), std::vector<int> { 23,83,23,84,23,85,24,81,24,82,24,83,24,84,24,85,24,86,24,87,25,80,25,82,25,84,25,86,25,87,25,88,26,86,27,79,27,81,27,83,27,85,27,86,28,79,28,80,28,81,28,82,28,83,28,84,28,85,28,86,28,87,28,88,29,79,29,80,29,81,29,82,29,83,29,84,29,85,29,86,29,87,29,88,30,78,30,79,30,80,30,81,30,82,30,83,30,84,30,85,30,86,30,87,30,88,30,90,31,80,31,81,31,82,31,83,31,84,31,85,31,86,31,87,31,88,31,89,31,90,32,80,32,81,32,82,32,83,32,84,32,85,32,86,32,87,32,88,32,89,32,90,33,80,33,81,33,82,33,83,33,84,33,85,33,86,33,87,33,88,33,89,33,90,34,80,34,81,34,82,34,83,34,84,34,85,34,86,34,87,34,88,34,89,34,90,35,78,35,79,35,80,35,81,35,82,35,83,35,84,35,85,35,86,35,87,35,88,35,89,35,90,36,79,36,80,36,81,36,82,36,83,36,84,36,85,36,86,36,87,36,88,36,89,37,79,37,80,37,81,37,82,37,83,37,84,37,85,37,86,37,87,37,88,37,89,38,79,38,80,38,81,38,82,38,83,38,84,38,85,38,86,38,87,38,88,38,89,39,80,39,81,39,82,39,83,39,84,39,85,39,86,39,87,39,88,40,80,40,81,40,82,40,83,40,84,40,85,40,86,40,87,40,88,41,81,41,82,41,83,41,84,41,85,41,86,41,87,42,83,42,84,42,85 } );
            addToCreepLocsMap(BWAPI::TilePosition(70, 25), std::vector<int> { 62,25,62,26,62,27,63,23,63,24,63,25,63,28,63,29,64,22,64,23,64,24,64,25,64,28,64,29,64,30,65,22,65,23,65,24,65,25,65,28,65,29,65,30,66,21,66,22,66,23,66,24,66,25,66,28,66,29,66,30,66,31,67,21,67,22,67,23,67,24,67,25,67,26,67,27,67,28,67,29,67,30,67,31,68,21,68,22,68,23,68,24,68,25,68,26,68,27,68,28,68,29,68,30,68,31,69,20,69,21,69,22,69,23,69,24,69,25,69,26,69,27,69,28,69,29,69,30,69,31,69,32,70,20,70,21,70,22,70,23,70,24,70,25,70,26,70,27,70,28,70,29,70,30,70,31,70,32,71,20,71,21,71,22,71,23,71,24,71,25,71,26,71,27,71,28,71,29,71,30,71,31,71,32,72,20,72,21,72,22,72,23,72,24,72,25,72,26,72,27,72,28,72,29,72,30,72,31,72,32,73,20,73,21,73,22,73,23,73,24,73,25,73,26,73,27,73,28,73,29,73,30,73,31,73,32,74,20,74,21,74,22,74,23,74,24,74,25,74,26,74,27,74,28,74,29,74,30,74,31,74,32,75,21,75,22,75,23,75,24,75,25,75,26,75,27,75,28,75,29,75,30,75,31,76,21,76,22,76,23,76,24,76,25,76,26,76,27,76,28,76,29,76,30,77,21,77,24,77,26,77,28,77,29 } );
            addToCreepLocsMap(BWAPI::TilePosition(93, 85), std::vector<int> { 100,81,100,82,100,84,100,86,100,88,101,82,102,82,102,83,102,85,102,87,102,89,102,90,103,83,103,84,103,85,103,86,103,87,103,88,103,89,104,85,104,86,104,87,85,85,85,86,85,87,86,83,86,84,86,85,86,86,86,87,86,88,86,89,87,82,87,83,87,84,87,85,87,86,87,87,87,88,87,89,87,90,88,82,88,83,88,84,88,85,88,86,88,87,88,88,88,89,88,90,89,81,89,82,89,83,89,84,89,85,89,86,89,87,89,88,89,89,89,90,89,91,90,81,90,82,90,83,90,84,90,85,90,86,90,87,90,88,90,89,90,90,90,91,91,81,91,82,91,83,91,84,91,85,91,86,91,87,91,88,91,89,91,90,91,91,92,80,92,81,92,82,92,83,92,84,92,85,92,86,92,87,92,88,92,89,92,90,92,91,92,92,93,82,93,83,93,84,93,85,93,86,93,87,93,88,93,89,93,90,93,91,93,92,94,82,94,83,94,84,94,85,94,86,94,87,94,88,94,89,94,90,94,91,94,92,95,82,95,83,95,84,95,85,95,86,95,87,95,88,95,89,95,90,95,91,95,92,96,82,96,83,96,84,96,85,96,86,96,87,96,88,96,89,96,90,96,91,96,92,97,80,97,81,97,82,97,83,97,84,97,85,97,86,97,87,97,88,97,89,97,90,97,92,98,81,98,82,98,83,98,84,98,85,98,86,98,87,98,88,98,89,98,90,99,81,99,82,99,83,99,84,99,85,99,86,99,87,99,88,99,89,99,90 } );
        }
        // Aztec map version 1.1 (map name converted to plain text contains "Aztec 1.1")
        else if (mapHash == "ba2fc0ed637e4ec91cc70424335b3c13e131b75a")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 100), std::vector<int> { 109,100,109,101,109,102,110,100,110,101,110,102,110,103,110,104,110,98,110,99,111,100,111,101,111,102,111,103,111,104,111,105,111,97,111,98,111,99,112,100,112,101,112,102,112,103,112,104,112,105,112,97,112,98,112,99,113,100,113,101,113,102,113,103,113,104,113,105,113,106,113,96,113,97,113,98,113,99,114,100,114,101,114,102,114,103,114,104,114,105,114,106,114,96,114,97,114,98,114,99,115,100,115,101,115,102,115,103,115,104,115,105,115,106,115,96,115,97,115,98,115,99,116,100,116,101,116,102,116,103,116,104,116,105,116,106,116,107,116,95,116,96,116,97,116,98,116,99,117,100,117,101,117,102,117,103,117,104,117,105,117,106,117,107,117,97,117,98,117,99,118,100,118,101,118,102,118,103,118,104,118,105,118,106,118,107,118,97,118,98,118,99,119,100,119,101,119,102,119,103,119,104,119,105,119,106,119,107,119,97,119,98,119,99,120,100,120,101,120,102,120,103,120,104,120,105,120,106,120,107,120,97,120,98,120,99,121,100,121,101,121,102,121,103,121,104,121,105,121,106,121,107,121,95,121,96,121,97,121,98,121,99,122,100,122,101,122,102,122,103,122,104,122,105,122,106,122,96,122,97,122,98,122,99,123,100,123,101,123,102,123,103,123,104,123,105,123,106,123,97,123,98,123,99,124,100,124,103,124,106,124,99,125,99,126,101,126,102,126,104,126,105,126,97,126,98,126,99,127,100,127,101,127,102,127,103,127,104,127,98,127,99 } );
            addToCreepLocsMap(BWAPI::TilePosition(68, 6), std::vector<int> { 60,6,60,7,60,8,61,10,61,4,61,5,61,6,61,7,61,8,61,9,62,10,62,11,62,3,62,5,62,7,62,9,63,9,64,2,64,4,64,6,64,8,64,9,65,10,65,11,65,2,65,3,65,4,65,5,65,6,65,7,65,8,65,9,66,10,66,11,66,12,66,2,66,3,66,4,66,5,66,6,66,7,66,8,66,9,67,1,67,10,67,11,67,12,67,13,67,2,67,3,67,4,67,5,67,6,67,7,67,8,67,9,68,10,68,11,68,12,68,13,68,3,68,4,68,5,68,6,68,7,68,8,68,9,69,10,69,11,69,12,69,13,69,3,69,4,69,5,69,6,69,7,69,8,69,9,70,10,70,11,70,12,70,13,70,3,70,4,70,5,70,6,70,7,70,8,70,9,71,10,71,11,71,12,71,13,71,3,71,4,71,5,71,6,71,7,71,8,71,9,72,1,72,10,72,11,72,12,72,13,72,2,72,3,72,4,72,5,72,6,72,7,72,8,72,9,73,10,73,11,73,12,73,2,73,3,73,4,73,5,73,6,73,7,73,8,73,9,74,10,74,11,74,12,74,2,74,3,74,4,74,5,74,6,74,7,74,8,74,9,75,10,75,11,75,12,75,2,75,3,75,4,75,5,75,6,75,7,75,8,75,9,76,10,76,11,76,3,76,4,76,5,76,6,76,7,76,8,76,9,77,10,77,11,77,3,77,4,77,5,77,6,77,7,77,8,77,9,78,10,78,4,78,5,78,6,78,7,78,8,78,9,79,6,79,7,79,8 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 83), std::vector<int> { 0,81,0,82,0,83,0,84,0,85,0,86,0,87,1,80,1,81,1,82,1,84,1,85,1,87,1,88,10,80,10,81,10,82,10,83,10,84,10,85,10,86,10,87,10,88,10,89,10,90,11,78,11,79,11,80,11,81,11,82,11,83,11,84,11,85,11,86,11,87,11,88,11,89,11,90,12,79,12,80,12,81,12,82,12,83,12,84,12,85,12,86,12,87,12,88,12,89,13,79,13,80,13,81,13,82,13,83,13,84,13,85,13,86,13,87,13,88,13,89,14,79,14,80,14,81,14,82,14,83,14,84,14,85,14,86,14,87,14,88,14,89,15,80,15,81,15,82,15,83,15,84,15,85,15,86,15,87,15,88,16,80,16,81,16,82,16,83,16,84,16,85,16,86,16,87,16,88,17,81,17,82,17,83,17,84,17,85,17,86,17,87,18,83,18,84,18,85,2,81,3,81,3,83,3,86,3,89,4,80,4,81,4,82,4,83,4,84,4,85,4,86,4,87,4,88,4,89,5,79,5,80,5,81,5,82,5,83,5,84,5,85,5,86,5,87,5,88,5,89,6,78,6,79,6,80,6,81,6,82,6,83,6,84,6,85,6,86,6,87,6,88,6,89,6,90,7,80,7,81,7,82,7,83,7,84,7,85,7,86,7,87,7,88,7,89,7,90,8,80,8,81,8,82,8,83,8,84,8,85,8,86,8,87,8,88,8,89,8,90,9,80,9,81,9,82,9,83,9,84,9,85,9,86,9,87,9,88,9,89,9,90 } );
        }
        // Neo Moon Glaive map version 2.1 (map name converted to plain text contains "| iCCup | MoonGlaive 2.1")
        else if (mapHash == "c8386b87051f6773f6b2681b0e8318244aa086a6")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 96), std::vector<int> { 109,96,109,97,109,98,110,100,110,94,110,95,110,96,110,97,110,98,110,99,111,100,111,93,111,94,111,95,111,96,111,97,111,98,111,99,112,100,112,101,112,93,112,94,112,95,112,96,112,97,112,98,112,99,113,100,113,101,113,102,113,92,113,93,113,94,113,95,113,96,113,97,113,98,113,99,114,100,114,101,114,102,114,92,114,93,114,94,114,95,114,96,114,97,114,98,114,99,115,100,115,101,115,102,115,92,115,93,115,94,115,95,115,96,115,97,115,98,115,99,116,100,116,101,116,102,116,103,116,91,116,92,116,93,116,94,116,95,116,96,116,97,116,98,116,99,117,100,117,101,117,102,117,103,117,93,117,94,117,95,117,96,117,97,117,98,117,99,118,100,118,101,118,102,118,103,118,93,118,94,118,95,118,96,118,97,118,98,118,99,119,100,119,101,119,102,119,103,119,93,119,94,119,95,119,96,119,97,119,98,119,99,120,100,120,101,120,102,120,103,120,93,120,94,120,95,120,96,120,97,120,98,120,99,121,100,121,101,121,102,121,103,121,91,121,92,121,93,121,94,121,95,121,96,121,97,121,98,121,99,122,100,122,101,122,102,122,92,122,93,122,94,122,95,122,96,122,97,122,98,122,99,123,100,123,101,123,92,123,93,123,94,123,95,123,96,123,97,123,98,123,99,124,92,124,93,124,95,124,97,124,99,125,93,126,100,126,101,126,93,126,94,126,96,126,98,127,100,127,94,127,95,127,96,127,97,127,98,127,99 } );
            addToCreepLocsMap(BWAPI::TilePosition(67, 6), std::vector<int> { 59,6,59,7,59,8,60,10,60,4,60,5,60,8,60,9,61,10,61,11,61,3,61,4,61,5,61,8,61,9,62,10,62,11,62,3,62,4,62,5,62,8,62,9,63,10,63,11,63,12,63,2,63,3,63,4,63,5,63,8,63,9,64,10,64,11,64,12,64,2,64,3,64,4,64,5,64,6,64,7,64,8,64,9,65,10,65,11,65,12,65,3,65,4,65,5,65,6,65,7,65,8,65,9,66,1,66,10,66,11,66,12,66,13,66,3,66,4,66,5,66,6,66,7,66,8,66,9,67,10,67,11,67,12,67,13,67,2,67,3,67,4,67,5,67,6,67,7,67,8,67,9,68,10,68,11,68,12,68,13,68,3,68,4,68,5,68,6,68,7,68,8,68,9,69,10,69,11,69,12,69,13,69,3,69,4,69,5,69,6,69,7,69,8,69,9,70,10,70,11,70,12,70,13,70,2,70,3,70,4,70,5,70,6,70,7,70,8,70,9,71,1,71,10,71,11,71,12,71,13,71,3,71,4,71,5,71,6,71,7,71,8,71,9,72,10,72,11,72,12,72,3,72,4,72,5,72,6,72,7,72,8,72,9,73,10,73,11,73,12,73,3,73,4,73,5,73,6,73,7,73,8,73,9,74,10,74,11,74,12,74,6,74,7,74,8,74,9,75,10,75,11,75,6,75,7,75,8,75,9,76,10,76,11,76,3,76,4,76,5,76,6,76,7,76,8,76,9,77,10,77,4,77,5,77,6,77,7,77,8,77,9,78,6,78,7,78,8 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 90), std::vector<int> { 0,88,0,89,0,90,0,91,0,92,0,93,0,94,1,87,1,88,1,90,1,92,1,94,1,95,10,87,10,88,10,89,10,90,10,91,10,92,10,93,10,94,10,95,10,96,10,97,11,85,11,86,11,87,11,88,11,89,11,90,11,91,11,92,11,93,11,94,11,95,11,96,11,97,12,86,12,87,12,88,12,89,12,90,12,91,12,92,12,93,12,94,12,95,12,96,13,86,13,87,13,88,13,89,13,90,13,91,13,92,13,93,13,94,13,95,13,96,14,86,14,87,14,88,14,89,14,90,14,91,14,92,14,93,14,94,14,95,14,96,15,87,15,88,15,89,15,90,15,91,15,92,15,93,15,94,15,95,16,87,16,88,16,89,16,90,16,91,16,92,16,93,16,94,16,95,17,88,17,89,17,90,17,91,17,92,17,93,17,94,18,90,18,91,18,92,2,95,3,89,3,91,3,93,3,95,3,96,4,87,4,88,4,89,4,90,4,91,4,92,4,93,4,94,4,95,4,96,5,86,5,87,5,88,5,89,5,90,5,91,5,92,5,93,5,94,5,95,5,96,6,85,6,86,6,87,6,88,6,89,6,90,6,91,6,92,6,93,6,94,6,95,6,96,6,97,7,87,7,88,7,89,7,90,7,91,7,92,7,93,7,94,7,95,7,96,7,97,8,87,8,88,8,89,8,90,8,91,8,92,8,93,8,94,8,95,8,96,8,97,9,87,9,88,9,89,9,90,9,91,9,92,9,93,9,94,9,95,9,96,9,97 } );
        }
        // Ride of Valkyries map version 1.0 (map name converted to plain text contains "Ride of Valkyries 1.0")
        else if (mapHash == "cd5d907c30d58333ce47c88719b6ddb2cba6612f")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 83), std::vector<int> { 109,83,109,84,109,85,110,81,110,82,110,83,110,84,110,85,110,86,110,87,111,80,111,81,111,82,111,83,111,84,111,85,111,86,111,87,111,88,112,80,112,81,112,82,112,83,112,84,112,85,112,86,112,87,112,88,113,79,113,80,113,81,113,82,113,83,113,84,113,85,113,86,113,87,113,88,113,89,114,79,114,80,114,81,114,82,114,83,114,84,114,85,114,86,114,87,114,88,114,89,115,79,115,80,115,81,115,82,115,83,115,84,115,85,115,86,115,87,115,88,115,89,116,78,116,79,116,80,116,81,116,82,116,83,116,84,116,85,116,86,116,87,116,88,116,89,116,90,117,78,117,79,117,80,117,81,117,82,117,83,117,84,117,85,117,86,117,87,117,88,118,78,118,79,118,80,118,81,118,82,118,83,118,84,118,85,118,86,118,87,118,88,119,78,119,79,119,80,119,81,119,82,119,83,119,84,119,85,119,86,119,87,119,88,120,78,120,79,120,80,120,81,120,82,120,83,120,84,120,85,120,86,120,87,120,88,121,78,121,79,121,80,121,81,121,82,121,83,121,84,121,85,121,86,121,87,121,88,121,89,121,90,122,79,122,80,122,81,122,82,122,83,122,84,122,85,122,86,122,87,122,88,122,89,123,79,123,80,123,81,123,82,123,83,123,84,123,85,123,86,123,87,123,88,123,89,124,79,124,82,124,83,124,84,124,85,124,87,124,89,125,84,126,80,126,81,126,84,126,86,126,88,127,81,127,82,127,83,127,84,127,85,127,86,127,87 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 83), std::vector<int> { 0,81,0,82,0,83,0,84,0,85,0,86,0,87,1,80,1,81,1,83,1,85,1,88,10,78,10,79,10,80,10,81,10,82,10,83,10,84,10,85,10,86,10,87,10,88,11,78,11,79,11,80,11,81,11,82,11,83,11,84,11,85,11,86,11,87,11,88,11,89,11,90,12,79,12,80,12,81,12,82,12,83,12,84,12,85,12,86,12,87,12,88,12,89,13,79,13,80,13,81,13,82,13,83,13,84,13,85,13,86,13,87,13,88,13,89,14,79,14,80,14,81,14,82,14,83,14,84,14,85,14,86,14,87,14,88,14,89,15,80,15,81,15,82,15,83,15,84,15,85,15,86,15,87,15,88,16,80,16,81,16,82,16,83,16,84,16,85,16,86,16,87,16,88,17,81,17,82,17,83,17,84,17,85,17,86,17,87,18,83,18,84,18,85,2,83,3,79,3,82,3,83,3,84,3,86,3,87,3,89,4,79,4,80,4,81,4,82,4,83,4,84,4,85,4,86,4,87,4,88,4,89,5,79,5,80,5,81,5,82,5,83,5,84,5,85,5,86,5,87,5,88,5,89,6,78,6,79,6,80,6,81,6,82,6,83,6,84,6,85,6,86,6,87,6,88,6,89,6,90,7,78,7,79,7,80,7,81,7,82,7,83,7,84,7,85,7,86,7,87,7,88,8,78,8,79,8,80,8,81,8,82,8,83,8,84,8,85,8,86,8,87,8,88,9,78,9,79,9,80,9,81,9,82,9,83,9,84,9,85,9,86,9,87,9,88 } );
        }
        // Fighting Spirit iCCup map version 1.3 (map name converted to plain text contains "| iCCup | Fighting Spirit 1.3")
        // This variant is used in SSCAIT. Note that this is different to the non-iCCup variant
        else if (mapHash == "d2f5633cc4bb0fca13cd1250729d5530c82c7451")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 117), std::vector<int> { 109,117,109,118,109,119,110,115,110,116,110,117,110,118,110,119,110,120,110,121,111,114,111,115,111,116,111,117,111,118,111,119,111,120,111,121,111,122,112,114,112,115,112,116,112,117,112,118,112,119,112,120,112,121,112,122,113,113,113,114,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,114,113,114,114,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,115,113,115,114,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,116,112,116,113,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,117,114,117,115,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,118,114,118,115,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,119,114,119,115,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,120,114,120,115,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,121,112,121,113,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,121,124,122,113,122,114,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,122,123,123,113,123,114,123,115,123,116,123,117,123,118,123,119,123,120,123,121,123,122,124,113,124,114,124,116,124,118,124,120,125,114,126,114,126,115,126,117,126,119,126,121,126,122,127,115,127,116,127,117,127,118,127,119,127,120,127,121 } );
            addToCreepLocsMap(BWAPI::TilePosition(117, 7), std::vector<int> { 109,7,109,8,109,9,110,10,110,11,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,12,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,2,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,4,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,4,119,5,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,2,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,3,123,4,123,5,123,6,123,7,123,8,123,9,124,10,124,3,124,4,124,6,124,8,125,4,126,11,126,12,126,4,126,5,126,7,126,9,127,10,127,11,127,5,127,6,127,7,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 116), std::vector<int> { 0,114,0,115,0,116,0,117,0,118,0,119,0,120,1,113,1,114,1,116,1,118,1,120,1,121,10,113,10,114,10,115,10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,11,111,11,112,11,113,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,12,112,12,113,12,114,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,13,112,13,113,13,114,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,14,112,14,113,14,114,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,15,113,15,114,15,115,15,116,15,117,15,118,15,119,15,120,15,121,16,113,16,114,16,115,16,116,16,117,16,118,16,119,16,120,16,121,17,114,17,115,17,116,17,117,17,118,17,119,17,120,18,116,18,117,18,118,2,113,3,112,3,113,3,115,3,117,3,119,4,112,4,113,4,114,4,115,4,116,4,117,4,118,4,119,4,120,4,121,5,112,5,113,5,114,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,6,111,6,112,6,113,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,7,113,7,114,7,115,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,8,113,8,114,8,115,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,9,113,9,114,9,115,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 6), std::vector<int> { 0,10,0,4,0,5,0,6,0,7,0,8,0,9,1,10,1,11,1,3,1,4,1,6,1,8,10,10,10,11,10,12,10,13,10,3,10,4,10,5,10,6,10,7,10,8,10,9,11,1,11,10,11,11,11,12,11,13,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,2,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,2,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,2,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,3,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,3,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,4,17,5,17,6,17,7,17,8,17,9,18,6,18,7,18,8,2,3,3,2,3,3,3,5,3,7,3,9,4,10,4,11,4,2,4,3,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,2,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,1,6,10,6,11,6,12,6,13,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,3,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,3,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,3,9,4,9,5,9,6,9,7,9,8,9,9 } );
        }
        // Neo Heartbreaker Ridge map version 2.0 (map name is not in English but says "2.0" at the end)
        else if (mapHash == "d9757c0adcfd61386dff8fe3e493e9e8ef9b45e3")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 56), std::vector<int> { 109,56,109,57,109,58,110,54,110,55,110,56,110,57,110,58,110,59,110,60,111,53,111,54,111,55,111,56,111,57,111,58,111,59,111,60,111,61,112,53,112,54,112,55,112,56,112,57,112,58,112,59,112,60,112,61,113,52,113,53,113,54,113,55,113,56,113,57,113,58,113,59,113,60,113,61,113,62,114,52,114,53,114,54,114,55,114,56,114,57,114,58,114,59,114,60,114,61,114,62,115,52,115,53,115,54,115,55,115,56,115,57,115,58,115,59,115,60,115,61,115,62,116,51,116,52,116,53,116,54,116,55,116,56,116,57,116,58,116,59,116,60,116,61,116,62,116,63,117,53,117,54,117,55,117,56,117,57,117,58,117,59,117,60,117,61,117,62,117,63,118,53,118,54,118,55,118,56,118,57,118,58,118,59,118,60,118,61,118,62,118,63,119,53,119,54,119,55,119,56,119,57,119,58,119,59,119,60,119,61,119,62,119,63,120,53,120,54,120,55,120,56,120,57,120,58,120,59,120,60,120,61,120,62,120,63,121,51,121,52,121,53,121,54,121,55,121,56,121,57,121,58,121,59,121,60,121,61,121,62,121,63,122,52,122,53,122,54,122,55,122,56,122,57,122,58,122,59,122,60,122,61,122,62,123,53,123,54,123,55,123,56,123,57,123,58,123,59,123,60,123,61,123,62,124,54,124,55,124,57,124,58,124,60,124,62,125,54,126,53,126,54,126,56,126,59,126,61,127,54,127,55,127,56,127,57,127,58,127,59,127,60 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 37), std::vector<int> { 0,35,0,36,0,37,0,38,0,39,0,40,0,41,1,34,1,35,1,37,1,40,1,42,10,34,10,35,10,36,10,37,10,38,10,39,10,40,10,41,10,42,10,43,10,44,11,32,11,33,11,34,11,35,11,36,11,37,11,38,11,39,11,40,11,41,11,42,11,43,11,44,12,33,12,34,12,35,12,36,12,37,12,38,12,39,12,40,12,41,12,42,12,43,13,33,13,34,13,35,13,36,13,37,13,38,13,39,13,40,13,41,13,42,13,43,14,33,14,34,14,35,14,36,14,37,14,38,14,39,14,40,14,41,14,42,14,43,15,34,15,35,15,36,15,37,15,38,15,39,15,40,15,41,15,42,16,34,16,35,16,36,16,37,16,38,16,39,16,40,16,41,16,42,17,35,17,36,17,37,17,38,17,39,17,40,17,41,18,37,18,38,18,39,2,35,3,35,3,36,3,38,3,39,3,41,3,43,4,34,4,35,4,36,4,37,4,38,4,39,4,40,4,41,4,42,4,43,5,33,5,34,5,35,5,36,5,37,5,38,5,39,5,40,5,41,5,42,5,43,6,32,6,33,6,34,6,35,6,36,6,37,6,38,6,39,6,40,6,41,6,42,6,43,6,44,7,34,7,35,7,36,7,37,7,38,7,39,7,40,7,41,7,42,7,43,7,44,8,34,8,35,8,36,8,37,8,38,8,39,8,40,8,41,8,42,8,43,8,44,9,34,9,35,9,36,9,37,9,38,9,39,9,40,9,41,9,42,9,43,9,44 } );
        }
        // Python map version 1.1 (map name converted to plain text contains "Python 1.1")
        // This version is used in AIIDE and SSCAIT
        else if (mapHash == "de2ada75fbc741cfa261ee467bf6416b10f9e301")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 40), std::vector<int> { 109,40,109,41,109,42,110,38,110,39,110,40,110,41,110,42,110,43,110,44,111,37,111,38,111,39,111,40,111,41,111,42,111,43,111,44,111,45,112,37,112,38,112,39,112,40,112,41,112,42,112,43,112,44,112,45,113,36,113,37,113,38,113,39,113,40,113,41,113,42,113,43,113,44,113,45,113,46,114,36,114,37,114,38,114,39,114,40,114,41,114,42,114,43,114,44,114,45,114,46,115,36,115,37,115,38,115,39,115,40,115,41,115,42,115,43,115,44,115,45,115,46,116,35,116,36,116,37,116,38,116,39,116,40,116,41,116,42,116,43,116,44,116,45,116,46,116,47,117,37,117,38,117,39,117,40,117,41,117,42,117,43,117,44,117,45,117,46,117,47,118,37,118,38,118,39,118,40,118,41,118,42,118,43,118,44,118,45,118,46,118,47,119,37,119,38,119,39,119,40,119,41,119,42,119,43,119,44,119,45,119,46,119,47,120,37,120,38,120,39,120,40,120,41,120,42,120,43,120,44,120,45,120,46,120,47,121,35,121,36,121,37,121,38,121,39,121,40,121,41,121,42,121,43,121,44,121,45,121,46,121,47,122,36,122,37,122,38,122,39,122,40,122,41,122,42,122,43,122,44,122,45,122,46,123,37,123,38,123,39,123,40,123,41,123,42,123,43,123,44,123,45,124,38,124,40,124,42,124,43,124,44,125,40,125,43,126,37,126,39,126,40,126,41,126,43,126,45,127,38,127,39,127,40,127,41,127,42,127,43,127,44 } );
            addToCreepLocsMap(BWAPI::TilePosition(42, 119), std::vector<int> { 34,119,34,120,34,121,35,117,35,118,35,119,35,120,35,121,35,122,35,123,36,116,36,118,36,119,36,120,36,122,36,123,36,124,37,119,37,123,38,117,38,119,38,121,38,123,39,116,39,117,39,118,39,119,39,120,39,121,39,122,39,123,39,124,40,115,40,116,40,117,40,118,40,119,40,120,40,121,40,122,40,123,40,124,40,125,41,114,41,115,41,116,41,117,41,118,41,119,41,120,41,121,41,122,41,123,41,124,41,125,42,116,42,117,42,118,42,119,42,120,42,121,42,122,42,123,42,124,42,125,43,116,43,117,43,118,43,119,43,120,43,121,43,122,43,123,43,124,43,125,44,116,44,117,44,118,44,119,44,120,44,121,44,122,44,123,44,124,44,125,45,116,45,117,45,118,45,119,45,120,45,121,45,122,45,123,45,124,45,125,46,114,46,115,46,116,46,117,46,118,46,119,46,120,46,121,46,122,46,123,46,124,46,125,47,115,47,116,47,117,47,118,47,119,47,120,47,121,47,122,47,123,47,124,47,125,48,115,48,116,48,117,48,118,48,119,48,120,48,121,48,122,48,123,48,124,48,125,49,115,49,116,49,117,49,118,49,119,49,120,49,121,49,122,49,123,49,124,49,125,50,116,50,117,50,118,50,119,50,120,50,121,50,122,50,123,50,124,51,116,51,117,51,118,51,119,51,120,51,121,51,122,51,123,51,124,52,117,52,118,52,119,52,120,52,121,52,122,52,123,53,119,53,120,53,121 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 86), std::vector<int> { 0,84,0,85,0,86,0,87,0,88,0,89,0,90,1,83,1,84,1,86,1,88,1,90,1,91,10,81,10,82,10,83,10,84,10,85,10,86,10,87,10,88,10,89,10,90,10,91,10,92,10,93,11,81,11,82,11,83,11,84,11,85,11,86,11,87,11,88,11,89,11,90,11,91,11,92,11,93,12,82,12,83,12,84,12,85,12,86,12,87,12,88,12,89,12,90,12,91,12,92,13,82,13,83,13,84,13,85,13,86,13,87,13,88,13,89,13,90,13,91,13,92,14,82,14,83,14,84,14,85,14,86,14,87,14,88,14,89,14,90,14,91,14,92,15,83,15,84,15,85,15,86,15,87,15,88,15,89,15,90,15,91,16,83,16,84,16,85,16,86,16,87,16,88,16,89,16,90,16,91,17,84,17,85,17,86,17,87,17,88,17,89,17,90,18,86,18,87,18,88,2,83,2,88,3,82,3,83,3,85,3,87,3,88,3,89,4,82,4,83,4,84,4,85,4,86,4,87,4,88,4,89,4,90,4,91,5,82,5,83,5,84,5,85,5,86,5,87,5,88,5,89,5,90,5,91,6,83,6,84,6,85,6,86,6,87,6,88,6,89,6,90,6,91,6,93,7,83,7,84,7,85,7,86,7,87,7,88,7,89,7,90,7,91,7,92,7,93,8,83,8,84,8,85,8,86,8,87,8,88,8,89,8,90,8,91,8,92,8,93,9,83,9,84,9,85,9,86,9,87,9,88,9,89,9,90,9,91,9,92,9,93 } );
            addToCreepLocsMap(BWAPI::TilePosition(83, 6), std::vector<int> { 75,6,75,7,75,8,76,10,76,4,76,5,76,6,76,7,76,8,76,9,77,10,77,11,77,3,77,4,77,5,77,6,77,7,77,8,77,9,78,10,78,11,78,3,78,4,78,5,78,6,78,7,78,8,78,9,79,10,79,11,79,12,79,2,79,3,79,4,79,5,79,6,79,7,79,8,79,9,80,10,80,11,80,12,80,2,80,3,80,4,80,5,80,6,80,7,80,8,80,9,81,10,81,11,81,12,81,2,81,3,81,4,81,5,81,6,81,7,81,8,81,9,82,1,82,10,82,11,82,12,82,13,82,2,82,3,82,4,82,5,82,6,82,7,82,8,82,9,83,10,83,11,83,12,83,13,83,3,83,4,83,5,83,6,83,7,83,8,83,9,84,10,84,11,84,12,84,13,84,3,84,4,84,5,84,6,84,7,84,8,84,9,85,10,85,11,85,12,85,13,85,3,85,4,85,5,85,6,85,7,85,8,85,9,86,10,86,11,86,12,86,13,86,3,86,4,86,5,86,6,86,7,86,8,86,9,87,1,87,10,87,11,87,12,87,13,87,2,87,3,87,4,87,5,87,6,87,7,87,8,87,9,88,10,88,11,88,12,88,2,88,3,88,4,88,5,88,6,88,7,88,8,88,9,89,10,89,11,89,3,89,4,89,5,89,6,89,7,89,8,89,9,90,10,90,4,90,5,90,6,90,8,91,10,91,5,92,10,92,11,92,3,92,5,92,7,92,9,93,10,93,4,93,5,93,6,93,7,93,8,93,9,94,6,94,7,94,8 } );
        }
        // Jade map version 1.0 (map name converted to plain text contains "Jade 1.0")
        else if (mapHash == "df21ac8f19f805e1e0d4e9aa9484969528195d9f")
        {
            addToCreepLocsMap(BWAPI::TilePosition(117, 117), std::vector<int> { 109,117,109,118,109,119,110,115,110,116,110,117,110,118,110,119,110,120,110,121,111,114,111,115,111,116,111,117,111,118,111,119,111,120,111,121,111,122,112,114,112,115,112,116,112,117,112,118,112,119,112,120,112,121,112,122,113,113,113,114,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,114,113,114,114,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,115,113,115,114,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,116,112,116,113,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,117,114,117,115,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,118,114,118,115,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,119,114,119,115,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,120,114,120,115,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,121,112,121,113,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,121,124,122,113,122,114,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,122,123,123,114,123,115,123,116,123,117,123,118,123,119,123,120,123,121,123,122,123,123,124,116,124,118,124,119,124,120,124,123,125,119,126,114,126,115,126,117,126,119,126,121,126,122,127,115,127,116,127,117,127,118,127,119,127,120,127,121 } );
            addToCreepLocsMap(BWAPI::TilePosition(117, 7), std::vector<int> { 109,7,109,8,109,9,110,10,110,11,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,12,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,13,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,13,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,10,115,11,115,12,115,13,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,14,116,2,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,14,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,14,118,4,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,14,119,4,119,5,119,6,119,7,119,8,119,9,120,10,120,11,120,12,120,13,120,14,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,13,121,14,121,2,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,12,122,13,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,11,123,12,123,13,123,4,123,5,123,6,123,7,123,8,123,9,124,11,124,13,124,5,124,8,124,9,125,5,126,10,126,12,126,4,126,5,126,6,126,7,127,10,127,11,127,5,127,6,127,7,127,8,127,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 7), std::vector<int> { 0,10,0,11,0,5,0,6,0,7,0,8,0,9,1,11,1,12,1,4,1,5,1,6,1,9,10,10,10,11,10,12,10,13,10,14,10,4,10,5,10,6,10,7,10,8,10,9,11,10,11,11,11,12,11,13,11,14,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,13,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,13,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,13,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,12,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,12,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,11,17,5,17,6,17,7,17,8,17,9,18,7,18,8,18,9,2,5,3,10,3,13,3,5,3,7,3,8,4,10,4,11,4,12,4,13,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,13,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,10,6,11,6,12,6,13,6,14,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,14,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,14,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,14,9,4,9,5,9,6,9,7,9,8,9,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 117), std::vector<int> { 0,117,0,118,0,119,1,115,1,116,1,117,1,118,1,119,1,120,1,121,10,114,10,115,10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,10,124,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,11,124,12,112,12,113,12,114,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,12,123,12,124,13,113,13,114,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,13,123,14,113,14,114,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,14,123,15,113,15,114,15,115,15,116,15,117,15,118,15,119,15,120,15,121,15,122,15,123,16,114,16,115,16,116,16,117,16,118,16,119,16,120,16,121,16,122,17,114,17,115,17,116,17,117,17,118,17,119,17,120,17,121,17,122,18,115,18,116,18,117,18,118,18,119,18,120,18,121,19,117,19,118,19,119,2,114,2,115,2,117,2,119,2,121,2,122,3,117,4,113,4,116,4,117,4,118,4,120,5,113,5,114,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,6,113,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,7,112,7,113,7,114,7,115,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,7,124,8,114,8,115,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,8,124,9,114,9,115,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123,9,124 } );
        }
        // La Mancha map version 1.1 (map name converted to plain text contains "La Mancha 1.1")
        else if (mapHash == "e47775e171fe3f67cc2946825f00a6993b5a415e")
        {
            addToCreepLocsMap(BWAPI::TilePosition(116, 117), std::vector<int> { 108,117,108,118,108,119,109,115,109,116,109,117,109,118,109,119,109,120,109,121,110,114,110,115,110,116,110,117,110,118,110,119,110,120,110,121,110,122,111,114,111,115,111,116,111,117,111,118,111,119,111,120,111,121,111,122,112,113,112,114,112,115,112,116,112,117,112,118,112,119,112,120,112,121,112,122,112,123,113,113,113,114,113,115,113,116,113,117,113,118,113,119,113,120,113,121,113,122,113,123,114,113,114,114,114,115,114,116,114,117,114,118,114,119,114,120,114,121,114,122,114,123,115,112,115,113,115,114,115,115,115,116,115,117,115,118,115,119,115,120,115,121,115,122,115,123,115,124,116,114,116,115,116,116,116,117,116,118,116,119,116,120,116,121,116,122,116,123,116,124,117,114,117,115,117,116,117,117,117,118,117,119,117,120,117,121,117,122,117,123,117,124,118,114,118,115,118,116,118,117,118,118,118,119,118,120,118,121,118,122,118,123,118,124,119,114,119,115,119,116,119,117,119,118,119,119,119,120,119,121,119,122,119,123,119,124,120,112,120,113,120,114,120,115,120,116,120,117,120,118,120,119,120,120,120,121,120,122,120,123,120,124,121,113,121,114,121,115,121,116,121,117,121,118,121,119,121,120,121,121,121,122,121,123,122,113,122,114,122,115,122,116,122,117,122,118,122,119,122,120,122,121,122,122,123,113,123,115,123,117,123,120,123,121,124,121,125,114,125,116,125,118,125,119,125,121,125,122,126,115,126,116,126,117,126,118,126,119,126,120,126,121,127,117,127,118,127,119 } );
            addToCreepLocsMap(BWAPI::TilePosition(116, 6), std::vector<int> { 109,4,109,5,109,9,110,10,110,11,110,3,110,4,110,5,110,6,110,7,110,8,110,9,111,10,111,11,111,3,111,4,111,5,111,6,111,7,111,8,111,9,112,10,112,11,112,12,112,2,112,3,112,4,112,5,112,6,112,7,112,8,112,9,113,10,113,11,113,12,113,2,113,3,113,4,113,5,113,6,113,7,113,8,113,9,114,10,114,11,114,12,114,2,114,3,114,4,114,5,114,6,114,7,114,8,114,9,115,1,115,10,115,11,115,12,115,13,115,2,115,3,115,4,115,5,115,6,115,7,115,8,115,9,116,10,116,11,116,12,116,13,116,3,116,4,116,5,116,6,116,7,116,8,116,9,117,10,117,11,117,12,117,13,117,3,117,4,117,5,117,6,117,7,117,8,117,9,118,10,118,11,118,12,118,13,118,3,118,4,118,5,118,6,118,7,118,8,118,9,119,10,119,11,119,12,119,13,119,3,119,4,119,5,119,6,119,7,119,8,119,9,120,1,120,10,120,11,120,12,120,13,120,2,120,3,120,4,120,5,120,6,120,7,120,8,120,9,121,10,121,11,121,12,121,2,121,3,121,4,121,5,121,6,121,7,121,8,121,9,122,10,122,11,122,2,122,3,122,4,122,5,122,6,122,7,122,8,122,9,123,10,123,2,123,4,123,6,123,9,124,10,125,10,125,11,125,3,125,5,125,7,125,8,126,10,126,4,126,5,126,6,126,7,126,8,126,9,127,6,127,7,127,8 } );
            addToCreepLocsMap(BWAPI::TilePosition(7, 6), std::vector<int> { 0,10,0,4,0,5,0,6,0,7,0,8,0,9,1,10,1,11,1,3,1,4,1,6,1,7,1,9,10,10,10,11,10,12,10,13,10,3,10,4,10,5,10,6,10,7,10,8,10,9,11,1,11,10,11,11,11,12,11,13,11,2,11,3,11,4,11,5,11,6,11,7,11,8,11,9,12,10,12,11,12,12,12,2,12,3,12,4,12,5,12,6,12,7,12,8,12,9,13,10,13,11,13,12,13,2,13,3,13,4,13,5,13,6,13,7,13,8,13,9,14,10,14,11,14,12,14,2,14,3,14,4,14,5,14,6,14,7,14,8,14,9,15,10,15,11,15,3,15,4,15,5,15,6,15,7,15,8,15,9,16,10,16,11,16,3,16,4,16,5,16,6,16,7,16,8,16,9,17,10,17,4,17,5,17,6,17,7,17,8,17,9,18,6,18,7,18,8,2,10,3,10,3,2,3,5,3,8,4,10,4,11,4,2,4,3,4,4,4,5,4,6,4,7,4,8,4,9,5,10,5,11,5,12,5,2,5,3,5,4,5,5,5,6,5,7,5,8,5,9,6,1,6,10,6,11,6,12,6,13,6,2,6,3,6,4,6,5,6,6,6,7,6,8,6,9,7,10,7,11,7,12,7,13,7,3,7,4,7,5,7,6,7,7,7,8,7,9,8,10,8,11,8,12,8,13,8,3,8,4,8,5,8,6,8,7,8,8,8,9,9,10,9,11,9,12,9,13,9,3,9,4,9,5,9,6,9,7,9,8,9,9 } );
            addToCreepLocsMap(BWAPI::TilePosition(8, 117), std::vector<int> { 0,117,0,118,0,119,1,115,1,116,1,117,1,118,1,119,1,120,1,121,10,114,10,115,10,116,10,117,10,118,10,119,10,120,10,121,10,122,10,123,10,124,11,114,11,115,11,116,11,117,11,118,11,119,11,120,11,121,11,122,11,123,11,124,12,112,12,113,12,114,12,115,12,116,12,117,12,118,12,119,12,120,12,121,12,122,12,123,12,124,13,113,13,114,13,115,13,116,13,117,13,118,13,119,13,120,13,121,13,122,13,123,14,113,14,114,14,115,14,116,14,117,14,118,14,119,14,120,14,121,14,122,14,123,15,113,15,114,15,115,15,116,15,117,15,118,15,119,15,120,15,121,15,122,15,123,16,114,16,115,16,116,16,117,16,118,16,119,16,120,16,121,16,122,17,114,17,115,17,116,17,117,17,118,17,119,17,120,17,121,17,122,18,117,18,118,18,119,18,120,18,121,19,117,19,118,19,119,2,114,2,115,2,117,2,118,2,120,2,121,2,122,3,121,4,113,4,116,4,119,4,121,5,113,5,114,5,115,5,116,5,117,5,118,5,119,5,120,5,121,5,122,6,113,6,114,6,115,6,116,6,117,6,118,6,119,6,120,6,121,6,122,6,123,7,112,7,113,7,114,7,115,7,116,7,117,7,118,7,119,7,120,7,121,7,122,7,123,7,124,8,114,8,115,8,116,8,117,8,118,8,119,8,120,8,121,8,122,8,123,8,124,9,114,9,115,9,116,9,117,9,118,9,119,9,120,9,121,9,122,9,123,9,124 } );
        }
        // Neo Chupung Ryeong map version 2.1 (map name is not in English but says "2.1" at the end)
        else if (mapHash == "f391700c3551e145852822ff95e27edd3173fae6")
        {
            addToCreepLocsMap(BWAPI::TilePosition(8, 103), std::vector<int> { 0,103,0,104,0,105,1,101,1,102,1,103,1,104,1,105,1,106,1,107,10,100,10,101,10,102,10,103,10,104,10,105,10,106,10,107,10,108,10,109,10,110,11,100,11,101,11,102,11,103,11,104,11,105,11,106,11,107,11,108,11,109,11,110,12,100,12,101,12,102,12,103,12,104,12,105,12,106,12,107,12,108,12,109,12,110,12,98,12,99,13,100,13,101,13,102,13,103,13,104,13,105,13,106,13,107,13,108,13,109,13,99,14,100,14,101,14,102,14,103,14,104,14,105,14,106,14,107,14,108,14,109,14,99,15,100,15,101,15,102,15,103,15,104,15,105,15,106,15,107,15,108,15,109,15,99,16,100,16,101,16,102,16,103,16,104,16,105,16,106,16,107,16,108,17,100,17,101,17,102,17,103,17,104,17,105,17,106,17,107,17,108,18,101,18,102,18,103,18,104,18,105,18,106,18,107,19,103,19,104,19,105,2,100,2,101,2,103,2,105,2,106,2,107,2,108,3,100,3,106,4,100,4,102,4,104,4,106,4,99,5,100,5,101,5,102,5,103,5,104,5,105,5,106,5,107,5,108,5,99,6,100,6,101,6,102,6,103,6,104,6,105,6,106,6,107,6,108,6,109,6,99,7,100,7,101,7,102,7,103,7,104,7,105,7,106,7,107,7,108,7,109,7,110,7,98,7,99,8,100,8,101,8,102,8,103,8,104,8,105,8,106,8,107,8,108,8,109,8,110,9,100,9,101,9,102,9,103,9,104,9,105,9,106,9,107,9,108,9,109,9,110 } );
            addToCreepLocsMap(BWAPI::TilePosition(84, 22), std::vector<int> { 76,22,76,23,76,24,77,20,77,21,77,22,77,23,77,24,77,25,77,26,78,19,78,20,78,21,78,22,78,23,78,24,78,25,78,26,78,27,79,19,79,20,79,21,79,22,79,23,79,24,79,25,79,26,79,27,80,18,80,19,80,20,80,21,80,22,80,23,80,24,80,25,80,26,80,27,80,28,81,18,81,19,81,20,81,21,81,22,81,23,81,24,81,25,81,26,81,27,81,28,82,18,82,19,82,20,82,21,82,22,82,23,82,24,82,25,82,26,82,27,82,28,83,17,83,18,83,19,83,20,83,21,83,22,83,23,83,24,83,25,83,26,83,27,83,28,83,29,84,19,84,20,84,21,84,22,84,23,84,24,84,25,84,26,84,27,84,28,84,29,85,19,85,20,85,21,85,22,85,23,85,24,85,25,85,26,85,27,85,28,85,29,86,19,86,20,86,21,86,22,86,23,86,24,86,25,86,26,86,27,86,28,86,29,87,19,87,20,87,21,87,22,87,23,87,24,87,25,87,26,87,27,87,28,87,29,88,17,88,18,88,19,88,20,88,21,88,22,88,23,88,24,88,25,88,26,88,27,88,28,88,29,89,18,89,19,89,20,89,21,89,22,89,23,89,24,89,25,89,26,89,27,89,28,90,19,90,20,90,21,90,22,90,23,90,24,90,25,90,26,90,27,90,28,91,21,91,23,91,25,91,27,91,28,92,21,92,27,93,19,93,20,93,21,93,22,93,24,93,26,93,27,94,20,94,21,94,22,94,23,94,24,94,25,94,26,95,22,95,23,95,24 } );
        }
    }

    /*

    // For the various maps and possible start locations, I dumped the tile positions of creep
    // around a zerg base at the start of the game and use this data (it's now hard-coded) when
    // scouting against a Zerg or Random race bot to help guess their start location. There may
    // be an easy algorithm to deduce the locations of the creep, but at the time, a submission
    // deadline was approaching and it was quicker than spending time trying to figure out
    // Broodwar's logic. Here's some old commented-out code I used to read and dump the data.

    static bool isCreepDataUpdateAttempted = false;
    if (!isCreepDataUpdateAttempted && myStartLoc != BWAPI::TilePositions::Unknown)
    {
        const std::string mapHash = Broodwar->mapHash();
        // Warning: violates StarcraftAITournamentManager rules cos it should use paths
        // bwapi-data/read/... and bwapi-data/write/... instead and would need a file
        // per opponent and rewrite entire file (don't just output differences).
        const std::string creepDataFilePath = "creep_data.txt";

        // Block to restrict scope of variables.
        {
            std::ifstream ifs(creepDataFilePath);
            if (ifs)
            {
                std::string line;
    
                std::string tmpMapHash;
                int tmpMyStartLocX;
                int tmpMyStartLocY;
                int tmpLocX;
                int tmpLocY;
                int tmpIsCompleteMapInfo;
                int tmpFrameNumSeen;
                int tmpIsCreep;
                int tmpIsDefinitelyZergEnemy;
                int tmpIsDefinitelyNoZergEnemy;
                int tmpIsMaybeZergEnemy;
                int tmpEOLSentinel;
    
                while(getline(ifs, line) && ifs)
                {
                    tmpMapHash = "";
    
                    std::istringstream iss(line);
    
                    iss >> tmpMapHash;
    
                    if (ifs && tmpMapHash == mapHash)
                    {
                        tmpMyStartLocX = -1;
                        tmpMyStartLocY = -1;
                        tmpLocX = -1;
                        tmpLocY = -1;
                        tmpIsCompleteMapInfo = -1;
                        tmpFrameNumSeen = -1;
                        tmpIsCreep = -1;
                        tmpIsDefinitelyZergEnemy = -1;
                        tmpIsDefinitelyNoZergEnemy = -1;
                        tmpIsMaybeZergEnemy = -1;
                        tmpEOLSentinel = -1;
    
                        iss >> tmpMyStartLocX;
                        iss >> tmpMyStartLocY;
                        iss >> tmpLocX;
                        iss >> tmpLocY;
                        iss >> tmpIsCompleteMapInfo;
                        iss >> tmpFrameNumSeen;
                        iss >> tmpIsCreep;
                        iss >> tmpIsDefinitelyZergEnemy;
                        iss >> tmpIsDefinitelyNoZergEnemy;
                        iss >> tmpIsMaybeZergEnemy;
                        // End-on-line sentinel should always be 1 (so we can check for incomplete lines/fields).
                        iss >> tmpEOLSentinel;
    
                        if (iss && tmpEOLSentinel == 1)
                        {
                            if (tmpFrameNumSeen == 0 && tmpIsCompleteMapInfo == 0 && tmpIsCreep == 1)
                            {
                                const BWAPI::TilePosition tmpStartLoc(tmpMyStartLocX, tmpMyStartLocY);
                                const BWAPI::TilePosition tmpLoc(tmpLocX, tmpLocY);
                                initialCreepLocsMap.val[tmpStartLoc].val.insert(tmpLoc);
                            }
                        }
                    }
                }
            }
        }

        // Block to restrict scope of variables.
        {
            std::ofstream ofs(creepDataFilePath, std::ios_base::out | std::ios_base::app);
            if (ofs)
            {
                for (int tmpX = 0; tmpX < Broodwar->mapWidth(); ++tmpX)
                {
                    for (int tmpY = 0; tmpY < Broodwar->mapHeight(); ++tmpY)
                    {
                        const BWAPI::TilePosition tmpLoc(tmpX, tmpY);
                        if (Broodwar->isVisible(tmpLoc))
                        {
                            if (Broodwar->hasCreep(tmpLoc) &&
                                (initialCreepLocsMap.val.find(myStartLoc) == initialCreepLocsMap.val.end() ||
                                 initialCreepLocsMap.val.at(myStartLoc).val.find(tmpLoc) == initialCreepLocsMap.val.at(myStartLoc).val.end()))
                            {
                                initialCreepLocsMap.val[myStartLoc].val.insert(tmpLoc);

                                ofs << mapHash << " ";
                                ofs << (int)(myStartLoc.x) << " ";
                                ofs << (int)(myStartLoc.y) << " ";
                                ofs << (int)(tmpX) << " ";
                                ofs << (int)(tmpY) << " ";
                                ofs << (Broodwar->isFlagEnabled(BWAPI::Flag::CompleteMapInformation) ? "1" : "0") << " ";
                                ofs << (int)(Broodwar->getFrameCount()) << " ";
                                ofs << (Broodwar->hasCreep(tmpLoc) ? "1" : "0") << " ";
                                ofs << (isARemainingEnemyZerg ? "1" : "0") << " ";
                                ofs << (isARemainingEnemyZerg ? "0" : "1") << " ";
                                ofs << (isARemainingEnemyZerg ? "1" : "0") << " ";
                                ofs << "1" << std::endl;
                            }
                        }
                    }
                }
            }
        }

        isCreepDataUpdateAttempted = true;
    }
    */

    // Here's some old commented-out experimental code I used while I was trying to figure out
    // Broodwar's logic.

    /*const int creepMaskWidth = 20;
    const int creepMaskHeight = 13;
    const int creepMaskStartLocX = 8;
    const int creepMaskStartLocY = 5;
    // Warning: be careful because the array is indexed via coordinates (y, x) not (x, y).
    const bool creepMask[creepMaskHeight][creepMaskWidth] =
        {
            { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
            { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
            { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
            { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
            { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
            { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
            { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
            { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
            { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
            { 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
            { 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
            { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 }
        };

    // TODO: Remove this debugging.
    if (Broodwar->getFrameCount() < 2)
    {
        const std::string mapHash = Broodwar->mapHash();

        for (int tmpXOffset = 0; tmpXOffset < creepMaskWidth; ++tmpXOffset)
        {
            const int tmpX = myStartLoc.x - creepMaskStartLocX + tmpXOffset;
            if (tmpX < 0)
            {
                continue;
            }

            if (tmpX >= Broodwar->mapWidth())
            {
                break;
            }

            for (int tmpYOffset = 0; tmpYOffset < creepMaskHeight; ++tmpYOffset)
            {
                const int tmpY = myStartLoc.y - creepMaskStartLocY + tmpYOffset;

                if (tmpY < 0)
                {
                    continue;
                }

                if (tmpY >= Broodwar->mapHeight())
                {
                    break;
                }

                const BWAPI::TilePosition tmpLoc = TilePosition(tmpX, tmpY);
                if (Broodwar->isVisible(tmpLoc) &&
                    Broodwar->getUnitsOnTile(tmpLoc, IsVisible && Exists && IsBuilding && !IsLifted).empty())
                {
                    if (creepMask[tmpYOffset][tmpXOffset] != Broodwar->hasCreep(tmpLoc))
                    {
                        std::ofstream ofs("creep_data_debug.txt", std::ios_base::out | std::ios_base::app);
                        if (ofs)
                        {
                            ofs << mapHash << " ";
                            ofs << (int)(myStartLoc.x) << " ";
                            ofs << (int)(myStartLoc.y) << " ";
                            ofs << (int)(tmpX) << " ";
                            ofs << (int)(tmpY) << " ";
                            ofs << (Broodwar->isFlagEnabled(BWAPI::Flag::CompleteMapInformation) ? "1" : "0") << " ";
                            ofs << (int)(Broodwar->getFrameCount()) << " ";
                            ofs << (Broodwar->hasCreep(tmpLoc) ? "1" : "0") << " ";
                            ofs << (isARemainingEnemyZerg ? "1" : "0") << " ";
                            ofs << (isARemainingEnemyZerg ? "0" : "1") << " ";
                            ofs << (isARemainingEnemyZerg ? "1" : "0") << " ";
                            ofs << "1" << std::endl;
                        }
                    }
                }
            }
        }
    }*/

    static BWAPI::TilePosition probableEnemyStartLoc = BWAPI::TilePositions::Unknown;
    BWAPI::TilePosition probableEnemyStartLocBasedOnCreep = BWAPI::TilePositions::Unknown;

    if (probableEnemyStartLoc != BWAPI::TilePositions::Unknown &&
        unscoutedOtherStartLocs.find(probableEnemyStartLoc) == unscoutedOtherStartLocs.end())
    {
        probableEnemyStartLoc = BWAPI::TilePositions::Unknown;
    }

    for (const BWAPI::TilePosition otherStartLoc : unscoutedOtherStartLocs)
    {
        // Check the tiles where the command centre/nexus/hatchery/lair/hive would be.
        const BWAPI::UnitType tmpUnitType = BWAPI::UnitTypes::Special_Start_Location;
        bool foundOne = false;
        for (int tmpX = otherStartLoc.x; tmpX < otherStartLoc.x + tmpUnitType.tileWidth(); ++tmpX)
        {
            for (int tmpY = otherStartLoc.y; tmpY < otherStartLoc.y + tmpUnitType.tileHeight(); ++tmpY)
            {
                const BWAPI::TilePosition tmpLoc = TilePosition(tmpX, tmpY);
                if (Broodwar->isVisible(tmpLoc) &&
                    Broodwar->getUnitsOnTile(tmpLoc, IsEnemy && IsVisible && Exists && IsBuilding && !IsLifted).empty())
                {
                    scoutedOtherStartLocs.insert(otherStartLoc);

                    if (probableEnemyStartLoc == otherStartLoc)
                    {
                        probableEnemyStartLoc = BWAPI::TilePositions::Unknown;
                    }

                    foundOne = true;
                    break;
                }
            }

            if (foundOne)
            {
                break;
            }
        }

        // Check the tiles where enemy creep would be.
        if (!foundOne && (isARemainingEnemyZerg || isARemainingEnemyRandomRace))
        {
            if (initialCreepLocsMap.val.find(otherStartLoc) != initialCreepLocsMap.val.end())
            {
                for (const BWAPI::TilePosition tmpLoc : initialCreepLocsMap.val.at(otherStartLoc).val)
                {
                    if (Broodwar->isVisible(tmpLoc) &&
                        // Note: a minor point that doesn't matter very much: don't also check for IsEnemy because creep
                        // does not spread on to non-Zerg buildings, so if there is a non-Zerg non-enemy building (e.g.
                        // my bunker if I am bunker-rushing an enemy Zerg player) on a tile we check for creep we can't
                        // use the hasCreep() check to determine whether or not there is an enemy nearby.
                        Broodwar->getUnitsOnTile(tmpLoc, IsVisible && Exists && IsBuilding && !IsLifted).empty())
                    {
                        // Notes about Broodwar->hasCreep(): it behaves pretty intuitively as you would expect,
                        // i.e. it is true iff Broodwar currently displays creep on the tile or if there is a building
                        // on the tile but creep would immediately be displayed on the tile if the building is destroyed
                        // or if it contains a completed extractor.
                        // So, for example, it would return true for tiles where your starting hatchery is, where the
                        // enemy's starting hatchery is, completed hatcheries, completed extractors, your buildings
                        // where there is creep e.g. spawning pool (note that if hatcheries/creep colonies near
                        // buildings it are destroyed and the creep recedes the tiles where it is will still all retain
                        // creep), but would return false for mineral patches or geysers (i.e. geysers without extractor
                        // although note that for extractors it returns false while incomplete then true after completed)
                        // near hatcheries, incomplete hatcheries built away from creep (note: if you start building a
                        // hatchery half on creep and half off creep, while it is morphing half of it will have creep and
                        // half will not and creep can spread over it e.g. due to creep colonies completed nearby).
                        // Note: creep does not spread on to non-Zerg buildings (i.e. immediately after you destroy it it
                        // will not have creep but creep will start to spread there if possible).
                        if (Broodwar->hasCreep(tmpLoc))
                        {
                            if (!isARemainingEnemyZerg && isARemainingEnemyRandomRace)
                            {
                                // We deduce that a random race enemy is zerg. This logic only supports 1v1 games properly currently.
                                isARemainingEnemyZerg = true;
                                isARemainingEnemyRandomRace = false;
                            }
                            
                            probableEnemyStartLocBasedOnCreep = otherStartLoc;
                            probableEnemyStartLoc = probableEnemyStartLocBasedOnCreep;
                            break;
                        }
                        else if (!isARemainingEnemyRandomRace)
                        {
                            scoutedOtherStartLocs.insert(otherStartLoc);

                            if (probableEnemyStartLoc == otherStartLoc)
                            {
                                probableEnemyStartLoc = BWAPI::TilePositions::Unknown;
                            }

                            break;
                        }
                    }
                }
            }
        }
    }

    for (const BWAPI::TilePosition otherStartLoc : scoutedOtherStartLocs)
    {
        unscoutedOtherStartLocs.erase(otherStartLoc);
    }

    static BWAPI::Unit mainBase = nullptr;
    static std::map<const BWAPI::Unit, BWAPI::Unit> gathererToResourceMap;
    auto gathererToResourceMapAuto = gathererToResourceMap;
    static std::map<const BWAPI::Unit, BWAPI::Unit> resourceToGathererMap;
    auto resourceToGathererMapAuto = resourceToGathererMap;

    if (mainBase == nullptr || !mainBase->exists())
    {
        mainBase =
            Broodwar->getClosestUnit(
                (myStartRoughPos != BWAPI::Positions::Unknown ?
                 myStartRoughPos :
                 BWAPI::Position((Broodwar->mapWidth() * BWAPI::TILEPOSITION_SCALE) / 2, (Broodwar->mapHeight() * BWAPI::TILEPOSITION_SCALE) / 2)),
                BWAPI::Filter::IsResourceDepot && BWAPI::Filter::IsOwned && BWAPI::Filter::IsCompleted && !BWAPI::Filter::IsLifted && BWAPI::Filter::Exists);
    }

    auto mainBaseAuto = mainBase;

    static BWAPI::Race enemyRaceInit;
    static BWAPI::Race enemyRaceScouted;

    static bool checkedEnemyDetails = false;
    if (!checkedEnemyDetails)
    {
        checkedEnemyDetails = true;

        const std::string AIDirPath = "bwapi-data/AI/";
        const std::string readDirPath = "bwapi-data/read/";
        const std::string writeDirPath = "bwapi-data/write/";

        const int myVersionMajor = 1;
        const int myVersionMinor = 7;
        const int myVersionUpdate = 0;
        const int myVersionPatch = 0;
        // Not used currently (always 0).
        const int myVersionBuildNum = 0;
        const std::string versionFieldDelimiter = ".";
        const std::string myVersionStr =
            std::to_string(myVersionMajor) + versionFieldDelimiter +
            std::to_string(myVersionMinor) + versionFieldDelimiter +
            std::to_string(myVersionUpdate) + versionFieldDelimiter +
            std::to_string(myVersionPatch) + versionFieldDelimiter +
            std::to_string(myVersionBuildNum);

        const std::string myBotNameHardCoded = "ZZZKBot";
        const std::string pathFieldDelimiter = "_";
        const std::string versionNumPrefix = "v";
        const std::string versusSignifierStr = "vs";
        const std::string filePrefix =
            myBotNameHardCoded + pathFieldDelimiter +
            versionNumPrefix + pathFieldDelimiter + myVersionStr + pathFieldDelimiter +
            Broodwar->self()->getRace().getName() + pathFieldDelimiter +
            versusSignifierStr;

        const std::string configFileExtension = "cfg";
        const std::string configFilePath = AIDirPath + Broodwar->self()->getName().data() + "." + configFileExtension;

        const std::string dataFileExtension = "dat";

        bool isEnemyWriteFileMissingOrUnreadable = false;
        bool isTooSmallFileDetected = false;
        bool isFileCopyNeeded = false;
        bool isFileCopyFailed = false;

        ss.is4PoolBO = true;
        ss.isSpeedlingBO = false;
        ss.isHydraRushBO = false;
        ss.isMutaRushBODecidedAfterScoutEnemyRace = false;
        ss.isMutaRushBO = true;
        ss.isMutaRushBOVsProtoss = ss.isMutaRushBO;
        ss.isMutaRushBOVsTerran = ss.isMutaRushBO;
        ss.isMutaRushBOVsZerg = ss.isMutaRushBO;
        ss.isSpeedlingPushDeferred = false;
        ss.isEnemyWorkerRusher = false;
        ss.isNumSunkensDecidedAfterScoutEnemyRace = false;
        ss.numSunkens = 1;
        ss.numSunkensVsProtoss = ss.numSunkens;
        ss.numSunkensVsTerran = ss.numSunkens;
        ss.numSunkensVsZerg = ss.numSunkens;

        static BWAPI::PlayerType enemyPlayerType;
        static BWAPI::TilePosition enemyStartLoc;
        static BWAPI::TilePosition enemyStartLocDeduced = BWAPI::TilePositions::Unknown;
        static std::string enemyName;
        static std::string enemyNameUpperCase;
        static std::string enemyFileName;
        static std::string enemyReadFilePath;

        auto beginsWith =
            [](const std::string& baseStr, const std::string& comparisonStr)
            {
                return (bool) (baseStr.compare(0, comparisonStr.size(), comparisonStr) == 0);
            };

        const Playerset players = Broodwar->getPlayers();

        for (auto p : players)
        {
            if (p->isEnemy(Broodwar->self()))
            {
                enemyPlayerID = static_cast<int>(p->getID());
                enemyPlayerType = p->getType();
                enemyRaceInit = p->getRace();
                enemyRaceScouted = enemyRaceInit;
                enemyStartLoc = p->getStartLocation();
                if (enemyStartLoc == BWAPI::TilePositions::Unknown &&
                    otherStartLocs.size() == 1)
                {
                    enemyStartLocDeduced = *otherStartLocs.begin();
                }
                enemyName = Broodwar->enemy() ? Broodwar->enemy()->getName() : "";
                enemyFileName = filePrefix + pathFieldDelimiter + enemyName + pathFieldDelimiter + enemyRaceInit.getName() + "." + dataFileExtension;
                enemyReadFilePath = readDirPath + enemyFileName;
                enemyWriteFilePath = writeDirPath + enemyFileName;
                enemyNameUpperCase = enemyName;
                std::transform(enemyNameUpperCase.begin(), enemyNameUpperCase.end(),enemyNameUpperCase.begin(), ::toupper);

                // Block to restrict scope of variables.
                {
                    std::ifstream ifs(configFilePath);
                    if (ifs)
                    {
                        bool found = false;
                        std::string line;
                        std::string tmpPlayerName;
                        std::string tmpRace;
                        bool tmpIs4PoolBO = false;
                        bool tmpIsSpeedlingBO = false;
                        bool tmpIsHydraRushBO = false;
                        bool tmpIsMutaRushBO = true;
                        bool tmpIsSpeedlingPushDeferred = false;
                        bool tmpIsEnemyWorkerRusher = false;
                        bool tmpIsNumSunkensDecidedAfterScoutEnemyRace = false;
                        int tmpNumSunkens = 1;
                        int tmpNumSunkensVsProtoss = tmpNumSunkens;
                        int tmpNumSunkensVsTerran = tmpNumSunkens;
                        int tmpNumSunkensVsZerg = tmpNumSunkens;
                        int tmpEOLSentinel = -1;
            
                        while(getline(ifs, line) && ifs)
                        {
                            std::istringstream iss(line);
            
                            iss >> tmpPlayerName;
                            iss >> tmpRace;
                            iss >> tmpIs4PoolBO;
                            iss >> tmpIsSpeedlingBO;
                            iss >> tmpIsHydraRushBO;
                            iss >> tmpIsMutaRushBO;
                            iss >> tmpIsSpeedlingPushDeferred;
                            iss >> tmpIsEnemyWorkerRusher;
                            iss >> tmpIsNumSunkensDecidedAfterScoutEnemyRace;
                            iss >> tmpNumSunkens;
                            iss >> tmpEOLSentinel;
                            if (iss && tmpEOLSentinel >= 0 && tmpPlayerName == enemyName && tmpRace == enemyRaceInit.getName())
                            {
                                if (tmpIsNumSunkensDecidedAfterScoutEnemyRace)
                                {
                                    tmpNumSunkensVsProtoss = tmpEOLSentinel;
                                    iss >> tmpNumSunkensVsTerran;
                                    iss >> tmpNumSunkensVsZerg;
                                    iss >> tmpEOLSentinel;
                                }
    
                                if (iss && tmpEOLSentinel == 1)
                                {
                                    ss.is4PoolBO = tmpIs4PoolBO;
                                    ss.isSpeedlingBO = tmpIsSpeedlingBO;
                                    ss.isHydraRushBO = tmpIsHydraRushBO;
                                    ss.isMutaRushBO = tmpIsMutaRushBO;
                                    ss.isSpeedlingPushDeferred = tmpIsSpeedlingPushDeferred;
                                    ss.isEnemyWorkerRusher = tmpIsEnemyWorkerRusher;
                                    ss.isNumSunkensDecidedAfterScoutEnemyRace = tmpIsNumSunkensDecidedAfterScoutEnemyRace;
                                    ss.numSunkens = tmpNumSunkens;
                                    found = true;
                                    break;
                                }
                            }
                        }
    
                        if (found)
                        {
                            break;
                        }
                    }
                }

                if (//enemyRaceInit == BWAPI::Races::Zerg &&
                    (enemyName == "Chris Coxe" ||
                     (beginsWith(enemyNameUpperCase, "CHRIS") && enemyNameUpperCase.find("COXE") != std::string::npos) ||
                     beginsWith(enemyNameUpperCase, "ZZZBOT") ||
                     beginsWith(enemyNameUpperCase, "ZZZK") ||
                     beginsWith(enemyNameUpperCase, "ZZZ")))
                {
                    ss.is4PoolBO = false;
                    ss.isSpeedlingBO = false;
                    ss.isHydraRushBO = false;
                    ss.isMutaRushBO = false;
                    ss.numSunkens = 0;
                    break;
                }
            }
        }

        // If found an enemy...
        if (enemyPlayerID >= 0)
        {
            isEnemyWriteFileMissingOrUnreadable = true;

            // Block to restrict scope of variables.
            {
                std::ifstream enemyWriteFileIFS(enemyWriteFilePath);
                if (enemyWriteFileIFS)
                {
                    // The file in the write folder already exists and could healthily be read.
                    enemyWriteFileIFS.seekg(0, std::ios::end);
                    std::ifstream::pos_type writeFileSize = enemyWriteFileIFS.tellg();
                    if (enemyWriteFileIFS)
                    {
                        isEnemyWriteFileMissingOrUnreadable = false;
                        std::ifstream enemyReadFileIFS(enemyReadFilePath, std::ios::binary);
                        if (enemyReadFileIFS)
                        {
                            // The file in the write folder already exists too and could healthily be read, so
                            // compare the file sizes to try to detect possible interruptions (e.g. process
                            // killed) while the file was being copied during a past game that may have led to
                            // a partially copied file (i.e. corrupt).
                            enemyReadFileIFS.seekg(0, std::ios::end);
                            std::ifstream::pos_type readFileSize = enemyReadFileIFS.tellg();
                            if (enemyReadFileIFS)
                            {
                                // Note: if it is the same size, for simplicity let's assume the content is identical.
                                if (writeFileSize < readFileSize)
                                {
                                    isTooSmallFileDetected = true;
                                    isFileCopyNeeded = true;
                                }
                            }
                        }
                    }
                }
            }
    
            const std::string tmpFileExtension = "tmp";
            const std::string tmpFilePath0 = enemyWriteFilePath + ".0." + tmpFileExtension;
            const std::string tmpFilePath1 = enemyWriteFilePath + ".1." + tmpFileExtension;

            if (isEnemyWriteFileMissingOrUnreadable || isFileCopyNeeded)
            {
                // Copy the file from the read folder to a temporary file in the write folder, then
                // use the temporary file to replace the file in the write folder if it already exists.
                // Then delete any temporary files if any still exist for whatever reason.
                // Afterwards, we will append to the file in the write folder.

                if (isFileCopyNeeded)
                {
                    isFileCopyFailed = true;
                }

                // Block to restrict scope of variables.
                {
                    std::ifstream enemyReadFileIFS(enemyReadFilePath, std::ios::binary);
                    if (enemyReadFileIFS)
                    {
                        isFileCopyNeeded = true;
                        isFileCopyFailed = true;
    
                        // Block to restrict scope of variables.
                        {
                            std::ofstream tmpFileOFS(tmpFilePath0, std::ios::binary);
                            if (tmpFileOFS)
                            {
                                tmpFileOFS << enemyReadFileIFS.rdbuf();
                                tmpFileOFS.flush();
                                if (tmpFileOFS && enemyReadFileIFS)
                                {
                                    isFileCopyFailed = false;
                                }
                            }
                        }
                    }
                }
    
                if (isFileCopyNeeded && !isFileCopyFailed)
                {
                    isFileCopyFailed = true;

                    // Note: probably doesn't exist.
                    remove(tmpFilePath1.c_str());

                    // Note: may not exist.
                    rename(enemyWriteFilePath.c_str(), tmpFilePath1.c_str());

                    int result = rename(tmpFilePath0.c_str(), enemyWriteFilePath.c_str());
                    if (result == 0)
                    {
                        isFileCopyFailed = false;
                    }
                }

                if (isFileCopyFailed)
                {
                    // TODO: produce some kind of error message?
                }
            }

            // Note: probably doesn't exist.
            remove(tmpFilePath1.c_str());
            // Note: probably doesn't exist.
            remove(tmpFilePath0.c_str());

            // TODO: abstract line and field read/write handler.
            // Block to restrict scope of variables.
            {
                std::ifstream enemyWriteFileIFS(enemyWriteFilePath);
                if (enemyWriteFileIFS)
                {
                    int tmpGameID = -1;
                    std::string line;
                    while(getline(enemyWriteFileIFS, line) && enemyWriteFileIFS)
                    {
                        ++tmpGameID;
                        std::vector<std::string> fields;
                        // Block to restrict scope of variables.
                        {
                            std::size_t pos = 0;
                            std::string field;
                            while ((pos = line.find(delim)) != std::string::npos)
                            {
                                field = line.substr(0, pos);
                                fields.push_back(field);
                                line.erase(0, pos + delim.length());
                            }
                            fields.push_back(line);
                        }

                        // Validate the types of fields in the line are in the expected sequence and complete,
                        // and read and validate the relevant fields into variables.

                        const size_t minInitUpdateFields = 91;
                        size_t minFieldsExpected = minInitUpdateFields;
                        if (fields.size() < minInitUpdateFields ||
                            fields.at(fields.size() - 1) != endOfLineSentinel ||
                            fields.at(fields.size() - 2) != endOfUpdateSentinel ||
                            fields.at(0) != startOfLineSentinel ||
                            fields.at(1) != endOfLineSentinel ||
                            fields.at(2) != startOfUpdateSentinel ||
                            fields.at(3) != endOfUpdateSentinel ||
                            fields.at(4) != initUpdateSignifier ||
                            fields.at(5) != dataFileExtension ||
                            fields.at(6) != pathFieldDelimiter ||
                            fields.at(7) != std::to_string(myVersionMajor) ||
                            fields.at(8) != std::to_string(myVersionMinor))
                        {
                            // TODO: corrupt or incomplete line detected (e.g. bot killed before onEnd()), so provide an error message?
                            continue;
                        }

                        // Variables for parsing relevant fields for learning purposes.
                        const int myRacePickedInd = 28;
                        std::string tmpMyRacePicked = fields.at(myRacePickedInd);
                        if (tmpMyRacePicked != Broodwar->self()->getRace().getName())
                        {
                            continue;
                        }

                        const int myRaceRolledInd = 29;
                        std::string tmpMyRaceRolled = fields.at(myRaceRolledInd);
                        if (tmpMyRaceRolled != Broodwar->self()->getRace().getName())
                        {
                            continue;
                        }

                        const int myStartLocXInd = 30;
                        int tmpMyStartLocX = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(myStartLocXInd));
                            if (! (iss >> tmpMyStartLocX))
                            {
                                continue;
                            }
                        }

                        const int myStartLocYInd = 31;
                        int tmpMyStartLocY = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(myStartLocYInd));
                            if (! (iss >> tmpMyStartLocY))
                            {
                                continue;
                            }
                        }

                        const int enemyPlayerIDInd = 33;
                        int tmpEnemyPlayerID = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(enemyPlayerIDInd));
                            if (! (iss >> tmpEnemyPlayerID))
                            {
                                continue;
                            }
                        }

                        const int enemyPlayerNameInd = 35;
                        std::string tmpEnemyPlayerName = fields.at(enemyPlayerNameInd);

                        const int enemyRacePickedInd = 36;
                        std::string tmpEnemyRacePicked = fields.at(enemyRacePickedInd);

                        const int enemyRaceInitInd = 37;
                        std::string tmpEnemyRaceInit = fields.at(enemyRaceInitInd);

                        const int enemyStartLocDeducedXInd = 38;
                        int tmpEnemyStartLocDeducedX = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(enemyStartLocDeducedXInd));
                            if (! (iss >> tmpEnemyStartLocDeducedX))
                            {
                                continue;
                            }
                        }

                        const int enemyStartLocDeducedYInd = 39;
                        int tmpEnemyStartLocDeducedY = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(enemyStartLocDeducedYInd));
                            if (! (iss >> tmpEnemyStartLocDeducedY))
                            {
                                continue;
                            }
                        }

                        const int mapHashInd = 46;
                        std::string tmpMapHash = fields.at(mapHashInd);

                        // TODO: could consider using these for learning too (and/or map area).
                        //int tmpMapWidth;
                        //int tmpMapHeight;

                        const int isCompleteMapInformationEnabledInd = 49;
                        bool tmpIsCompleteMapInformationEnabled = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isCompleteMapInformationEnabledInd));
                            if (! (iss >> tmpIsCompleteMapInformationEnabled))
                            {
                                continue;
                            }
                        }
                        if (tmpIsCompleteMapInformationEnabled) //&&
                            //!Broodwar->isFlagEnabled(BWAPI::Flag::CompleteMapInformation))
                        {
                            continue;
                        }

                        const int numStartLocationsInd = 51;
                        int tmpNumStartLocations = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(numStartLocationsInd));
                            if (! (iss >> tmpNumStartLocations))
                            {
                                continue;
                            }
                        }
                        if (tmpNumStartLocations < 0)
                        {
                            // TODO: corrupt or incomplete line detected (e.g. bot killed before onEnd()), so provide an error message?
                            continue;
                        }
                        // If there are exactly two start locations on this map then deduce where the enemy start location is.
                        if (BWAPI::TilePosition(tmpEnemyStartLocDeducedX, tmpEnemyStartLocDeducedY) == BWAPI::TilePositions::Unknown &&
                            tmpNumStartLocations == 2)
                        {
                            for (int startLocXInd = numStartLocationsInd + 1, startLocYInd = numStartLocationsInd + 2; startLocXInd < numStartLocationsInd + (tmpNumStartLocations * 2); startLocXInd += 2, startLocYInd += 2)
                            {
                                int tmpStartLocX = -1;
                                // Block to restrict scope of variables.
                                {
                                    std::istringstream iss(fields.at(startLocXInd));
                                    if (! (iss >> tmpStartLocX))
                                    {
                                        continue;
                                    }
                                }
        
                                int tmpStartLocY = -1;
                                // Block to restrict scope of variables.
                                {
                                    std::istringstream iss(fields.at(startLocYInd));
                                    if (! (iss >> tmpStartLocY))
                                    {
                                        continue;
                                    }
                                }

                                if (tmpStartLocX != tmpMyStartLocX &&
                                    tmpStartLocY != tmpMyStartLocY &&
                                    BWAPI::TilePosition(tmpStartLocX, tmpStartLocY) != BWAPI::TilePositions::Unknown)
                                {
                                    tmpEnemyStartLocDeducedX = tmpStartLocX;
                                    tmpEnemyStartLocDeducedY = tmpStartLocY;
                                    break;
                                }
                            }
                        }

                        const int tmpOffset = tmpNumStartLocations * 2;
                        minFieldsExpected += tmpOffset;

                        if (fields.size() < minFieldsExpected ||
                            fields.at(minFieldsExpected - 2) != endOfUpdateSentinel)
                        {
                            // TODO: corrupt or incomplete line detected (e.g. bot killed before onEnd()), so provide an error message?
                            continue;
                        }

                        // TODO: could consider using these for learning too.
                        //std::string tmpGameType;
                        //int tmpLatency;
                        //int tmpLatencyFrames;
                        //bool tmpIsLatComEnabled;

                        const int timerAtGameStartInd = 74 + tmpOffset;
                        int tmpTimerAtGameStart = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(timerAtGameStartInd));
                            if (! (iss >> tmpTimerAtGameStart))
                            {
                                continue;
                            }
                        }

                        const int is4PoolBOInd = 74 + tmpOffset;
                        bool tmpIs4PoolBO = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(is4PoolBOInd));
                            if (! (iss >> tmpIs4PoolBO))
                            {
                                continue;
                            }
                        }

                        const int isSpeedlingBOInd = 75 + tmpOffset;
                        bool tmpIsSpeedlingBO = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isSpeedlingBOInd));
                            if (! (iss >> tmpIsSpeedlingBO))
                            {
                                continue;
                            }
                        }

                        const int isHydraRushBOInd = 76 + tmpOffset;
                        bool tmpIsHydraRushBO = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isHydraRushBOInd));
                            if (! (iss >> tmpIsHydraRushBO))
                            {
                                continue;
                            }
                        }

                        const int isMutaRushBODecidedAfterScoutEnemyRaceInd = 77 + tmpOffset;
                        bool tmpIsMutaRushBODecidedAfterScoutEnemyRace = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isMutaRushBODecidedAfterScoutEnemyRaceInd));
                            if (! (iss >> tmpIsMutaRushBODecidedAfterScoutEnemyRace))
                            {
                                continue;
                            }
                        }

                        const int isMutaRushBOInd = 78 + tmpOffset;
                        bool tmpIsMutaRushBO = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isMutaRushBOInd));
                            if (! (iss >> tmpIsMutaRushBO))
                            {
                                continue;
                            }
                        }

                        const int isMutaRushBOVsProtossInd = 79 + tmpOffset;
                        bool tmpIsMutaRushBOVsProtoss = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isMutaRushBOVsProtossInd));
                            if (! (iss >> tmpIsMutaRushBOVsProtoss))
                            {
                                continue;
                            }
                        }

                        const int isMutaRushBOVsTerranInd = 80 + tmpOffset;
                        bool tmpIsMutaRushBOVsTerran = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isMutaRushBOVsTerranInd));
                            if (! (iss >> tmpIsMutaRushBOVsTerran))
                            {
                                continue;
                            }
                        }

                        const int isMutaRushBOVsZergInd = 81 + tmpOffset;
                        bool tmpIsMutaRushBOVsZerg = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isMutaRushBOVsZergInd));
                            if (! (iss >> tmpIsMutaRushBOVsZerg))
                            {
                                continue;
                            }
                        }

                        const int isSpeedlingPushDeferredInd = 82 + tmpOffset;
                        bool tmpIsSpeedlingPushDeferred = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isSpeedlingPushDeferredInd));
                            if (! (iss >> tmpIsSpeedlingPushDeferred))
                            {
                                continue;
                            }
                        }

                        const int isEnemyWorkerRusherInd = 83 + tmpOffset;
                        bool tmpIsEnemyWorkerRusher = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isEnemyWorkerRusherInd));
                            if (! (iss >> tmpIsEnemyWorkerRusher))
                            {
                                continue;
                            }
                        }

                        const int isNumSunkensDecidedAfterScoutEnemyRaceInd = 84 + tmpOffset;
                        bool tmpIsNumSunkensDecidedAfterScoutEnemyRace = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isNumSunkensDecidedAfterScoutEnemyRaceInd));
                            if (! (iss >> tmpIsNumSunkensDecidedAfterScoutEnemyRace))
                            {
                                continue;
                            }
                        }

                        const int numSunkensInd = 85 + tmpOffset;
                        int tmpNumSunkens = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(numSunkensInd));
                            if (! (iss >> tmpNumSunkens))
                            {
                                continue;
                            }
                        }

                        const int numSunkensVsProtossInd = 86 + tmpOffset;
                        int tmpNumSunkensVsProtoss = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(numSunkensVsProtossInd));
                            if (! (iss >> tmpNumSunkensVsProtoss))
                            {
                                continue;
                            }
                        }

                        const int numSunkensVsTerranInd = 87 + tmpOffset;
                        int tmpNumSunkensVsTerran = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(numSunkensVsTerranInd));
                            if (! (iss >> tmpNumSunkensVsTerran))
                            {
                                continue;
                            }
                        }

                        const int numSunkensVsZergInd = 88 + tmpOffset;
                        int tmpNumSunkensVsZerg = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(numSunkensVsZergInd));
                            if (! (iss >> tmpNumSunkensVsZerg))
                            {
                                continue;
                            }
                        }

                        StratSettings tmpStratSettings =
                        {
                            tmpIs4PoolBO,
                            tmpIsSpeedlingBO,
                            tmpIsHydraRushBO,
                            tmpIsMutaRushBODecidedAfterScoutEnemyRace,
                            tmpIsMutaRushBO,
                            tmpIsMutaRushBOVsProtoss,
                            tmpIsMutaRushBOVsTerran,
                            tmpIsMutaRushBOVsZerg,
                            tmpIsSpeedlingPushDeferred,
                            tmpIsEnemyWorkerRusher,
                            tmpIsNumSunkensDecidedAfterScoutEnemyRace,
                            tmpNumSunkens,
                            tmpNumSunkensVsProtoss,
                            tmpNumSunkensVsTerran,
                            tmpNumSunkensVsZerg
                        };

                        if (fields.size() < minFieldsExpected + 1 ||
                            fields.at(minFieldsExpected - 1) != startOfUpdateSentinel)
                        {
                            // TODO: corrupt or incomplete line detected (e.g. bot killed before onEnd()), so provide an error message?
                            continue;
                        }

                        int tmpEnemyPlayerLeftFrameCount = -1;
                        bool tmpIsEnemyPlayerLeft = false;

                        int tmpEnemyRaceScoutedFrameCount = -1;
                        std::string tmpEnemyRaceScouted;
                        if (tmpEnemyRaceInit != BWAPI::Races::Unknown.getName())
                        {
                            tmpEnemyRaceScouted = tmpEnemyRaceInit;
                        }

                        bool isSkipping = false;
                        while (fields.at(minFieldsExpected) == onPlayerLeftUpdateSignifier ||
                               fields.at(minFieldsExpected) == raceScoutedUpdateSignifier)
                        {
                            const int tmpUpdateSignifierInd = minFieldsExpected;
                            const std::string tmpUpdateSignifier = fields.at(tmpUpdateSignifierInd);
                            if (tmpUpdateSignifier == onPlayerLeftUpdateSignifier)
                            {
                                minFieldsExpected += 10;
                            }
                            else if (tmpUpdateSignifier == raceScoutedUpdateSignifier)
                            {
                                minFieldsExpected += 9;
                            }

                            if (fields.size() < minFieldsExpected ||
                                fields.at(minFieldsExpected - 2) != endOfUpdateSentinel)
                            {
                                // TODO: corrupt or incomplete line detected (e.g. bot killed before onEnd()), so provide an error message?
                                isSkipping = true;
                                break;
                            }

                            if (fields.size() < minFieldsExpected + 1 ||
                                fields.at(minFieldsExpected - 1) != startOfUpdateSentinel)
                            {
                                // TODO: corrupt or incomplete line detected (e.g. bot killed before onEnd()), so provide an error message?
                                isSkipping = true;
                                break;
                            }

                            if (tmpUpdateSignifier == onPlayerLeftUpdateSignifier)
                            {
                                const int onEnemyPlayerLeftFrameCountInd = tmpUpdateSignifierInd + 1;
                                int tmpOnEnemyPlayerLeftFrameCount = -1;
                                // Block to restrict scope of variables.
                                {
                                    std::istringstream iss(fields.at(onEnemyPlayerLeftFrameCountInd));
                                    if (! (iss >> tmpOnEnemyPlayerLeftFrameCount))
                                    {
                                        isSkipping = true;
                                        break;
                                    }
                                }

                                const int onEnemyPlayerLeftPlayerIDInd = tmpUpdateSignifierInd + 6;
                                int tmpOnEnemyPlayerLeftPlayerID = -1;
                                // Block to restrict scope of variables.
                                {
                                    std::istringstream iss(fields.at(onEnemyPlayerLeftPlayerIDInd));
                                    if (! (iss >> tmpOnEnemyPlayerLeftPlayerID))
                                    {
                                        isSkipping = true;
                                        break;
                                    }
                                }

                                if (tmpOnEnemyPlayerLeftPlayerID == tmpEnemyPlayerID)
                                {
                                    tmpIsEnemyPlayerLeft = true;
                                    tmpEnemyPlayerLeftFrameCount = tmpOnEnemyPlayerLeftFrameCount;
                                }
                            }
                            else if (tmpUpdateSignifier == raceScoutedUpdateSignifier)
                            {
                                const int enemyRaceScoutedFrameCountInd = tmpUpdateSignifierInd + 1;
                                // Block to restrict scope of variables.
                                {
                                    std::istringstream iss(fields.at(enemyRaceScoutedFrameCountInd));
                                    if (! (iss >> tmpEnemyRaceScoutedFrameCount))
                                    {
                                        isSkipping = true;
                                        break;
                                    }
                                }

                                const int enemyRaceScoutedInd = tmpUpdateSignifierInd + 6;
                                tmpEnemyRaceScouted = fields.at(enemyRaceScoutedInd);
                            }
                        }

                        if (isSkipping)
                        {
                            continue;
                        }

                        if (enemyRaceInit != BWAPI::Races::Unknown)
                        {
                            if (tmpEnemyRaceInit != BWAPI::Races::Unknown.getName())
                            {
                                if (tmpEnemyRaceInit != enemyRaceInit.getName())
                                {
                                    continue;
                                }
                            }
                            else
                            {
                                // Assumption that the enemy race was eventually scouted and was
                                // scouted early enough in the game for there not to be much
                                // difference in how the whole game played out compared with if
                                // the race had been known at the start of the game.
                                if (tmpEnemyRaceScouted != enemyRaceInit.getName())
                                {
                                    continue;
                                }
                            }
                        }

                        if (fields.at(minFieldsExpected) != onEndUpdateSignifier)
                        {
                            // TODO: corrupt or incomplete line detected (e.g. bot killed before onEnd()) or oversized line detected,
                            // so provide an error message?
                            continue;
                        }

                        minFieldsExpected += 9;
                        if (fields.size() < minFieldsExpected)
                        {
                            // TODO: corrupt or incomplete line detected (e.g. bot killed before onEnd()), so provide an error message?
                            continue;
                        }

                        if (fields.size() > minFieldsExpected)
                        {
                            // TODO: corrupt or oversized line detected, so provide an error message?
                            continue;
                        }

                        const int onEndFrameCountInd = minFieldsExpected - 8;
                        int tmpOnEndFrameCount = -1;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(onEndFrameCountInd));
                            if (! (iss >> tmpOnEndFrameCount))
                            {
                                continue;
                            }
                        }

                        const int isWinnerInd = minFieldsExpected - 3;
                        bool tmpIsWinner = false;
                        // Block to restrict scope of variables.
                        {
                            std::istringstream iss(fields.at(isWinnerInd));
                            if (! (iss >> tmpIsWinner))
                            {
                                continue;
                            }
                        }

                        // Add the data to lookup maps for learning purposes.
                        auto& enemyRaceInitMap = learningMap.enemyRaceInitMap;
                        auto& enemyRaceScoutedMap = enemyRaceInitMap.val[tmpEnemyRaceInit];
                        auto& numStartLocationsMap = enemyRaceScoutedMap.val[tmpEnemyRaceScouted];
                        auto& mapHashMap = numStartLocationsMap.val[tmpNumStartLocations];
                        auto& myStartLocationMap = mapHashMap.val[tmpMapHash];
                        auto& enemyStartLocDeducedMap = myStartLocationMap.val[BWAPI::TilePosition(tmpMyStartLocX, tmpMyStartLocY)];
                        auto& outcomeMap = enemyStartLocDeducedMap.val[BWAPI::TilePosition(tmpEnemyStartLocDeducedX, tmpEnemyStartLocDeducedY)];
                        auto& timerAtGameStartMap = outcomeMap.val[tmpIsWinner];
                        auto& gameIDSet = timerAtGameStartMap.val[tmpTimerAtGameStart];
                        gameIDSet.val.insert(tmpGameID);
                        gameIDToOnEndFrameCount.emplace(tmpGameID, tmpOnEndFrameCount);
                        gameIDToStratSettings.emplace(tmpGameID, tmpStratSettings);
                        ++learningMap.numOutcomes[tmpIsWinner];
                        ++enemyRaceInitMap.numOutcomes[tmpIsWinner][tmpStratSettings];
                        ++enemyRaceScoutedMap.numOutcomes[tmpIsWinner][tmpStratSettings];
                        ++numStartLocationsMap.numOutcomes[tmpIsWinner][tmpStratSettings];
                        ++mapHashMap.numOutcomes[tmpIsWinner][tmpStratSettings];
                        ++myStartLocationMap.numOutcomes[tmpIsWinner][tmpStratSettings];
                        ++enemyStartLocDeducedMap.numOutcomes[tmpIsWinner][tmpStratSettings];
                        ++outcomeMap.numOutcomes[tmpIsWinner][tmpStratSettings];

                        if ((outcomeMap.gameIDIfWonLastGame >= 0 && tmpTimerAtGameStart >= outcomeMap.val[true].val.rbegin()->first) ||
                            (outcomeMap.gameIDIfLostLastGame >= 0 && tmpTimerAtGameStart >= outcomeMap.val[false].val.rbegin()->first) ||
                            (outcomeMap.gameIDIfWonLastGame < 0 && outcomeMap.gameIDIfLostLastGame < 0))
                        {
                            if (tmpIsWinner)
                            {
                                outcomeMap.gameIDIfWonLastGame = tmpGameID;
                                enemyStartLocDeducedMap.gameIDIfWonLastGame = tmpGameID;
                                myStartLocationMap.gameIDIfWonLastGame = tmpGameID;
                                mapHashMap.gameIDIfWonLastGame = tmpGameID;
                                numStartLocationsMap.gameIDIfWonLastGame = tmpGameID;
                                enemyRaceScoutedMap.gameIDIfWonLastGame = tmpGameID;
                                enemyRaceInitMap.gameIDIfWonLastGame = tmpGameID;
                                learningMap.gameIDIfWonLastGame = tmpGameID;
                                outcomeMap.gameIDIfLostLastGame = -1;
                                enemyStartLocDeducedMap.gameIDIfLostLastGame = -1;
                                myStartLocationMap.gameIDIfLostLastGame = -1;
                                mapHashMap.gameIDIfLostLastGame = -1;
                                numStartLocationsMap.gameIDIfLostLastGame = -1;
                                enemyRaceScoutedMap.gameIDIfLostLastGame = -1;
                                enemyRaceInitMap.gameIDIfLostLastGame = -1;
                                learningMap.gameIDIfLostLastGame = -1;
                            }
                            else
                            {
                                outcomeMap.gameIDIfLostLastGame = tmpGameID;
                                enemyStartLocDeducedMap.gameIDIfLostLastGame = tmpGameID;
                                myStartLocationMap.gameIDIfLostLastGame = tmpGameID;
                                mapHashMap.gameIDIfLostLastGame = tmpGameID;
                                numStartLocationsMap.gameIDIfLostLastGame = tmpGameID;
                                enemyRaceScoutedMap.gameIDIfLostLastGame = tmpGameID;
                                enemyRaceInitMap.gameIDIfLostLastGame = tmpGameID;
                                learningMap.gameIDIfLostLastGame = tmpGameID;
                                outcomeMap.gameIDIfWonLastGame = -1;
                                enemyStartLocDeducedMap.gameIDIfWonLastGame = -1;
                                myStartLocationMap.gameIDIfWonLastGame = -1;
                                mapHashMap.gameIDIfWonLastGame = -1;
                                numStartLocationsMap.gameIDIfWonLastGame = -1;
                                enemyRaceScoutedMap.gameIDIfWonLastGame = -1;
                                enemyRaceInitMap.gameIDIfWonLastGame = -1;
                                learningMap.gameIDIfWonLastGame = -1;
                            }
                        }
                    }
                }
            }

            int mostSpecificLostGameID = -1;
            // Return value is whether it ends up updating the strategy settings.
            auto updateStratSettingsLambda =
                [this, &mostSpecificLostGameID](auto& mapWithNumOutcomesMap) -> bool
                {
                    if (mapWithNumOutcomesMap.gameIDIfWonLastGame >= 0)
                    {
                        ss = gameIDToStratSettings[mapWithNumOutcomesMap.gameIDIfWonLastGame];
                        return true;
                    }
                    else
                    {
                        if (mostSpecificLostGameID < 0 && mapWithNumOutcomesMap.gameIDIfLostLastGame >= 0)
                        {
                            mostSpecificLostGameID = mapWithNumOutcomesMap.gameIDIfLostLastGame;
                        }

                        // If we have won with any other strategies in this scenario, pick a strategy from
                        // amongst the other strategies we have won with in this scenario, but pick randomly,
                        // weighted by each strategy's individual win ratio in this scenario.
                        std::map<StratSettings, double> ssToTotWinRatiosSoFarMap;
                        double totWinRatios = 0.0;
                        double expectation = 0.0;
                        for (auto& ssToWinsIter : mapWithNumOutcomesMap.numOutcomes[true])
                        {
                            const StratSettings ssTmp = ssToWinsIter.first;
                            if (mostSpecificLostGameID >= 0 && gameIDToStratSettings[mostSpecificLostGameID] == ssTmp)
                            {
                                continue;
                            }

                            const double numWins = ssToWinsIter.second;
                            if (numWins == 0.0)
                            {
                                continue;
                            }
        
                            auto& ssToLossesIter = mapWithNumOutcomesMap.numOutcomes[false].find(ssTmp);
                            const double numOutcomes =
                                numWins +
                                (ssToLossesIter == mapWithNumOutcomesMap.numOutcomes[false].end() ? 0.0 : ssToLossesIter->second);
                            // Just for safety (shouldn't happen).
                            if (numOutcomes == 0.0)
                            {
                                continue;
                            }
                            const double winRatio = numWins / numOutcomes;
                            expectation = ((expectation * totWinRatios) + (winRatio * winRatio)) / (totWinRatios + winRatio);
                            totWinRatios += winRatio;
                            ssToTotWinRatiosSoFarMap[ssTmp] = totWinRatios;
                        }

                        // Treat expectation of 75% win ratio or more as good enough.
                        if (expectation >= 0.75)
                        {
                            const double randDouble = ((double)rand() / RAND_MAX) * totWinRatios;
                            for (auto& ssToTotWinRatiosSoFarIter : ssToTotWinRatiosSoFarMap)
                            {
                                if (randDouble <= ssToTotWinRatiosSoFarIter.second)
                                {
                                    ss = ssToTotWinRatiosSoFarIter.first;
                                    return true;
                                }
                            }
                        }
                    }

                    return false;
                };

            // Choose strategy settings based on the learning data lookup maps.
            // For the most specific scenario that matches the current scenario and we have
            // played a game for, if we won the last game we played in that scenario, use the
            // same strategy settings as we used then.
            bool isUpdatedStratSettings = false;
            auto& enemyRaceInitMap = learningMap.enemyRaceInitMap;
            auto& enemyRaceInitIter = enemyRaceInitMap.val.find(enemyRaceInit.getName());
            if (enemyRaceInitIter != learningMap.enemyRaceInitMap.val.end())
            {
                auto& enemyRaceScoutedMap = enemyRaceInitIter->second;
                auto& enemyRaceScoutedIter = enemyRaceScoutedMap.val.find(enemyRaceInit.getName());
                if (enemyRaceScoutedIter != enemyRaceScoutedMap.val.end())
                {
                    auto& numStartLocationsMap = enemyRaceScoutedIter->second;
                    auto& numStartLocationsIter = numStartLocationsMap.val.find(Broodwar->getStartLocations().size());
                    if (numStartLocationsIter != numStartLocationsMap.val.end())
                    {
                        auto& mapHashMap = numStartLocationsIter->second;
                        auto& mapHashIter = mapHashMap.val.find(Broodwar->mapHash());
                        if (mapHashIter != mapHashMap.val.end())
                        {
                            auto& myStartLocationMap = mapHashIter->second;
                            auto& myStartLocIter = myStartLocationMap.val.find(myStartLoc);
                            if (myStartLocIter != myStartLocationMap.val.end())
                            {
                                auto& enemyStartLocDeducedMap = myStartLocIter->second;
                                auto& enemyStartLocDeducedIter = enemyStartLocDeducedMap.val.find(enemyStartLocDeduced);
                                if (enemyStartLocDeducedIter != enemyStartLocDeducedMap.val.end())
                                {
                                    auto& outcomeMap = enemyStartLocDeducedIter->second;
                                    isUpdatedStratSettings = updateStratSettingsLambda(outcomeMap);
                                }

                                if (!isUpdatedStratSettings)
                                {
                                    isUpdatedStratSettings = updateStratSettingsLambda(enemyStartLocDeducedMap);
                                }
                            }

                            if (!isUpdatedStratSettings)
                            {
                                isUpdatedStratSettings = updateStratSettingsLambda(myStartLocationMap);
                            }
                        }

                        if (!isUpdatedStratSettings)
                        {
                            isUpdatedStratSettings = updateStratSettingsLambda(mapHashMap);
                        }
                    }

                    if (!isUpdatedStratSettings)
                    {
                        isUpdatedStratSettings = updateStratSettingsLambda(numStartLocationsMap);
                    }
                }

                if (!isUpdatedStratSettings)
                {
                    isUpdatedStratSettings = updateStratSettingsLambda(enemyRaceScoutedMap);
                }
            }

            if (!isUpdatedStratSettings)
            {
                isUpdatedStratSettings = updateStratSettingsLambda(enemyRaceInitMap);
            }

            if (!isUpdatedStratSettings)
            {
                // If have lost at least 5 games and win ratio is less than 80%...
                if (learningMap.numOutcomes[false] >= 5 &&
                    learningMap.numOutcomes[false] * 4 > learningMap.numOutcomes[true])
                {
                    // I.E. is4PoolBO vs isSpeedlingBO vs isHydraRushBO vs neither.
                    const int randBONum = rand() % 4;

                    ss.is4PoolBO = (randBONum == 0);
                    ss.isSpeedlingBO = (randBONum == 1);
                    ss.isHydraRushBO = (randBONum == 2);
                    ss.isMutaRushBODecidedAfterScoutEnemyRace =
                        (isMapPlasma_v_1_0 &&
                         enemyRaceInit == BWAPI::Races::Unknown &&
                         (rand() % 100 < 20));
                    ss.isMutaRushBO = (rand() % 100 < 85);
                    ss.isMutaRushBOVsProtoss = true;
                    ss.isMutaRushBOVsTerran = true;
                    ss.isMutaRushBOVsZerg = true;
                    if (ss.isMutaRushBODecidedAfterScoutEnemyRace)
                    {
                        while (ss.isMutaRushBOVsProtoss == ss.isMutaRushBOVsTerran &&
                               ss.isMutaRushBOVsTerran == ss.isMutaRushBOVsZerg)
                        {
                            ss.isMutaRushBOVsProtoss = (rand() % 100 < 85);
                            ss.isMutaRushBOVsTerran = (rand() % 100 < 85);
                            ss.isMutaRushBOVsZerg = (rand() % 100 < 85);
                        }
                    }

                    ss.isSpeedlingPushDeferred = (randBONum == 1 && (rand() % 100 < 25));
                    ss.isEnemyWorkerRusher = (rand() % 100 < 15);

                    const int randNumSunkensIn = rand() % 100;
                    ss.numSunkens = 0;
                    if (randNumSunkensIn < 20)
                    {
                        ss.numSunkens = 0;
                    }
                    else if (randNumSunkensIn < 50)
                    {
                        ss.numSunkens = 1;
                    }
                    else if (randNumSunkensIn < 65)
                    {
                        ss.numSunkens = 2;
                    }
                    else if (randNumSunkensIn < 75)
                    {
                        ss.numSunkens = 3;
                    }
                    else if (randNumSunkensIn < 80)
                    {
                        ss.numSunkens = 5;
                    }
                    else if (randNumSunkensIn < 85)
                    {
                        ss.numSunkens = 8;
                    }
                    else if (randNumSunkensIn < 90)
                    {
                        ss.numSunkens = 11;
                    }
                    else if (randNumSunkensIn < 95)
                    {
                        ss.numSunkens = 15;
                    }
                    else
                    {
                        ss.numSunkens = 19;
                    }
    
                    ss.isNumSunkensDecidedAfterScoutEnemyRace =
                        (enemyRaceInit == BWAPI::Races::Unknown &&
                         (rand() % 100 < 20));

                    ss.numSunkensVsProtoss = 1;
                    ss.numSunkensVsTerran = 1;
                    ss.numSunkensVsZerg = 1;
                    if (ss.isNumSunkensDecidedAfterScoutEnemyRace)
                    {
                        while (ss.numSunkensVsProtoss == ss.numSunkensVsTerran &&
                               ss.numSunkensVsTerran == ss.numSunkensVsZerg)
                        {
                            ss.numSunkensVsProtoss = (ss.numSunkens + ((rand() % 8) - 4));
                            ss.numSunkensVsTerran = (ss.numSunkens + ((rand() % 8) - 4));
                            ss.numSunkensVsZerg = (ss.numSunkens + ((rand() % 8) - 4));

                            ss.numSunkensVsProtoss *= (ss.numSunkensVsProtoss < 0 ? -1 : 1);
                            ss.numSunkensVsTerran *= (ss.numSunkensVsTerran < 0 ? -1 : 1);
                            ss.numSunkensVsZerg *= (ss.numSunkensVsZerg < 0 ? -1 : 1);
                        }
                    }
                }
            }
    
            // Append some info to a file for the enemy in the write folder.
            // Block to restrict scope of variables.
            {
                std::ostringstream oss;
    
                oss << startOfLineSentinel << delim;
                oss << endOfLineSentinel << delim;
    
                oss << startOfUpdateSentinel << delim;
                oss << endOfUpdateSentinel << delim;

                // Update type.
                oss << initUpdateSignifier << delim;

                oss << dataFileExtension << delim;
                oss << pathFieldDelimiter << delim;
                oss << myVersionMajor << delim;
                oss << myVersionMinor << delim;
                oss << myVersionUpdate << delim;
                oss << myVersionPatch << delim;
                oss << myVersionBuildNum << delim;
                oss << versionNumPrefix << delim;
                oss << versionFieldDelimiter << delim;
                oss << myVersionStr << delim;
                oss << __DATE__ << delim;
                oss << __TIME__ << delim;
                oss << Broodwar->getClientVersion() << delim;
                oss << Broodwar->isDebug() << delim;
                oss << myBotNameHardCoded << delim;
                oss << Broodwar->getPlayers().size() << delim;
                oss << Broodwar->enemies().size() << delim;
                oss << Broodwar->allies().size() << delim;
                oss << Broodwar->observers().size() << delim;
                oss << static_cast<int>(Broodwar->self()->getID()) << delim;
                oss << Broodwar->self()->getType() << delim;
                // TODO: player names may contain unusual characters, so sanitize by stripping or replacing them
                // (esp. tabs because I use tab as a delimiter).
                oss << Broodwar->self()->getName() << delim;
                // Race picked in the game lobby.
                // Currently, BWAPI doesn't provide a way to know whether Random or a specific race was picked
                // (see https://github.com/bwapi/bwapi/issues/679).
                oss << Broodwar->self()->getRace() << delim;
                // Race rolled if Random race was picked, otherwise it is the particular race that was picked.
                oss << Broodwar->self()->getRace() << delim;
                oss << myStartLoc.x << delim;
                oss << myStartLoc.y << delim;
                oss << versusSignifierStr << delim;
                oss << enemyPlayerID << delim;
                oss << enemyPlayerType << delim;
                // TODO: player names may contain unusual characters, so sanitize by stripping or replacing them
                // (esp. tabs because I use tab as a delimiter).
                oss << enemyName << delim;
                // Race picked in the game lobby.
                // Currently, BWAPI doesn't provide a way to know whether Random or a specific race was picked
                // if enemy units are visible at the start of the game, e.g. if Flag::CompleteMapInformation is
                // enabled (see https://github.com/bwapi/bwapi/issues/679).
                oss << enemyRaceInit << delim;
                // Race rolled if Random race was picked, otherwise it is the particular race that was picked
                // (if known, e.g. if CompleteMapInfo is enabled or their units are close enough for our units
                // to see at the start of the game).
                oss << enemyRaceInit << delim;
                // According to the BWAPI source code, it is the special coordinates used to represent Unknown,
                // unless the CompleteMapInformation flag is enabled.
                // Note that if there are exactly two start locations on this map then it is possible to deduce
                // where the enemy start location is.
                oss << enemyStartLocDeduced.x << delim;
                oss << enemyStartLocDeduced.y << delim;
                oss << isEnemyWriteFileMissingOrUnreadable << delim;
                // This may indicate that some game data had been detected as potentially lost/corrupted.
                // Theoretically this shouldn't have been caused by a process being killed part-way through
                // copying a file from the AI or read folder to the write folder (because intermediary files
                // are used to avoid this problem). Perhaps the user or tournament management software did it.
                oss << isTooSmallFileDetected << delim;
                oss << isFileCopyNeeded << delim;
                oss << isFileCopyFailed << delim;
                // TODO: map names may contain unusual characters, so sanitize by stripping or replacing them
                // (esp. tabs because I use tab as a delimiter).
                oss << Broodwar->mapName() << delim;
                // TODO: map names could theoretically contain unusual characters, so sanitize by stripping or
                // replacing them (esp. tabs because I use tab as a delimiter).
                oss << Broodwar->mapFileName() << delim;
                oss << Broodwar->mapHash() << delim;
                oss << Broodwar->mapWidth() << delim;
                oss << Broodwar->mapHeight() << delim;
                //oss << Broodwar->isFlagEnabled(BWAPI::Flag::CompleteMapInformation) << delim;
                //oss << Broodwar->isFlagEnabled(BWAPI::Flag::UserInput) << delim;
                oss << Broodwar->getStartLocations().size() << delim;
                // May be useful for scripts that do lookups based on possible start locations.
                for (const BWAPI::TilePosition loc : Broodwar->getStartLocations())
                {
                    oss << loc.x << delim;
                    oss << loc.y << delim;
                }

                oss << Broodwar->getInstanceNumber() << delim;
                oss << Broodwar->getRandomSeed() << delim;
                oss << Broodwar->isMultiplayer() << delim;
                oss << Broodwar->isBattleNet() << delim;
                oss << Broodwar->isReplay() << delim;
                oss << Broodwar->getGameType() << delim;
                //oss << Broodwar->getLatency() << delim;
                oss << Broodwar->getLatencyFrames() << delim;
                oss << Broodwar->getLatencyTime() << delim;
                oss << Broodwar->isLatComEnabled() << delim;

                // Note: I would also record command optimization level, local speed, frame skip
                // but unfortunately there doesn't seem to be a way to query for them except to test whether
                // the tournament module lets you set them (but I don't want to set them).
    
                oss << Broodwar->isGUIEnabled() << delim;

                // TODO: _dupenv_s is not portable.
                char *pValue;
                size_t len;
                errno_t err = _dupenv_s(&pValue, &len, "NUMBER_OF_PROCESSORS");
                if (err == 0)
                {
                    oss << pValue;
                }
                oss << delim;
                free(pValue);
                err = _dupenv_s(&pValue, &len, "PROCESSOR_ARCHITECTURE");
                if (err == 0)
                {
                    oss << pValue;
                }
                oss << delim;
                free(pValue);
                err = _dupenv_s(&pValue, &len, "PROCESSOR_IDENTIFIER");
                if (err == 0)
                {
                    oss << pValue;
                }
                oss << delim;
                free(pValue);
    
                oss << Broodwar->getFrameCount() << delim;
                oss << Broodwar->elapsedTime() << delim;

                // Get time now.
                // TODO: localtime_s is not portable.
                oss << timerAtGameStart << delim;
                // Seconds since frame 0.
                oss << "0" << delim;
                struct tm buf;
                errno_t errNo = localtime_s(&buf, &timerAtGameStart);
                if (errNo == 0)
                {
                    oss << std::put_time(&buf, "%Y-%m-%d");
                }
                oss << delim;
                if (errNo == 0)
                {
                    oss << std::put_time(&buf, "%H:%M:%S");
                }
                oss << delim;
                if (errNo == 0)
                {
                    oss << std::put_time(&buf, "%z");
                }
                oss << delim;
                if (errNo == 0)
                {
                    oss << std::put_time(&buf, "%Z");
                }
                oss << delim;

                oss << ss.is4PoolBO << delim;
                oss << ss.isSpeedlingBO << delim;
                oss << ss.isHydraRushBO << delim;
                oss << ss.isMutaRushBODecidedAfterScoutEnemyRace << delim;
                oss << ss.isMutaRushBO << delim;
                oss << ss.isMutaRushBOVsProtoss << delim;
                oss << ss.isMutaRushBOVsTerran << delim;
                oss << ss.isMutaRushBOVsZerg << delim;
                oss << ss.isSpeedlingPushDeferred << delim;
                oss << ss.isEnemyWorkerRusher << delim;
                oss << ss.isNumSunkensDecidedAfterScoutEnemyRace << delim;
                oss << ss.numSunkens << delim;
                oss << ss.numSunkensVsProtoss << delim;
                oss << ss.numSunkensVsTerran << delim;
                oss << ss.numSunkensVsZerg << delim;

                oss << endOfUpdateSentinel << delim;
    
                // Block to restrict scope of variables.
                {
                    // Append to the file for the enemy in the write folder.
                    std::ofstream enemyWriteFileOFS(enemyWriteFilePath, std::ios_base::out | std::ios_base::app);
                    if (enemyWriteFileOFS)
                    {
                        enemyWriteFileOFS.seekp(0, std::ios_base::end);
                        // Prepend a newline (unless the file is empty).
                        if (!enemyWriteFileOFS || (long long)(enemyWriteFileOFS.tellp()) != 0)
                        {
                            enemyWriteFileOFS << "\n";
                        }
        
                        enemyWriteFileOFS << oss.str();
                        enemyWriteFileOFS.flush();
                    }
                }
            }
        }

        /*
        // Feature for easily testing variant(s) of my my bot simply by renaming my player name.
        // 4pool into whatever -> speedling into whatever
        // speedling into whatever -> 4pool into whatever
        // neither into whatever -> speedling into whatever
        // For sanity, only 1 sunken in 4pool.
        if (Broodwar->self()->getName().compare(0, std::string("ZZZKBot").size(), "ZZZKBot") != 0 && Broodwar->self()->getName() != "Chris Coxe")
        {
            if (ss.isSpeedlingBO && ss.numSunkens > 1)
            {
                ss.numSunkens = 1;
            }
    
            if (ss.is4PoolBO || ss.isSpeedlingBO)
            {
                ss.is4PoolBO = !ss.is4PoolBO;
                ss.isSpeedlingBO = !ss.isSpeedlingBO;
            }
            else
            {
                ss.isSpeedlingBO = true;
            }
        }
        */
    }

    if (isMapPlasma_v_1_0 && ss.isMutaRushBODecidedAfterScoutEnemyRace && !isARemainingEnemyRandomRace)
    {
        if (isARemainingEnemyProtoss)
        {
            ss.isMutaRushBO = ss.isMutaRushBOVsProtoss;
        }
        else if (isARemainingEnemyTerran)
        {
            ss.isMutaRushBO = ss.isMutaRushBOVsTerran;
        }
        else if (isARemainingEnemyZerg)
        {
            ss.isMutaRushBO = ss.isMutaRushBOVsZerg;
        }
    }

    if (!isMapPlasma_v_1_0 && ss.isNumSunkensDecidedAfterScoutEnemyRace && !isARemainingEnemyRandomRace)
    {
        if (isARemainingEnemyProtoss)
        {
            ss.numSunkens = ss.numSunkensVsProtoss;
        }
        else if (isARemainingEnemyTerran)
        {
            ss.numSunkens = ss.numSunkensVsTerran;
        }
        else if (isARemainingEnemyZerg)
        {
            ss.numSunkens = ss.numSunkensVsZerg;
        }
    }

    // When Unknown enemy race becomes known, append some info to a file for the enemy in the write folder.
    if (enemyPlayerID >= 0 && enemyRaceInit == BWAPI::Races::Unknown)
    {
        const BWAPI::Player enemyPlayer = Broodwar->getPlayer(static_cast<PlayerID>(enemyPlayerID));
        if (enemyPlayer)
        {
            const BWAPI::Race enemyRaceScoutedNew = enemyPlayer->getRace();
            if (enemyRaceScoutedNew != enemyRaceScouted)
            {
                enemyRaceScouted = enemyRaceScoutedNew;

                std::ostringstream oss;
        
                oss << startOfUpdateSentinel << delim;

                // Update type.
                oss << raceScoutedUpdateSignifier << delim;
        
                oss << Broodwar->getFrameCount() << delim;
                oss << Broodwar->elapsedTime() << delim;
        
                // Get time now.
                // Note: localtime_s is not portable but w/e.
                std::time_t timer = std::time(nullptr);
                oss << (timer - timerAtGameStart) << delim;
                struct tm buf;
                errno_t errNo = localtime_s(&buf, &timer);
                if (errNo == 0)
                {
                    oss << std::put_time(&buf, "%Y-%m-%d");
                }
                oss << delim;
                if (errNo == 0)
                {
                    oss << std::put_time(&buf, "%H:%M:%S");
                }
                oss << delim;
        
                // Race rolled if Random race was picked, otherwise it is the particular race that was picked
                // (if known, e.g. if CompleteMapInfo is enabled or their units are close enough for our units
                // to see).
                oss << enemyRaceScouted << delim;
        
                oss << endOfUpdateSentinel << delim;
        
                // Block to restrict scope of variables.
                {
                    // Append to the file for the enemy in the write folder.
                    std::ofstream enemyWriteFileOFS(enemyWriteFilePath, std::ios_base::out | std::ios_base::app);
                    if (enemyWriteFileOFS)
                    {
                        enemyWriteFileOFS << oss.str();
                        enemyWriteFileOFS.flush();
                    }
                }
            }
        }
    }

    const int transitionOutOf4PoolFrameCountThresh = (ss.isSpeedlingBO || ss.isHydraRushBO || !ss.is4PoolBO) ? 0 : (8 * 60 * 24);
    const int frameCount = Broodwar->getFrameCount();
    // We ignore stolen gas, at least until a time near when we plan to make an extractor.
    auto isNotStolenGas =
        [&mainBaseAuto, &transitionOutOf4PoolFrameCountThresh, &frameCount](const Unit& tmpUnit)
        {
            return
                !tmpUnit->getType().isRefinery() ||
                mainBaseAuto == nullptr ||
                frameCount + (60 * 24) >= transitionOutOf4PoolFrameCountThresh ||
                mainBaseAuto->getDistance(tmpUnit) > 256;
        };

    static std::set<BWAPI::Position> lastKnownEnemyUnliftedBuildingsAnywherePosSet;
    // Block to restrict scope of variables.
    {
        std::set<BWAPI::Position> vacantPosSet;
        for (const BWAPI::Position pos : lastKnownEnemyUnliftedBuildingsAnywherePosSet)
        {
            if (Broodwar->isVisible(TilePosition(pos)) &&
                Broodwar->getUnitsOnTile(
                    TilePosition(pos),
                    IsEnemy && IsVisible && Exists && IsBuilding && !IsLifted &&
                    isNotStolenGas).empty())
            {
                vacantPosSet.insert(pos);
            }
        }

        for (const BWAPI::Position pos : vacantPosSet)
        {
            lastKnownEnemyUnliftedBuildingsAnywherePosSet.erase(pos);
        }
    }

    // TODO: add separate logic for enemy overlords (because e.g. on maps with 3 or more start locations
    // the first enemy overlord I see is not necessarily from the start position
    // nearest it).
    static BWAPI::Position firstEnemyNonWorkerSeenPos = BWAPI::Positions::Unknown;
    static BWAPI::Position closestEnemySeenPos = BWAPI::Positions::Unknown;
    static BWAPI::Position furthestEnemySeenPos = BWAPI::Positions::Unknown;
    static bool isClosestEnemySeenAnOverlord = false;

    const Unitset& allUnits = Broodwar->getAllUnits();
    for (auto& u : allUnits)
    {
        if (u->exists() && u->isVisible() && u->getPlayer() && u->getPlayer()->isEnemy(Broodwar->self()))
        {
            if (u->getType().isBuilding() && !u->isLifted() && isNotStolenGas(u))
            {
                lastKnownEnemyUnliftedBuildingsAnywherePosSet.insert(u->getPosition());
            }

            if (myStartRoughPos != BWAPI::Positions::Unknown &&
                firstEnemyNonWorkerSeenPos == BWAPI::Positions::Unknown &&
                !u->getType().isWorker() /*&&
                u->getType() != BWAPI::UnitTypes::Zerg_Overlord*/)
            {
                if (closestEnemySeenPos == BWAPI::Positions::Unknown || myStartRoughPos.getDistance(u->getPosition()) < myStartRoughPos.getDistance(closestEnemySeenPos))
                {
                    closestEnemySeenPos = u->getPosition();
                    isClosestEnemySeenAnOverlord = u->getType() == BWAPI::UnitTypes::Zerg_Overlord;
                }

                if (furthestEnemySeenPos == BWAPI::Positions::Unknown || myStartRoughPos.getDistance(u->getPosition()) > myStartRoughPos.getDistance(furthestEnemySeenPos))
                {
                    furthestEnemySeenPos = u->getPosition();
                }
            }
        }
    }

    if (firstEnemyNonWorkerSeenPos == BWAPI::Positions::Unknown && furthestEnemySeenPos != BWAPI::Positions::Unknown)
    {
        firstEnemyNonWorkerSeenPos = furthestEnemySeenPos;
    }

    if (probableEnemyStartLoc == BWAPI::TilePositions::Unknown)
    {
        BWAPI::TilePosition probableEnemyStartLocBasedOnEnemyUnits = BWAPI::TilePositions::Unknown;
        if (furthestEnemySeenPos != BWAPI::Positions::Unknown)
        {
            for (const BWAPI::TilePosition loc : unscoutedOtherStartLocs)
            {
                if (probableEnemyStartLocBasedOnEnemyUnits == BWAPI::TilePositions::Unknown ||
                    furthestEnemySeenPos.getDistance(getRoughPos(loc, BWAPI::UnitTypes::Special_Start_Location)) < furthestEnemySeenPos.getDistance(getRoughPos(probableEnemyStartLocBasedOnEnemyUnits, BWAPI::UnitTypes::Special_Start_Location)))
                {
                    probableEnemyStartLocBasedOnEnemyUnits = loc;
                }
            }
        }
    
        if (closestEnemySeenPos != BWAPI::Positions::Unknown && probableEnemyStartLocBasedOnEnemyUnits != BWAPI::TilePositions::Unknown)
        {
            if (!isClosestEnemySeenAnOverlord &&
                myStartRoughPos != BWAPI::Positions::Unknown &&
                myStartRoughPos.getDistance(closestEnemySeenPos) < getRoughPos(probableEnemyStartLocBasedOnEnemyUnits, BWAPI::UnitTypes::Special_Start_Location).getDistance(closestEnemySeenPos))
            {
                // We send combat units to other starting positions in order of their closeness,
                // and 4pool should get combat units faster than any other build, so on most maps
                // our first 6 lings should see their combat units before it gets closer to us than
                // we are to them if they are at one of the two closest other start positions.
                // If not then don't try to guess where they are. TODO: This logic doesn't always
                // work if the enemy made proxy (e.g. proxy rax/gateway) though.
                probableEnemyStartLocBasedOnEnemyUnits = BWAPI::TilePositions::Unknown;
            }
        }

        if (probableEnemyStartLocBasedOnEnemyUnits != BWAPI::TilePositions::Unknown)
        {
            probableEnemyStartLoc = probableEnemyStartLocBasedOnEnemyUnits;
        }
    }

    if (Broodwar->getLatencyFrames() != 2 && Broodwar->getFrameCount() % 2 != 1)
    {
        return;
    }

    const Unitset& myUnits = Broodwar->self()->getUnits();

    // For some reason supplyUsed() takes a few frames get adjusted after an extractor starts morphing,
    // so count it myself.
    // TODO: check what happens after I start getting gas because the BWAPI doco makes it sound as
    // though the worker currently in the extractor could be missed.
    int supplyUsed = 0;

    // Special logic to find an enemy unit to attack if there are multiple
    // enemy non-building units (or a single SCV) near any of our workers (e.g. an
    // enemy worker rush) and at least one enemy unit is in one of our worker's
    // weapon range, or at least one of our buildings is low health.
    Unitset myCompletedWorkers;
    BWAPI::Unit lowLifeDrone = nullptr;
    static BWAPI::Unit scoutingWorker = nullptr;
    static BWAPI::Unit scoutingZergling = nullptr;
    bool isBuildingLowLife = false;

    // Count units by type myself because Broodwar->self()->allUnitCount() etc does
    // not count the unit(s) within eggs/lurker eggs/cocoons. Notes:
    // Hydras/mutalisks that are morphing into lurkers/guardians/devourers are only
    // counted as the incomplete type they are morphing to (the count for the type
    // they are morphing from is not increased).
    // The counts might not count the worker currently inside the extractor, if any.
    // Eggs, lurker eggs and cocoons have their own count (in addition to counting
    // what they contain).
    std::map<const BWAPI::UnitType, int> allUnitCount;
    std::map<const BWAPI::UnitType, int> incompleteUnitCount;
    std::map<const BWAPI::UnitType, int> completedUnitCount;

    std::map<const BWAPI::TilePosition, int> numNonOverlordUnitsTargetingStartLoc;

    static bool isScoutingUsingWorker = (!ss.isSpeedlingBO && !ss.isEnemyWorkerRusher && ss.is4PoolBO);
    static bool isScoutingUsingZergling = (!isScoutingUsingWorker && !ss.is4PoolBO && ss.isSpeedlingBO);
    static bool isNeedScoutingWorker = isScoutingUsingWorker;
    static bool isNeedToMorphScoutingWorker = isScoutingUsingWorker;
    int spireRemainingBuildTime = 0;

    for (auto& u : myUnits)
    {
        if (!u->exists())
        {
            continue;
        }

        ++allUnitCount[u->getType()];
        if (u->isCompleted())
        {
            ++completedUnitCount[u->getType()];
        }
        else
        {
            ++incompleteUnitCount[u->getType()];
            if (u->getType() == BWAPI::UnitTypes::Zerg_Spire)
            {
                spireRemainingBuildTime = u->getRemainingBuildTime();
            }
        }

        supplyUsed += u->getType().supplyRequired();
        if (u->getType() == BWAPI::UnitTypes::Zerg_Egg || u->getType() == BWAPI::UnitTypes::Zerg_Lurker_Egg || u->getType() == BWAPI::UnitTypes::Zerg_Cocoon)
        {
            const BWAPI::UnitType buildType = u->getBuildType();
            if (buildType != BWAPI::UnitTypes::None &&
                buildType != BWAPI::UnitTypes::Unknown)
            {
                int tmpCount = buildType.isTwoUnitsInOneEgg() ? 2 : 1;
                allUnitCount[buildType] += tmpCount;
                incompleteUnitCount[buildType] += tmpCount;
                supplyUsed += buildType.supplyRequired() * tmpCount;
            }
        }

        if (u->getType().isWorker() && u->isCompleted())
        {
            myCompletedWorkers.insert(u);
            if (isScoutingUsingWorker && isNeedScoutingWorker && !isNeedToMorphScoutingWorker && scoutingWorker == nullptr && unscoutedOtherStartLocs.size() > 1 &&
                u->isIdle())
            {
                scoutingWorker = u;
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Zergling && u->isCompleted())
        {
            if (isScoutingUsingZergling && scoutingZergling == nullptr && unscoutedOtherStartLocs.size() > 1 && probableEnemyStartLoc == BWAPI::TilePositions::Unknown)
            {
                scoutingZergling = u;
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Drone && u->getHitPoints() <= 10)
        {
            // Don't interrupt its attack if it is attacking.
            if (u->getLastCommand().getType() == BWAPI::UnitCommandTypes::None ||
                Broodwar->getFrameCount() >= u->getLastCommandFrame() + 2 - (Broodwar->getLatencyFrames() > 2 ? u->getLastCommandFrame() % 2 : 0) ||
                !(u->getTarget() && u->getTarget()->getPlayer() && u->getTarget()->getPlayer()->isEnemy(Broodwar->self())))
            {
                lowLifeDrone = u;
            }
        }

        if (u->getType().isBuilding() && u->isCompleted() &&
            u->getHitPoints() + u->getShields() < ((u->getType().maxHitPoints() + u->getType().maxShields()) * 3) / 10)
        {
            isBuildingLowLife = true;
        }

        if (u->getType() != BWAPI::UnitTypes::Zerg_Overlord)
        {
            const int tmpX = (int) getClientInfo(u, scoutingTargetStartLocXInd);
            const int tmpY = (int) getClientInfo(u, scoutingTargetStartLocYInd);
            if (tmpX != 0 || tmpY != 0)
            {
                ++numNonOverlordUnitsTargetingStartLoc[TilePosition(tmpX, tmpY)];
            }
        }
    }

    // Stop scouting with worker if we are low on workers.
    if (scoutingWorker != nullptr && myCompletedWorkers.size() < 4)
    {
        isNeedScoutingWorker = false;
        scoutingWorker = nullptr;
    }

    static bool isScoutingWorkerReadyToScout = false;

    /*if (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60)
    {
        supplyUsed = Broodwar->self()->supplyUsed();
    }*/

    // Worker/base defence logic.
    bool workersShouldRetaliate = false;
    bool shouldDefend = false;
    BWAPI::Unit workerAttackTargetUnit = nullptr;
    for (auto& u : myCompletedWorkers)
    {
        const Unitset& attackableEnemyNonBuildingThreatUnits =
            u->getUnitsInRadius(
                224,
                IsEnemy && IsVisible && IsDetected && Exists &&
                CanAttack &&
                !IsBuilding &&
                [&u, &mainBaseAuto](Unit& tmpUnit)
                {
                    return (mainBaseAuto ? mainBaseAuto->getDistance(tmpUnit) < 224 : true) && u->canAttack(tmpUnit);
                });

        if (isBuildingLowLife ||
            attackableEnemyNonBuildingThreatUnits.size() >= 2 ||
            (myCompletedWorkers.size() > 1 &&
             Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Drone) > (isScoutingWorkerReadyToScout ? 1 : 0) &&
             // Note: using allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] rather than incompleteUnitCount[BWAPI::UnitTypes::Zerg_Extractor]
             // because BWAPI seems to think it is completed.
             myCompletedWorkers.size() + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] + allUnitCount[BWAPI::UnitTypes::Zerg_Spawning_Pool] < 4))
        {
            workersShouldRetaliate = true;
            shouldDefend = true;
        }
        else if (!attackableEnemyNonBuildingThreatUnits.empty() &&
                 (*attackableEnemyNonBuildingThreatUnits.begin())->getType() == BWAPI::UnitTypes::Terran_SCV)
        {
            workersShouldRetaliate = true;
        }

        if (workersShouldRetaliate)
        {
            const BWAPI::Unit& tmpEnemyUnit =
                Broodwar->getBestUnit(
                    [&u](const BWAPI::Unit& bestSoFarUnit, const BWAPI::Unit& curUnit)
                    {
                        if (u->isInWeaponRange(curUnit) != u->isInWeaponRange(bestSoFarUnit))
                        {
                            return u->isInWeaponRange(curUnit) ? curUnit : bestSoFarUnit;
                        }

                        return curUnit->getHitPoints() + curUnit->getShields() + curUnit->getType().armor() + curUnit->getDefenseMatrixPoints() < bestSoFarUnit->getHitPoints() + bestSoFarUnit->getShields() + bestSoFarUnit->getType().armor() + bestSoFarUnit->getDefenseMatrixPoints() ? curUnit : bestSoFarUnit;
                    },
                    IsEnemy && IsVisible && IsDetected && Exists &&
                    CanAttack &&
                    !IsBuilding &&
                    [&u, &mainBaseAuto, &shouldDefend](Unit& tmpUnit)
                    {
                        return
                            (shouldDefend ?
                             (mainBaseAuto ? mainBaseAuto->getDistance(tmpUnit) < 224 : true) :
                             u->isInWeaponRange(tmpUnit)) &&
                            u->canAttack(tmpUnit);
                    },
                    u->getPosition(),
                    std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + 224);

            if (tmpEnemyUnit != nullptr)
            {
                if (workerAttackTargetUnit == nullptr ||
                    tmpEnemyUnit->getHitPoints() + tmpEnemyUnit->getShields() + tmpEnemyUnit->getType().armor() + tmpEnemyUnit->getDefenseMatrixPoints() < workerAttackTargetUnit->getHitPoints() + workerAttackTargetUnit->getShields() + workerAttackTargetUnit->getType().armor() + workerAttackTargetUnit->getDefenseMatrixPoints())
                {
                    workerAttackTargetUnit = tmpEnemyUnit;
                }
            }
        }
    }

    if (workerAttackTargetUnit == nullptr && isBuildingLowLife)
    {
        for (auto& u : myUnits)
        {
            if (u->exists() && u->getType().isBuilding() && u->isCompleted() &&
                u->getHitPoints() + u->getShields() < ((u->getType().maxHitPoints() + u->getType().maxShields()) * 3) / 10)
            {
                workerAttackTargetUnit =
                    u->getClosestUnit(
                        IsEnemy && IsVisible && IsDetected && Exists &&
                        CanAttack &&
                        !IsBuilding &&
                        !IsFlying &&
                        !IsInvincible,
                        256);

                if (workerAttackTargetUnit != nullptr)
                {
                    break;
                }
            }
        }
    }

    // Checks whether BWAPI already has a command pending to be executed for the specified unit.
    const auto latencyFrames = Broodwar->getLatencyFrames();
    auto noCmdPending =
        [&frameCount, &latencyFrames](const BWAPI::Unit& tmpUnit)
        {
            return
                (bool)
                (tmpUnit->getLastCommand().getType() == BWAPI::UnitCommandTypes::None ||
                 frameCount >= tmpUnit->getLastCommandFrame() + (latencyFrames > 2 ? latencyFrames - (tmpUnit->getLastCommandFrame() % 2) : latencyFrames));
        };

    // Logic to make a building.
    // TODO: support making buildings concurrently (rather than designing each building's prerequisites to avoid this situation).
    // TODO: support making more than one building of a particular type.
    // Note: geyser is only used when building an extractor.
    static BWAPI::Unit geyser = nullptr;
    auto geyserAuto = geyser;
    auto makeUnit =
        [&mainBaseAuto, &allUnitCount, &getRoughPos, &gathererToResourceMapAuto, &resourceToGathererMapAuto, &lowLifeDrone, &geyserAuto, &noCmdPending, &frameCount, this](
            const BWAPI::UnitType& buildingType,
            BWAPI::Unit& reservedBuilder,
            BWAPI::TilePosition& targetBuildLoc,
            int& frameLastCheckedBuildLoc,
            const int checkBuildLocFreqFrames,
            const bool isNeeded)
        {
            BWAPI::UnitType builderType = buildingType.whatBuilds().first;

            // TODO: support making more than one building of a particular type.
            if ((allUnitCount[buildingType] > (buildingType != BWAPI::UnitTypes::Zerg_Hatchery ? 0 : 1)) ||
                (reservedBuilder &&
                 (!reservedBuilder->exists() ||
                  reservedBuilder->getType() != builderType)))
            {
                reservedBuilder = nullptr;
            }

            BWAPI::Unit oldReservedBuilder = reservedBuilder;
            const int oldUnitCount = allUnitCount[buildingType];
        
            // TODO: support making more than one building of a particular type.
            if (allUnitCount[buildingType] <= (buildingType != BWAPI::UnitTypes::Zerg_Hatchery ? 0 : 1) && isNeeded)
            {
                BWAPI::Unit builder = reservedBuilder;
                reservedBuilder = nullptr;

                if (buildingType == BWAPI::UnitTypes::Zerg_Extractor && lowLifeDrone)
                {
                    builder = lowLifeDrone;
                }

                auto isAvailableToBuild =
                    [&mainBaseAuto, &noCmdPending](Unit& tmpUnit)
                    {
                        return !tmpUnit->isConstructing() && noCmdPending(tmpUnit);
                    };

                if (builder == nullptr && mainBaseAuto)
                {        
                    builder = mainBaseAuto->getClosestUnit(GetType == builderType && IsIdle && !IsCarryingSomething && IsOwned && isAvailableToBuild);
                    if (builder == nullptr)
                        builder = mainBaseAuto->getClosestUnit(GetType == builderType && IsGatheringMinerals && !IsCarryingSomething && IsOwned && isAvailableToBuild);
                    // In case we are being worker rushed, don't necessarily wait for workers to return their
                    // minerals/gas powerup because we should start building the pool asap and the workers are
                    // likely to be almost always fighting.
                    if (buildingType == BWAPI::UnitTypes::Zerg_Spawning_Pool)
                    {
                        if (builder == nullptr)
                            builder = mainBaseAuto->getClosestUnit(GetType == builderType && IsIdle && !IsCarryingGas && IsOwned && isAvailableToBuild);
                        if (builder == nullptr)
                            builder = mainBaseAuto->getClosestUnit(GetType == builderType && IsGatheringMinerals && IsOwned && isAvailableToBuild);
                        if (builder == nullptr)
                            builder = mainBaseAuto->getClosestUnit(GetType == builderType && IsGatheringGas && !IsCarryingGas && IsOwned && isAvailableToBuild);
                        if (builder == nullptr)
                            builder = mainBaseAuto->getClosestUnit(GetType == builderType && IsGatheringGas && IsOwned && isAvailableToBuild);
                    }
                }

                // If a unit was found
                if (builder && (builder != oldReservedBuilder || isAvailableToBuild(builder)))
                {
                    if (targetBuildLoc == BWAPI::TilePositions::None ||
                        targetBuildLoc == BWAPI::TilePositions::Unknown ||
                        targetBuildLoc == BWAPI::TilePositions::Invalid ||
                        frameCount >= frameLastCheckedBuildLoc + checkBuildLocFreqFrames)
                    {
                        if (buildingType == BWAPI::UnitTypes::Zerg_Extractor && mainBaseAuto)
                        {
                            geyserAuto =
                                mainBaseAuto->getClosestUnit(
                                    GetType == BWAPI::UnitTypes::Resource_Vespene_Geyser &&
                                    BWAPI::Filter::Exists,
                                    256);
        
                            if (geyserAuto)
                            {
                                targetBuildLoc = geyserAuto->getTilePosition();
                            }
                        }
                        else
                        {
                            targetBuildLoc = Broodwar->getBuildLocation(buildingType, builder->getTilePosition());
                        }

                        frameLastCheckedBuildLoc = frameCount;
                    }

                    if (targetBuildLoc != BWAPI::TilePositions::None &&
                        targetBuildLoc != BWAPI::TilePositions::Unknown &&
                        targetBuildLoc != BWAPI::TilePositions::Invalid)
                    {
                        if (builder->canBuild(buildingType))
                        {
                            if (builder->canBuild(buildingType, targetBuildLoc) &&
                                (buildingType != BWAPI::UnitTypes::Zerg_Extractor ||
                                 (geyserAuto &&
                                  geyserAuto->exists() &&
                                  geyserAuto->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser)))
                            {
                                if (buildingType == BWAPI::UnitTypes::Zerg_Extractor &&
                                    builder->canRightClick(geyserAuto) &&
                                    builder->getDistance(geyserAuto) > 16)
                                {
                                    builder->rightClick(geyserAuto);
                                }
                                else
                                {
                                    builder->build(buildingType, targetBuildLoc);
                                }

                                reservedBuilder = builder;
                            }
                            else
                            {
                                targetBuildLoc = BWAPI::TilePositions::None;
                                if (buildingType == BWAPI::UnitTypes::Zerg_Extractor)
                                {
                                    geyserAuto = nullptr;
                                }
                            }
                        }
                        // Not enough minerals or it is not available (e.g. UMS game type).
                        else if (buildingType == BWAPI::UnitTypes::Zerg_Spawning_Pool && Broodwar->self()->isUnitAvailable(buildingType))
                        {
                            // Not enough minerals, so send a worker out to the build location so it is on or nearer the
                            // position when we have enough minerals.
                            const Position targetBuildPos = getRoughPos(targetBuildLoc, buildingType);
                            if (builder->canRightClick(targetBuildPos))
                            {
                                builder->rightClick(targetBuildPos);
                                reservedBuilder = builder;
                            }
                            else if (builder->canMove())
                            {
                                builder->move(targetBuildPos);
                                reservedBuilder = builder;
                            }
                        }

                        if (reservedBuilder != nullptr)
                        {
                            if (gathererToResourceMapAuto.find(reservedBuilder) != gathererToResourceMapAuto.end() && resourceToGathererMapAuto.find(gathererToResourceMapAuto.at(reservedBuilder)) != resourceToGathererMapAuto.end() && resourceToGathererMapAuto.at(gathererToResourceMapAuto.at(reservedBuilder)) == reservedBuilder)
                            {
                                resourceToGathererMapAuto.erase(gathererToResourceMapAuto.at(reservedBuilder));
                            }
    
                            gathererToResourceMapAuto.erase(reservedBuilder);
                        }
                    }
                }
            }

            if (oldReservedBuilder != nullptr &&
                // TODO: support making more than one building of a particular type.
                ((oldUnitCount <= (buildingType != BWAPI::UnitTypes::Zerg_Hatchery ? 0 : 1) && !isNeeded) ||
                 (reservedBuilder != nullptr &&
                  reservedBuilder != oldReservedBuilder)) &&
                oldReservedBuilder->getLastCommand().getType() != BWAPI::UnitCommandTypes::None &&
                noCmdPending(oldReservedBuilder) &&
                oldReservedBuilder->canStop())
            {
                oldReservedBuilder->stop();

                if (gathererToResourceMapAuto.find(oldReservedBuilder) != gathererToResourceMapAuto.end() && resourceToGathererMapAuto.find(gathererToResourceMapAuto.at(oldReservedBuilder)) != resourceToGathererMapAuto.end() && resourceToGathererMapAuto.at(gathererToResourceMapAuto.at(oldReservedBuilder)) == oldReservedBuilder)
                {
                    resourceToGathererMapAuto.erase(gathererToResourceMapAuto.at(oldReservedBuilder));
                }

                gathererToResourceMapAuto.erase(oldReservedBuilder);
            }

            // TODO: support making more than one building of a particular type.
            if (oldUnitCount > (buildingType != BWAPI::UnitTypes::Zerg_Hatchery ? 0 : 1) || !isNeeded)
            {
                reservedBuilder = nullptr;
            }
            else if (reservedBuilder == nullptr)
            {
                reservedBuilder = oldReservedBuilder;
            }
        };

    int numWorkersTrainedThisFrame = 0;

    const BWAPI::UnitType groundArmyUnitType =
        !ss.isHydraRushBO ? UnitTypes::Zerg_Zergling : UnitTypes::Zerg_Hydralisk;

    const UnitType groundArmyBuildingType =
        (allUnitCount[UnitTypes::Zerg_Spawning_Pool] == 0 || !ss.isHydraRushBO) ? UnitTypes::Zerg_Spawning_Pool : UnitTypes::Zerg_Hydralisk_Den;

    // We are 4-pool'ing, hence the figure 24 (i.e. start moving a worker to the build location before we have enough minerals).
    // If drone(s) have died then don't move the builder until we have the full amount of minerals required.
    static Unit groundArmyBuildingBuilder = nullptr;
    // Block to restrict scope of variables.
    {
        static BWAPI::TilePosition groundArmyBuildingLoc = BWAPI::TilePositions::None;
        static int frameLastCheckedGroundArmyBuildingLoc = 0;
        const int checkGroundArmyBuildingLocFreqFrames = (10 * 24);
        makeUnit(
            groundArmyBuildingType, groundArmyBuildingBuilder, groundArmyBuildingLoc, frameLastCheckedGroundArmyBuildingLoc, checkGroundArmyBuildingLocFreqFrames,
            // Note: using allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] rather than incompleteUnitCount[BWAPI::UnitTypes::Zerg_Extractor]
            // because BWAPI seems to think it is completed.
            allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] >= 4 &&
            (Broodwar->canMake(groundArmyBuildingType) ||
             (groundArmyBuildingType == BWAPI::UnitTypes::Zerg_Spawning_Pool &&
              Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Drone) == 0 &&
              Broodwar->self()->minerals() >= groundArmyBuildingType.mineralPrice() - 24)));
    }

    bool isStartedTransitioning =
        ((Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60) &&
         (ss.isHydraRushBO ||
          !ss.isSpeedlingBO ||
          (Broodwar->getFrameCount() > (5 * 60 * 24) &&
           Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost) == Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost))));

    // Use the extractor trick whenever possible when supply-blocked, or when a drone is very low life,
    // or morph an extractor for gathering gas if the time is right.
    static Unit extractorBuilder = nullptr;
    // Block to restrict scope of variables.
    {
        static BWAPI::TilePosition extractorLoc = BWAPI::TilePositions::None;
        static int frameLastCheckedExtractorLoc = 0;
        const int checkExtractorLocFreqFrames = (1 * 24);
        makeUnit(
            BWAPI::UnitTypes::Zerg_Extractor, extractorBuilder, extractorLoc, frameLastCheckedExtractorLoc, checkExtractorLocFreqFrames,
            Broodwar->canMake(BWAPI::UnitTypes::Zerg_Extractor) &&
            (((supplyUsed == Broodwar->self()->supplyTotal() || supplyUsed == Broodwar->self()->supplyTotal() - 1) &&
              Broodwar->self()->minerals() >= 84 &&
              !(Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60)) ||
             lowLifeDrone != nullptr ||
             (allUnitCount[BWAPI::UnitTypes::Zerg_Spawning_Pool] > 0 &&
              allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame >= 9 &&
              (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60) &&
              (ss.isSpeedlingBO ||
               ss.isHydraRushBO ||
               (isStartedTransitioning &&
                (Broodwar->self()->minerals() >=
                 BWAPI::UnitTypes::Zerg_Extractor.mineralPrice() +
                 ((allUnitCount[BWAPI::UnitTypes::Zerg_Creep_Colony] + allUnitCount[BWAPI::UnitTypes::Zerg_Sunken_Colony] >= ss.numSunkens) ?
                  0 :
                  (((ss.numSunkens - allUnitCount[BWAPI::UnitTypes::Zerg_Creep_Colony]) * BWAPI::UnitTypes::Zerg_Creep_Colony.mineralPrice()) +
                   ((ss.numSunkens - allUnitCount[BWAPI::UnitTypes::Zerg_Sunken_Colony]) * BWAPI::UnitTypes::Zerg_Sunken_Colony.mineralPrice())))))))));
    }

    // Morph creep colony/ies late-game.
    // Block to restrict scope of variables.
    {
        static Unit creepColonyBuilder = nullptr;
        static BWAPI::TilePosition creepColonyLoc = BWAPI::TilePositions::None;
        static int frameLastCheckedCreepColonyLoc = 0;
        const int checkCreepColonyLocFreqFrames = (10 * 24);
        makeUnit(
            BWAPI::UnitTypes::Zerg_Creep_Colony, creepColonyBuilder, creepColonyLoc, frameLastCheckedCreepColonyLoc, checkCreepColonyLocFreqFrames,
            Broodwar->canMake(BWAPI::UnitTypes::Zerg_Creep_Colony) &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Spawning_Pool] > 0 &&
            ss.numSunkens > 0 &&
            incompleteUnitCount[BWAPI::UnitTypes::Zerg_Creep_Colony] == 0 &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Creep_Colony] + allUnitCount[BWAPI::UnitTypes::Zerg_Sunken_Colony] < ss.numSunkens &&
            (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60 || (ss.is4PoolBO && myCompletedWorkers.size() >= 3 && (Broodwar->self()->deadUnitCount(groundArmyUnitType) > 14 || Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Drone) >= 2))) &&
            ((ss.isEnemyWorkerRusher && myCompletedWorkers.size() >= 4) ||
             (((ss.isSpeedlingBO || ss.isHydraRushBO) ? (myCompletedWorkers.size() >= 6 && (Broodwar->self()->deadUnitCount(groundArmyUnitType) > (ss.isSpeedlingBO ? 10 : 2) || Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Drone) >= 2)) : (myCompletedWorkers.size() >= 3 && (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60 || (Broodwar->self()->deadUnitCount(groundArmyUnitType) > 14 || Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Drone) >= 2) && allUnitCount[BWAPI::UnitTypes::Zerg_Creep_Colony] + allUnitCount[BWAPI::UnitTypes::Zerg_Sunken_Colony] < 1))) ||
              allUnitCount[BWAPI::UnitTypes::Zerg_Lair] + allUnitCount[BWAPI::UnitTypes::Zerg_Hive] > 0)));
    }

    // Morph another hatchery late-game.
    // Block to restrict scope of variables.
    {
        static Unit hatcheryBuilder = nullptr;
        static BWAPI::TilePosition hatcheryLoc = BWAPI::TilePositions::None;
        static int frameLastCheckedHatcheryLoc = 0;
        const int checkHatcheryLocFreqFrames = (10 * 24);
        makeUnit(
            BWAPI::UnitTypes::Zerg_Hatchery, hatcheryBuilder, hatcheryLoc, frameLastCheckedHatcheryLoc, checkHatcheryLocFreqFrames,
            Broodwar->canMake(BWAPI::UnitTypes::Zerg_Hatchery) &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Hatchery] + allUnitCount[BWAPI::UnitTypes::Zerg_Lair] + allUnitCount[BWAPI::UnitTypes::Zerg_Hive] <= 1 &&
            isStartedTransitioning &&
            (Broodwar->self()->minerals() >=
             BWAPI::UnitTypes::Zerg_Hatchery.mineralPrice() +
             ((allUnitCount[BWAPI::UnitTypes::Zerg_Creep_Colony] + allUnitCount[BWAPI::UnitTypes::Zerg_Sunken_Colony] >= ss.numSunkens) ?
              0 :
              (((ss.numSunkens - allUnitCount[BWAPI::UnitTypes::Zerg_Creep_Colony]) * BWAPI::UnitTypes::Zerg_Creep_Colony.mineralPrice()) +
               ((ss.numSunkens - allUnitCount[BWAPI::UnitTypes::Zerg_Sunken_Colony]) * BWAPI::UnitTypes::Zerg_Sunken_Colony.mineralPrice()))) +
             (allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] >= 1 ?
              0 :
              BWAPI::UnitTypes::Zerg_Extractor.mineralPrice()) +
             (allUnitCount[BWAPI::UnitTypes::Zerg_Lair] + allUnitCount[BWAPI::UnitTypes::Zerg_Hive] >= 1 ?
              0 :
              BWAPI::UnitTypes::Zerg_Lair.mineralPrice())));
    }

    // Morph to queen's nest late-game.
    // Block to restrict scope of variables.
    {
        static Unit queensNestBuilder = nullptr;
        static BWAPI::TilePosition queensNestLoc = BWAPI::TilePositions::None;
        static int frameLastCheckedQueensNestLoc = 0;
        const int checkQueensNestLocFreqFrames = (10 * 24);
        makeUnit(
            BWAPI::UnitTypes::Zerg_Queens_Nest, queensNestBuilder, queensNestLoc, frameLastCheckedQueensNestLoc, checkQueensNestLocFreqFrames,
            Broodwar->canMake(BWAPI::UnitTypes::Zerg_Queens_Nest) &&
            (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60) &&
            (!ss.isMutaRushBO ||
             (allUnitCount[BWAPI::UnitTypes::Zerg_Spire] > 0 &&
              Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Mutalisk) > 0)));
    }

    // Morph to spire late-game. Check that a queen's nest and hive are already morphing/completed
    // because I prefer mutalisks or guardians before hydralisks,
    // and otherwise builders may try to build at the same place and fail.
    // Block to restrict scope of variables.
    {
        static Unit spireBuilder = nullptr;
        static BWAPI::TilePosition spireLoc = BWAPI::TilePositions::None;
        static int frameLastCheckedSpireLoc = 0;
        const int checkSpireLocFreqFrames = (10 * 24);
        makeUnit(
            BWAPI::UnitTypes::Zerg_Spire, spireBuilder, spireLoc, frameLastCheckedSpireLoc, checkSpireLocFreqFrames,
            Broodwar->canMake(BWAPI::UnitTypes::Zerg_Spire) &&
            (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60) &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] == 0 &&
            (ss.isMutaRushBO ||
             (allUnitCount[BWAPI::UnitTypes::Zerg_Queens_Nest] > 0 &&
              allUnitCount[BWAPI::UnitTypes::Zerg_Hive] > 0)));
    }

    // Morph to hydralisk den late-game. Check that a queen's nest and hive and spire
    // and some guardians are already morphing/completed
    // because I prefer mutalisks or guardians before hydralisks,
    // and otherwise builders may try to build at the same place and fail.
    // Block to restrict scope of variables.
    {
        static Unit hydraDenBuilder = nullptr;
        static BWAPI::TilePosition hydraDenLoc = BWAPI::TilePositions::None;
        static int frameLastCheckedHydraDenLoc = 0;
        const int checkHydraDenLocFreqFrames = (10 * 24);
        makeUnit(
            BWAPI::UnitTypes::Zerg_Hydralisk_Den, hydraDenBuilder, hydraDenLoc, frameLastCheckedHydraDenLoc, checkHydraDenLocFreqFrames,
            Broodwar->canMake(BWAPI::UnitTypes::Zerg_Hydralisk_Den) &&
            (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60) &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Queens_Nest] > 0 &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Hive] > 0 &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Spire] + allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] > 0 &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Guardian] > 8);
    }

    // Morph to ultralisk cavern late-game. Check that a hyrdalisk den and greater spire
    // and some guardians are already morphing/completed
    // because I prefer guardians before ultras.
    // Block to restrict scope of variables.
    {
        static Unit ultraCavernBuilder = nullptr;
        static BWAPI::TilePosition ultraCavernLoc = BWAPI::TilePositions::None;
        static int frameLastCheckedUltraCavernLoc = 0;
        const int checkUltraCavernLocFreqFrames = (10 * 24);
        makeUnit(
            BWAPI::UnitTypes::Zerg_Ultralisk_Cavern, ultraCavernBuilder, ultraCavernLoc, frameLastCheckedUltraCavernLoc, checkUltraCavernLocFreqFrames,
            Broodwar->canMake(BWAPI::UnitTypes::Zerg_Ultralisk_Cavern) &&
            (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60) &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Hydralisk_Den] > 0 &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] > 0 &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Guardian] > 8);
    }

    // A horrible way of making just enough gatherers gather gas, but it seems to work ok, so don't worry about it for the time being.
    for (auto& u : myUnits)
    {
        if (u->getType() == BWAPI::UnitTypes::Zerg_Extractor && u->isCompleted())
        {
            static int lastAddedGathererToRefinery = 0;
            if (Broodwar->getFrameCount() > lastAddedGathererToRefinery + (3 * 24))
            {
                BWAPI::Unit gasGatherer = u->getClosestUnit(
                    IsOwned && Exists && GetType == BWAPI::UnitTypes::Zerg_Drone &&
                    (CurrentOrder == BWAPI::Orders::MoveToGas || CurrentOrder == BWAPI::Orders::WaitForGas),
                    256);

                if (myCompletedWorkers.size() <= 6 ||
                    (ss.isSpeedlingBO &&
                     (/*Broodwar->self()->gas() >= BWAPI::UpgradeTypes::Metabolic_Boost.gasPrice() ||*/
                      Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Metabolic_Boost) /*||
                      Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost) == Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost)*/)) ||
                    (Broodwar->self()->gas() >= 675) ||
                    ((!ss.isSpeedlingBO ||
                      Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost) == Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost)) &&
                     !isStartedTransitioning))
                {
                    if (gasGatherer != nullptr)
                    {
                        if (!gasGatherer->isConstructing() && noCmdPending(gasGatherer) && gasGatherer->canStop())
                        {
                            gasGatherer->stop();
                            continue;
                        }
                    }
                }
                else
                {
                    if (gasGatherer == nullptr && myCompletedWorkers.size() > 6)
                    {        
                        auto isAvailableToGatherFrom =
                            [&u, &noCmdPending](Unit& tmpUnit)
                            {
                                return !tmpUnit->isConstructing() && noCmdPending(tmpUnit) && tmpUnit->canGather(u);
                            };

                        BWAPI::Unit newGasGatherer = u->getClosestUnit(GetType == BWAPI::UnitTypes::Zerg_Drone && IsIdle && !IsCarryingSomething && IsOwned && isAvailableToGatherFrom);
                        if (newGasGatherer == nullptr)
                            newGasGatherer = u->getClosestUnit(GetType == BWAPI::UnitTypes::Zerg_Drone && IsGatheringMinerals && !IsCarryingSomething && IsOwned && isAvailableToGatherFrom);
                        if (newGasGatherer == nullptr)
                            newGasGatherer = u->getClosestUnit(GetType == BWAPI::UnitTypes::Zerg_Drone && IsIdle && !IsCarryingGas && IsOwned && isAvailableToGatherFrom);
                        if (newGasGatherer == nullptr)
                            newGasGatherer = u->getClosestUnit(GetType == BWAPI::UnitTypes::Zerg_Drone && IsGatheringMinerals && IsOwned && isAvailableToGatherFrom);
                        // If a unit was found
                        if (newGasGatherer)
                        {
                            newGasGatherer->gather(u);
        
                            if (resourceToGathererMap.find(u) != resourceToGathererMap.end())
                            {
                                gathererToResourceMap.erase(resourceToGathererMap.at(u));
                            }
        
                            auto itr1 = resourceToGathererMap.find(newGasGatherer);
                            if (itr1 == resourceToGathererMap.end())
                              resourceToGathererMap.insert(std::make_pair(u, newGasGatherer));
                            else
                              itr1->second = newGasGatherer;
                            //resourceToGathererMap[u] = newGasGatherer;
                            auto itr2 = gathererToResourceMap.find(u);
                            if (itr2 == gathererToResourceMap.end())
                              gathererToResourceMap.insert(std::make_pair(newGasGatherer, u));
                            else
                              itr2->second = u;
                            //gathererToResourceMap[newGasGatherer] = u;
                            lastAddedGathererToRefinery = Broodwar->getFrameCount();
                        }
                    }
        
                    break;
                }
            }
        }
    }

    Unitset myFreeGatherers;

    // The main loop.
    for (auto& u : myUnits)
    {
        if (u->getLastCommandFrame() == Broodwar->getFrameCount() && u->getLastCommand().getType() != BWAPI::UnitCommandTypes::None)
        {
            // Already issued a command to this unit this frame (e.g. a build command) so skip this unit.
            continue;
        }

        if (!u->canCommand() || u->isStuck())
            continue;

        // Cancel morph when appropriate if we are using the extractor trick.
        if (u->getType() == BWAPI::UnitTypes::Zerg_Extractor && !u->isCompleted())
        {
            if (Broodwar->getFrameCount() < transitionOutOf4PoolFrameCountThresh &&
                supplyUsed < 60 &&
                (completedUnitCount[BWAPI::UnitTypes::Zerg_Spawning_Pool] == 0 ||
                 (supplyUsed >= Broodwar->self()->supplyTotal() - 1 ||
                  ((Broodwar->self()->minerals() < 50 ||
                    allUnitCount[BWAPI::UnitTypes::Zerg_Larva] == 0) &&
                   supplyUsed < Broodwar->self()->supplyTotal() - 3) ||
                  u->getRemainingBuildTime() <= Broodwar->getRemainingLatencyFrames() + 2)))
            {
                if (u->canCancelMorph())
                {
                    u->cancelMorph();
                    continue;
                }
            }
        }

        // Cancel pool if we have no completed/incomplete drones and definitely wouldn't be able
        // to get any drones after it completes (assuming any incomplete hatcheries are left to complete).
        if (u->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool &&
            !u->isCompleted())
        {
            // Note: using allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] rather than incompleteUnitCount[BWAPI::UnitTypes::Zerg_Extractor]
            // because BWAPI seems to think it is completed.
            if (allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] == 0 &&
                (Broodwar->self()->minerals() < 50 ||
                 (allUnitCount[BWAPI::UnitTypes::Zerg_Hatchery] == 0 &&
                  allUnitCount[BWAPI::UnitTypes::Zerg_Lair] == 0 &&
                  allUnitCount[BWAPI::UnitTypes::Zerg_Hive] == 0 &&
                  allUnitCount[BWAPI::UnitTypes::Zerg_Larva] == 0)))
            {
                if (u->canCancelMorph())
                {
                    u->cancelMorph();
                    continue;
                }
            }
        }

        // Cancel egg if it doesn't contain a drone and we have no completed/incomplete drones and definitely wouldn't be able
        // to get any drones if it were to complete (assuming any incomplete hatcheries are left to complete).
        // Note: using allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] rather than incompleteUnitCount[BWAPI::UnitTypes::Zerg_Extractor]
        // because BWAPI seems to think it is completed.
        if (completedUnitCount[BWAPI::UnitTypes::Zerg_Spawning_Pool] > 0 &&
            u->getType() == BWAPI::UnitTypes::Zerg_Egg &&
            !u->isCompleted() &&
            u->getBuildType() != BWAPI::UnitTypes::None &&
            u->getBuildType() != BWAPI::UnitTypes::Unknown &&
            allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] == 0 &&
            (Broodwar->self()->minerals() < 50 ||
             (allUnitCount[BWAPI::UnitTypes::Zerg_Hatchery] == 0 &&
              allUnitCount[BWAPI::UnitTypes::Zerg_Lair] == 0 &&
              allUnitCount[BWAPI::UnitTypes::Zerg_Hive] == 0 &&
              allUnitCount[BWAPI::UnitTypes::Zerg_Larva] == 0)))
        {
            if (u->canCancelMorph())
            {
                u->cancelMorph();
                continue;
            }
        }

        // Cancel pool if a drone has died and we have a low number of workers.
        if (u->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool &&
            !u->isCompleted())
        {
            // Note: using allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] rather than incompleteUnitCount[BWAPI::UnitTypes::Zerg_Extractor]
            // because BWAPI seems to think it is completed.
            if (allUnitCount[BWAPI::UnitTypes::Zerg_Spawning_Pool] == 1 &&
                Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Drone) > (isScoutingWorkerReadyToScout ? 1 : 0) &&
                allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] < 3)
            {
                if (u->canCancelMorph())
                {
                    u->cancelMorph();
                    continue;
                }
            }
        }

        // Ignore the unit if it is incomplete or busy constructing
        if (!u->isCompleted() || u->isConstructing())
            continue;

        // For speedling build or late game, upgrade metabolic boost when possible.
        if ((ss.isSpeedlingBO ||
             Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Mutalisk) + Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Guardian) > 0) &&
            u->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool)
        {
            if (u->canUpgrade(BWAPI::UpgradeTypes::Metabolic_Boost))
            {
                u->upgrade(BWAPI::UpgradeTypes::Metabolic_Boost);
                continue;
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Spawning_Pool &&
            Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Mutalisk) + Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Guardian) > 0)
        {
            if (u->canUpgrade(BWAPI::UpgradeTypes::Adrenal_Glands))
            {
                u->upgrade(BWAPI::UpgradeTypes::Adrenal_Glands);
                continue;
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Spire || u->getType() == BWAPI::UnitTypes::Zerg_Greater_Spire)
        {
            if (u->canUpgrade(BWAPI::UpgradeTypes::Zerg_Flyer_Attacks) &&
                allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] > 0)
            {
                u->upgrade(BWAPI::UpgradeTypes::Zerg_Flyer_Attacks);
                continue;
            }

            if (u->canUpgrade(BWAPI::UpgradeTypes::Zerg_Flyer_Carapace) &&
                allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] > 0)
            {
                u->upgrade(BWAPI::UpgradeTypes::Zerg_Flyer_Attacks);
                continue;
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Hydralisk_Den)
        {
            if (u->canUpgrade(BWAPI::UpgradeTypes::Grooved_Spines))
            {
                u->upgrade(BWAPI::UpgradeTypes::Grooved_Spines);
                continue;
            }

            if (u->canUpgrade(BWAPI::UpgradeTypes::Muscular_Augments))
            {
                u->upgrade(BWAPI::UpgradeTypes::Muscular_Augments);
                continue;
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Ultralisk_Cavern)
        {
            if (u->canUpgrade(BWAPI::UpgradeTypes::Chitinous_Plating))
            {
                u->upgrade(BWAPI::UpgradeTypes::Chitinous_Plating);
                continue;
            }

            if (u->canUpgrade(BWAPI::UpgradeTypes::Anabolic_Synthesis))
            {
                u->upgrade(BWAPI::UpgradeTypes::Anabolic_Synthesis);
                continue;
            }
        }

        // Could also take into account higher ground advantage, cover advantage (e.g. in trees), HP regen, shields regen,
        // effects of spells like dark swarm. The list is endless.
        auto getBestEnemyThreatUnitLambda =
            [&u](const BWAPI::Unit& bestSoFarUnit, const BWAPI::Unit& curUnit)
            {
                if (curUnit->isPowered() != bestSoFarUnit->isPowered())
                {
                    return curUnit->isPowered() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isLockedDown() != bestSoFarUnit->isLockedDown())
                {
                    return !curUnit->isLockedDown() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isMaelstrommed() != bestSoFarUnit->isMaelstrommed())
                {
                    return !curUnit->isMaelstrommed() ? curUnit : bestSoFarUnit;
                }

                // Prefer to attack units that can return fire or could be tactical threats in certain scenarios.
                const BWAPI::UnitType curUnitType = curUnit->getType();
                const BWAPI::UnitType bestSoFarUnitType = bestSoFarUnit->getType();
                const BWAPI::WeaponType curUnitWeaponType =
                    u->isFlying() ? curUnitType.airWeapon() : curUnitType.groundWeapon();
                const BWAPI::WeaponType bestSoFarUnitWeaponType =
                    u->isFlying() ? bestSoFarUnitType.airWeapon() : bestSoFarUnitType.groundWeapon();
                if (curUnitWeaponType != bestSoFarUnitWeaponType)
                {
                    if (curUnitWeaponType == BWAPI::WeaponTypes::None &&
                        // FYI, Protoss_Carrier, Hero_Gantrithor, Protoss_Reaver, Hero_Warbringer are the
                        // only BWAPI::UnitType's that have no weapon but UnitType::canAttack() returns true.
                        curUnitType.canAttack() &&
                        curUnitType != BWAPI::UnitTypes::Terran_Bunker &&
                        curUnitType != BWAPI::UnitTypes::Protoss_High_Templar &&
                        curUnitType != BWAPI::UnitTypes::Zerg_Defiler &&
                        curUnitType != BWAPI::UnitTypes::Protoss_Dark_Archon &&
                        curUnitType != BWAPI::UnitTypes::Terran_Science_Vessel &&
                        curUnitType != BWAPI::UnitTypes::Zerg_Queen &&
                        curUnitType != BWAPI::UnitTypes::Protoss_Shuttle &&
                        curUnitType != BWAPI::UnitTypes::Terran_Dropship &&
                        curUnitType != BWAPI::UnitTypes::Protoss_Observer &&
                        curUnitType != BWAPI::UnitTypes::Zerg_Overlord &&
                        curUnitType != BWAPI::UnitTypes::Terran_Medic &&
                        curUnitType != BWAPI::UnitTypes::Terran_Nuclear_Silo &&
                        curUnitType != BWAPI::UnitTypes::Zerg_Nydus_Canal /*&&
                        // TODO: re-enable Terran_Comsat_Station after add any
                        // logic to produce cloaked units.
                        curUnitType != BWAPI::UnitTypes::Terran_Comsat_Station*/)
                    {
                        return bestSoFarUnit;
                    }

                    if (bestSoFarUnitWeaponType == BWAPI::WeaponTypes::None &&
                        // FYI, Protoss_Carrier, Hero_Gantrithor, Protoss_Reaver, Hero_Warbringer are the
                        // only BWAPI::UnitType's that have no weapon but UnitType::canAttack() returns true.
                        bestSoFarUnitType.canAttack() &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Terran_Bunker &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Protoss_High_Templar &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Zerg_Defiler &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Protoss_Dark_Archon &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Terran_Science_Vessel &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Zerg_Queen &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Protoss_Shuttle &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Terran_Dropship &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Protoss_Observer &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Zerg_Overlord &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Terran_Medic &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Terran_Nuclear_Silo &&
                        bestSoFarUnitType != BWAPI::UnitTypes::Zerg_Nydus_Canal /*&&
                        // TODO: re-enable Terran_Comsat_Station after add any
                        // logic to produce cloaked units.
                        bestSoFarUnitType != BWAPI::UnitTypes::Terran_Comsat_Station*/)
                    {
                        return curUnit;
                    }
                }

                auto unitTypeScoreLambda = [](const BWAPI::UnitType& unitType) -> int
                    {
                        return
                            unitType == BWAPI::UnitTypes::Protoss_Pylon ? 30000 :
                            unitType == BWAPI::UnitTypes::Protoss_Nexus ? 29000 :
                            unitType == BWAPI::UnitTypes::Terran_Command_Center ? 28000 :
                            unitType == BWAPI::UnitTypes::Zerg_Hive ? 27000 :
                            unitType == BWAPI::UnitTypes::Zerg_Lair ? 26000 :
                            unitType == BWAPI::UnitTypes::Zerg_Hatchery ? 25000 :
                            unitType == BWAPI::UnitTypes::Zerg_Greater_Spire ? 24000 :
                            unitType == BWAPI::UnitTypes::Zerg_Spire ? 23000 :
                            unitType == BWAPI::UnitTypes::Terran_Starport ? 22000 :
                            unitType == BWAPI::UnitTypes::Protoss_Stargate ? 21000 :
                            unitType == BWAPI::UnitTypes::Terran_Factory ? 20000 :
                            unitType == BWAPI::UnitTypes::Terran_Barracks ? 19000 :
                            unitType == BWAPI::UnitTypes::Zerg_Spawning_Pool ? 18000 :
                            unitType == BWAPI::UnitTypes::Zerg_Hydralisk_Den ? 17000 :
                            unitType == BWAPI::UnitTypes::Zerg_Queens_Nest ? 16000 :
                            unitType == BWAPI::UnitTypes::Protoss_Templar_Archives ? 15000 :
                            unitType == BWAPI::UnitTypes::Protoss_Gateway ? 14000 :
                            unitType == BWAPI::UnitTypes::Protoss_Cybernetics_Core ? 13000 :
                            unitType == BWAPI::UnitTypes::Protoss_Shield_Battery ? 12000 :
                            unitType == BWAPI::UnitTypes::Protoss_Forge ? 11000 :
                            unitType == BWAPI::UnitTypes::Protoss_Citadel_of_Adun ? 10000 :
                            unitType == BWAPI::UnitTypes::Terran_Academy ? 9000 :
                            unitType == BWAPI::UnitTypes::Terran_Engineering_Bay ? 8000 :
                            unitType == BWAPI::UnitTypes::Zerg_Creep_Colony ? 7000 :
                            unitType == BWAPI::UnitTypes::Zerg_Evolution_Chamber ? 6000 :
                            unitType == BWAPI::UnitTypes::Zerg_Lurker_Egg ? 5000 :
                            unitType == BWAPI::UnitTypes::Zerg_Egg ? 4000 :
                            unitType == BWAPI::UnitTypes::Zerg_Larva ? 3000 :
                            unitType == BWAPI::UnitTypes::Zerg_Spore_Colony ? 2000 :
                            unitType == BWAPI::UnitTypes::Terran_Missile_Turret ? 1000 :
                            unitType == BWAPI::UnitTypes::Terran_Supply_Depot ? -1000 :
                            unitType.isRefinery() ? -2000 :
                            unitType == BWAPI::UnitTypes::Terran_Covert_Ops ? -3000 :
                            unitType == BWAPI::UnitTypes::Terran_Control_Tower ? -4000 :
                            unitType == BWAPI::UnitTypes::Terran_Machine_Shop ? -5000 :
                            unitType == BWAPI::UnitTypes::Terran_Comsat_Station ? -6000 :
                            unitType == BWAPI::UnitTypes::Protoss_Scarab ? -7000 :
                            unitType == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine ? -8000 :
                            unitType == BWAPI::UnitTypes::Zerg_Infested_Terran ? -9000 :
                            0;
                    };

                const int curUnitTypeScore = unitTypeScoreLambda(curUnitType);
                const int bestSoFarUnitTypeScore = unitTypeScoreLambda(bestSoFarUnitType);
                if (curUnitTypeScore != bestSoFarUnitTypeScore)
                {
                    return curUnitTypeScore > bestSoFarUnitTypeScore ? curUnit : bestSoFarUnit;
                }

                // If the set of units being considered only contains workers or contains no workers
                // then this should work as intended.
                if (curUnit->getType().isWorker() && bestSoFarUnit->getType().isWorker() &&
                    !u->isInWeaponRange(curUnit) && !u->isInWeaponRange(bestSoFarUnit) &&
                    u->getDistance(curUnit) != u->getDistance(bestSoFarUnit))
                {
                    return (u->getDistance(curUnit) < u->getDistance(bestSoFarUnit)) ? curUnit : bestSoFarUnit;
                }

                const int curUnitLifeForceScore =
                    curUnit->getHitPoints() + curUnit->getShields() + curUnitType.armor() + curUnit->getDefenseMatrixPoints();
                const int bestSoFarUnitLifeForceScore =
                    bestSoFarUnit->getHitPoints() + bestSoFarUnit->getShields() + bestSoFarUnitType.armor() + bestSoFarUnit->getDefenseMatrixPoints();
                if (curUnitLifeForceScore != bestSoFarUnitLifeForceScore)
                {
                    return curUnitLifeForceScore < bestSoFarUnitLifeForceScore ? curUnit : bestSoFarUnit;
                }

                // Whether irradiate is good or bad is very situational (it depends whether it is
                // positioned amongst more of my units than the enemy's) but for now let's assume
                // it is positioned amongst more of mine. TODO: add special logic once my bot can
                // cast irradiate.
                if (curUnit->isIrradiated() != bestSoFarUnit->isIrradiated())
                {
                    return !curUnit->isIrradiated() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isBeingHealed() != bestSoFarUnit->isBeingHealed())
                {
                    return !curUnit->isBeingHealed() ? curUnit : bestSoFarUnit;
                }

                if (curUnitType.regeneratesHP() != bestSoFarUnitType.regeneratesHP())
                {
                    return !curUnitType.regeneratesHP() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isRepairing() != bestSoFarUnit->isRepairing())
                {
                    return curUnit->isRepairing() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isConstructing() != bestSoFarUnit->isConstructing())
                {
                    return curUnit->isConstructing() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isPlagued() != bestSoFarUnit->isPlagued())
                {
                    return !curUnit->isPlagued() ? curUnit : bestSoFarUnit;
                }

                if ((curUnit->getTarget() == u) != (bestSoFarUnit->getTarget() == u) || (curUnit->getOrderTarget() == u) != (bestSoFarUnit->getOrderTarget() == u))
                {
                    return ((curUnit->getTarget() == u && bestSoFarUnit->getTarget() != u) || (curUnit->getOrderTarget() == u && bestSoFarUnit->getOrderTarget() != u)) ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isAttacking() != bestSoFarUnit->isAttacking())
                {
                    return curUnit->isAttacking() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->getSpellCooldown() != bestSoFarUnit->getSpellCooldown())
                {
                    return curUnit->getSpellCooldown() < bestSoFarUnit->getSpellCooldown() ? curUnit : bestSoFarUnit;
                }

                if (!u->isFlying())
                {
                    if (curUnit->getGroundWeaponCooldown() != bestSoFarUnit->getGroundWeaponCooldown())
                    {
                        return curUnit->getGroundWeaponCooldown() < bestSoFarUnit->getGroundWeaponCooldown() ? curUnit : bestSoFarUnit;
                    }
                }
                else
                {    
                    if (curUnit->getAirWeaponCooldown() != bestSoFarUnit->getAirWeaponCooldown())
                    {
                        return curUnit->getAirWeaponCooldown() < bestSoFarUnit->getAirWeaponCooldown() ? curUnit : bestSoFarUnit;
                    }
                }

                if (curUnit->isStartingAttack() != bestSoFarUnit->isStartingAttack())
                {
                    return !curUnit->isStartingAttack() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isAttackFrame() != bestSoFarUnit->isAttackFrame())
                {
                    return !curUnit->isAttackFrame() ? curUnit : bestSoFarUnit;
                }

                // Prefer stationary targets (because more likely to hit them).
                if (curUnit->isHoldingPosition() != bestSoFarUnit->isHoldingPosition())
                {
                    return curUnit->isHoldingPosition() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isMoving() != bestSoFarUnit->isMoving())
                {
                    return !curUnit->isMoving() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isBraking() != bestSoFarUnit->isBraking())
                {
                    if (curUnit->isMoving() && bestSoFarUnit->isMoving())
                    {
                        return curUnit->isBraking() ? curUnit : bestSoFarUnit;
                    }
                    else if (!curUnit->isMoving() && !bestSoFarUnit->isMoving())
                    {
                        return !curUnit->isBraking() ? curUnit : bestSoFarUnit;
                    }
                }

                if (curUnit->isAccelerating() != bestSoFarUnit->isAccelerating())
                {
                    if (curUnit->isMoving() && bestSoFarUnit->isMoving())
                    {
                        return !curUnit->isAccelerating() ? curUnit : bestSoFarUnit;
                    }
                    else if (!curUnit->isMoving() && !bestSoFarUnit->isMoving())
                    {
                        return !curUnit->isAccelerating() ? curUnit : bestSoFarUnit;
                    }
                }

                // Prefer to attack enemy units that are morphing. Assume here that armor has already taken into account properly above.
                if (curUnit->isMorphing() != bestSoFarUnit->isMorphing())
                {
                    return curUnit->isMorphing() ? curUnit : bestSoFarUnit;
                }

                // Prefer to attack enemy units that are being constructed.
                if (curUnit->isBeingConstructed() != bestSoFarUnit->isBeingConstructed())
                {
                    return curUnit->isBeingConstructed() ? curUnit : bestSoFarUnit;
                }

                // Prefer to attack enemy units that are incomplete.
                if (curUnit->isCompleted() != bestSoFarUnit->isCompleted())
                {
                    return !curUnit->isCompleted() ? curUnit : bestSoFarUnit;
                }

                // Prefer to attack bunkers.
                // Note: getType()->canAttack() is false for a bunker.
                if ((curUnitType == BWAPI::UnitTypes::Terran_Bunker || bestSoFarUnitType == BWAPI::UnitTypes::Terran_Bunker) &&
                    curUnitType != bestSoFarUnitType)
                {
                    return curUnitType == BWAPI::UnitTypes::Terran_Bunker ? curUnit : bestSoFarUnit;
                }

                // Prefer to attack enemy units that can attack.
                if (curUnitType.canAttack() != bestSoFarUnitType.canAttack())
                {
                    return curUnitType.canAttack() ? curUnit : bestSoFarUnit;
                }

                // Prefer to attack workers.
                if (curUnitType.isWorker() != bestSoFarUnitType.isWorker())
                {
                    return curUnitType.isWorker() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isCarryingGas() != bestSoFarUnit->isCarryingGas())
                {
                    return curUnit->isCarryingGas() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isCarryingMinerals() != bestSoFarUnit->isCarryingMinerals())
                {
                    return curUnit->isCarryingMinerals() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isGatheringMinerals() != bestSoFarUnit->isGatheringMinerals())
                {
                    return curUnit->isGatheringMinerals() ? curUnit : bestSoFarUnit;
                }

                // For now, let's prefer to attack mineral gatherers than gas gatherers,
                // because gas gatherers generally take longer to kill because they keep
                // going into the refinery/assimilator/extractor.
                if (curUnit->isGatheringGas() != bestSoFarUnit->isGatheringGas())
                {
                    return curUnit->isGatheringGas() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->getPowerUp() != bestSoFarUnit->getPowerUp())
                {
                    if (bestSoFarUnit->getPowerUp() == nullptr)
                    {
                        return curUnit;
                    }
                    else if (curUnit->getPowerUp() == nullptr)
                    {
                        return bestSoFarUnit;
                    }
                }

                if (curUnit->isBlind() != bestSoFarUnit->isBlind())
                {
                    return !curUnit->isBlind() ? curUnit : bestSoFarUnit;
                }

                if ((curUnitType == BWAPI::UnitTypes::Protoss_Carrier || curUnitType == UnitTypes::Hero_Gantrithor) &&
                    (bestSoFarUnitType == BWAPI::UnitTypes::Protoss_Carrier || bestSoFarUnitType == UnitTypes::Hero_Gantrithor) &&
                    curUnit->getInterceptorCount() != bestSoFarUnit->getInterceptorCount())
                {
                    return curUnit->getInterceptorCount() > bestSoFarUnit->getInterceptorCount() ? curUnit : bestSoFarUnit;
                }

                if (u->getDistance(curUnit) != u->getDistance(bestSoFarUnit))
                {
                    return (u->getDistance(curUnit) < u->getDistance(bestSoFarUnit)) ? curUnit : bestSoFarUnit;
                }

                if (curUnit->getAcidSporeCount() != bestSoFarUnit->getAcidSporeCount())
                {
                    return curUnit->getAcidSporeCount() < bestSoFarUnit->getAcidSporeCount() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->getKillCount() != bestSoFarUnit->getKillCount())
                {
                    return curUnit->getKillCount() < bestSoFarUnit->getKillCount() ? curUnit : bestSoFarUnit;
                }

                if (curUnit->isIdle() != bestSoFarUnit->isIdle())
                {
                    return !curUnit->isIdle() ? curUnit : bestSoFarUnit;
                }

                // TODO: The meaning of isUnderAttack() is more like  "was attacked recently" and from the forums it sounds
                // like it is a GUI thing and affected by the real clock (not the in-game clock) so if games are played at
                // high speed it is misleading, but let's check it anyway as lowest priority until I can come up with more
                // reliable logic. Could also check whether any of our other units are targeting it (if that info is
                // accessible).
                if (curUnit->isUnderAttack() != bestSoFarUnit->isUnderAttack())
                {
                    return curUnit->isUnderAttack() ? curUnit : bestSoFarUnit;
                }

                return bestSoFarUnit;
            };

        const BWAPI::UnitType airForceUnitType = BWAPI::UnitTypes::Zerg_Mutalisk;

        if (u->getType() == airForceUnitType.whatBuilds().first)
        {
            if (allUnitCount[airForceUnitType] <= (ss.isMutaRushBO ? 40 : 12) &&
                ((ss.isMutaRushBO && Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Mutalisk) == 0) ||
                 (allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] > 0 &&
                  // Wait for enough gas and minerals to be able to make a mutalisk and morph at least all the existing mutalisks into guardians
                  // (but not necessary the new mutalisk).
                  Broodwar->self()->gas() >= BWAPI::UnitTypes::Zerg_Mutalisk.gasPrice() + (allUnitCount[BWAPI::UnitTypes::Zerg_Mutalisk] * BWAPI::UnitTypes::Zerg_Guardian.gasPrice()) &&
                  Broodwar->self()->minerals() >= BWAPI::UnitTypes::Zerg_Mutalisk.mineralPrice() + (allUnitCount[BWAPI::UnitTypes::Zerg_Mutalisk] * BWAPI::UnitTypes::Zerg_Guardian.mineralPrice()))))
            {
                // Train more air combat units.
                if ((u->getType() == UnitTypes::Zerg_Larva || u->getTrainingQueue().size() < 2) && noCmdPending(u))
                {
                    if (u->canTrain(airForceUnitType))
                    {
                        u->train(airForceUnitType);
                        continue;
                    }
                }
            }
        }

        const BWAPI::UnitType workerUnitType = Broodwar->self()->getRace().getWorker();

        if (u->getType().isWorker())
        {
            // Attack enemy units of opportunity (e.g. enemy worker scout(s) that are harassing my gatherers).
            // Don't issue a new command if we are a low life drone planning to build an extractor.
            if ((u != lowLifeDrone || u != extractorBuilder) &&
                (u->isIdle() || u->getLastCommand().getType() == BWAPI::UnitCommandTypes::None ||
                 Broodwar->getFrameCount() >= u->getLastCommandFrame() + 2 - (Broodwar->getLatencyFrames() > 2 ? u->getLastCommandFrame() % 2 : 0)))
            {
                // Add some frames to cover frame(s) that might be needed to change direction.
                if (u->getGroundWeaponCooldown() <= Broodwar->getRemainingLatencyFrames() + 2)
                {
                    const BWAPI::Unit bestAttackableEnemyNonBuildingUnit =
                        workerAttackTargetUnit != nullptr ?
                        workerAttackTargetUnit :
                        Broodwar->getBestUnit(
                            getBestEnemyThreatUnitLambda,
                            IsEnemy && IsVisible && IsDetected && Exists &&
                            // Ignore buildings because we do not want to waste mining time, and I don't think we need
                            // to worry about manner pylon or gas steal because the current 4pool-only version in theory shouldn't
                            // place workers where they can get stuck by a manner pylon on most maps, and gas steal is
                            // rarely much of a hinderance except on large maps because we need lots of lings to be in a
                            // situation to use the extractor trick (it just stops us healing drones with the extractor trick).
                            // The lings will attack buildings near my base when they spawn anyway.
                            !IsBuilding &&
                            [&u](Unit& tmpUnit) { return u->isInWeaponRange(tmpUnit) && u->canAttack(tmpUnit); },
                            u->getPosition(),
                            std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()));
    
                    if (bestAttackableEnemyNonBuildingUnit && u->canAttack(bestAttackableEnemyNonBuildingUnit))
                    {
                        const BWAPI::Unit oldOrderTarget = u->getTarget();
                        if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableEnemyNonBuildingUnit)
                        {
                            u->attack(bestAttackableEnemyNonBuildingUnit);
    
                            if (gathererToResourceMap.find(u) != gathererToResourceMap.end() && resourceToGathererMap.find(gathererToResourceMap.at(u)) != resourceToGathererMap.end() && resourceToGathererMap.at(gathererToResourceMap.at(u)) == u)
                            {
                                resourceToGathererMap.erase(gathererToResourceMap.at(u));
                            }
    
                            gathererToResourceMap.erase(u);
                        }

                        continue;
                    }
                }

                // Don't issue a return cargo or gather command if we are a scouting worker or a drone planning to build a building.
                if (u != extractorBuilder && u != groundArmyBuildingBuilder && u->getLastCommand().getType() != BWAPI::UnitCommandTypes::Build)
                {
                    // If idle or were targeting an enemy unit or are no longer carrying minerals...
                    const bool isNewCmdNeeded = u->isIdle() || (u->getTarget() && u->getTarget()->getPlayer() && u->getTarget()->getPlayer()->isEnemy(Broodwar->self()));
                    if (isNewCmdNeeded ||
                        (!u->isCarryingMinerals() && (int) getClientInfo(u, wasJustCarryingMineralsInd) == wasJustCarryingMineralsTrueVal))
                    {
                        if (!u->isCarryingMinerals() && (int) getClientInfo(u, wasJustCarryingMineralsInd) == wasJustCarryingMineralsTrueVal)
                        {
                            // Reset indicator about carrying minerals because we aren't carrying minerals now.
                            // Note: setClientInfo may also be called at the end of this and some other frames
                            // (but not necessarily at the end of all frames because there's logic at the start
                            // of each frame to return if the frame count modulo is a certain value).
                            setClientInfo(u, wasJustCarryingMineralsDefaultVal, wasJustCarryingMineralsInd);
                            setClientInfo(u, Broodwar->getFrameCount(), frameLastReturnedMineralsInd);
                        }

                        if (u != scoutingWorker || (int) getClientInfo(u, frameLastReturnedMineralsInd) == 0)
                        {
                            // Order workers carrying a resource to return them to the center,
                            // otherwise find a mineral patch to harvest.
                            if (isNewCmdNeeded &&
                                (u->isCarryingGas() || u->isCarryingMinerals()))
                            {
                                if (u->canReturnCargo())
                                {
                                    if (u->getLastCommand().getType() != BWAPI::UnitCommandTypes::Return_Cargo)
                                    {
                                        u->returnCargo();
                                    }
        
                                    continue;
                                }
                            }
                            // The worker cannot harvest anything if it is carrying a powerup such as a flag.
                            else if (!u->getPowerUp())
                            {
                                if (u->canGather())
                                {
                                    myFreeGatherers.insert(u);
                                }
                            }
                        }
                    }
                }
            }

            if (u != scoutingWorker || (int) getClientInfo(u, frameLastReturnedMineralsInd) == 0)
            {
                continue;
            }
            else
            {
                isScoutingWorkerReadyToScout = true;
            }
        }
        else if (u->getType() == workerUnitType.whatBuilds().first)
        {
            // Train more workers if we have less than 3 (or 6 against enemy worker rush until pool is building), or needed late-game.
            // Note: one of the workers could currently be doing the extractor trick.
            // Note: using allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] rather than incompleteUnitCount[BWAPI::UnitTypes::Zerg_Extractor]
            // because BWAPI seems to think it is completed.
            if ((allUnitCount[BWAPI::UnitTypes::Zerg_Spawning_Pool] == 0 &&
                 // If we have lost drones to enemy worker rush then keep adding a few extra drones until they start to pop.
                 ((Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Drone) > (isScoutingWorkerReadyToScout ? 1 : 0) &&
                   allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] < 6 &&
                   myCompletedWorkers.size() + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] < 4) ||
                  allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] < 4)) ||
                (Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Zerg_Spawning_Pool) > 0 &&
                 (allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] < 3)) ||
                (allUnitCount[BWAPI::UnitTypes::Zerg_Spawning_Pool] > 0 &&
                 (isScoutingUsingWorker && unscoutedOtherStartLocs.size() > 1 && isNeedScoutingWorker && isNeedToMorphScoutingWorker)) ||
                ((Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh || supplyUsed >= 60) &&
                 // Save larvae while spire is being morphed so have some larvae available for mutalisks when it finishes.
                 // TODO: this might not count the worker currently inside the extractor.
                 allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame < (ss.isEnemyWorkerRusher && ss.numSunkens > 0 && allUnitCount[BWAPI::UnitTypes::Zerg_Sunken_Colony] == 0 ? 7 : (((ss.isSpeedlingBO || ss.isHydraRushBO) && allUnitCount[BWAPI::UnitTypes::Zerg_Lair] + allUnitCount[BWAPI::UnitTypes::Zerg_Hive] == 0) ? (ss.isSpeedlingBO ? 9 : 11) : ((allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame <= 12 || Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Mutalisk) > 0 || !ss.isMutaRushBO || incompleteUnitCount[BWAPI::UnitTypes::Zerg_Spire] != 1 || allUnitCount[BWAPI::UnitTypes::Zerg_Spire] + allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] != 1 || spireRemainingBuildTime > 1000) ? 14 : 12)))))
            {
                if ((u->getType() == UnitTypes::Zerg_Larva || u->getTrainingQueue().size() < 2) && noCmdPending(u))
                {
                    if (u->canTrain(workerUnitType))
                    {
                        u->train(workerUnitType);
                        ++numWorkersTrainedThisFrame;
                        isNeedToMorphScoutingWorker = false;
                        continue;
                    }
                }
            }
        }
        else if (u->getType() == BWAPI::UnitTypes::Zerg_Hatchery)
        {
            // Morph to lair late-game.
            static bool issuedMorphLairCmd = false;
            if (u->canMorph(BWAPI::UnitTypes::Zerg_Lair) &&
                allUnitCount[BWAPI::UnitTypes::Zerg_Lair] + allUnitCount[BWAPI::UnitTypes::Zerg_Hive] == 0 &&
                ((!ss.isSpeedlingBO && !ss.isHydraRushBO) ||
                 (!ss.isHydraRushBO && isStartedTransitioning) ||
                 (ss.isHydraRushBO &&
                  (Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Hydralisk) > 2 ||
                   (Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Grooved_Spines) == Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Grooved_Spines) &&
                    Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Muscular_Augments) == Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Muscular_Augments))))))
            {
                u->morph(BWAPI::UnitTypes::Zerg_Lair);
                issuedMorphLairCmd = true;
                continue;
            }
        }
        else if (u->getType() == BWAPI::UnitTypes::Zerg_Creep_Colony)
        {
            // Morph to sunken late-game.
            static bool issuedMorphSunkenColonyCmd = false;
            if (u->canMorph(BWAPI::UnitTypes::Zerg_Sunken_Colony) && allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame >= 1)
            {
                u->morph(BWAPI::UnitTypes::Zerg_Sunken_Colony);
                issuedMorphSunkenColonyCmd = true;
                continue;
            }
        }
        else if (u->getType() == BWAPI::UnitTypes::Zerg_Lair)
        {
            // Morph to hive late-game.
            static bool issuedMorphHiveCmd = false;
            if (u->canMorph(BWAPI::UnitTypes::Zerg_Hive) && allUnitCount[BWAPI::UnitTypes::Zerg_Hive] == 0)
            {
                u->morph(BWAPI::UnitTypes::Zerg_Hive);
                issuedMorphHiveCmd = true;
                continue;
            }
        }
        else if (u->getType() == BWAPI::UnitTypes::Zerg_Spire)
        {
            // Morph to greater spire late-game.
            static bool issuedMorphGreaterSpireCmd = false;
            if (u->canMorph(BWAPI::UnitTypes::Zerg_Greater_Spire) && allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] == 0)
            {
                u->morph(BWAPI::UnitTypes::Zerg_Greater_Spire);
                issuedMorphGreaterSpireCmd = true;
                continue;
            }
        }
        else if (u->getType() == BWAPI::UnitTypes::Zerg_Mutalisk)
        {
            // Morph a limited number of guardians late-game.
            // Not until at least one mutalisk has died though (because mutalisks may be all we need,
            // e.g. against lifted buildings or an enemy that doesn't get any anti-air).
            if (u->canMorph(BWAPI::UnitTypes::Zerg_Guardian) && allUnitCount[UnitTypes::Zerg_Guardian] <= 8 &&
                (!ss.isMutaRushBO || Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Mutalisk) > 0))
            {
                u->morph(BWAPI::UnitTypes::Zerg_Guardian);
                continue;
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Ultralisk.whatBuilds().first)
        {
            // Train more ultralisk units.
            if (allUnitCount[UnitTypes::Zerg_Guardian] >= 8 &&
                allUnitCount[BWAPI::UnitTypes::Zerg_Ultralisk] <= 6 &&
                !isMapPlasma_v_1_0 &&
                noCmdPending(u))
            {
                if (u->canTrain(BWAPI::UnitTypes::Zerg_Ultralisk))
                {
                    u->train(BWAPI::UnitTypes::Zerg_Ultralisk);
                    continue;
                }
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Hydralisk.whatBuilds().first)
        {
            // Train more hydralisk units.
            if (allUnitCount[UnitTypes::Zerg_Guardian] >= 8 &&
                allUnitCount[BWAPI::UnitTypes::Zerg_Hydralisk] <= 20 &&
                noCmdPending(u))
            {
                if (u->canTrain(BWAPI::UnitTypes::Zerg_Hydralisk))
                {
                    u->train(BWAPI::UnitTypes::Zerg_Hydralisk);
                    continue;
                }
            }
        }

        if (u->getType() == BWAPI::UnitTypes::Zerg_Zergling.whatBuilds().first)
        {
            // Train more zergling units.
            if (allUnitCount[UnitTypes::Zerg_Guardian] >= 8 &&
                allUnitCount[BWAPI::UnitTypes::Zerg_Zergling] <= 30 &&
                noCmdPending(u))
            {
                if (u->canTrain(BWAPI::UnitTypes::Zerg_Zergling))
                {
                    u->train(BWAPI::UnitTypes::Zerg_Zergling);
                    continue;
                }
            }
        }

        if (u->getType() == groundArmyUnitType.whatBuilds().first)
        {
            // Train more ground combat units.
            if (((Broodwar->getFrameCount() < transitionOutOf4PoolFrameCountThresh &&
                  ((ss.numSunkens > 0 && allUnitCount[UnitTypes::Zerg_Creep_Colony] + allUnitCount[UnitTypes::Zerg_Sunken_Colony] > 0) ||
                   !((ss.isSpeedlingBO || ss.isHydraRushBO) ? (myCompletedWorkers.size() >= 6 && Broodwar->self()->deadUnitCount(groundArmyUnitType) > 10) : (myCompletedWorkers.size() >= 3 && Broodwar->self()->deadUnitCount(groundArmyUnitType) > 14)))) ||
                 (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh &&
                  (((completedUnitCount[UnitTypes::Zerg_Ultralisk_Cavern] == 0 &&
                     (Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Mutalisk) + Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Guardian) > 0 ||
                      !ss.isMutaRushBO ||
                      allUnitCount[BWAPI::UnitTypes::Zerg_Spire] + allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] != 1)) ||
                    allUnitCount[UnitTypes::Zerg_Ultralisk] >= 2) &&
                   allUnitCount[groundArmyUnitType] <= 30))) &&
                (!ss.isHydraRushBO ||
                 Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Grooved_Spines) ||
                 Broodwar->self()->isUpgrading(BWAPI::UpgradeTypes::Muscular_Augments) ||
                 (Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Grooved_Spines) == Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Grooved_Spines) &&
                  Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Muscular_Augments) == Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Muscular_Augments))) &&
                !isMapPlasma_v_1_0 &&
                ((u->getType() == UnitTypes::Zerg_Larva || u->getTrainingQueue().size() < 2) && noCmdPending(u)))
            {
                if (u->canTrain(groundArmyUnitType))
                {
                    u->train(groundArmyUnitType);
                    continue;
                }
            }
        }

        // A resource depot is a Command Center, Nexus, or Hatchery/Lair/Hive.
        if (u->getType().isResourceDepot())
        {
            const UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
            // Commented out because for 4pool we probably shouldn't ever make overlords
            // cos we want all available larvae available to replenish ling count if ling(s) die.
            // We could use the amount of larvae available as a threshold but there is a risk many
            // lings could soon die and we wouldn't have any larvae available to re-make them.
            //if (incompleteUnitCount[supplyProviderType] == 0 &&
            //    ((Broodwar->getFrameCount() < transitionOutOf4PoolFrameCountThresh && supplyUsed >= Broodwar->self()->supplyTotal()) ||
            //     (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh && Broodwar->self()->supplyTotal() < 400)))
            if ((Broodwar->getFrameCount() < transitionOutOf4PoolFrameCountThresh &&
                 Broodwar->self()->supplyTotal() + (incompleteUnitCount[supplyProviderType] * supplyProviderType.supplyProvided()) < 18) ||
                (Broodwar->getFrameCount() >= transitionOutOf4PoolFrameCountThresh &&
                 Broodwar->self()->supplyTotal() + (incompleteUnitCount[supplyProviderType] * supplyProviderType.supplyProvided()) < 400 &&
                 supplyUsed + (((allUnitCount[BWAPI::UnitTypes::Zerg_Hatchery] + allUnitCount[BWAPI::UnitTypes::Zerg_Lair] + allUnitCount[BWAPI::UnitTypes::Zerg_Hive]) * ((Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Mutalisk) == 0 && ss.isMutaRushBO && incompleteUnitCount[BWAPI::UnitTypes::Zerg_Spire] > 0 && allUnitCount[BWAPI::UnitTypes::Zerg_Spire] + allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] == 1) ? 3 : 1) + 1) * (allUnitCount[BWAPI::UnitTypes::Zerg_Ultralisk_Cavern] > 0 ? BWAPI::UnitTypes::Zerg_Ultralisk.supplyRequired() : (allUnitCount[BWAPI::UnitTypes::Zerg_Spire] + allUnitCount[BWAPI::UnitTypes::Zerg_Greater_Spire] + allUnitCount[BWAPI::UnitTypes::Zerg_Defiler_Mound] + allUnitCount[BWAPI::UnitTypes::Zerg_Queens_Nest] + allUnitCount[BWAPI::UnitTypes::Zerg_Hydralisk_Den] > 0 ? BWAPI::UnitTypes::Zerg_Mutalisk.supplyRequired() : BWAPI::UnitTypes::Zerg_Drone.supplyRequired()))) > Broodwar->self()->supplyTotal() + (incompleteUnitCount[supplyProviderType] * supplyProviderType.supplyProvided()) &&
                 (isStartedTransitioning || (supplyUsed + 2 > Broodwar->self()->supplyTotal() + (incompleteUnitCount[supplyProviderType] * supplyProviderType.supplyProvided())))))
            {
                static int lastIssuedBuildSupplyProviderCmd = 0;
                if (Broodwar->getFrameCount() >= lastIssuedBuildSupplyProviderCmd + (10 * 24))
                {
                    // Retrieve a unit that is capable of constructing the supply needed
                    Unit supplyBuilder = u->getClosestUnit(GetType == supplyProviderType.whatBuilds().first && (IsIdle || IsGatheringMinerals) && IsOwned);
                    // If a unit was found
                    if (supplyBuilder)
                    {
                        if (supplyProviderType.isBuilding())
                        {
                            const TilePosition targetBuildLoc = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
                            if (Broodwar->isValid(targetBuildLoc) && supplyBuilder->canBuild(supplyProviderType, targetBuildLoc))
                            {
                                // Order the builder to construct the supply structure
                                supplyBuilder->build(supplyProviderType, targetBuildLoc);
                                lastIssuedBuildSupplyProviderCmd = Broodwar->getFrameCount();
                                continue;
                            }
                        }
                        else if (supplyBuilder->canTrain(supplyProviderType))
                        {
                            // Train the supply provider (Zerg_Overlord) if the provider is not a structure
                            supplyBuilder->train(supplyProviderType);
                            lastIssuedBuildSupplyProviderCmd = Broodwar->getFrameCount();
                            continue;
                        }
                    }
                }
            }
        }
        // Attempt to detect and fix bugged (frozen) ground units caused by bug in Broodwar.
        // TODO: improve this to speed up detection/fix.
        else if (u->canStop() &&
                 u->canAttack() &&
                 u->canMove() &&
                 !u->isFlying() &&
                 !u->isAttacking() &&
                 (int) getClientInfo(u, frameLastStoppedInd) + (3 * 24) < Broodwar->getFrameCount() &&
                 (int) getClientInfo(u, frameLastAttackingInd) + std::max(Broodwar->self()->weaponDamageCooldown(u->getType()), u->getType().airWeapon().damageCooldown()) + (3 * 24) < Broodwar->getFrameCount() &&
                 (int) getClientInfo(u, frameLastChangedPosInd) > 0 && (int) getClientInfo(u, frameLastChangedPosInd) + (3 * 24) < Broodwar->getFrameCount() &&
                 noCmdPending(u))
        {
            u->stop();
            setClientInfo(u, Broodwar->getFrameCount(), frameLastStoppedInd);
            continue;
        }
        else if (u->canAttack() &&
                 // Add some frames to cover frame(s) that might be needed to change direction.
                 (std::max(u->getGroundWeaponCooldown(), u->getAirWeaponCooldown()) > 0 ? std::max(u->getGroundWeaponCooldown(), u->getAirWeaponCooldown()) > Broodwar->getRemainingLatencyFrames() + 2 : !u->isAttackFrame()) &&
                 noCmdPending(u))
        {
            // I.E. in-range enemy unit that is a threat to this particular unit
            // (so for example, an enemy zergling is not a threat to my mutalisk).
            const BWAPI::Unit bestAttackableInRangeEnemySelfThreatUnit =
                // Could also take into account higher ground advantage, cover advantage (e.g. in trees), HP regen, shields regen,
                // effects of spells like dark swarm. The list is endless.
                Broodwar->getBestUnit(
                    getBestEnemyThreatUnitLambda,
                    IsEnemy && IsVisible && IsDetected && Exists &&
                    !IsWorker &&
                    // Warning: some calls like tmpUnit->canAttack(tmpUnit2) and tmpUnit2->isVisible(tmpUnit->getPlayer())
                    // will always return false because tmpUnit is not commandable by Broodwar->self() and BWAPI doesn't seem to update
                    // unit visibility info correctly for other players than Broodwar->self().
                    // I check !IsLockedDown etc becuase rather than attacking them we would rather fall through and attack workers if possible.
                    !IsLockedDown && !IsMaelstrommed && !IsStasised &&
                    (CanAttack ||
                     GetType == BWAPI::UnitTypes::Terran_Bunker ||
                     GetType == BWAPI::UnitTypes::Protoss_High_Templar ||
                     GetType == BWAPI::UnitTypes::Zerg_Defiler ||
                     GetType == BWAPI::UnitTypes::Protoss_Dark_Archon ||
                     GetType == BWAPI::UnitTypes::Terran_Science_Vessel ||
                     GetType == BWAPI::UnitTypes::Zerg_Queen ||
                     GetType == BWAPI::UnitTypes::Protoss_Shuttle ||
                     GetType == BWAPI::UnitTypes::Terran_Dropship ||
                     // TODO: re-enable Protoss_Observer after add any logic to produce cloaked units. It could also be used for scouting but never mind.
                     //GetType == BWAPI::UnitTypes::Protoss_Observer ||
                     // TODO: re-enable Zerg_Overlord after add any logic to produce cloaked units. It could also be used for scouting and transport but never mind.
                     //GetType == BWAPI::UnitTypes::Zerg_Overlord ||
                     GetType == BWAPI::UnitTypes::Terran_Medic ||
                     GetType == BWAPI::UnitTypes::Terran_Nuclear_Silo ||
                     GetType == BWAPI::UnitTypes::Zerg_Nydus_Canal /*||
                     // TODO: re-enable Terran_Comsat_Station after add any
                     // logic to produce cloaked units.
                     GetType == BWAPI::UnitTypes::Terran_Comsat_Station*/) &&
                    [&u](Unit& tmpUnit)
                    {
                        return
                            u->canAttack(tmpUnit) &&
                            u->isInWeaponRange(tmpUnit) &&
                            // TODO: add special logic for zerglings.
                            (tmpUnit->getType() != BWAPI::UnitTypes::Terran_Bunker ||
                             u->getType() != BWAPI::UnitTypes::Zerg_Zergling) &&
                            (!tmpUnit->getType().canAttack() ||
                             (!u->isFlying() ? tmpUnit->getType().groundWeapon() : tmpUnit->getType().airWeapon()) != BWAPI::WeaponTypes::None);
                    },
                    u->getPosition(),
                    std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + std::max(Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()), Broodwar->self()->weaponMaxRange(u->getType().airWeapon())));

            if (bestAttackableInRangeEnemySelfThreatUnit)
            {
                const BWAPI::Unit oldOrderTarget = u->getTarget();
                if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableInRangeEnemySelfThreatUnit)
                {
                    u->attack(bestAttackableInRangeEnemySelfThreatUnit);
                }
                continue;
            }

            // I.E. a nearby enemy unit that is a threat to this particular unit
            // (so for example, an enemy zergling is not a threat to my mutalisk).
            const BWAPI::Unit bestAttackableEnemySelfThreatUnit =
                // Could also take into account higher ground advantage, cover advantage (e.g. in trees), HP regen, shields regen,
                // effects of spells like dark swarm. The list is endless.
                Broodwar->getBestUnit(
                    getBestEnemyThreatUnitLambda,
                    IsEnemy && IsVisible && IsDetected && Exists &&
                    !IsWorker &&
                    // Warning: some calls like tmpUnit->canAttack(tmpUnit2) and tmpUnit2->isVisible(tmpUnit->getPlayer())
                    // will always return false because tmpUnit is not commandable by Broodwar->self() and BWAPI doesn't seem to update
                    // unit visibility info correctly for other players than Broodwar->self().
                    // I check !IsLockedDown etc becuase rather than attacking them we would rather fall through and attack workers if possible.
                    !IsLockedDown && !IsMaelstrommed && !IsStasised &&
                    (CanAttack ||
                     GetType == BWAPI::UnitTypes::Terran_Bunker ||
                     GetType == BWAPI::UnitTypes::Protoss_High_Templar ||
                     GetType == BWAPI::UnitTypes::Zerg_Defiler ||
                     GetType == BWAPI::UnitTypes::Protoss_Dark_Archon ||
                     GetType == BWAPI::UnitTypes::Terran_Science_Vessel ||
                     GetType == BWAPI::UnitTypes::Zerg_Queen ||
                     GetType == BWAPI::UnitTypes::Protoss_Shuttle ||
                     GetType == BWAPI::UnitTypes::Terran_Dropship ||
                     // TODO: re-enable Protoss_Observer after add any logic to produce cloaked units. It could also be used for scouting but never mind.
                     //GetType == BWAPI::UnitTypes::Protoss_Observer ||
                     // TODO: re-enable Zerg_Overlord after add any logic to produce cloaked units. It could also be used for scouting and transport but never mind.
                     //GetType == BWAPI::UnitTypes::Zerg_Overlord ||
                     GetType == BWAPI::UnitTypes::Terran_Medic ||
                     GetType == BWAPI::UnitTypes::Terran_Nuclear_Silo ||
                     GetType == BWAPI::UnitTypes::Zerg_Nydus_Canal /*||
                     // TODO: re-enable Terran_Comsat_Station after add any
                     // logic to produce cloaked units.
                     GetType == BWAPI::UnitTypes::Terran_Comsat_Station*/) &&
                    [&u, this](Unit& tmpUnit)
                    {
                        return
                            u->canAttack(tmpUnit) &&
                            // TODO: add special logic for zerglings.
                            (tmpUnit->getType() != BWAPI::UnitTypes::Terran_Bunker ||
                             (u->getType() != BWAPI::UnitTypes::Zerg_Zergling &&
                              // Ignore ghosts long range for now - assume there are marine(s) in the bunker.
                              // Only attack if we are in the bunker's range or we can out-range the bunker.
                              (tmpUnit->getDistance(u) <= tmpUnit->getPlayer()->weaponMaxRange(BWAPI::UnitTypes::Terran_Marine.groundWeapon()) ||
                               u->getPlayer()->weaponMaxRange(u->getType().groundWeapon()) > tmpUnit->getPlayer()->weaponMaxRange(BWAPI::UnitTypes::Terran_Marine.groundWeapon())))) &&
                            ((!tmpUnit->getType().canAttack() ||
                              (!u->isFlying() ? tmpUnit->getType().groundWeapon() : tmpUnit->getType().airWeapon()) != BWAPI::WeaponTypes::None) &&
                             tmpUnit->getDistance(u) <= (int) (std::max(std::max(tmpUnit->getPlayer()->weaponMaxRange(!u->isFlying() ? tmpUnit->getType().groundWeapon() : tmpUnit->getType().airWeapon()),
                                                                                 u->getPlayer()->weaponMaxRange(!tmpUnit->isFlying() ? u->getType().groundWeapon() : u->getType().airWeapon())),
                                                                        112)
                                                               + 32) &&
                             tmpUnit->getClosestUnit(
                                 Exists && GetPlayer == Broodwar->self(),
                                 (int) (std::max(std::max((!u->isFlying() ? tmpUnit->getPlayer()->weaponMaxRange(tmpUnit->getType().groundWeapon()) : tmpUnit->getPlayer()->weaponMaxRange(tmpUnit->getType().airWeapon())),
                                                          (!tmpUnit->isFlying() ? u->getPlayer()->weaponMaxRange(u->getType().groundWeapon()) : u->getPlayer()->weaponMaxRange(u->getType().airWeapon()))),
                                                 112))) != nullptr);
                    },
                    u->getPosition(),
                    // Note: 384 is the max range of any weapon (i.e. siege tank's weapon).
                    // FYI, the max sight range of any unit is 352, and the max seek range of any unit is 288.
                    std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + 384 + 112 + 32);

            if (bestAttackableEnemySelfThreatUnit)
            {
                const BWAPI::Unit oldOrderTarget = u->getTarget();
                if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableEnemySelfThreatUnit)
                {
                    u->attack(bestAttackableEnemySelfThreatUnit);
                }
                continue;
            }

            // Attack enemy worker targets of opportunity.
            const BWAPI::Unit bestAttackableInRangeEnemyWorkerUnit =
                Broodwar->getBestUnit(
                    getBestEnemyThreatUnitLambda,
                    IsEnemy && IsVisible && IsDetected && Exists && IsWorker &&
                    [&u](Unit& tmpUnit) { return u->canAttack(tmpUnit) && u->isInWeaponRange(tmpUnit); },
                    u->getPosition(),
                    std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + std::max(Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()), Broodwar->self()->weaponMaxRange(u->getType().airWeapon())));

            if (bestAttackableInRangeEnemyWorkerUnit)
            {
                const BWAPI::Unit oldOrderTarget = u->getTarget();
                if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableInRangeEnemyWorkerUnit)
                {
                    u->attack(bestAttackableInRangeEnemyWorkerUnit);
                }
                continue;
            }

            // Defend base if necessary, e.g. against worker rush, but base race rather than defend
            // if we have no workers left.
            if (allUnitCount[BWAPI::UnitTypes::Zerg_Drone] + numWorkersTrainedThisFrame + allUnitCount[BWAPI::UnitTypes::Zerg_Extractor] > 0)
            {
                BWAPI::Unit defenceAttackTargetUnit = nullptr;
                if (shouldDefend && workerAttackTargetUnit && u->canAttack(workerAttackTargetUnit))
                {
                    defenceAttackTargetUnit = workerAttackTargetUnit;
                }
                else if (Broodwar->self()->deadUnitCount(BWAPI::UnitTypes::Zerg_Drone) > (isScoutingWorkerReadyToScout ? 1 : 0) &&
                         mainBase != nullptr)
                {
                    // Defend my base (even if have to return all the way to my base) if my workers or a building
                    // are threatened e.g. by an enemy worker rush.
                    defenceAttackTargetUnit =
                        Broodwar->getBestUnit(
                            getBestEnemyThreatUnitLambda,
                            IsEnemy && IsVisible && IsDetected && Exists &&
                            CanAttack &&
                            !IsBuilding &&
                            [&u](Unit& tmpUnit)
                            {
                                return u->canAttack(tmpUnit);
                            },
                            mainBase->getPosition(),
                            896);
                }
    
                if (defenceAttackTargetUnit && u->canAttack(defenceAttackTargetUnit) &&
                    ((Broodwar->getFrameCount() < transitionOutOf4PoolFrameCountThresh && supplyUsed < 60) || u->getDistance(defenceAttackTargetUnit) < 896) &&
                    // I.E. during muta rush phase, don't defend with mutalisks against particular unit types - prefer to harass initially instead.
                    (!ss.isMutaRushBO || u->getType() != BWAPI::UnitTypes::Zerg_Mutalisk || Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Mutalisk) > 0 ||
                     (// Ignore vulture because their speed may make them too elusive.
                      defenceAttackTargetUnit->getType() != BWAPI::UnitTypes::Terran_Vulture &&
                      defenceAttackTargetUnit->getType() != BWAPI::UnitTypes::Terran_Goliath &&
                      defenceAttackTargetUnit->getType() != BWAPI::UnitTypes::Terran_Marine &&
                      defenceAttackTargetUnit->getType() != BWAPI::UnitTypes::Zerg_Scourge &&
                      defenceAttackTargetUnit->getType() != BWAPI::UnitTypes::Zerg_Queen)))
                {
                    const BWAPI::Unit oldOrderTarget = u->getTarget();
                    if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != defenceAttackTargetUnit)
                    {
                        u->attack(defenceAttackTargetUnit);
                    }
                    continue;
                }
            }

            // Continue statement to avoid sending out mutalisks if we are making guardians and
            // don't have enough guardians yet, because we don't want to kamikaze the mutalisks.
            if (u->getType() == BWAPI::UnitTypes::Zerg_Mutalisk &&
                allUnitCount[UnitTypes::Zerg_Guardian] <= 8 &&
                (!ss.isMutaRushBO || Broodwar->self()->deadUnitCount(UnitTypes::Zerg_Mutalisk) > 0) &&
                (!mainBaseAuto ||
                 u->getDistance(mainBaseAuto) < 512))
            {
                continue;
            }

            // We ignore stolen gas, at least until a time near when we plan to make an extractor.
            const Unit closestEnemyUnliftedBuildingAnywhere =
                u->getClosestUnit(
                    IsEnemy && IsVisible && Exists && IsBuilding && !IsLifted &&
                    isNotStolenGas);

            const BWAPI::Position closestEnemyUnliftedBuildingAnywherePos =
                closestEnemyUnliftedBuildingAnywhere ? closestEnemyUnliftedBuildingAnywhere->getPosition() : BWAPI::Positions::Unknown;

            if (closestEnemyUnliftedBuildingAnywhere)
            {
                // Distance multiplier is arbitrary - the value seems to result in ok movement behaviour.
                if (u->getDistance(closestEnemyUnliftedBuildingAnywhere) <=
                    (int) (std::max(std::max(Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()),
                                             Broodwar->self()->weaponMaxRange(u->getType().airWeapon())), 256) * 1))
                {
                    const BWAPI::Unit bestAttackableEnemyWorkerUnit =
                        Broodwar->getBestUnit(
                            getBestEnemyThreatUnitLambda,
                            IsEnemy && IsVisible && IsDetected && Exists && IsWorker &&
                            [&u, &closestEnemyUnliftedBuildingAnywhere, this](Unit& tmpUnit)
                            {
                                return u->canAttack(tmpUnit) &&
                                    tmpUnit->getDistance(u) <= (int) (224 + 32) &&
                                    tmpUnit->getDistance(closestEnemyUnliftedBuildingAnywhere) <= Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()) + 224 &&
                                    tmpUnit->getClosestUnit(Exists && GetPlayer == Broodwar->self(), (int) (224)) != nullptr;
                            },
                            u->getPosition(),
                            std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + 224 + 32);

                    if (bestAttackableEnemyWorkerUnit)
                    {
                        const BWAPI::Unit oldOrderTarget = u->getTarget();
                        if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableEnemyWorkerUnit)
                        {
                            u->attack(bestAttackableEnemyWorkerUnit);
                        }
                        continue;
                    }

                    const BWAPI::Unit bestAttackableInRangeEnemyTacticalUnit =
                        Broodwar->getBestUnit(
                            getBestEnemyThreatUnitLambda,
                            IsEnemy && IsVisible && IsDetected && Exists &&
                            !IsWorker &&
                            (CanAttack ||
                             GetType == BWAPI::UnitTypes::Terran_Bunker ||
                             GetType == BWAPI::UnitTypes::Protoss_High_Templar ||
                             GetType == BWAPI::UnitTypes::Zerg_Defiler ||
                             GetType == BWAPI::UnitTypes::Protoss_Dark_Archon ||
                             GetType == BWAPI::UnitTypes::Terran_Science_Vessel ||
                             GetType == BWAPI::UnitTypes::Zerg_Queen ||
                             GetType == BWAPI::UnitTypes::Protoss_Shuttle ||
                             GetType == BWAPI::UnitTypes::Terran_Dropship ||
                             // TODO: re-enable Protoss_Observer after add any logic to produce cloaked units. It could also be used for scouting but never mind.
                             //GetType == BWAPI::UnitTypes::Protoss_Observer ||
                             // TODO: re-enable Zerg_Overlord after add any logic to produce cloaked units. It could also be used for scouting and transport but never mind.
                             //GetType == BWAPI::UnitTypes::Zerg_Overlord ||
                             GetType == BWAPI::UnitTypes::Terran_Medic ||
                             GetType == BWAPI::UnitTypes::Terran_Nuclear_Silo ||
                             GetType == BWAPI::UnitTypes::Zerg_Nydus_Canal /*||
                             // TODO: re-enable Terran_Comsat_Station after add any
                             // logic to produce cloaked units.
                             GetType == BWAPI::UnitTypes::Terran_Comsat_Station*/) &&
                            [&u](Unit& tmpUnit) { return u->canAttack(tmpUnit) && u->isInWeaponRange(tmpUnit); },
                            u->getPosition(),
                            std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + std::max(Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()), Broodwar->self()->weaponMaxRange(u->getType().airWeapon())));

                    if (bestAttackableInRangeEnemyTacticalUnit)
                    {
                        const BWAPI::Unit oldOrderTarget = u->getTarget();
                        if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableInRangeEnemyTacticalUnit)
                        {
                            u->attack(bestAttackableInRangeEnemyTacticalUnit);
                        }
                        continue;
                    }

                    // Distance multiplier is arbitrary - the value seems to result in ok movement behaviour.
                    // Less than for closestAttackableEnemyThreatUnit because we would slightly prefer to attack
                    // closer enemy units that can't retaliate than further away ones that can.
                    const BWAPI::Unit bestAttackableEnemyTacticalUnit =
                        Broodwar->getBestUnit(
                            getBestEnemyThreatUnitLambda,
                            IsEnemy && IsVisible && IsDetected && Exists &&
                            !IsWorker &&
                            (CanAttack ||
                             GetType == BWAPI::UnitTypes::Terran_Bunker ||
                             GetType == BWAPI::UnitTypes::Protoss_High_Templar ||
                             GetType == BWAPI::UnitTypes::Zerg_Defiler ||
                             GetType == BWAPI::UnitTypes::Protoss_Dark_Archon ||
                             GetType == BWAPI::UnitTypes::Terran_Science_Vessel ||
                             GetType == BWAPI::UnitTypes::Zerg_Queen ||
                             GetType == BWAPI::UnitTypes::Protoss_Shuttle ||
                             GetType == BWAPI::UnitTypes::Terran_Dropship ||
                             GetType == BWAPI::UnitTypes::Protoss_Observer ||
                             GetType == BWAPI::UnitTypes::Zerg_Overlord ||
                             GetType == BWAPI::UnitTypes::Terran_Medic ||
                             GetType == BWAPI::UnitTypes::Terran_Nuclear_Silo ||
                             GetType == BWAPI::UnitTypes::Zerg_Nydus_Canal /*||
                             // TODO: re-enable Terran_Comsat_Station after add any
                             // logic to produce cloaked units.
                             GetType == BWAPI::UnitTypes::Terran_Comsat_Station*/) &&
                            [&u, &closestEnemyUnliftedBuildingAnywhere, this](Unit& tmpUnit)
                            {
                                return
                                    u->canAttack(tmpUnit) &&
                                    (tmpUnit == closestEnemyUnliftedBuildingAnywhere ||
                                     tmpUnit->getDistance(closestEnemyUnliftedBuildingAnywhere) <=
                                         (!tmpUnit->isFlying() ? Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()) : Broodwar->self()->weaponMaxRange(u->getType().airWeapon()))
                                         + 224);
                            },
                            u->getPosition(),
                            std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + (int) (std::max(std::max(Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()),
                                                     Broodwar->self()->weaponMaxRange(u->getType().airWeapon())),
                                            96)
                                   * 1));

                    if (bestAttackableEnemyTacticalUnit)
                    {
                        const BWAPI::Unit oldOrderTarget = u->getTarget();
                        if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableEnemyTacticalUnit)
                        {
                            u->attack(bestAttackableEnemyTacticalUnit);
                        }
                        continue;
                    }

                    // Attack buildings if in range. May be useful against wall-ins.
                    // Commenting-out for the time being.
                    /*const BWAPI::Unit bestAttackableInRangeEnemyNonWorkerUnit =
                        Broodwar->getBestUnit(
                            getBestEnemyThreatUnitLambda,
                            IsEnemy && IsVisible && IsDetected && Exists && !IsWorker &&
                            [&u](Unit& tmpUnit) { return u->canAttack(tmpUnit) && u->isInWeaponRange(tmpUnit); },
                            u->getPosition(),
                            std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + std::max(Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()), Broodwar->self()->weaponMaxRange(u->getType().airWeapon())));
                    if (bestAttackableInRangeEnemyNonWorkerUnit)
                    {
                        const BWAPI::Unit oldOrderTarget = u->getTarget();
                        if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableInRangeEnemyNonWorkerUnit)
                        {
                            u->attack(bestAttackableInRangeEnemyNonWorkerUnit);
                        }
                        continue;
                    }*/

                    // Distance multiplier should be the same as for closestEnemyUnliftedBuildingAnywhere.
                    const BWAPI::Unit bestAttackableEnemyNonWorkerUnit =
                        Broodwar->getBestUnit(
                            getBestEnemyThreatUnitLambda,
                            IsEnemy && IsVisible && IsDetected && Exists && !IsWorker &&
                            [&u, &closestEnemyUnliftedBuildingAnywhere, this](Unit& tmpUnit)
                            {
                                return
                                    u->canAttack(tmpUnit) &&
                                    (tmpUnit == closestEnemyUnliftedBuildingAnywhere ||
                                     tmpUnit->getDistance(closestEnemyUnliftedBuildingAnywhere) <=
                                         (!tmpUnit->isFlying() ? Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()) : Broodwar->self()->weaponMaxRange(u->getType().airWeapon()))
                                         + 224);
                            },
                            u->getPosition(),
                            std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + (int) (std::max(std::max(Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()),
                                                     Broodwar->self()->weaponMaxRange(u->getType().airWeapon())),
                                            256)
                                   * 1));
                    if (bestAttackableEnemyNonWorkerUnit)
                    {
                        const BWAPI::Unit oldOrderTarget = u->getTarget();
                        if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != bestAttackableEnemyNonWorkerUnit)
                        {
                            u->attack(bestAttackableEnemyNonWorkerUnit);
                        }
                        continue;
                    }
                }
            }

            if (u != scoutingWorker &&
                u != scoutingZergling &&
                ((ss.isSpeedlingBO &&
                  (Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost) != Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Metabolic_Boost) ||
                   (ss.isSpeedlingPushDeferred && Broodwar->getFrameCount() < 5300))) ||
                 (ss.isHydraRushBO &&
                  (Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Grooved_Spines) != Broodwar->self()->getMaxUpgradeLevel(BWAPI::UpgradeTypes::Grooved_Spines)))))
            {
                continue;
            }

            // Populate targetPos and/or targetStartLocs then we will decide which to use (or something else).
            BWAPI::Position targetPos = BWAPI::Positions::Unknown;
            std::vector<BWAPI::TilePosition> targetStartLocs;

            if (targetPos == BWAPI::Positions::Unknown &&
                closestEnemyUnliftedBuildingAnywherePos != BWAPI::Positions::Unknown && closestEnemyUnliftedBuildingAnywherePos != BWAPI::Positions::None)
            {
                targetPos = closestEnemyUnliftedBuildingAnywherePos;
            }

            if (targetPos == BWAPI::Positions::Unknown)
            {
                BWAPI::Position bestPos = BWAPI::Positions::Unknown;
                int closestDist = std::numeric_limits<int>::max();
                for (const BWAPI::Position pos : lastKnownEnemyUnliftedBuildingsAnywherePosSet)
                {
                    const int dist = u->getDistance(pos);
                    if (dist < closestDist)
                    {
                        bestPos = pos;
                        closestDist = dist;
                    }
                }

                if (bestPos != BWAPI::Positions::Unknown)
                {
                    targetPos = bestPos;
                }
            }


            if (targetStartLocs.empty() && !enemyStartLocs.empty())
            {
                // For simplicity, let's only attack the first enemy (rather than randomizing to pick one & remembering
                // what we picked for future frames or possibly splitting our army).
                // TODO: skip allies?
                targetStartLocs.push_back(*enemyStartLocs.begin());
            }

            if (targetStartLocs.empty() && probableEnemyStartLoc != BWAPI::TilePositions::Unknown)
            {
                targetStartLocs.push_back(probableEnemyStartLoc);
            }

            if (targetStartLocs.empty() && !unscoutedOtherStartLocs.empty())
            {
                const int tmpX = (int) getClientInfo(u, scoutingTargetStartLocXInd);
                const int tmpY = (int) getClientInfo(u, scoutingTargetStartLocYInd);
                if ((tmpX != 0 || tmpY != 0) &&
                    unscoutedOtherStartLocs.find(TilePosition(tmpX, tmpY)) != unscoutedOtherStartLocs.end())
                {
                    targetStartLocs.push_back(TilePosition(tmpX, tmpY));
                }
                else
                {
                    for (const BWAPI::TilePosition loc : unscoutedOtherStartLocs)
                    {
                        targetStartLocs.push_back(loc);
                    }
                }
            }
            
            if (targetPos != BWAPI::Positions::Unknown && !targetStartLocs.empty() && u->isFlying() && u->getType().groundWeapon() != BWAPI::WeaponTypes::None)
            {
                // This makes mutalisks/guardians head towards the possible enemy base rather than towards the closest
                // of the last seen enemy buildings.
                targetPos = BWAPI::Positions::Unknown;
            }

            // Attack enemy lifted buildings.
            if (targetPos == BWAPI::Positions::Unknown && targetStartLocs.empty() &&
                u->getType().airWeapon() != BWAPI::WeaponTypes::None)
            {
                const BWAPI::Unit closestAttackableEnemyLiftedBuildingUnit =
                    u->getClosestUnit(
                        IsEnemy && IsVisible && Exists && IsLifted &&
                        [&u](Unit& tmpUnit) { return u->canAttack(tmpUnit); } );

                if (closestAttackableEnemyLiftedBuildingUnit)
                {
                    const BWAPI::Unit oldOrderTarget = u->getTarget();
                    if (u->isIdle() || oldOrderTarget == nullptr || oldOrderTarget != closestAttackableEnemyLiftedBuildingUnit)
                    {
                        u->attack(closestAttackableEnemyLiftedBuildingUnit);
                    }
                    continue;
                }
            }

            if (targetPos == BWAPI::Positions::Unknown && targetStartLocs.empty())
            {
                const int tmpX = (int) getClientInfo(u, scoutingTargetPosXInd);
                const int tmpY = (int) getClientInfo(u, scoutingTargetPosYInd);
                // If en-route to a position that isn't visible or isn't clear then continue going there.
                // Occasionally re-randomize late-game cos the unit may not have a path to get there.
                if ((tmpX != 0 || tmpY != 0) &&
                    Broodwar->getFrameCount() % (60 * 24) >= 6 &&
                    (!Broodwar->isVisible(TilePosition(Position(tmpX, tmpY))) ||
                     !Broodwar->getUnitsOnTile(TilePosition(Position(tmpX, tmpY)), IsEnemy && IsVisible && Exists && IsBuilding && !IsLifted).empty()))
                {
                    targetPos = Position(tmpX, tmpY);
                }
                else
                {
                    // Target a random position - preferably one that is not visible.
                    BWAPI::Position pos;
                    for (int i = 0; i < 10; ++i)
                    {
                        pos =
                            Position(rand() % (Broodwar->mapWidth() * BWAPI::TILEPOSITION_SCALE),
                                     rand() % (Broodwar->mapHeight() * BWAPI::TILEPOSITION_SCALE));

                        if (!Broodwar->isVisible(TilePosition(pos)))
                        {
                            break;
                        }
                    }

                    targetPos = pos;
                }
            }

            if (targetPos != BWAPI::Positions::Unknown || !targetStartLocs.empty())
            {
                BWAPI::Position pos = BWAPI::Positions::None;
                BWAPI::TilePosition locIfAny = BWAPI::TilePositions::None;
                if (targetPos != BWAPI::Positions::Unknown)
                {
                    pos = targetPos;
                }
                else if (targetStartLocs.size() == 1)
                {
                    locIfAny = targetStartLocs.front();
                }
                else
                {
                    // Target the closest target position that has less than a certain number of units
                    // assigned to it, or if they all have at least that amount then target the one that
                    // has the least assigned to it.
                    std::sort(
                        targetStartLocs.begin(),
                        targetStartLocs.end(),
                        [&u, &getRoughPos](const BWAPI::TilePosition loc1, const BWAPI::TilePosition loc2)
                        {
                            return (u->getDistance(getRoughPos(loc1, BWAPI::UnitTypes::Special_Start_Location)) < u->getDistance(getRoughPos(loc2, BWAPI::UnitTypes::Special_Start_Location)));
                        });
    
                    BWAPI::TilePosition startLocWithFewestUnits = BWAPI::TilePositions::None;
                    for (const BWAPI::TilePosition startLoc : targetStartLocs)
                    {
                        if (((u != scoutingWorker && u != scoutingZergling) || possibleOverlordScoutLocs.find(startLoc) != possibleOverlordScoutLocs.end()) /*&&
                            (numNonOverlordUnitsTargetingStartLoc.find(startLoc) == numNonOverlordUnitsTargetingStartLoc.end() ||
                             numNonOverlordUnitsTargetingStartLoc.at(startLoc) < 6)*/)
                        {
                            locIfAny = startLoc;
                            ++numNonOverlordUnitsTargetingStartLoc[locIfAny];
                            break;
                        }
                        else if (startLocWithFewestUnits == BWAPI::TilePositions::None ||
                                 (numNonOverlordUnitsTargetingStartLoc.find(startLocWithFewestUnits) != numNonOverlordUnitsTargetingStartLoc.end() &&
                                  (numNonOverlordUnitsTargetingStartLoc.find(startLoc) == numNonOverlordUnitsTargetingStartLoc.end() ||
                                   numNonOverlordUnitsTargetingStartLoc.at(startLoc) < numNonOverlordUnitsTargetingStartLoc.at(startLocWithFewestUnits))))
                        {
                            startLocWithFewestUnits = startLoc;
                        }
                    }
    
                    if (locIfAny == BWAPI::TilePositions::None && startLocWithFewestUnits != BWAPI::TilePositions::None)
                    {
                        locIfAny = startLocWithFewestUnits;
                        ++numNonOverlordUnitsTargetingStartLoc[locIfAny];
                    }
                }

                if (pos == BWAPI::Positions::None && locIfAny != BWAPI::TilePositions::None)
                {
                    pos = getRoughPos(locIfAny, BWAPI::UnitTypes::Special_Start_Location);
                }

                if (pos != BWAPI::Positions::None)
                {
                    bool isCmdIssued = false;
                    if (u->canRightClick(pos))
                    {
                        // Dunno if rightClick'ing rather than moving is ever beneficial in these scenarios
                        // or whether it is possible to be able to do one but not the other, but let's prefer
                        // rightClick'ing over moving just in case (although it would probably only possibly
                        // matter if the command optimization option level is zero).
                        u->rightClick(pos);
                        isCmdIssued = true;
                    }    
                    else if (u->canMove())
                    {
                        u->move(pos);
                        isCmdIssued = true;
                    }

                    if (isCmdIssued)
                    {
                        if (locIfAny == BWAPI::TilePositions::None)
                        {
                            setClientInfo(u, pos.x, scoutingTargetPosXInd);
                            setClientInfo(u, pos.y, scoutingTargetPosYInd);
                        }
                        else
                        {
                            setClientInfo(u, locIfAny.x, scoutingTargetStartLocXInd);
                            setClientInfo(u, locIfAny.y, scoutingTargetStartLocYInd);
                        }

                        // Using a continue statement because we have just issued a command to this unit.
                        continue;
                    }
                }
            }
        }
        else if (u->getType() == UnitTypes::Zerg_Overlord)
        {
            if (!noCmdPending(u))
            {
                continue;
            }

            const BWAPI::Position oldTargetPos = u->getTargetPosition();

            // After we have started pulling an overlord back to base, don't send it out again.
            if (myStartRoughPos != BWAPI::Positions::Unknown && oldTargetPos == myStartRoughPos)
            {
                continue;
            }

            BWAPI::Position targetPos = BWAPI::Positions::Unknown;
            BWAPI::TilePosition targetStartLoc = BWAPI::TilePositions::Unknown;

            // If we know where an enemy starting location is and an enemy is known to be Terran race
            // then don't risk sending out overlords / return them back to our base for safety.
            // If being attacked then return back to our base for safety.
            // Note: isUnderAttack is misleading / not reliable as mentioned elsewhere in this src but never mind.
            // Return overlords back to our base for safety after a few minutes of in-game time or if we see a risk
            // (or against a Terran, when see any enemy building or roughly when a Terran could get first marine).
            if (myStartRoughPos != BWAPI::Positions::Unknown &&
                (!enemyStartLocs.empty() ||
                 (isARemainingEnemyTerran && otherStartLocs.size() == 1) ||
                 (isARemainingEnemyTerran && (!lastKnownEnemyUnliftedBuildingsAnywherePosSet.empty() || ((ss.isSpeedlingBO || ss.isHydraRushBO) ? probableEnemyStartLoc != BWAPI::TilePositions::Unknown : Broodwar->getFrameCount() >= 2600))) ||
                 u->isUnderAttack() ||
                 Broodwar->getBestUnit(
                     getBestEnemyThreatUnitLambda,
                     IsEnemy && IsVisible && Exists && !IsWorker &&
                     (CanAttack ||
                      // Pull overlords back if we see special buildings like hydra den that are likely to mean
                      // the enemy will produce units that can kill the overlord somehow (e.g. even psi storm).
                      // Not Terran_Barracks because there is already logic based on frame count to cover marines.
                      //GetType == BWAPI::UnitTypes::Terran_Barracks ||
                      GetType == BWAPI::UnitTypes::Zerg_Hydralisk_Den ||
                      GetType == BWAPI::UnitTypes::Protoss_Stargate ||
                      GetType == BWAPI::UnitTypes::Terran_Starport ||
                      GetType == BWAPI::UnitTypes::Terran_Control_Tower ||
                      GetType == BWAPI::UnitTypes::Zerg_Spire ||
                      GetType == BWAPI::UnitTypes::Zerg_Greater_Spire ||
                      GetType == BWAPI::UnitTypes::Protoss_Fleet_Beacon ||
                      GetType == BWAPI::UnitTypes::Protoss_Arbiter_Tribunal ||
                      GetType == BWAPI::UnitTypes::Terran_Science_Facility ||
                      GetType == BWAPI::UnitTypes::Terran_Physics_Lab ||
                      GetType == BWAPI::UnitTypes::Terran_Covert_Ops ||
                      GetType == BWAPI::UnitTypes::Terran_Nuclear_Silo ||
                      GetType == BWAPI::UnitTypes::Protoss_Templar_Archives ||
                      GetType == BWAPI::UnitTypes::Terran_Bunker ||
                      GetType == BWAPI::UnitTypes::Protoss_High_Templar ||
                      GetType == BWAPI::UnitTypes::Zerg_Defiler ||
                      GetType == BWAPI::UnitTypes::Protoss_Dark_Archon ||
                      GetType == BWAPI::UnitTypes::Terran_Science_Vessel ||
                      GetType == BWAPI::UnitTypes::Zerg_Queen ||
                      GetType == BWAPI::UnitTypes::Protoss_Shuttle ||
                      GetType == BWAPI::UnitTypes::Terran_Dropship ||
                      //GetType == BWAPI::UnitTypes::Protoss_Observer ||
                      //GetType == BWAPI::UnitTypes::Zerg_Overlord ||
                      GetType == BWAPI::UnitTypes::Terran_Medic ||
                      GetType == BWAPI::UnitTypes::Terran_Nuclear_Silo ||
                      GetType == BWAPI::UnitTypes::Zerg_Nydus_Canal /*||
                      GetType == BWAPI::UnitTypes::Terran_Comsat_Station*/) &&
                     [&u](Unit& tmpUnit) { return !tmpUnit->getType().canAttack() || tmpUnit->getType().airWeapon() != BWAPI::WeaponTypes::None; },
                     u->getPosition(),
                     std::max(u->getType().dimensionLeft(), std::max(u->getType().dimensionUp(), std::max(u->getType().dimensionRight(), u->getType().dimensionDown()))) + (int) (std::max(std::max(Broodwar->self()->weaponMaxRange(u->getType().groundWeapon()),
                                                 Broodwar->self()->weaponMaxRange(u->getType().airWeapon())),
                                        1024)
                                   * 1)) != nullptr))
            {
                targetPos = myStartRoughPos;
            }

            if (targetPos == BWAPI::Positions::Unknown && targetStartLoc == BWAPI::TilePositions::Unknown && !unscoutedOtherStartLocs.empty() &&
                ((ss.isSpeedlingBO || ss.isHydraRushBO) || Broodwar->getFrameCount() < (5 * 60 * 24)))
            {
                const int tmpX = (int) getClientInfo(u, scoutingTargetStartLocXInd);
                const int tmpY = (int) getClientInfo(u, scoutingTargetStartLocYInd);
                if ((tmpX != 0 || tmpY != 0) &&
                    unscoutedOtherStartLocs.find(TilePosition(tmpX, tmpY)) != unscoutedOtherStartLocs.end())
                {
                    targetStartLoc = TilePosition(tmpX, tmpY);
                }
                else
                {
                    if (!possibleOverlordScoutLocs.empty())
                    {
                        int closestOtherStartPosDistance = std::numeric_limits<int>::max();
                        for (const BWAPI::TilePosition loc : possibleOverlordScoutLocs)
                        {
                            const int dist = u->getDistance(getRoughPos(loc, BWAPI::UnitTypes::Special_Start_Location));
                            if (dist < closestOtherStartPosDistance)
                            {
                                targetStartLoc = loc;
                                closestOtherStartPosDistance = dist;
                            }
                        }
                    }
                }
            }

            if (targetPos == BWAPI::Positions::Unknown && targetStartLoc == BWAPI::TilePositions::Unknown)
            {
                // Commented out randomizing - for now let's return overlords back to our base for safety.
                //// Target a random position.
                //targetPos = Position(rand() % (Broodwar->mapWidth() * BWAPI::TILEPOSITION_SCALE),
                //                     rand() % (Broodwar->mapHeight() * BWAPI::TILEPOSITION_SCALE));
                if (myStartRoughPos != BWAPI::Positions::Unknown)
                {
                    targetPos = myStartRoughPos;
                }
            }

            if (targetPos == BWAPI::Positions::Unknown && targetStartLoc != BWAPI::TilePositions::Unknown)
            {
                targetPos = getRoughPos(targetStartLoc, BWAPI::UnitTypes::Special_Start_Location);
            }

            if (targetPos != BWAPI::Positions::Unknown)
            {
                bool isCmdIssued = false;
                if (u->canRightClick(targetPos))
                {
                    // Dunno if rightClick'ing rather than moving is ever beneficial in these scenarios
                    // or whether it is possible to be able to do one but not the other, but let's prefer
                    // rightClick'ing over moving just in case (although it would probably only possibly
                    // matter if the command optimization option level is zero).
                    u->rightClick(targetPos);
                    isCmdIssued = true;
                }    
                else if (u->canMove())
                {
                    u->move(targetPos);
                    isCmdIssued = true;
                }

                if (isCmdIssued)
                {
                    if (targetStartLoc == BWAPI::TilePositions::Unknown)
                    {
                        setClientInfo(u, targetPos.x, scoutingTargetPosXInd);
                        setClientInfo(u, targetPos.y, scoutingTargetPosYInd);
                    }
                    else
                    {
                        possibleOverlordScoutLocs.erase(targetStartLoc);
                        setClientInfo(u, targetStartLoc.x, scoutingTargetStartLocXInd);
                        setClientInfo(u, targetStartLoc.y, scoutingTargetStartLocYInd);
                    }

                    // Using a continue statement because we have just issued a command to this unit.
                    continue;
                }
            }
        }
    }

    // Mineral gathering commands.
    if (!myFreeGatherers.empty())
    {
        // The first stage assigns free gatherers to free mineral patches near our starting base:
        // Prioritise the mineral patches according to shortest distance to our starting base
        // (note that the set of free mineral patches shrinks while this algorithm is executing,
        // i.e. remove a mineral patch from the set whenever a gatherer is assigned to that patch,
        // to avoid multiple gatherers being assigned to the same mineral patch that was initially free,
        // at least until the next stage),
        // then combinations of free gatherer and mineral patch are prioritised according to shortest
        // total distance from the gatherer to the mineral patch plus mineral patch to our starting base.
        //
        // The second stage simply assigns each remaining gatherer to whatever mineral patch is
        // closest to our starting base. If there are multiple mineral patches nearby our starting base
        // that are the same distance to our starting base then ties are broken by picking the mineral
        // patch closest to the gatherer (i.e. unfortunately in many cases they will all be assigned
        // to the same mineral patch but never mind).
        if (mainBase)
        {
            Unitset& freeMinerals =
                mainBase->getUnitsInRadius(
                    256,
                    BWAPI::Filter::IsMineralField &&
                    BWAPI::Filter::Exists &&
                    [&myFreeGatherers, &resourceToGathererMapAuto, &gathererToResourceMapAuto](BWAPI::Unit& tmpUnit)
                    {
                        if (tmpUnit->getResources() <= 0)
                        {
                            return false;
                        }
    
                        std::map<const BWAPI::Unit, BWAPI::Unit>::iterator resourceToGathererMapIter = resourceToGathererMapAuto.find(tmpUnit);
                        if (resourceToGathererMapIter == resourceToGathererMapAuto.end())
                        {
                            return true;
                        }
    
                        BWAPI::Unit& gatherer = resourceToGathererMapIter->second;
                        if (!gatherer->exists() ||
                            myFreeGatherers.contains(gatherer) ||
                            // Commented this out because it was causing workers' paths to cross (inefficient?).
                            //// Override workers that are currently returning from a mineral patch to a depot.
                            //gatherer->getOrder() == BWAPI::Orders::ResetCollision ||
                            //gatherer->getOrder() == BWAPI::Orders::ReturnMinerals ||
                            // Override workers that are not currently gathering minerals.
                            !gatherer->isGatheringMinerals())
                        {
                            return true;
                        }
    
                        std::map<const BWAPI::Unit, BWAPI::Unit>::iterator gathererToResourceMapIter = gathererToResourceMapAuto.find(gatherer);
                        if (gathererToResourceMapIter == gathererToResourceMapAuto.end())
                        {
                            return true;
                        }
    
                        BWAPI::Unit& resource = gathererToResourceMapIter->second;
                        return resource != tmpUnit;
                    });

            // Each element of the set is a mineral.
            struct MineralSet { std::set<BWAPI::Unit> val; };
            // The key is the total distance from mineral to depot plus distance from gatherer to mineral.
            struct TotCostMap { std::map<int, MineralSet> val; };
            // The key is the distance from mineral to depot.
            struct MineralToDepotCostMap { std::map<int, TotCostMap> val; };
            // The key is the gatherer.
            struct CostMap { std::map<const BWAPI::Unit, MineralToDepotCostMap> val; };
    
            CostMap costMap;
            bool isGatherPossible = false;

            // Fill in the cost map.
            for (auto& mineral : freeMinerals)
            {
                const int mineralToDepotCost = mineral->getDistance(mainBase);
                for (auto& gatherer : myFreeGatherers)
                {
                    if (gatherer->canGather(mineral))
                    {
                        costMap.val[gatherer].val[mineralToDepotCost].val[gatherer->getDistance(mineral) + mineralToDepotCost].val.emplace(mineral);
                        isGatherPossible = true;
                    }
                }
            }

            while (isGatherPossible)
            {
                isGatherPossible = false;
                BWAPI::Unit bestGatherer = nullptr;
                BWAPI::Unit bestMineral = nullptr;
                int bestMineralToDepotCost = std::numeric_limits<int>::max();
                int bestTotCost = std::numeric_limits<int>::max();

                for (std::map<const BWAPI::Unit, MineralToDepotCostMap>::iterator gathererIter = costMap.val.begin(); gathererIter != costMap.val.end(); )
                {
                    if (!myFreeGatherers.contains(gathererIter->first))
                    {
                        costMap.val.erase(gathererIter++);
                        continue;
                    }

                    bool isGatherPossibleForGatherer = false;
                    BWAPI::Unit mineral = nullptr;
                    int mineralToDepotCost = std::numeric_limits<int>::max();
                    int totCost = std::numeric_limits<int>::max();

                    for (std::map<int, TotCostMap>::iterator mineralToDepotCostIter = gathererIter->second.val.begin(); mineralToDepotCostIter != gathererIter->second.val.end(); )
                    {
                        bool isGatherPossibleForMineralToDepotCost = false;
                        if (mineralToDepotCostIter->first > bestMineralToDepotCost)
                        {
                            // Need to assume it is still possible.
                            isGatherPossibleForGatherer = true;
                            break;
                        }
    
                        for (std::map<int, MineralSet>::iterator totCostIter = mineralToDepotCostIter->second.val.begin(); totCostIter != mineralToDepotCostIter->second.val.end(); )
                        {
                            bool isGatherPossibleForTotCost = false;
                            if (mineralToDepotCostIter->first == bestMineralToDepotCost && totCostIter->first > bestTotCost)
                            {
                                // Need to assume it is still possible.
                                isGatherPossibleForMineralToDepotCost = true;
                                break;
                            }
    
                            for (std::set<BWAPI::Unit>::iterator mineralIter = totCostIter->second.val.begin(); mineralIter != totCostIter->second.val.end(); )
                            {
                                if (!freeMinerals.contains(*mineralIter))
                                {
                                    totCostIter->second.val.erase(mineralIter++);
                                    continue;
                                }

                                mineral = *mineralIter;
                                isGatherPossibleForTotCost = true;
                                mineralToDepotCost = mineralToDepotCostIter->first;
                                totCost = totCostIter->first;
                                break;
                            }

                            if (!isGatherPossibleForTotCost)
                            {
                                mineralToDepotCostIter->second.val.erase(totCostIter++);
                                continue;
                            }
                            else
                            {
                                isGatherPossibleForMineralToDepotCost = true;
                                break;
                            }
                        }

                        if (!isGatherPossibleForMineralToDepotCost)
                        {
                            gathererIter->second.val.erase(mineralToDepotCostIter++);
                            continue;
                        }
                        else
                        {
                            isGatherPossibleForGatherer = true;
                            break;
                        }
                    }

                    if (!isGatherPossibleForGatherer)
                    {
                        myFreeGatherers.erase(gathererIter->first);
                        costMap.val.erase(gathererIter++);
                        continue;
                    }
                    else
                    {
                        if (mineralToDepotCost < bestMineralToDepotCost || (mineralToDepotCost == bestMineralToDepotCost && totCost < bestTotCost))
                        {
                            isGatherPossible = true;
                            bestGatherer = gathererIter->first;
                            bestMineral = mineral;
                            bestMineralToDepotCost = mineralToDepotCost;
                            bestTotCost = totCost;
                        }

                        ++gathererIter;
                        continue;
                    }
                }

                if (isGatherPossible)
                {
                    bestGatherer->gather(bestMineral);

                    myFreeGatherers.erase(bestGatherer);
                    freeMinerals.erase(bestMineral);
                
                    if (gathererToResourceMap.find(bestGatherer) != gathererToResourceMap.end() && resourceToGathererMap.find(gathererToResourceMap.at(bestGatherer)) != resourceToGathererMap.end() && resourceToGathererMap.at(gathererToResourceMap.at(bestGatherer)) == bestGatherer)
                    {
                        resourceToGathererMap.erase(gathererToResourceMap.at(bestGatherer));
                    }
                
                    auto itr1 = resourceToGathererMap.find(bestGatherer);
                    if (itr1 == resourceToGathererMap.end())
                      resourceToGathererMap.insert(std::make_pair(bestMineral, bestGatherer));
                    else
                      itr1->second = bestGatherer;
                    //resourceToGathererMap[bestMineral] = bestGatherer;
                    auto itr2 = gathererToResourceMap.find(bestMineral);
                    if (itr2 == gathererToResourceMap.end())
                      gathererToResourceMap.insert(std::make_pair(bestGatherer, bestMineral));
                    else
                      itr1->second = bestMineral;
                    //gathererToResourceMap[bestGatherer] = bestMineral;
                    continue;
                }
            }

            for (auto& u : myFreeGatherers)
            {
                BWAPI::Unit mineralField = nullptr;
                mineralField = Broodwar->getBestUnit(
                    [&u, &mainBaseAuto](const BWAPI::Unit& bestSoFarUnit, const BWAPI::Unit& curUnit)
                    {
                        if (curUnit->getDistance(mainBaseAuto) != bestSoFarUnit->getDistance(mainBaseAuto))
                        {
                            return curUnit->getDistance(mainBaseAuto) < bestSoFarUnit->getDistance(mainBaseAuto) ? curUnit : bestSoFarUnit;
                        }

                        return u->getDistance(curUnit) < u->getDistance(bestSoFarUnit) ? curUnit : bestSoFarUnit;
                    },
                    BWAPI::Filter::IsMineralField &&
                    BWAPI::Filter::Exists &&
                    [&u](BWAPI::Unit& tmpUnit)
                    {
                        return tmpUnit->getResources() > 0 && u->canGather(tmpUnit);
                    },
                    mainBaseAuto->getPosition(),
                    std::max(mainBaseAuto->getType().dimensionLeft(), std::max(mainBaseAuto->getType().dimensionUp(), std::max(mainBaseAuto->getType().dimensionRight(), mainBaseAuto->getType().dimensionDown()))) + 256);

                if (mineralField == nullptr)
                {
                    mineralField = mainBase->getClosestUnit(
                        BWAPI::Filter::IsMineralField &&
                        BWAPI::Filter::Exists &&
                        [&u](BWAPI::Unit& tmpUnit)
                        {
                            return tmpUnit->getResources() > 0 && u->canGather(tmpUnit);
                        });
                }

                if (mineralField)
                {
                    u->gather(mineralField);
                
                    if (gathererToResourceMap.find(u) != gathererToResourceMap.end() && resourceToGathererMap.find(gathererToResourceMap.at(u)) != resourceToGathererMap.end() && resourceToGathererMap.at(gathererToResourceMap.at(u)) == u)
                    {
                        resourceToGathererMap.erase(gathererToResourceMap.at(u));
                    }
                    
                    auto itr1 = resourceToGathererMap.find(u);
                    if (itr1 == resourceToGathererMap.end())
                      resourceToGathererMap.insert(std::make_pair(mineralField, u));
                    else
                      itr1->second = u;
                    //resourceToGathererMap[mineralField] = u;
                    auto itr2 = gathererToResourceMap.find(mineralField);
                    if (itr2 == gathererToResourceMap.end())
                      gathererToResourceMap.insert(std::make_pair(u, mineralField));
                    else
                      itr2->second = mineralField;
                    //gathererToResourceMap[u] = mineralField;
                    continue;
                }
            }
        }
    }

    // Update client info for each of my units (so can check it in future frames).
    for (auto& u : myUnits)
    {
        if (u->exists() && u->isCompleted() && u->getType() != BWAPI::UnitTypes::Zerg_Larva && u->getType() != BWAPI::UnitTypes::Zerg_Egg)
        {
            const int newX = u->getPosition().x;
            const int newY = u->getPosition().y;
            if ((int) getClientInfo(u, posXInd) != newX || (int) getClientInfo(u, posYInd) != newY)
            {
                setClientInfo(u, Broodwar->getFrameCount(), frameLastChangedPosInd);
            }
    
            setClientInfo(u, newX, posXInd);
            setClientInfo(u, newY, posYInd);

            if (u->isAttacking())
            {
                setClientInfo(u, Broodwar->getFrameCount(), frameLastAttackingInd);
            }

            if (u->isAttackFrame())
            {
                setClientInfo(u, Broodwar->getFrameCount(), frameLastAttackFrameInd);
            }

            if (u->isStartingAttack())
            {
                setClientInfo(u, Broodwar->getFrameCount(), frameLastStartingAttackInd);
            }
    
            if (u->getType().isWorker() && u->isCarryingMinerals())
            {
                setClientInfo(u, wasJustCarryingMineralsTrueVal, wasJustCarryingMineralsInd);
            }

            if (u->getGroundWeaponCooldown() > (int) getClientInfo(u, lastGroundWeaponCooldownInd))
            {
                setClientInfo(u, u->getGroundWeaponCooldown(), lastPeakGroundWeaponCooldownInd);
                setClientInfo(u, Broodwar->getFrameCount(), lastPeakGroundWeaponCooldownFrameInd);
            }
            setClientInfo(u, u->getGroundWeaponCooldown(), lastGroundWeaponCooldownInd);
            
            if (u->getAirWeaponCooldown() > (int) getClientInfo(u, lastAirWeaponCooldownInd))
            {
                setClientInfo(u, u->getAirWeaponCooldown(), lastPeakAirWeaponCooldownInd);
                setClientInfo(u, Broodwar->getFrameCount(), lastPeakAirWeaponCooldownFrameInd);
            }
            setClientInfo(u, u->getAirWeaponCooldown(), lastAirWeaponCooldownInd);
        }
    }
}

void ZZZKBotAIModule::onSendText(std::string text)
{
    // Send the text to the game if it is not being processed.
    Broodwar->sendText("%s", text.c_str());

    // Make sure to use %s and pass the text as a parameter,
    // otherwise you may run into problems when you use the %(percent) character!
}

void ZZZKBotAIModule::onReceiveText(BWAPI::Player player, std::string text)
{
}

void ZZZKBotAIModule::onPlayerLeft(BWAPI::Player player)
{
    if (Broodwar->isReplay())
    {
        return;
    }

    if (player == nullptr)
    {
        return;
    }

    // I don't know if it's possible, but I am adding this check just in case it is needed if
    // it's possible that onPlayerLeft() might be called multiple times for the same player
    // for whatever reason, e.g. perhaps it could happen while the game is paused/unpaused?
    // Better safe than sorry because we do not want to spam the output file while the game is
    // paused.
    static std::set<PlayerID> playerIDsLeft;
    if (playerIDsLeft.find(player->getID()) != playerIDsLeft.end())
    {
        return;
    }
    playerIDsLeft.insert(player->getID());

    if (enemyPlayerID >= 0)
    {
        std::ostringstream oss;
    
        oss << startOfUpdateSentinel << delim;
    
        // Update type.
        oss << onPlayerLeftUpdateSignifier << delim;
    
        oss << Broodwar->getFrameCount() << delim;
        oss << Broodwar->elapsedTime() << delim;
    
        // Get time now.
        // Note: localtime_s is not portable but w/e.
        std::time_t timer = std::time(nullptr);
        oss << (timer - timerAtGameStart) << delim;
        struct tm buf;
        errno_t errNo = localtime_s(&buf, &timer);
        if (errNo == 0)
        {
            oss << std::put_time(&buf, "%Y-%m-%d");
        }
        oss << delim;
        if (errNo == 0)
        {
            oss << std::put_time(&buf, "%H:%M:%S");
        }
        oss << delim;

        oss << static_cast<int>(player->getID()) << delim;
        // TODO: player names may contain unusual characters, so sanitize by stripping or replacing them
        // (esp. tabs because I use tab as a delimiter).
        oss << player->getName() << delim;
    
        oss << endOfUpdateSentinel << delim;
    
        // Block to restrict scope of variables.
        {
            // Append to the file for the enemy in the write folder.
            std::ofstream enemyWriteFileOFS(enemyWriteFilePath, std::ios_base::out | std::ios_base::app);
            if (enemyWriteFileOFS)
            {
                enemyWriteFileOFS << oss.str();
                enemyWriteFileOFS.flush();
            }
        }
    }
}

void ZZZKBotAIModule::onNukeDetect(BWAPI::Position target)
{
    // Check if the target is a valid position
    if (Broodwar->isValid(target))
    {
        // if so, print the location of the nuclear strike target
        Broodwar << "Nuclear Launch Detected at " << target << std::endl;
    }
    else
    {
        Broodwar << "Nuclear Launch Detected at unknown position" << std::endl;
    }

    // You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void ZZZKBotAIModule::onUnitDiscover(BWAPI::Unit unit)
{
}

void ZZZKBotAIModule::onUnitEvade(BWAPI::Unit unit)
{
}

void ZZZKBotAIModule::onUnitShow(BWAPI::Unit unit)
{
}

void ZZZKBotAIModule::onUnitHide(BWAPI::Unit unit)
{
}

void ZZZKBotAIModule::onUnitCreate(BWAPI::Unit unit)
{
    if (Broodwar->isReplay())
    {
        // if we are in a replay, then we will print out the build order of the structures
        if (unit->getType().isBuilding() && !unit->getPlayer()->isNeutral())
        {
            int seconds = Broodwar->getFrameCount()/24;
            int minutes = seconds/60;
            seconds %= 60;
            Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, unit->getPlayer()->getName().data(), unit->getType().c_str());
        }
    }
}

void ZZZKBotAIModule::onUnitDestroy(BWAPI::Unit unit)
{
}

void ZZZKBotAIModule::onUnitMorph(BWAPI::Unit unit)
{
    if (Broodwar->isReplay())
    {
        // if we are in a replay, then we will print out the build order of the structures
        if (unit->getType().isBuilding() && !unit->getPlayer()->isNeutral())
        {
            int seconds = Broodwar->getFrameCount()/24;
            int minutes = seconds/60;
            seconds %= 60;
            Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().data(), unit->getType().c_str());
        }
    }
}

void ZZZKBotAIModule::onUnitRenegade(BWAPI::Unit unit)
{
}

void ZZZKBotAIModule::onSaveGame(std::string gameName)
{
    Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void ZZZKBotAIModule::onUnitComplete(BWAPI::Unit unit)
{
}
