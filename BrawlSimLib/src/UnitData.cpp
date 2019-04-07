#include "../../BrawlSimLib/include/BrawlSim/UnitData.hpp"

namespace impl
{

	UnitData::UnitData(const BWAPI::Player& player, const BWAPI::UnitType& ut)
		: id(0)
		, type(ut)
		, player(player)
		, unit(nullptr)
	{
		pre_sim_score = initialUnitScore(type);
	}

	UnitData::UnitData(const BWAPI::Unit& u)
		: id(u->getID())
		, type(u->getType())
		, player(u->getPlayer())
		, unit(u)
	{
		pre_sim_score = initialUnitScore(type);
	}

	UnitData::~UnitData()
	{

	}

	/// Return a initial score for a unittype based on economic value
	int UnitData::initialUnitScore(const BWAPI::UnitType& ut)
	{
		return static_cast<int>(ut.mineralPrice()
			+ (1.25 * ut.gasPrice())
			+ (25 * (ut.supplyRequired() / 2)));
	}

namespace UnitUtil
{
	// If the self player has the tech/buildings to build the requested unittype
	bool buildableUnit(const BWAPI::UnitType& ut)
	{
		return BWAPI::Broodwar->canMake(ut);
	}

}

}