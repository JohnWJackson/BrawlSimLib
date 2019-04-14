#pragma once

#include <iostream>
#include <random>
#include <numeric>
#include <set>

#include "BWAPI.h"
#include "..\..\external\FAP\FAP\include\FAP.hpp"

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
		/// <param name = "army_size">
		///		Approximate number of units for each force's army composition in the sim. Default 10.
		/// </param>
		/// <param name = "sims">
		///		Number of sims to perform for each UnitType. Default 1.
		/// </param>
		void simulateEach(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int army_size = 10, const int sims = 1);

		/// @Overload - Same as above but with an enemy BWAPI::UnitType::set instead of BWAPI::Unitset
		//void simulateEach(const BWAPI::UnitType::set& friendly_types, const BWAPI::UnitType::set& enemy_types, int army_size = 10, const int sims = 1);

		//void simulateSets(const BWAPI::UnitType::set& friendly_types, const BWAPI::UnitType::set& enemy_types, int army_size = 10);
		//void simulateSets(const BWAPI::UnitType::set& friendly_types, const BWAPI::UnitType::set& enemy_types, int army_size = 10);

		/// <summary> Return the optimal BWAPI::UnitType after running a sim </summary>
		BWAPI::UnitType getOptimalUnit();

		/// <summary>Return a sorted vector of UnitType and score pairs in descending order with the most
		///     optimal/highest scored UnitType at the top</summary>
		std::vector<std::pair<BWAPI::UnitType, int>> getUnitRanks();

		/// <summary>Draw the 'would-be' optimal unit of the given friendly UnitTypes to the screen at the desired coordinates.
		///     Best for Zerg or finding the best overall unit</summary>
		///
		/// <param name = "x">
		///		X coordinate frame position
		/// </param>
		/// <param name = "y">
		///		Y coordinate frame position
		/// <param>
		void drawOptimalUnit(const int x, const int y);

		/// @overload - same function as above but with a Position
		void drawOptimalUnit(const BWAPI::Position& pos);

		void drawUnitRank(const int x, const int y);

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

		std::vector <UnitData>							friendly_data;
		std::map<UnitData, int>							enemy_data;

		std::map<BWAPI::UnitType, int>					post_scores;

		BWAPI::UnitType									optimal_unit = BWAPI::UnitTypes::None;
		std::vector<std::pair<BWAPI::UnitType, int>>	unit_ranks;

		bool isValidType(const BWAPI::UnitType& type);

		void setScaledEnemy(const BWAPI::Unitset& units, int& army_size);

		void addEnemyToFAP();
		void addFriendlyTypeToFAP(const BWAPI::UnitType& type, const int army_size);

		void setOptimalUnit();

		void setPostRank(const int army_size);
		void sortRanks();
	};

	//namespace Diag
	//{
	//	/// <summary>Draw the 'would-be' optimal unit of the given friendly UnitTypes to the screen at the desired coordinates.
	//	///     Best for Zerg or finding the best overall unit</summary>
	//	///
	//	/// <param name = "x">
	//	///		X coordinate frame position
	//	/// </param>
	//	/// <param name = "y">
	//	///		Y coordinate frame position
	//	/// <param>
	//	void drawOptimalUnit(Brawl& brawl, const int x, const int y);
	//	/// @overload - same function as above but with a Position
	//	void drawOptimalUnit(Brawl& brawl, const BWAPI::Position& pos);

	//	void drawUnitRank(Brawl& brawl, const int x, const int y);

	//	/// @TODO Fix this to display each optimal unit being built simultaneously
	//	/*/// @Overload
	//	/// <summary>Draw the 'would-be' optimal unit of a particular building on top of the building. Best for Terran/Protoss</summary>
	//	///
	//	/// <param name = "building">
	//	///		The building to be drawn over
	//	/// </param>
	//	void drawOptimalUnit(const BWAPI::Unit& building);*/
	//}
}