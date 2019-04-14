#pragma once

#include "..\BrawlSim.hpp"

class UnitData
{
public:
	BWAPI::UnitType			type;
	BWAPI::Player			player;

	int						pre_score;

	UnitData(const BWAPI::UnitType& u, const BWAPI::Player& p);

	/// Convert a UnitData to a FAP::Unit. Must be in header for decl(auto)
	auto convertToFAPUnit() const
	{
		int groundDamage(player->damage(type.groundWeapon()));
		int groundCooldown(type.groundWeapon().damageFactor() && type.maxGroundHits() ? player->weaponDamageCooldown(type) / (type.groundWeapon().damageFactor() * type.maxGroundHits()) : 0);
		int groundMaxRange(player->weaponMaxRange(type.groundWeapon()));
		int groundMinRange(type.groundWeapon().minRange());
		BWAPI::DamageType groundDamageType(type.groundWeapon().damageType());

		int airDamage(player->damage(type.airWeapon()));
		int airCooldown(type.airWeapon().damageFactor() && type.maxAirHits() ? type.airWeapon().damageCooldown() / (type.airWeapon().damageFactor() * type.maxAirHits()) : 0);
		int airMaxRange(player->weaponMaxRange(type.airWeapon()));
		int airMinRange(type.airWeapon().minRange());
		BWAPI::DamageType airDamageType(type.airWeapon().damageType());

		int attackerCount(0);
		bool stimmed(false);

		switch (type)
		{
		case BWAPI::UnitTypes::Protoss_Carrier:
			groundDamage = player->damage(BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon());
			groundDamageType = BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon().damageType();
			groundCooldown = 5;
			groundMaxRange = 32 * 8;

			airDamage = groundDamage;
			airDamageType = groundDamageType;
			airCooldown = groundCooldown;
			airMaxRange = groundMaxRange;

			attackerCount = 4 + 4 * player->getUpgradeLevel(BWAPI::UpgradeTypes::Carrier_Capacity); //assume carrier has 4 interceptors unless upgraded
			break;

		case BWAPI::UnitTypes::Protoss_Reaver:
			groundDamage = player->damage(BWAPI::WeaponTypes::Scarab);
			groundDamageType = BWAPI::UnitTypes::Protoss_Scarab.groundWeapon().damageType();

			attackerCount = 5 + 5 * player->getUpgradeLevel(BWAPI::UpgradeTypes::Reaver_Capacity); //assume reaver has 5 scarabs unless upgraded
			break;

		case BWAPI::UnitTypes::Terran_Marine:
			stimmed = player->hasResearched(BWAPI::TechTypes::Stim_Packs);
			if (stimmed)
			{
				groundCooldown /= 2;
				airCooldown /= 2;
			}
			break;
		}

		return FAP::makeUnit<UnitData*>()
			.setData(const_cast<UnitData*>(this))

			.setUnitType(type)
			.setUnitSize(type.size())

			.setSpeed(static_cast<float>(player->topSpeed(type)))
			.setPosition(positionMCFAP())
			.setElevation() // default elevation -1

			.setHealth(type.maxHitPoints())
			.setMaxHealth(type.maxHitPoints())

			.setShields(type.maxShields())
			.setShieldUpgrades(player->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields))
			.setMaxShields(type.maxShields())
			.setArmor(player->armor(type))

			.setGroundDamage(groundDamage)
			.setGroundCooldown(groundCooldown)
			.setGroundMaxRange(groundMaxRange)
			.setGroundMinRange(groundMinRange)
			.setGroundDamageType(groundDamageType)

			.setAirDamage(airDamage)
			.setAirCooldown(airCooldown)
			.setAirMaxRange(airMaxRange)
			.setAirMinRange(airMinRange)
			.setAirDamageType(airDamageType)

			.setAttackerCount(attackerCount)
			.setStimmed(stimmed)

			.setFlying(type.isFlyer())
			.setOrganic(type.isOrganic())

			// Already calculated above but have to .set for FAP Flags. Might disable the FAP Flags later
			.setSpeedUpgrade(NULL)
			.setArmorUpgrades(NULL)
			.setAttackUpgrades(NULL)
			.setAttackSpeedUpgrade(NULL)
			.setAttackCooldownRemaining(NULL)
			.setRangeUpgrade(NULL)
			;
	}

	inline bool operator< (const UnitData& other) const
	{
		return this->type.getID() < other.type.getID();
	}

private:
	/// Return a map of valid simmable UnitTypes and a starting economic-based score
	int initialScore() const;

	/// Generate a random position for the unit based on the unit speed
	BWAPI::Position positionMCFAP() const;
};