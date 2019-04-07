#pragma once

#include "..\BrawlSim.hpp"


namespace impl 
{

/// Forward Declarations
class UnitData;
class StoredUnit;

template <bool is_friendly>
class PlayerData
{
public:
	BWAPI::Player									player_;
	BWAPI::Race										race_;

	std::vector<BWAPI::UnitType>					sim_types_;

	std::map<UnitData, int>							combat_units_;

	std::map<BWAPI::Unit, StoredUnit>				unit_map_;

	std::vector<StoredUnit>							units_;

	std::map<BWAPI::UpgradeType, int>				upgrades_map_; // <upgradetype, upgrade_level>

	PlayerData() 
	{
		setPlayer();
		setRace();
		setSimTypes();
	}

	/// Update the player's data
	template <bool friendly = is_friendly, typename std::enable_if<friendly>::type* = nullptr>
	void update()
	{
		buildUpgradesMap();
	}

	/// Update the enemy's data
	template <bool friendly = is_friendly, typename std::enable_if<!friendly>::type* = nullptr>
	void update()
	{
		if (race_ == BWAPI::Races::Unknown)
		{
			if (player_->getRace() != BWAPI::Races::Unknown)
			{
				setRace();
				setSimTypes();
			}
		}

		updateEnemyUnits();
		buildUpgradesMap();
	}

private:
	/// Store the self player that we are working with
	template <bool friendly = is_friendly, typename std::enable_if<friendly>::type* = nullptr>
	void setPlayer()
	{
		player_ = BWAPI::Broodwar->self();
	}
	/// Store the enemy player that we are working with
	template <bool friendly = is_friendly, typename std::enable_if<!friendly>::type* = nullptr>
	void setPlayer()
	{
		player_ = BWAPI::Broodwar->enemy();
	}

	/// Set the players Race 
	void setRace()
	{
		race_ = player_->getRace();
	}

	/// Set the self player unit upgrades (e.g. Zerg_Melee_Attacks)
	template <bool friendly = is_friendly, typename std::enable_if<friendly>::type* = nullptr>
	void buildUpgradesMap() 
	{
		BWAPI::UpgradeType::set upgrade_set = BWAPI::UpgradeTypes::allUpgradeTypes();
		for (const auto& upgrade : upgrade_set) 
		{
			int level = player_->getUpgradeLevel(upgrade);
			upgrades_map_[upgrade] = level;
		}
	}
	/// Set the enemy player unit upgrades (e.g. Zerg_Melee_Attacks)
	template <bool friendly = is_friendly, typename std::enable_if<!friendly>::type* = nullptr>
	void buildUpgradesMap()
	{
		BWAPI::UpgradeType::set upgrade_set = BWAPI::UpgradeTypes::allUpgradeTypes();
		for (const auto& upgrade : upgrade_set) 
		{
			int observed_level = player_->getUpgradeLevel(upgrade);
			if (observed_level > upgrades_map_[upgrade]) 
			{
				int new_level = std::max(observed_level, upgrades_map_[upgrade]);
				upgrades_map_[upgrade] = new_level;
			}
		}
	}

