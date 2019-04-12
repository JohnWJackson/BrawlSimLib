#include "..\..\BrawlSimLib\include\BrawlSim.hpp"
#include <set>

namespace BrawlSim
{
	// @TODO: Check to make sure all unsimmable unittypes are removed and that all simmable are included (abilities simmable?)
	/// Checks if a UnitType is a suitable type for the sim
	bool Brawl::isValidType(const BWAPI::UnitType& type)
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

	/// Add valid friendly units to the FAP sim and score them. Return map of the UnitType and score
	void Brawl::addFriendlyToFAP(const BWAPI::UnitType::set& friendly_types, const int army_size)
	{
		for (const auto& ut : friendly_types)
		{
			if (isValidType(ut))
			{
				if (ut.isTwoUnitsInOneEgg()) //zerglings and scourges
				{
					for (int i = 0; i < army_size * 2; ++i) // add double the units
					{
						friendly_data.push_back(UnitData(ut, BWAPI::Broodwar->self())); // Store the UnitData
						MCfap.addUnitPlayer1(friendly_data.back().convertToFAPUnit()); // put the last UnitData into the sim
						//fap_object.addUnitPlayer1(ud->convertToFAPUnit());
					}
				}
				else
				{
					for (int i = 0; i < army_size; ++i)
					{
						friendly_data.push_back(UnitData(ut, BWAPI::Broodwar->self())); // Store the UnitData
						MCfap.addUnitPlayer1(friendly_data.back().convertToFAPUnit()); // put the last UnitData into the sim
						//fap_object.addUnitPlayer1(ud->convertToFAPUnit());
					}
				}
			}
		}
	}

	/// Scale enemy unit composition to the sim_size and add valid UnitTypes to the sim
	void Brawl::addEnemyToFAP(const BWAPI::Unitset& units, int& army_size)
	{
		BWAPI::Player enemy = BWAPI::Broodwar->enemy();

		int unit_total = 0;
		std::map<BWAPI::UnitType, int> count;

		for (const auto& u : units) //count unittypes
		{
			if (isValidType(u->getType()))
			{
				++count[u->getType()];
				++unit_total;
			}
		}

		if (unit_total == army_size) // already scaled
		{
			for (const auto& ut : count)
			{
				for (int i = 0; i < ut.second; ++i)
				{
					enemy_data.push_back(UnitData(ut.first, BWAPI::Broodwar->enemy())); // Store the UnitData
					MCfap.addUnitPlayer2(enemy_data.back().convertToFAPUnit()); // put the last UnitData into the sim
					//UnitData* ud = new UnitData(ut.first, enemy);
					//fap_object.addUnitPlayer2(ud->convertToFAPUnit());
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
					enemy_data.push_back(UnitData(ut.first, BWAPI::Broodwar->enemy())); // Store the UnitData
					MCfap.addUnitPlayer2(enemy_data.back().convertToFAPUnit()); // put the last UnitData into the sim
					//UnitData* ud = new UnitData(ut.first, enemy);
					//fap_object.addUnitPlayer2(ud->convertToFAPUnit());
				}
			}
			army_size = actual_sim_size; //set the sim size to match the enemy's size
		}
	}

	/// Updates the UnitData's score to post FAP sim values
	void Brawl::updateFAPScores()
	{
		std::map<BWAPI::UnitType, int> count;
		std::map<BWAPI::UnitType, int> post_scores;

		// Accumulate total score for each unittype and increment the count;
		for (auto& fu : *MCfap.getState().first)
		{
			double proportion_health = (fu.health + fu.shields) / (double)(fu.maxHealth + fu.maxShields);
			BWAPI::Broodwar->sendText(std::to_string(proportion_health).c_str());

			fu.data->post_score = static_cast<int>((count[fu.unitType] * fu.data->post_score + (proportion_health * fu.data->pre_score)) / ++count[fu.unitType]);
			//post_scores[fu.unitType] += static_cast<int>(proportion_health * fu.data->score);
		}
	}

