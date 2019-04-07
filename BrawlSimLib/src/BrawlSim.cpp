#include "../../BrawlSimLib/include/BrawlSim.hpp"

namespace BrawlSim
{

	namespace
	{

		/// Convert a UnitData to a FAP::Unit
		auto convertToFAPUnit(const impl::UnitData& ud)
		{
			int groundDamage(ud.player->damage(ud.type.groundWeapon()));
			int groundCooldown(ud.type.groundWeapon().damageFactor() && ud.type.maxGroundHits() ? ud.player->weaponDamageCooldown(ud.type) / (ud.type.groundWeapon().damageFactor() * ud.type.maxGroundHits()) : 0);
			int groundMaxRange(ud.player->weaponMaxRange(ud.type.groundWeapon()));
			int groundMinRange(ud.type.groundWeapon().minRange());
			BWAPI::DamageType groundDamageType(ud.type.groundWeapon().damageType());

			int airDamage(ud.player->damage(ud.type.airWeapon()));
			int airCooldown(ud.type.airWeapon().damageFactor() && ud.type.maxAirHits() ? ud.type.airWeapon().damageCooldown() / (ud.type.airWeapon().damageFactor() * ud.type.maxAirHits()) : 0);
			int airMaxRange(ud.player->weaponMaxRange(ud.type.airWeapon()));
			int airMinRange(ud.type.airWeapon().minRange());
			BWAPI::DamageType airDamageType(ud.type.airWeapon().damageType());

			if (ud.type == BWAPI::UnitTypes::Protoss_Carrier)
			{
				groundDamage = ud.player->damage(BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon());
				groundDamageType = BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon().damageType();
				groundCooldown = 5;
				groundMaxRange = 32 * 8;

				airDamage = groundDamage;
				airDamageType = groundDamageType;
				airCooldown = groundCooldown;
				airMaxRange = groundMaxRange;
			}
			else if (ud.type == BWAPI::UnitTypes::Protoss_Reaver)
			{
				groundDamage = ud.player->damage(BWAPI::WeaponTypes::Scarab);
			}

			return FAP::makeUnit<impl::UnitData*>()
				.setData(&ud)

				.setUnitType(ud.type)
				//.setUnitSize(ud.type.size())

				//.setSpeed(static_cast<float>(ud.player->topSpeed(ud.type)))
				//.setPosition(positionMCFAP(ud))

				//.setHealth(ud.type.maxHitPoints())
				//.setMaxHealth(ud.type.maxHitPoints())

				//.setArmor(ud.player->armor(ud.type))
				//.setShields(ud.type.maxShields())
				//.setMaxShields(ud.type.maxShields())
				//.setShieldUpgrades(ud.type == BWAPI::UnitTypes::Protoss_Zealot ? ud.player->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields) : 0)

				//.setGroundDamage(groundDamage)
				//.setGroundCooldown(groundCooldown)
				//.setGroundMaxRange(groundMaxRange)
				//.setGroundMinRange(groundMinRange)
				//.setGroundDamageType(groundDamageType)

				//.setAirDamage(airDamage)
				//.setAirCooldown(airCooldown)
				//.setAirMaxRange(airMaxRange)
				//.setAirMinRange(airMinRange)
				//.setAirDamageType(airDamageType)

				//.setAttackerCount(ud.type == BWAPI::UnitTypes::Protoss_Carrier ? 5 : 0) //assume carriers have 5 interceptors
				//.setStimmed(ud.type == BWAPI::UnitTypes::Terran_Marine ? ud.player->hasResearched(BWAPI::TechTypes::Stim_Packs) : 0)

				//.setFlying(ud.type.isFlyer())
				//.setOrganic(ud.type.isOrganic());
				;
		}

