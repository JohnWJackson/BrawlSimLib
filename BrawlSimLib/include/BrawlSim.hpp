#pragma once

#include <random>
#include <numeric>

#include "BWAPI.h"
#include "..\..\external\FAP\FAP\include\FAP.hpp"

#include "BrawlSim\UnitData.hpp"


namespace BrawlSim
{
	/// <summary>Returns the optimal UnitType of a FAP simulation using Monte Carlo sim positions</summary>
	/// Checks to make sure units/unittypes are valid simulation units.
	/// Doesn't check if the return UnitType is actually buildable.
	/// 
	/// <param name = "friendly_types">
	///		BWAPI's custom set for unittypes. Desirable unique UnitTypes to sim for the "best" out of.
	/// </param>
	/// <param name = "enemy_units">
	///		BWAPI's custom set for units. Enemy units that the friendly UnitType will fight against.
	/// </param>
	/// <param name = "sim_size">
	///		Approximate number of units for each force's army composition in the sim. Default 10.
	/// </param>
	///
	/// <return>
	///		"best" UnitType out of the FAP Sim. Returns BWAPI::UnitType::None if nothing buildable or empty friendly_types
	/// </return>
	BWAPI::UnitType returnOptimalUnit(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int sim_size = 10);


	/// <summary>Draw the 'would-be' optimal unit of the given friendly UnitTypes to the screen at the desired coordinates.
	/// Best for Zerg or finding the best overall unit</summary>
	///
	/// <param name = "x">
	///		X coordinate frame position
	/// </param>
	/// <param name = "y">
	///		Y coordinate frame position
	/// <param>
	void drawOptimalUnit(const int x, const int y);
	
	/// @Overload
	/// <summary>Draw the 'would-be' optimal unit of a particular building on top of the building. Best for Terran/Protoss</summary>
	///
	/// <param name = "building">
	///		The building to be drawn over
	/// </param>
	void drawOptimalUnit(const BWAPI::Unit& building);



	/// The stored previous optimal unit. For drawOptimalUnit.
	static BWAPI::UnitType last_optimal = BWAPI::UnitTypes::None;
}
