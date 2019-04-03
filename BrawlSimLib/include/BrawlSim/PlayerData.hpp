#pragma once

#include "..\BrawlSim.hpp"


namespace impl 
{

template <bool is_friendly>
class PlayerData
{
public:
	BWAPI::Player							player_;
	BWAPI::Race								race_;

	/* unit_map_ definition is different for friendly and enemy players
	** @is_friendly: map<BWAPI::UnitType, int sim_scores>
	** @!is_friendly: map<BWAPI::UnitType, int unit_count> */
	std::map<BWAPI::UnitType, int>			unit_map_;

	std::map<BWAPI::UpgradeType, int>		upgrades_map_; // map of UpgradeTypes and upgrade level

	PlayerData() 
	{
		setPlayer();
		setRace();
		buildUnitMap();
		buildUpgradesMap();
	}

private:
	/// Set of all possible upgrade types
	const BWAPI::UpgradeType::set upgrade_set = BWAPI::UpgradeTypes::allUpgradeTypes();


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

///------------------------------Friendly Player----------------------------------------------------
	/// Builds a map of all valid potential simmable combat units of the friendly player's race
	template <bool friendly = is_friendly, typename std::enable_if<friendly>::type* = nullptr>
	void buildUnitMap() 
	{
		switch (race_) 
		{
		case BWAPI::Races::Zerg: 
			unit_map_ = { { BWAPI::UnitTypes::None,             0 }, { BWAPI::UnitTypes::Zerg_Ultralisk,  0 }, { BWAPI::UnitTypes::Zerg_Mutalisk,               0 },
						  { BWAPI::UnitTypes::Zerg_Scourge,     0 }, { BWAPI::UnitTypes::Zerg_Hydralisk,  0 }, { BWAPI::UnitTypes::Zerg_Zergling,               0 },
						  { BWAPI::UnitTypes::Zerg_Lurker,      0 }, { BWAPI::UnitTypes::Zerg_Guardian,   0 }, { BWAPI::UnitTypes::Zerg_Devourer,               0 } };
			break;
		case BWAPI::Races::Terran:
			unit_map_ = { { BWAPI::UnitTypes::None,             0 }, { BWAPI::UnitTypes::Terran_Marine,   0 }, { BWAPI::UnitTypes::Terran_Battlecruiser,        0 },
						  { BWAPI::UnitTypes::Terran_Firebat,   0 }, { BWAPI::UnitTypes::Terran_Goliath,  0 }, { BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, 0 },
						  { BWAPI::UnitTypes::Terran_Medic,     0 }, { BWAPI::UnitTypes::Terran_Valkyrie, 0 }, { BWAPI::UnitTypes::Terran_Vulture,              0 }, 
						  { BWAPI::UnitTypes::Terran_Wraith,    0 } };
			break;
		case BWAPI::Races::Protoss:
			unit_map_ = { { BWAPI::UnitTypes::None,             0 }, { BWAPI::UnitTypes::Protoss_Arbiter, 0 }, { BWAPI::UnitTypes::Protoss_Archon,              0 },
						  { BWAPI::UnitTypes::Protoss_Carrier,  0 }, { BWAPI::UnitTypes::Protoss_Corsair, 0 }, { BWAPI::UnitTypes::Protoss_Dark_Templar,        0 },
						  { BWAPI::UnitTypes::Protoss_Dragoon,  0 }, { BWAPI::UnitTypes::Protoss_Reaver,  0 }, { BWAPI::UnitTypes::Protoss_Scout,               0 },
						  { BWAPI::UnitTypes::Protoss_Zealot,   0 } };
			break;
		// Should never happen
		default:
			BWAPI::Broodwar->sendText("ERROR: BrawlSim unable to detect players race");
		}

		// Remove units from the map that we dont have the tech or supply to build and set the initial Unit Score
		for (auto it = unit_map_.begin(); it != unit_map_.end(); ) 
		{
			if (it->first == BWAPI::UnitTypes::None)
				++it;

			else if (!UnitUtil::isValidUnit<true>(it->first))
				it = unit_map_.erase(it);

			else 
			{
				// Set score
				it->second = UnitUtil::initialUnitScore(it->first);
				++it;
			}
		}
	}

///------------------------------Enemy Player----------------------------------------------------
	/// Builds a map of all valid potential simmable combat units of the enemy player's race
	template <bool friendly = is_friendly, typename std::enable_if<!friendly>::type* = nullptr>
	void buildUnitMap() 
	{
		// Be ready for early game units before enemy has been spotted
		switch (race_)
		{
		case BWAPI::Races::Zerg:
			unit_map_ = { { BWAPI::UnitTypes::Zerg_Zergling,  0 } };
			break;
		case BWAPI::Races::Terran:
			unit_map_ = { { BWAPI::UnitTypes::Terran_Marine,  0 } };
			break;
		case BWAPI::Races::Protoss:
			unit_map_ = { { BWAPI::UnitTypes::Protoss_Zealot, 0 } };
			break;
		// if enemy race is random it returns as unknown if not yet sighted
		case BWAPI::Races::Unknown:
			unit_map_ = { { BWAPI::UnitTypes::Protoss_Zealot, 0 }, { BWAPI::UnitTypes::Terran_Marine, 0 }, { BWAPI::UnitTypes::Zerg_Zergling,  0 } };
			return;
		}

		// Add seen enemy units to unit_map_ and set initial Unit Score
		if (player_->allUnitCount() > 0) 
		{
			BWAPI::Unitset units = player_->getUnits();
			for (const auto& u : units)
			{
				BWAPI::UnitType ut = u->getType();
				
				if (UnitUtil::isValidUnit<false>(ut))
				{					
					unit_map_[ut]++; //add to map if not added and increment the count
				}
			}
		}
	}

};

/// Explicit instantiation
extern template class PlayerData<true>;
extern template class PlayerData<false>;

}