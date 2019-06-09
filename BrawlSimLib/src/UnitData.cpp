#include "..\..\BrawlSimLib\include\BrawlSim\UnitData.hpp"

UnitData::UnitData(const BWAPI::UnitType& u, const BWAPI::Player& p)
	: type(u)
	, player(p)
	, eco_score(initialEcoScore())
	, survival_rate(survivalScore())
{
}

/// Return a map of valid simmable UnitTypes and a starting economic-based score
int UnitData::initialEcoScore() const
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

/// Survival rate of a unit from http://basil.bytekeeper.org/stats.html (5/2/19)
/// Rate = (Created - Destroyed) / Created
double UnitData::survivalScore() const
{
	if (type.getRace() == BWAPI::Races::Zerg)
	{
		switch (type)
		{
		case BWAPI::UnitTypes::Zerg_Zergling:
			return (3006333 - 2406700) / 3006333.0;
		case BWAPI::UnitTypes::Zerg_Scourge:
			return (91936 - 84880) / 91936.0;
		case BWAPI::UnitTypes::Zerg_Hydralisk:
			return (596532 - 375965) / 596532.0;
		case BWAPI::UnitTypes::Zerg_Mutalisk:
			return (332894 - 163463) / 332894.0;
		case BWAPI::UnitTypes::Zerg_Lurker:
			return (81650 - 51993) / 81650.0;
		case BWAPI::UnitTypes::Zerg_Guardian:
			return (6206 - 2406) / 6206.0;
		case BWAPI::UnitTypes::Zerg_Devourer:
			return (1353 - 680) / 1353.0;
		case BWAPI::UnitTypes::Zerg_Ultralisk:
			return (11769 - 5016) / 11769.0;
		}
	}
	else if (type.getRace() == BWAPI::Races::Protoss)
	{
		switch (type)
		{
		case BWAPI::UnitTypes::Protoss_Zealot:
			return (1184533 - 848823) / 1184533.0;
		case BWAPI::UnitTypes::Protoss_Dragoon:
			return (793539 - 475569) / 793539.0;
		case BWAPI::UnitTypes::Protoss_Dark_Templar:
			return (80670 - 45363) / 80670.0;
		case BWAPI::UnitTypes::Protoss_Corsair:
			return (39440 - 21326) / 39440.0;
		case BWAPI::UnitTypes::Protoss_Scout:
			return (1961 - 1125) / 1961.0;
		case BWAPI::UnitTypes::Protoss_Reaver:
			return (35619 - 18228) / 35619.0;
		case BWAPI::UnitTypes::Protoss_Arbiter:
			return (7139 - 1682) / 7139.0;
		case BWAPI::UnitTypes::Protoss_Archon:
			return (10652 - 5803) / 10652.0;
		case BWAPI::UnitTypes::Protoss_Carrier:
			return (52100 - 10046) / 52100.0;
		}
	}
	else if (type.getRace() == BWAPI::Races::Terran)
	{
		switch (type)
		{
		case BWAPI::UnitTypes::Terran_Marine:
			return (1552978 - 1175725) / 1552978.0;
		case BWAPI::UnitTypes::Terran_Medic:
			return (165745 - 99173) / 165745.0;
		case BWAPI::UnitTypes::Terran_Firebat:
			return (77668 - 58791) / 77668.0;
		case BWAPI::UnitTypes::Terran_Ghost:
			return (15923 - 10122) / 15923.0;
		case BWAPI::UnitTypes::Terran_Vulture:
			return (802517 - 583869) / 802517.0;
		case BWAPI::UnitTypes::Terran_Goliath:
			return (250987 - 158862) / 250987.0;
		case BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode:
		case BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode:
			return (381571 - 51902) / 381571.0;
		case BWAPI::UnitTypes::Terran_Wraith:
			return (66741 - 38469) / 66741.0;
		case BWAPI::UnitTypes::Terran_Valkyrie:
			return (7764 - 4525) / 7764.0;
		case BWAPI::UnitTypes::Terran_Battlecruiser:
			return (10447 - 3793) / 10447.0;
		}
	}
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