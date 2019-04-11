#include "..\..\BrawlSimLib\include\BrawlSim.hpp"
#include <set>

namespace BrawlSim
{
	std::vector<std::pair<BWAPI::UnitType, int>> unit_ranks;

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

		/// Return a map of valid simmable UnitTypes and a starting economic-based score
		int initialScore(const BWAPI::UnitType& type)
		{
			int supply = type.supplyRequired();
			int min_cost = type.mineralPrice();
			int gas_cost = type.gasPrice();

			switch (type)
			{
			case BWAPI::UnitTypes::Protoss_Carrier: //Assume carriers are loaded with 4 interceptors unless upgraded
				min_cost += BWAPI::UnitTypes::Protoss_Interceptor.mineralPrice() * (4 + 4 * BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Carrier_Capacity));
				gas_cost += BWAPI::UnitTypes::Protoss_Interceptor.gasPrice() * (4 + 4 * BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Carrier_Capacity));
				break;

			case BWAPI::UnitTypes::Zerg_Lurker:
				min_cost += BWAPI::UnitTypes::Zerg_Hydralisk.mineralPrice();
				gas_cost += BWAPI::UnitTypes::Zerg_Hydralisk.gasPrice();
				supply += BWAPI::UnitTypes::Zerg_Hydralisk.supplyRequired();
				break;

			case BWAPI::UnitTypes::Zerg_Devourer: // Morph from mutas
			case BWAPI::UnitTypes::Zerg_Guardian:
				min_cost += BWAPI::UnitTypes::Zerg_Mutalisk.mineralPrice();
				gas_cost += BWAPI::UnitTypes::Zerg_Mutalisk.gasPrice();
				supply += BWAPI::UnitTypes::Zerg_Mutalisk.supplyRequired();
				break;

			case BWAPI::UnitTypes::Protoss_Archon:
				min_cost += BWAPI::UnitTypes::Protoss_High_Templar.mineralPrice() * 2;
				gas_cost += BWAPI::UnitTypes::Protoss_High_Templar.gasPrice() * 2;
				supply += BWAPI::UnitTypes::Protoss_High_Templar.supplyRequired() * 2;
				break;

			case BWAPI::UnitTypes::Protoss_Dark_Archon:
				min_cost += BWAPI::UnitTypes::Protoss_Dark_Templar.mineralPrice() * 2;
				gas_cost += BWAPI::UnitTypes::Protoss_Dark_Templar.gasPrice() * 2;
				supply += BWAPI::UnitTypes::Protoss_Dark_Templar.supplyRequired() * 2;
				break;

