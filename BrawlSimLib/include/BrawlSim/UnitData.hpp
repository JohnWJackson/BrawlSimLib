#pragma once

#include "..\BrawlSim.hpp"

namespace impl
{

	class UnitData
	{
	public:
		BWAPI::UnitType			type;
		BWAPI::Player			player;

		int						pre_sim_score;

		
		UnitData(const BWAPI::UnitType& u, const BWAPI::Player& p);

		/// Convert a UnitData to a FAP::Unit. Must be in template for decltype(auto) conversion
		auto convertToFAPUnit()
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

			if (type == BWAPI::UnitTypes::Protoss_Carrier)
			{
				groundDamage = player->damage(BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon());
				groundDamageType = BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon().damageType();
				groundCooldown = 5;
				groundMaxRange = 32 * 8;

				airDamage = groundDamage;
				airDamageType = groundDamageType;
				airCooldown = groundCooldown;
				airMaxRange = groundMaxRange;

				attackerCount = 5; //assume carrier has 5 interceptors
			}
			else if (type == BWAPI::UnitTypes::Protoss_Reaver)
			{
				groundDamage = player->damage(BWAPI::WeaponTypes::Scarab);
				attackerCount = 5; //assume reaver has 5 scarabs (max is 5 without upgrade, 10 with)
			}

			return FAP::makeUnit<UnitData*>()
				.setData(this)

				.setUnitType(type)
				.setUnitSize(type.size())

				.setSpeed(static_cast<float>(player->topSpeed(type)))
				.setPosition(positionMCFAP())

				.setHealth(type.maxHitPoints())
				.setMaxHealth(type.maxHitPoints())

				.setArmor(player->armor(type))
				.setShields(type.maxShields())
				.setMaxShields(type.maxShields())
				.setShieldUpgrades(type == BWAPI::UnitTypes::Protoss_Zealot ? player->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields) : 0)

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
				.setStimmed(type == BWAPI::UnitTypes::Terran_Marine ? player->hasResearched(BWAPI::TechTypes::Stim_Packs) : 0)

				.setElevation(0)
				.setFlying(type.isFlyer())
				.setOrganic(type.isOrganic())

				// These are already calculated above but have to be set to something else they throw FAP flags
				.setArmorUpgrades(NULL)
				.setAttackUpgrades(NULL)
				.setShieldUpgrades(NULL)
				.setSpeedUpgrade(NULL)
				.setAttackSpeedUpgrade(NULL)
				.setAttackCooldownRemaining(NULL)
				.setRangeUpgrade(NULL)
				;
		}

		/// Return a initial score for a unittype based on economic value
		inline int initialUnitScore(const BWAPI::UnitType& ut) const
		{
			return static_cast<int>(ut.mineralPrice() + (1.25 * ut.gasPrice()) + (25 * (ut.supplyRequired() / 2)));
		}

	private:
		/// Generate a random position for the unit based on the unit speed
		BWAPI::Position positionMCFAP() const;
	};

}