		/// Generate a random position for the unit based on the unit speed
		BWAPI::Position positionMCFAP(const impl::UnitData& ud)
		{
			std::default_random_engine generator;  //Will be used to obtain a seed for the random number engine

			double speed = ud.player->topSpeed(ud.type);
			std::uniform_int_distribution<int> dis(static_cast<int>(-speed) * 4, static_cast<int>(speed) * 4);

			int rand_x = dis(generator);
			int rand_y = dis(generator);

			return BWAPI::Position(rand_x, rand_y);
		}

		/// Add all a players units to the FAP simulation
		template <bool friendly>
		void addToMCFAP(FAP::FastAPproximation<impl::UnitData*>& fap_object, impl::PlayerData<friendly>& player)
		{
			for (const auto& u : player.combat_units_)
			{			
				auto fap_unit = convertToFAPUnit(u.first);

				if (friendly)
				{
					fap_object.addUnitPlayer1(fap_unit); //std::move rvalue cast
				}

				else
				{
					fap_object.addUnitPlayer2(std::move(fap_unit)); //std::move rvalue cast
				}
			}
		}

		/// Get the Total FAP Score of units for the player
		template <bool friendly>
		int getTotalFAPScore(FAP::FastAPproximation<impl::UnitData*>& fap)
		{
			auto addScores = [&](int currentScore, auto FAPunit)
			{
				return static_cast<int>(currentScore + FAPunit.data->pre_sim_score_);
			};

			if (friendly && fap.getState().first && !fap.getState().first->empty())
			{
				return std::accumulate(fap.getState().first->begin(), 
									   fap.getState().first->end(), 
					                   0, 
									   addScores);
			}
			else if (!friendly && fap.getState().second && !fap.getState().second->empty())
			{
				return std::accumulate(fap.getState().second->begin(),
									   fap.getState().second->end(),
									   0,
									   addScores);
			}
			else return 0;
		}

		/// Updates the FAP Score of players units after sim
		void updateFAPvalue(std::vector<FAP::FAPUnit<impl::UnitData*>>& fap_vector)
		{
			for (auto& fu : fap_vector)
			{
				if (fu.data)
				{
					double proportion_health = (fu.health + fu.shields) / static_cast<double>(fu.maxHealth + fu.maxShields);
					fu.data->post_sim_score = static_cast<int>(fu.data->pre_sim_score * proportion_health);
				}
			}
		}


	}

	///Simply returns the unittype that is the "best" of a BuildFAP sim.
	BWAPI::UnitType Brawl::returnOptimalUnit()
	{	
		MCfap.clear();

		player.update();
		enemy.update();

		addToMCFAP(MCfap, player);
		addToMCFAP(MCfap, enemy);

		MCfap.simulate(); //default 96 frames
		
		int friendly_fap_score = getTotalFAPScore<true>(MCfap);
		int enemy_fap_score = getTotalFAPScore<false>(MCfap);

		updateFAPvalue(*MCfap.getState().first);

		int best_sim_score = INT_MIN;
		BWAPI::UnitType build_type = BWAPI::UnitTypes::None;
		for (const auto& fu : *MCfap.getState().first) 
		{
			// there are several cases where the test return ties, ex: cannot see enemy units and they appear "empty", extremely one-sided combat...
			if (fu.data->post_sim_score > best_sim_score)
			{
				best_sim_score = fu.data->post_sim_score;
				build_type = fu.data->type;
			}
			// there are several cases where the test return ties, ex: cannot see enemy units and they appear "empty", extremely one-sided combat...
			else if (fu.data->post_sim_score == best_sim_score)
			{
				// if the current unit is "flexible" with regard to air and ground units, then keep it and continue to consider the next unit.
				if (build_type.airWeapon() != BWAPI::WeaponTypes::None && build_type.groundWeapon() != BWAPI::WeaponTypes::None)
				{
					continue;
				}
				// if the tying unit is "flexible", then let's use that one.
				else if (fu.data->type.airWeapon() != BWAPI::WeaponTypes::None && fu.data->type.groundWeapon() != BWAPI::WeaponTypes::None)
				{
					build_type = fu.data->type;
				}
			}
		}
		return build_type;
	}

}