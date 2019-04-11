#include "..\..\BrawlSimLib\include\BrawlSim\UnitData.hpp"

namespace impl
{
	UnitData::UnitData(const BWAPI::UnitType& u, const BWAPI::Player& p)
		: type(u)
		, player(p)
	{
	}

	/// Generate a random position for the unit based on the unit speed
	BWAPI::Position UnitData::positionMCFAP() const
	{
		std::default_random_engine generator;  //Will be used to obtain a seed for the random number engine

		double speed = player->topSpeed(type);
		std::uniform_int_distribution<int> dis(static_cast<int>(-speed) * 4, static_cast<int>(speed) * 4);

		int rand_x = dis(generator);
		int rand_y = dis(generator);

		return BWAPI::Position(rand_x, rand_y);
	}
}