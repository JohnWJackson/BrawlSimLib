#pragma once

#include "..\BrawlSim.hpp"

namespace impl
{

class UnitData 
{
public:
	int						id;
	BWAPI::Player			player;
	BWAPI::UnitType			type;
	BWAPI::Unit				unit;

	int						pre_sim_score;
	int						post_sim_score;

	UnitData(const BWAPI::Player& player, const BWAPI::UnitType& ut);
	UnitData(const BWAPI::Unit& u);
	~UnitData();

	bool operator< (const UnitData& lhs) const
	{
		return id < lhs.id;
	}

private:

	/// Return a initial score for a unittype based on economic value
	int initialUnitScore(const BWAPI::UnitType& ut);

};

//typedef std::map<BWAPI::Unit, StoredUnit> SUMap;
//
//class StoredUnit
//{
//	SUMap unit_map;
//
//public:
//	StoredUnit(const BWAPI::Unit& u)
//		: id(u->getID())
//	{
//		unit_map[u] = *this;
//	}
//
//	bool operator==(const BWAPI::Unit& u)
//	{
//		if (id == u->getID())
//		{
//			return true;
//		}
//		else
//		{
//			return false;
//		}
//	}
//
//	void removeUnit(BWAPI::Unit& unit)
//	{
//		unit_map.erase(unit);
//	}
//
//};

namespace UnitUtil 
{
	/// If the self player has the tech/buildings to build the requested unittype
	bool buildableUnit(const BWAPI::UnitType& ut);
}

}
