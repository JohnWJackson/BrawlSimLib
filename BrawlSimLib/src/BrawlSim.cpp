#include "..\..\BrawlSimLib\include\BrawlSim.hpp"
#include "..\..\BrawlSimLib\include\BrawlSim\UnitData.hpp"


namespace BrawlSim
{

	namespace
	{

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

		/// Convert a UnitData to a FAP::Unit
		auto convertToFAPUnit(impl::UnitData& ud)
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

				.setUnitType(ud.type)
				.setUnitSize(ud.type.size())

				.setSpeed(static_cast<float>(ud.player->topSpeed(ud.type)))
				.setPosition(positionMCFAP(ud))

				.setHealth(ud.type.maxHitPoints())
				.setMaxHealth(ud.type.maxHitPoints())

				.setArmor(ud.player->armor(ud.type))
				.setShields(ud.type.maxShields())
				.setMaxShields(ud.type.maxShields())
				.setShieldUpgrades(ud.type == BWAPI::UnitTypes::Protoss_Zealot ? ud.player->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields) : 0)

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

				.setAttackerCount(ud.type == BWAPI::UnitTypes::Protoss_Carrier ? 5 : 0) //assume carriers have 5 interceptors
				.setStimmed(ud.type == BWAPI::UnitTypes::Terran_Marine ? ud.player->hasResearched(BWAPI::TechTypes::Stim_Packs) : 0)

				.setElevation(0)
				.setFlying(ud.type.isFlyer())
				.setOrganic(ud.type.isOrganic())

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

		/// Checks if a UnitType is a suitable type for the sim
		bool isValidType(const BWAPI::UnitType& type)
		{
			// No workers, buildings, or interceptors
			if (type.isWorker() ||
				type.isBuilding() ||
				type == BWAPI::UnitTypes::Protoss_Interceptor)
			{
				return false;
			}

			return
				type.canAttack() ||
				type.isDetector() ||
				type == BWAPI::UnitTypes::Zerg_Queen ||
				type == BWAPI::UnitTypes::Zerg_Defiler ||
				type == BWAPI::UnitTypes::Terran_Medic ||
				type == BWAPI::UnitTypes::Protoss_High_Templar ||
				type == BWAPI::UnitTypes::Protoss_Dark_Archon;
		}

		/// Converts valid friendly UnitTypes to FAP::Units and add to the sim
		void addFriendlyToFAP(FAP::FastAPproximation<impl::UnitData*>& fap_object, const std::vector<BWAPI::UnitType>& units, const int sim_size)
		{
			BWAPI::Player self = BWAPI::Broodwar->self();

			for (const auto& ut : units)
			{
				if (ut.isTwoUnitsInOneEgg()) //zerglings and scourges
				{
					impl::UnitData ud{ ut, self };

					for (int i = 0; i < sim_size * 2; ++i) // add double the units
					{
						fap_object.addUnitPlayer1(convertToFAPUnit(ud));
					}
				}

				else if (isValidType(ut))
				{
					impl::UnitData ud{ ut, self };

					for (int i = 0; i < sim_size; ++i)
					{
						fap_object.addUnitPlayer1(convertToFAPUnit(ud));
					}
				}
			}
		}

		/// Scale enemy unit composition to the sim_size and add valid UnitTypes to the sim
		void addEnemyToFAP(FAP::FastAPproximation<impl::UnitData*>& fap_object, const BWAPI::Unitset& units, int& sim_size)
		{
			BWAPI::Player enemy = BWAPI::Broodwar->enemy();

			int unit_total = 0;
			std::map<BWAPI::UnitType, int> count;
			for (auto u : units)
			{
				BWAPI::UnitType ut = u->getType();
				if (isValidType(ut))
				{
					count[ut]++;
					++unit_total;
				}
			}

			if (count.size() == sim_size) // already scaled
			{
				for (const auto& ut : count)
				{
					impl::UnitData ud{ ut.first, enemy };
					fap_object.addUnitPlayer2(convertToFAPUnit(ud));
				}
			}
			else  //scale the army composition before adding
			{
				int actual_sim_size = 0; //Original sim_size not guarenteed
				for (const auto& ut : count)
				{
					double percent = ut.second / unit_total;
					int scaled_count = static_cast<int>(std::round(percent * sim_size));
					actual_sim_size += scaled_count;

					impl::UnitData ud{ ut.first, enemy };
					for (int i = 0; i < scaled_count; ++i)
					{
						fap_object.addUnitPlayer2(convertToFAPUnit(ud));
					}
				}
				sim_size = actual_sim_size; //set the sim size to match the enemy's size
			}
		}

		/// Get the Total FAP Score of units for the player
		/*int getTotalFAPScore(FAP::FastAPproximation<impl::UnitData*>& fap)
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
		}*/

		/// Updates the FAP Score of players units after sim
		std::map<BWAPI::UnitType, int> getFAPscores(std::vector<FAP::FAPUnit<impl::UnitData*>>& fap_vector)
		{
			std::map<BWAPI::UnitType, int> scores;
			for (auto& fu : fap_vector)
			{
				if (fu.data)
				{
					double proportion_health = (fu.health + fu.shields) / static_cast<double>(fu.maxHealth + fu.maxShields);
					scores[fu.data->type] = static_cast<int>(fu.data->pre_sim_score * proportion_health);
				}
			}

			return scores;
		}

		BWAPI::UnitType getBestScoredType(const std::map<BWAPI::UnitType, int>& scores)
		{
			int best_sim_score = INT_MIN;
			BWAPI::UnitType build_type = BWAPI::UnitTypes::None;

			for (const auto& u : scores)
			{
				// there are several cases where the test return ties, ex: cannot see enemy units and they appear "empty", extremely one-sided combat...
				if (u.second > best_sim_score)
				{
					best_sim_score = u.second;
					build_type = u.first;
				}
				// there are several cases where the test return ties, ex: cannot see enemy units and they appear "empty", extremely one-sided combat...
				else if (u.second == best_sim_score)
				{
					// if the current unit is "flexible" with regard to air and ground units, then keep it and continue to consider the next unit.
					if (build_type.airWeapon() != BWAPI::WeaponTypes::None && build_type.groundWeapon() != BWAPI::WeaponTypes::None)
					{
						continue;
					}
					// if the tying unit is "flexible", then let's use that one.
					else if (u.first.airWeapon() != BWAPI::WeaponTypes::None && u.first.groundWeapon() != BWAPI::WeaponTypes::None)
					{
						build_type = u.first;
					}
				}
			}

			return build_type;
		}

	}

	///Simply returns the unittype that is the "best" of a BuildFAP sim.
	BWAPI::UnitType returnOptimalUnit(const std::vector<BWAPI::UnitType>& friendly_types, const BWAPI::Unitset& enemy_units, int sim_size)
	{	
		FAP::FastAPproximation<impl::UnitData*> MCfap;

		addEnemyToFAP(MCfap, enemy_units, sim_size); // Add Enemy first - changes the sim_size
		addFriendlyToFAP(MCfap, friendly_types, sim_size);

		MCfap.simulate(); //default 96 frames
		
		//int friendly_fap_score = getTotalFAPScore<true>(MCfap);
		//int enemy_fap_score = getTotalFAPScore<false>(MCfap);

		std::map<BWAPI::UnitType, int> scores{ getFAPscores(*MCfap.getState().first) };

		BWAPI::UnitType res{ getBestScoredType(scores) };
		
		return res;
	}

}