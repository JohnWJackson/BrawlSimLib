#include "..\..\BrawlSimLib\include\BrawlSim\UnitData.hpp"

UnitData::UnitData(const BWAPI::UnitType& u, const BWAPI::Player& p)
	: type(u)
	, player(p)
	, pre_score(initialScore())
	, post_score(-1)
{
}

/// Return a map of valid simmable UnitTypes and a starting economic-based score
int UnitData::initialScore() const
{
	int supply = type.supplyRequired();
	int min_cost = type.mineralPrice();
	int gas_cost = type.gasPrice();

	switch (type)
	{
	case BWAPI::UnitTypes::Protoss_Carrier: //Assume carriers are loaded with 4 interceptors unless upgraded
		min_cost += BWAPI::UnitTypes::Protoss_Interceptor.mineralPrice() * (4 + 4 * player->getUpgradeLevel(BWAPI::UpgradeTypes::Carrier_Capacity));
		gas_cost += BWAPI::UnitTypes::Protoss_Interceptor.gasPrice() * (4 + 4 * player->getUpgradeLevel(BWAPI::UpgradeTypes::Carrier_Capacity));
		break;

	case BWAPI::UnitTypes::Zerg_Lurker:
		min_cost += BWAPI::UnitTypes::Zerg_Hydralisk.mineralPrice();
		gas_cost += BWAPI::UnitTypes::Zerg_Hydralisk.gasPrice();
		supply += BWAPI::UnitTypes::Zerg_Hydralisk.supplyRequired();
		break;

	case BWAPI::UnitTypes::Zerg_Devourer: // Morph from mutas
	case BWAPI::UnitTypes::Zerg_Guardian:
		min_cost += BWAPI::UnitTypes::Zerg_Mutalisk.mineralPrice();
		gas_cost += BWAPI::UnitTypes::Zerg_Mutalisk.gasPrice();
		supply += BWAPI::UnitTypes::Zerg_Mutalisk.supplyRequired();
		break;

	case BWAPI::UnitTypes::Protoss_Archon:
		min_cost += BWAPI::UnitTypes::Protoss_High_Templar.mineralPrice() * 2;
		gas_cost += BWAPI::UnitTypes::Protoss_High_Templar.gasPrice() * 2;
		supply += BWAPI::UnitTypes::Protoss_High_Templar.supplyRequired() * 2;
		break;

	case BWAPI::UnitTypes::Protoss_Dark_Archon:
		min_cost += BWAPI::UnitTypes::Protoss_Dark_Templar.mineralPrice() * 2;
		gas_cost += BWAPI::UnitTypes::Protoss_Dark_Templar.gasPrice() * 2;
		supply += BWAPI::UnitTypes::Protoss_Dark_Templar.supplyRequired() * 2;
		break;

	case BWAPI::UnitTypes::Protoss_Reaver: // Assume Reavers are loaded with 5 scarabs unless upgraded
		min_cost += BWAPI::UnitTypes::Protoss_Scarab.mineralPrice() * (5 + 5 * player->getUpgradeLevel(BWAPI::UpgradeTypes::Reaver_Capacity));
		gas_cost += BWAPI::UnitTypes::Protoss_Scarab.gasPrice() * (5 + 5 * player->getUpgradeLevel(BWAPI::UpgradeTypes::Reaver_Capacity));
		break;

	case BWAPI::UnitTypes::Zerg_Zergling: //2 units per egg
	case BWAPI::UnitTypes::Zerg_Scourge:
		supply /= 2;
		break;
	}

	int stock_value = static_cast<int>(min_cost + (1.25 * gas_cost) + (25 * supply));

	return stock_value;
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