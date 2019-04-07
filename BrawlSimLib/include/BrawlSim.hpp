#pragma once

#include <random>
#include <numeric>

#include "BWAPI.h"
#include "..\..\external\FAP\FAP\include\FAP.hpp"

#include "BrawlSim\PlayerData.hpp"
#include "BrawlSim\UnitData.hpp"


namespace BrawlSim
{
	class UnitData;
	class PlayerData;

class Brawl
{
public:
	/// Returns the BWAPI::UnitType that is the "best" of a
	/// BuildFAP Monte Carlo simulation
	BWAPI::UnitType returnOptimalUnit();

private:
	FAP::FastAPproximation<impl::UnitData*> MCfap;

	impl::PlayerData<true>	player;
	impl::PlayerData<false>	enemy;
};

}
