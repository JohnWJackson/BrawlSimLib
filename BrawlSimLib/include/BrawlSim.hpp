#pragma once

#include <random>
#include <numeric>

#include "BWAPI.h"
#include "..\..\external\FAP\FAP\include\FAP.hpp"

#include "BrawlSim\UnitData.hpp"


namespace BrawlSim
{
	/* Returns the optimal UnitType of a FAP simulation using Monte Carlo sim positions
	** Checks to make sure units/unittypes are valid simulation units.
	** Doesn't check if the return UnitType is actually buildable.
	** 
	** @param  BWAPI::UnitType::set friendly_types - BWAPI's custom set for unittypes. Desirable unique UnitTypes to sim for the "best" out of.
	** @param  BWAPI::Unitset enemy_units - BWAPI's custom set for units. Enemy units that the friendly UnitType will fight against.
	** @param  int sim_size - Approximate number of units for each force's army composition in the sim. Default 10.
	**
	** @return BWAPI::UnitType - "best" UnitType out of the FAP Sim.
	*/
	BWAPI::UnitType returnOptimalUnit(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int sim_size = 10);

	/* Draw the 'would-be' optimal unit to the screen at the desired coordinates
	**
	** @param  x - X coordinate frame position
	** @param  y - Y coordinate frame position
	*/
	void drawOptimalUnit(const int x, const int y);


	// The stored optimal unit
	static BWAPI::UnitType optimal_unit = BWAPI::UnitTypes::None;
}
