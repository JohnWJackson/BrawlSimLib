#pragma once

#include <random>
#include <numeric>

#include "BWAPI.h"
#include "..\..\external\FAP\FAP\include\FAP.hpp"

#include "BrawlSim\PlayerData.hpp"
#include "BrawlSim\UnitData.hpp"


namespace BrawlSim
{
	class Brawl
	{
	public:
		//Simply returns the unittype that is the "best" of a BuildFAP sim.
		BWAPI::UnitType returnOptimalUnit();

	private:
		FAP::FastAPproximation<impl::UnitData*> MCfap;

		impl::PlayerData<true>	player;
		impl::PlayerData<false>	enemy;
	};

}