	/// Set the scored unit_ranks vector and sort the vector in descending order
	void Brawl::setUnitRanks()
	{
		// Only add one of each type to the rank
		BWAPI::UnitType last_type;

		// If we haven't run a fap sim: we have no post_score so use pre_scores
		if (MCfap.getState().first->front().data->post_score == -1)
		{
			for (auto& fu : *MCfap.getState().first)
			{
				if (fu.unitType != last_type)
				{
					unit_ranks.push_back(std::make_pair(fu.unitType, fu.data->pre_score));
					last_type = fu.unitType;
				}
			}
		}
		// We have run a fap sim: use post_scores
		else
		{
			for (auto& fu : *MCfap.getState().first)
			{
				if (fu.unitType != last_type)
				{
					unit_ranks.push_back(std::make_pair(fu.unitType, fu.data->post_score));
					last_type = fu.unitType;
				}
			}
		}

		// Sort in descending order
		sort(unit_ranks.begin(), unit_ranks.end(), [](std::pair<BWAPI::UnitType, int>& lhs, std::pair<BWAPI::UnitType, int>& rhs)
		{
			return lhs.second > rhs.second;
		});
	}

	/// Set the optimal unit to the friendly UnitType with the highest score. If scores are tied, chooses more "flexible" UnitType.
	void Brawl::setOptimalUnit()
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
				optimal_unit = res;
			}
		}
	}

	/// Simulate each friendly UnitType against the composition of enemy units
	void Brawl::simulateEach(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int army_size, const int sims)
	{
		// Returns BWAPI::UnitType::None if no simmable friendly UnitData
		if (friendly_types.empty())
		{
			optimal_unit = BWAPI::UnitTypes::None; // No best unit
			return;
		}
		// Return best initial (economic) score if there are no enemy units to sim against
		else if (enemy_units.empty())
		{
			addFriendlyToFAP(friendly_types, army_size); // Add Friendly units to FAP and get initial unit scores

			setUnitRanks(); // Rank economic scores
			setOptimalUnit(); // Get best economic unittype, run after setting/sorting unit ranks
			return;
		}
		// Normal sim
		else
		{
			addEnemyToFAP(enemy_units, army_size); // Add Enemy first - changes the sim_size

			addFriendlyToFAP(friendly_types, army_size); // Add Friendly units to FAP and get initial unit scores

			MCfap.simulate(); //default 96 frames

			updateFAPScores();

			setUnitRanks(); // Rank the post sim scores
			setOptimalUnit(); // Run after setting/sorting unit ranks
		}
	}

	///// Returns the unittype that is the "best" of a BuildFAP sim.
	//void simulate(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, int army_size, const int sims)
	//{
	//	std::map<BWAPI::UnitType, int> scores;
	//	std::vector<UnitData> friendly_ud;

	//	// Convert each UnitType to UnitData and get initial score
	//	for (const auto& ut : friendly_types)
	//	{
	//		if (isValidType(ut))
	//		{
	//			UnitData ud{ ut, BWAPI::Broodwar->self() };

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

	//	FAP::FastAPproximation<UnitData*> MCfap;

	//	addEnemyToFAP(MCfap, enemy_units, sim_size); // Add Enemy first - changes the sim_size
	//	addFriendlyToFAP(MCfap, friendly_ud, sim_size);

	//	MCfap.simulate(); //default 96 frames

	//	updateScores(*MCfap.getState().first, scores);

	//	// Store the last optimal unit if we had one
	//	res = getBestScoredType(scores);
	//	last_optimal = res;

	//	return res;
	//}

	/// Return top scored friendly unittype of the sim
	BWAPI::UnitType Brawl::getOptimalUnit()
	{
		return optimal_unit;
	}

	/// Return BWAPI::UnitType and score of each unit in sim
	std::vector<std::pair<BWAPI::UnitType, int>> Brawl::getUnitRanks()
	{
		return unit_ranks;
	}

	/// Draw the OVERALL most optimal unit to the desired coordinates
	void Brawl::drawOptimalUnit(const int x, const int y)
	{
		BWAPI::Broodwar->drawTextScreen(x, y, "Brawl Unit: %s", getOptimalUnit().c_str());
	}
	/// @Overload
	void Brawl::drawOptimalUnit(const BWAPI::Position& pos)
	{
		BWAPI::Broodwar->drawTextScreen(pos, "Brawl Unit: %s", getOptimalUnit().c_str());
	}

	void Brawl::drawUnitRank(const int x, const int y)
	{
		int spacing = 0;
		for (const auto& u : getUnitRanks())
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