	/// Builds a map of all valid potential simmable combat units of the friendly player's race
	void setSimTypes() 
	{
		switch (race_) 
		{
		case BWAPI::Races::Zerg:
			sim_types_ = { BWAPI::UnitTypes::Zerg_Ultralisk, BWAPI::UnitTypes::Zerg_Mutalisk,
					  BWAPI::UnitTypes::Zerg_Scourge,   BWAPI::UnitTypes::Zerg_Hydralisk, 
				      BWAPI::UnitTypes::Zerg_Zergling,  BWAPI::UnitTypes::Zerg_Lurker,
					  BWAPI::UnitTypes::Zerg_Guardian,  BWAPI::UnitTypes::Zerg_Devourer };
			break;
		case BWAPI::Races::Terran:
			sim_types_ = { BWAPI::UnitTypes::Terran_Marine,  BWAPI::UnitTypes::Terran_Vulture,
					  BWAPI::UnitTypes::Terran_Wraith,  BWAPI::UnitTypes::Terran_Firebat,
					  BWAPI::UnitTypes::Terran_Goliath, BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode,
					  BWAPI::UnitTypes::Terran_Medic,   BWAPI::UnitTypes::Terran_Battlecruiser,
				      BWAPI::UnitTypes::Terran_Valkyrie };
			break;
		case BWAPI::Races::Protoss:
			sim_types_ = { BWAPI::UnitTypes::Protoss_Arbiter, BWAPI::UnitTypes::Protoss_Archon,
					  BWAPI::UnitTypes::Protoss_Zealot,  BWAPI::UnitTypes::Protoss_Carrier, 
					  BWAPI::UnitTypes::Protoss_Corsair, BWAPI::UnitTypes::Protoss_Dark_Templar,
					  BWAPI::UnitTypes::Protoss_Dragoon, BWAPI::UnitTypes::Protoss_Reaver, 
				      BWAPI::UnitTypes::Protoss_Scout };
			break;
		// Random race, will only happen for enemy before beeing seen
		case BWAPI::Races::Unknown:
			break;
		// Should never happen
		default:
			BWAPI::Broodwar->sendText("ERROR: BrawlSim unable to detect player's race");
		}
	}

	void buildUnitData()
	{
		for (const auto& ut : sim_types_)
		{
			combat_units_[UnitData(ut)]++;
		}
	}

	///// Builds a map of all valid potential simmable combat units of the enemy player's race
	//template <bool friendly = is_friendly, typename std::enable_if<!friendly>::type* = nullptr>
	//void setSimUnits() 
	//{
	//	std::vector<BWAPI::UnitType> units;

	//	// Be ready for early game units before enemy has been spotted
	//	switch (race_)
	//	{
	//	case BWAPI::Races::Zerg:
	//		units = { BWAPI::UnitTypes::Zerg_Zergling };
	//		break;
	//	case BWAPI::Races::Terran:
	//		units = { BWAPI::UnitTypes::Terran_Marine };
	//		break;
	//	case BWAPI::Races::Protoss:
	//		units = { BWAPI::UnitTypes::Protoss_Zealot };
	//		break;
	//	// if enemy race is random (Unknown)
	//	case BWAPI::Races::Unknown:
	//		units = { BWAPI::UnitTypes::Zerg_Zergling, 
	//			      BWAPI::UnitTypes::Terran_Marine, 
	//			      BWAPI::UnitTypes::Protoss_Zealot };
	//		return;
	//	// Should never happen
	//	default:
	//		BWAPI::Broodwar->sendText("ERROR: BrawlSim unable to detect enemy's race");
	//	}

	//	for (const auto& ut : units)
	//	{
	//		combat_units_.push_back(UnitData(ut));
	//	}
	//}

	/// Add seen enemy units to the combat sim
	void updateEnemyUnits()
	{
		BWAPI::Unitset allUnits = BWAPI::Broodwar->enemy()->getUnits();
		for (const auto& unit : allUnits)
		{
			if (isSimmableUnit(unit))
			{
				combat_units_[UnitData(unit)]++;
			

				auto it = std::find(units_.begin(), units_.end(), unit);
				if (it != units_.end())
				{
					std::iter_swap(it, units_.end() - 1); // move the unit to end of vector and erase
					units_.erase(units_.end() - 1);		 
				}

			}
		}
	}

	/// If the unit is a simmable combat unittype
	bool isSimmableUnit(const BWAPI::Unit& u)
	{
		BWAPI::UnitType ut = u->getType();
		for (const auto& type : sim_types_)
		{
			if (ut == type)
			{
				return true;
			}
		}
		return false;
	}

};

/// Explicit instantiation
extern template class PlayerData<true>;
extern template class PlayerData<false>;

}