			case BWAPI::UnitTypes::Protoss_Reaver: // Assume Reavers are loaded with 5 scarabs unless upgraded
				min_cost += BWAPI::UnitTypes::Protoss_Scarab.mineralPrice() * (5 + 5 * BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Reaver_Capacity));
				gas_cost += BWAPI::UnitTypes::Protoss_Scarab.gasPrice() * (5 + 5 * BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Reaver_Capacity));
				break;

			case BWAPI::UnitTypes::Zerg_Zergling: //2 units per egg
			case BWAPI::UnitTypes::Zerg_Scourge:
				supply /= 2;
				break;
			}

			int stock_value = static_cast<int>(min_cost + (1.25 * gas_cost) + (25 * supply));

			return stock_value;
		}

		/// Add valid friendly units to the FAP sim and score them. Return map of the UnitType and score
		std::map<BWAPI::UnitType, int> addFriendlyToFAP(FAP::FastAPproximation<impl::UnitData*>& fap_object, const BWAPI::UnitType::set& friendly_types, const int army_size)
		{
			std::map<BWAPI::UnitType, int> unit_scores;

			for (const auto& ut : friendly_types)
			{
				if (isValidType(ut))
				{
					// Set initial economic score for the unit
					unit_scores[ut] = initialScore(ut);

					if (ut.isTwoUnitsInOneEgg()) //zerglings and scourges
					{
						for (int i = 0; i < army_size * 2; ++i) // add double the units
						{
							fap_object.addUnitPlayer1(impl::UnitData{ ut, BWAPI::Broodwar->self() }.convertToFAPUnit());
						}
					}
					else
					{
						for (int i = 0; i < army_size; ++i)
						{
							fap_object.addUnitPlayer1(impl::UnitData{ ut, BWAPI::Broodwar->self() }.convertToFAPUnit());
						}
					}
				}
			}

			return unit_scores;
		}

		/// Scale enemy unit composition to the sim_size and add valid UnitTypes to the sim
		void addEnemyToFAP(FAP::FastAPproximation<impl::UnitData*>& fap_object, const BWAPI::Unitset& units, int& army_size)
		{
			BWAPI::Player enemy = BWAPI::Broodwar->enemy();

			int unit_total = 0;
			std::map<BWAPI::UnitType, int> count;

			for (const auto& u : units) //count unittypes
			{
				BWAPI::UnitType ut = u->getType();
				if (isValidType(ut))
				{
					++count[ut];
					++unit_total;
				}
			}

			if (unit_total == army_size) // already scaled
			{
				for (const auto& ut : count)
				{
					for (int i = 0; i < ut.second; ++i)
					{
						fap_object.addUnitPlayer2(impl::UnitData{ ut.first, enemy }.convertToFAPUnit());
					}
				}
			}
			else  //scale the army composition before adding
			{
				int actual_sim_size = 0; //Original sim_size not guaranteed
				for (const auto& ut : count)
				{
					double percent = ut.second / (double)unit_total;
					int scaled_count = std::lround(percent * army_size);
					actual_sim_size += scaled_count;

					for (int i = 0; i < scaled_count; ++i)
					{
						fap_object.addUnitPlayer2(impl::UnitData{ ut.first, enemy }.convertToFAPUnit());
					}
				}
				army_size = actual_sim_size; //set the sim size to match the enemy's size
			}
		}

		/// Updates the UnitData's score to post FAP sim values
		std::map<BWAPI::UnitType, int> updateFAPScores(std::vector<FAP::FAPUnit<impl::UnitData*>>& fap_vector, std::map<BWAPI::UnitType, int>& scores)
		{
			std::map<BWAPI::UnitType, int> count;
			std::map<BWAPI::UnitType, int> post_scores;
			// Accumulate total score for each unittype and increment the count;
			for (auto& fu : fap_vector)
			{
				double proportion_health = (fu.health + fu.shields) / (double)(fu.maxHealth + fu.maxShields);
				BWAPI::Broodwar->sendText(std::to_string(proportion_health).c_str());
				post_scores[fu.unitType] += static_cast<int>(proportion_health * scores[fu.unitType]);
				++count[fu.unitType];
				BWAPI::Broodwar->sendText(std::to_string(proportion_health).c_str());
			}
			for (auto& u : post_scores)
			{
				u.second /= count[u.first];
			}

			return post_scores;
		}

		/// Copy unit_scores map to the unit_ranks vector and sort the vector in descending order
		void setUnitRanks(std::map<BWAPI::UnitType, int>& unit_scores)
		{
			// Clear old ranks first
			unit_ranks.clear();

			for (auto& u : unit_scores)
			{
				unit_ranks.push_back(std::make_pair(u.first, u.second));
			}

			sort(unit_ranks.begin(), unit_ranks.end(), [](std::pair<BWAPI::UnitType, int>& lhs, std::pair<BWAPI::UnitType, int>& rhs)
			{
				return lhs.second > rhs.second;
			});
		}

		/// Return the UnitType with the highest score. If scores are tied, chooses more "flexible" UnitType.
		BWAPI::UnitType getBestScoredType()
		{
			int best_sim_score = INT_MIN;
			BWAPI::UnitType res = BWAPI::UnitTypes::None;

			// Fap_vector is sorted so just check scores that are equal to the first unit's score
			for (const auto& u : unit_ranks)
			{
				// there are several cases where the test return ties, ex: cannot see enemy units and they appear "empty", extremely one-sided combat...
				if (u.second > best_sim_score)
				{
					best_sim_score = u.second;
					res = u.first;
				}
				// there are several cases where the t est return ties, ex: cannot see enemy units and they appear "empty", extremely one-sided combat...
				else if (u.second == best_sim_score)
				{
					// if the current unit is "flexible" with regard to air and ground units, then keep it and continue to consider the next unit.
					if (res.airWeapon() != BWAPI::WeaponTypes::None && res.groundWeapon() != BWAPI::WeaponTypes::None)
					{
						continue;
					}
					// if the tying unit is "flexible", then let's use that one.
					else if (u.first.airWeapon() != BWAPI::WeaponTypes::None && u.first.groundWeapon() != BWAPI::WeaponTypes::None)
					{
						res = u.first;
					}
				}
				// Scores are getting lower, return what we have
				else
				{
					return res;
				}
			}

			return res;
		}
	}

	/// Simulate each friendly UnitType against the composition of enemy units
	void simulateEach(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int army_size, const int sims)
	{
		// Returns BWAPI::UnitType::None if no simmable friendly UnitData
		if (friendly_types.empty())
		{
			unit_ranks.clear(); // Nothing to rank
			optimal_unit = BWAPI::UnitTypes::None; // No best unit

			return;
		}
		// Return best initial (economic) score if there are no enemy units to sim against
		else if (enemy_units.empty())
		{
			FAP::FastAPproximation<impl::UnitData*> MCfap;
			MCfap.clear();

			// Add Friendly units to FAP and get initial unit scores
			std::map<BWAPI::UnitType, int> pre_scores = addFriendlyToFAP(MCfap, friendly_types, army_size);

			setUnitRanks(pre_scores); // Rank economic scores
			optimal_unit = getBestScoredType(); // Get the best economic unit

			return;
		}
		// Normal sim
		else
		{
			FAP::FastAPproximation<impl::UnitData*> MCfap;

			// Add Enemy first - changes the sim_size
			addEnemyToFAP(MCfap, enemy_units, army_size);
			// Add Friendly units to FAP and get initial unit scores
			std::map<BWAPI::UnitType, int> pre_scores = addFriendlyToFAP(MCfap, friendly_types, army_size);

			MCfap.simulate(); //default 96 frames

			for (const auto& u : *MCfap.getState().first)
			{
				BWAPI::Broodwar->sendText(std::to_string(u.health).c_str());
			}

			std::map<BWAPI::UnitType, int> post_scores = updateFAPScores(*MCfap.getState().first, pre_scores);

			setUnitRanks(post_scores); // Rank the post sim scores
			optimal_unit = getBestScoredType(); // Get the unit with best sim score
		}
	}

	BWAPI::UnitType getOptimalUnit()
	{
		return optimal_unit;
	}

	///// Returns the unittype that is the "best" of a BuildFAP sim.
	//void simulate(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int army_size, const int sims)
	//{
	//	std::map<BWAPI::UnitType, int> scores;
	//	std::vector<impl::UnitData> friendly_ud;

	//	// Convert each UnitType to UnitData and get initial score
	//	for (const auto& ut : friendly_types)
	//	{
	//		if (isValidType(ut))
	//		{
	//			impl::UnitData ud{ ut, BWAPI::Broodwar->self() };

	//			friendly_ud.push_back(ud);

	//			scores[ut] = ud.score;
	//		}
	//	}

	//	// Returns BWAPI::UnitType::None if no simmable friendly UnitData
	//	if (friendly_ud.empty())
	//	{
	//		return res;
	//	}

	//	// Return best initial score if no enemy units
	//	if (enemy_units.empty())
	//	{
	//		res = getBestScoredType(scores);
	//		last_optimal = res;
	//		return res;
	//	}

	//	FAP::FastAPproximation<impl::UnitData*> MCfap;

	//	addEnemyToFAP(MCfap, enemy_units, sim_size); // Add Enemy first - changes the sim_size
	//	addFriendlyToFAP(MCfap, friendly_ud, sim_size);

	//	MCfap.simulate(); //default 96 frames

	//	updateScores(*MCfap.getState().first, scores);

	//	// Store the last optimal unit if we had one
	//	res = getBestScoredType(scores);
	//	last_optimal = res;

	//	return res;
	//}

	/// Return BWAPI::UnitType and score of each unit in sim
	std::vector<std::pair<BWAPI::UnitType, int>> getUnitRanks()
	{
		return unit_ranks;
	}

	namespace Diag
	{
		/// Draw the OVERALL most optimal unit to the desired coordinates
		void drawOptimalUnit(const int x, const int y)
		{
			BWAPI::Broodwar->drawTextScreen(x, y, "Brawl Unit: %s", optimal_unit.c_str());
		}
		/// @Overload
		void drawOptimalUnit(const BWAPI::Position& pos)
		{
			BWAPI::Broodwar->drawTextScreen(pos, "Brawl Unit: %s", optimal_unit.c_str());
		}

		void drawUnitRank(const int x, const int y)
		{
			int spacing = 0;
			for (const auto& u : unit_ranks)
			{
				BWAPI::Broodwar->drawTextScreen(x, y + spacing, u.first.c_str());
				BWAPI::Broodwar->drawTextScreen(x + 200, y + spacing, std::to_string(u.second).c_str());
				spacing += 12;
			}
		}

		/// @TODO Fix this to display each optimal unit being built simultaneously
		///// Draw the most optimal unit for a specific building UnitType
		//void drawOptimalUnit(const BWAPI::Unit& building)
		//{
		//	if (building->getType().isBuilding() && building->isCompleted())
		//	{
		//		if (building->getType() == last_optimal.whatBuilds().first)
		//		{
		//			BWAPI::Broodwar->drawTextMap(building->getPosition(), last_optimal.c_str());
		//		}
		//	}
		//}
	}
}