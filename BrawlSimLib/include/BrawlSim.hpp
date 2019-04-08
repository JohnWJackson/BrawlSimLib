#pragma once

#include <random>
#include <numeric>

#include "BWAPI.h"
#include "..\..\external\FAP\FAP\include\FAP.hpp"

#include "BrawlSim\UnitData.hpp"


namespace BrawlSim
{
	/* Returns the optimal UnitType of a FAP simulation using Monte Carlo sim positions
	**
	** @param   BWAPI::UnitType::set friendly_types - BWAPI's custom set for unittypes. Desirable combat UnitTypes to sim for the "best" out of
	** @param   BWAPI::Unitset enemy_units - BWAPI's custom set for units. Enemy units that the friendly UnitType will fight against
	** @param   int sim_size - Approximate number of units for each force in the sim. If less than enemy units, makes an army composition for desired size. Default 10.
	**														
	** @return  BWAPI::UnitType - "best" UnitType of a FAP Sim
	*/
	BWAPI::UnitType returnOptimalUnit(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int sim_size = 10);
}
