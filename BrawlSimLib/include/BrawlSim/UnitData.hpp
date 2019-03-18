#pragma once

#include "..\BrawlSim.hpp"


namespace BrawlSim 
{

class UnitData 
{
public:
	int						pre_sim_score_;
	int						post_sim_score_;


	template <bool friendly>
	UnitData(const BWAPI::UnitType& ut, PlayerData<friendly>& player)
		:
		type_(ut),
		speed_tech_(false),
		units_inside_object_(0),
		is_flying_(ut.isFlyer()),
		shields_(ut.maxShields()),
		health_(ut.maxHitPoints())
	{
		positionMCFAP(player);
		setProperSpeed(player);

		pre_sim_score_ = UnitUtil::initialUnitScore(type_);
	}

	/// Convert a BWAPI::UnitType to a FAP Unit
	template <bool friendly>
	auto convertToFAPUnit(PlayerData<friendly>& player)
	{
		int armor_upgrades = player.upgrades_map_.at(type_.armorUpgrade()) + 2 * (type_ == BWAPI::UnitTypes::Zerg_Ultralisk * player.upgrades_map_.at(BWAPI::UpgradeTypes::Chitinous_Plating));

		int gun_upgrades = std::max(player.upgrades_map_.at(type_.groundWeapon().upgradeType()),
			                        player.upgrades_map_.at(type_.airWeapon().upgradeType()));

		int shield_upgrades = static_cast<int>(shields_ > 0) * (player.upgrades_map_.at(BWAPI::UpgradeTypes::Protoss_Plasma_Shields));

		bool range_upgrade =
			(type_ == BWAPI::UnitTypes::Zerg_Hydralisk   && player.upgrades_map_[BWAPI::UpgradeTypes::Grooved_Spines] > 0) ||
			(type_ == BWAPI::UnitTypes::Protoss_Dragoon  && player.upgrades_map_[BWAPI::UpgradeTypes::Singularity_Charge] > 0) ||
			(type_ == BWAPI::UnitTypes::Terran_Marine    && player.upgrades_map_[BWAPI::UpgradeTypes::U_238_Shells] > 0) ||
			(type_ == BWAPI::UnitTypes::Terran_Goliath   && player.upgrades_map_[BWAPI::UpgradeTypes::Charon_Boosters] > 0);

		bool attack_speed_upgrade =
			(type_ == BWAPI::UnitTypes::Zerg_Zergling    && player.upgrades_map_[BWAPI::UpgradeTypes::Adrenal_Glands] > 0);

		if (type_ == BWAPI::UnitTypes::Protoss_Carrier   && player.upgrades_map_[BWAPI::UpgradeTypes::Carrier_Capacity] > 0) 
		{
			units_inside_object_ = 3 * (2 + 4 * player.upgrades_map_.at(BWAPI::UpgradeTypes::Carrier_Capacity));
		}

		return FAP::makeUnit<UnitData*>()
			.setData(this)
			.setUnitType(type_)
			.setPosition(pos_)
			.setHealth(health_)
			.setShields(shields_)
			.setFlying(is_flying_)
			.setElevation(0)
			.setAttackerCount(units_inside_object_)
			.setArmorUpgrades(armor_upgrades)
			.setAttackUpgrades(gun_upgrades)
			.setShieldUpgrades(shield_upgrades)
			.setSpeedUpgrade(speed_tech_)
			.setAttackSpeedUpgrade(attack_speed_upgrade)
			.setAttackCooldownRemaining(0)
			.setStimmed(0)
			.setRangeUpgrade(range_upgrade)
			;
	}

private:
	BWAPI::UnitType			 type_;
	BWAPI::Position			 pos_;

	double					 speed_;

	int						 health_;
	int						 shields_;
	int						 units_inside_object_;  // For carrier interceptors, always 0 unless type_ is a carrier

	bool					 is_flying_;
	bool					 speed_tech_;           //if we have speed tech for this unit


	/// Get top speed of a unit type including upgrades
	template <bool friendly>
	void setProperSpeed(PlayerData<friendly>& player)
	{
		bool speed_tech_ =
			(type_ == BWAPI::UnitTypes::Zerg_Zergling    && player.upgrades_map_[BWAPI::UpgradeTypes::Metabolic_Boost] > 0) ||
			(type_ == BWAPI::UnitTypes::Zerg_Hydralisk   && player.upgrades_map_[BWAPI::UpgradeTypes::Muscular_Augments] > 0) ||
			(type_ == BWAPI::UnitTypes::Zerg_Overlord    && player.upgrades_map_[BWAPI::UpgradeTypes::Pneumatized_Carapace] > 0) ||
			(type_ == BWAPI::UnitTypes::Zerg_Ultralisk   && player.upgrades_map_[BWAPI::UpgradeTypes::Anabolic_Synthesis] > 0) ||
			(type_ == BWAPI::UnitTypes::Protoss_Scout    && player.upgrades_map_[BWAPI::UpgradeTypes::Gravitic_Thrusters] > 0) ||
			(type_ == BWAPI::UnitTypes::Protoss_Zealot   && player.upgrades_map_[BWAPI::UpgradeTypes::Leg_Enhancements] > 0) ||
			(type_ == BWAPI::UnitTypes::Terran_Vulture   && player.upgrades_map_[BWAPI::UpgradeTypes::Ion_Thrusters] > 0);
		
		speed_tech_ ? speed_ = type_.topSpeed() * 1.5 : speed_ = type_.topSpeed();
	}

	/// Generate a random position for the unit based on the unit speed
	template <bool friendly>
	void positionMCFAP(PlayerData<friendly>& player)
	{
		std::default_random_engine generator;  //Will be used to obtain a seed for the random number engine

		std::uniform_int_distribution<int> dis(static_cast<int>(-speed_) * 4, static_cast<int>(speed_) * 4);

		int rand_x = dis(generator);
		int rand_y = dis(generator);

		pos_ = BWAPI::Position(rand_x, rand_y);
	}
};


namespace UnitUtil 
{
	/// FRIENDLY --- If we have the tech/buildings to build the requested unit
	template <bool friendly, typename std::enable_if<friendly>::type* = nullptr>
	bool isValidUnit(const BWAPI::UnitType& ut)
	{
		return BWAPI::Broodwar->canMake(ut);
	}
	/// ENEMY -- If unit is simmable
	template <bool friendly, typename std::enable_if<!friendly>::type* = nullptr>
	bool isValidUnit(const BWAPI::UnitType& ut)
	{
		if (ut.isWorker())
		{
			return false;
		}

		if (ut == BWAPI::UnitTypes::Protoss_Interceptor)
		{
			return false;
		}
		
		return ut.airWeapon() != BWAPI::WeaponTypes::None || 
			   ut.groundWeapon() != BWAPI::WeaponTypes::None ||
			   ut == BWAPI::UnitTypes::Terran_Medic ||
			   ut == BWAPI::UnitTypes::Protoss_Carrier ||
			   ut == BWAPI::UnitTypes::Protoss_Reaver;
	}

	/// Return a initial score for a unittype based on economic value
	inline int initialUnitScore(const BWAPI::UnitType& ut) 
	{
		return static_cast<int>( ut.mineralPrice() 
							   + (1.25 * ut.gasPrice()) 
			                   + (25 * (ut.supplyRequired() / 2)) );
	}

}

}
