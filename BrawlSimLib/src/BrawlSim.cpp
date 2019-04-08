#include "..\..\BrawlSimLib\include\BrawlSim.hpp"


namespace BrawlSim
{

	namespace
	{
		// @TODO: Check to make sure all unsimmable unittypes are removed and that all simmable are included (abilities simmable?)
		/// Checks if a UnitType is a suitable type for the sim
		bool isValidType(const BWAPI::UnitType& type)
		{
			// No workers, buildings, heroes, 0 supply types, or unbuildable unittypes
			if (type.isWorker() ||
				type.isHero() ||
				type.supplyRequired() == 0 ||
				type == BWAPI::UnitTypes::Protoss_Interceptor ||
				type == BWAPI::UnitTypes::Protoss_Scarab ||
				type == BWAPI::UnitTypes::Zerg_Infested_Terran)
			{
				return false;
			}

			return
				type.canAttack() ||
				type == BWAPI::UnitTypes::Terran_Medic;
		}

		/// Add friendly units to the FAP sim.
		void addFriendlyToFAP(FAP::FastAPproximation<impl::UnitData*>& fap_object, std::vector<impl::UnitData>& friendly_ud, const int sim_size)
		{
			for (auto& u : friendly_ud)
			{
				if (u.type.isTwoUnitsInOneEgg()) //zerglings and scourges
				{
					for (int i = 0; i < sim_size * 2; ++i) // add double the units
					{
						fap_object.addUnitPlayer1(u.convertToFAPUnit());
					}
				}
				else
				{
					for (int i = 0; i < sim_size; ++i)
					{
						fap_object.addUnitPlayer1(u.convertToFAPUnit());
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
			for (auto u : units) //count unittypes
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
					fap_object.addUnitPlayer2(ud.convertToFAPUnit());
				}
			}
			else  //scale the army composition before adding
			{
				int actual_sim_size = 0; //Original sim_size not guaranteed
				for (const auto& ut : count) 
				{
					double percent = ut.second / unit_total;
					int scaled_count = static_cast<int>(std::round(percent * sim_size));
					actual_sim_size += scaled_count;

					impl::UnitData ud{ ut.first, enemy };
					for (int i = 0; i < scaled_count; ++i)
					{
						fap_object.addUnitPlayer2(ud.convertToFAPUnit());
					}
				}
				sim_size = actual_sim_size; //set the sim size to match the enemy's size
			}
		}

		/// Updates the score map to post FAP sim values
		void updateScores(std::vector<FAP::FAPUnit<impl::UnitData*>>& fap_vector, std::map<BWAPI::UnitType, int>& scores)
		{
			for (auto& fu : fap_vector)
			{
				double proportion_health = (fu.health + fu.shields) / static_cast<double>(fu.maxHealth + fu.maxShields);
				scores[fu.unitType] = static_cast<int>(fu.data->pre_sim_score * proportion_health);
			}
		}

		/// Return the UnitType with the highest score. If scores are tied, chooses more "flexible" UnitType.
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

	/// Returns the unittype that is the "best" of a BuildFAP sim.
	BWAPI::UnitType returnOptimalUnit(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int sim_size)
	{	
		// Returns BWAPI::UnitType::None
		if (friendly_types.empty())
		{
			return optimal_unit;
		}

		std::map<BWAPI::UnitType, int> scores;
		std::vector<impl::UnitData> friendly_ud;

		// Convert each UnitType to UnitData and get initial score
		for (const auto& ut : friendly_types)
		{
			if (isValidType(ut))
			{
				impl::UnitData ud{ ut, BWAPI::Broodwar->self() };

				friendly_ud.push_back(ud);

				scores[ut] = ud.pre_sim_score;
			}
		}

		// Return best initial score
		if (enemy_units.empty())
		{
			optimal_unit = getBestScoredType(scores);
			return optimal_unit;
		}

		FAP::FastAPproximation<impl::UnitData*> MCfap;

		addEnemyToFAP(MCfap, enemy_units, sim_size); // Add Enemy first - changes the sim_size
		addFriendlyToFAP(MCfap, friendly_ud, sim_size);

		MCfap.simulate(); //default 96 frames

		updateScores(*MCfap.getState().first, scores);

		optimal_unit = getBestScoredType(scores);

		return optimal_unit;
	}

	/// Draw the optimal unit to the desired coordinates
	void drawOptimalUnit(const int x, const int y)
	{
		BWAPI::Broodwar->drawTextScreen(x, y, "Brawl Unit: %s", optimal_unit.c_str());
	}

}