#pragma once

#include <iostream>
#include <random>
#include <numeric>
#include <set>

#include "BWAPI.h"
#include "FAP.hpp"

#include "BrawlSim\UnitData.hpp"

class UnitData;

namespace BrawlSim
{
	class Brawl
	{
	public:
		/// <summary>FAP simulates each friendly UnitType against the composition of enemy Units/UnitTypes
		///     using Monte Carlo sim positions</summary>
		/// Checks to make sure units/unittypes are valid simulation units.
		/// Doesn't check if the UnitTypes are actually buildable.
		///
		/// <param name = "friendly_types">
		///		BWAPI's custom set for unittypes. Desirable UnitTypes to sim for the "best" out of.
		/// </param>
		/// <param name = "enemy_units">
		///		BWAPI's custom set for units. Enemy units that the friendly UnitType will fight against.
		/// </param>
		/// <param name = "scoring_type">
		///		The scoring system used to rank units in the system. 0 = http://basil.bytekeeper.org/stats.htmleconomic survival rates, 1 = economic scores, 2 = Normalized to 1. Default is 0.
		/// </param>
		/// <param name = "army_size">
		///		Approximate number of units for each force's army composition in the sim. Default -1 for no scaling.
		/// </param>
		/// <param name = "sims">
		///		Number of sims to perform for each UnitType. Default 1.
		/// </param>
		void simulateEach(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, const int scoring_type = 0, int army_size = -1, const int sims = 1);

		void simulateForces(const BWAPI::Unitset& friendly_units, const BWAPI::Unitset& enemy_units, const int sims = 1);

		/// <summary> Return the optimal BWAPI::UnitType after running a sim </summary>
		BWAPI::UnitType getOptimalUnit() const;

		/// <summary> Return a sorted vector of UnitType and score pairs in descending order with the most
		///     optimal/highest scored UnitType at the top </summary>
		std::vector<std::pair<BWAPI::UnitType, double>> getUnitRanks() const;

		/// <summary> Return a std::pair of the BWAPI::Player and int score of the force with the highest score remaining after a simulation (I.e. the winning player).
		///		Returns std::pair of BWAPI::Broodwar->self() and NULL if scores are even </summary>
		std::pair<BWAPI::Player, int> getBestForce() const;

		/// <summary>Draw the 'would-be' optimal unit of the given friendly UnitTypes to the screen after a simulateEach() simulation
		void drawOptimalUnit(const int x, const int y) const;
		void drawOptimalUnit(const BWAPI::Position& pos) const;

		/// <summary> Draw the unit ranks to the game screen after a simulateEach() simulation</summary>
		void drawUnitRank(const int x, const int y) const;

		/// <summary> Draw the winning force and score to the screen after a simulateForces() simulation</summary>
		void drawBestForce(const int x, const int y) const;

		/// @TODO Fix this to display each optimal unit being built simultaneously
		/*/// @Overload
		/// <summary>Draw the 'would-be' optimal unit of a particular building on top of the building. Best for Terran/Protoss</summary>
		///
		/// <param name = "building">
		///		The building to be drawn over
		/// </param>
		void drawOptimalUnit(const BWAPI::Unit& building);*/

	private:
		FAP::FastAPproximation<UnitData*>				MCfap;

		std::vector<UnitData>							friendly_data;
		std::map<UnitData, int>							enemy_data;

		int												friendly_score = 0;
		int												enemy_score = 0;
		bool											valid_enemies = true;

		BWAPI::UnitType									optimal_unit = BWAPI::UnitTypes::None;
		std::vector<std::pair<BWAPI::UnitType, double>>	unit_ranks;

		/// TO DO - Condense these into enum bitset flags for static_asserts
		static bool										simEachFlag;
		static bool										simForcesFlag;

		bool isValidType(const BWAPI::UnitType& type);

		void addEnemyTypes(const BWAPI::Unitset& units, const BWAPI::UnitType& friendly_type, int army_size);
		void addFriendlyType(const BWAPI::UnitType& type, int& army_size);

		/// TO DO - Condense these into one function with enum flags
		void checkAliveUnits(std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>>& friendly_pre_units, std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>>& enemy_pre_units);
		void checkAliveFriendly(std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>>& friendly_pre_units);
		void checkAliveEnemy(std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>>& enemy_pre_units);

		void setPostRank(const int scoring_type, const int army_size);
		void sortRanks();

		void setOptimalUnit();

		void resetFlags();
	};
}