#include "..\..\BrawlSimLib\include\BrawlSim.hpp"

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

	/// Scale enemy unit composition and add the enemy types to the sim
	void Brawl::addEnemyTypes(const BWAPI::Unitset& units, const BWAPI::UnitType& friendly_type, int army_size)
	{
		if (enemy_data.empty()) // only build enemy data once
		{
			int unit_total = 0;
			for (const auto& u : units) //count unittypes
			{
				if (isValidType(u->getType()))
				{
					enemy_data[UnitData(u->getType(), BWAPI::Broodwar->enemy())]++;
					++unit_total;
				}
			}

			for (const auto& ut : enemy_data)
			{
				double percent = ut.second / (double)unit_total;
				int scaled_count = std::lround(percent * army_size);

				enemy_score += ut.first.pre_score * scaled_count;
				enemy_data[ut.first] = scaled_count;
			}
		}

		int attack_count = 0;
		valid_enemies = true;
		for (auto& u : enemy_data)
		{
			if ((friendly_type.maxAirHits() && u.first.type.isFlyer()) || (friendly_type.maxGroundHits() && !u.first.type.isFlyer()))
			{
				attack_count++;
			}
			for (int i = 0; i < u.second; ++i)
			{
				MCfap.addUnitPlayer2(u.first.convertToFAPUnit());
			}
		}
		if (attack_count == 0) //the friendly type can't attack any of the enemy units in the sim
		{
			unit_ranks.push_back(std::make_pair(friendly_type, 0));
			valid_enemies = false;
		}
	}

	/// Add valid friendly units to the FAP sim and score them. Return map of the UnitType and score
	void Brawl::addFriendlyType(const BWAPI::UnitType& type, int& army_size)
	{
		friendly_data.push_back(UnitData(type, BWAPI::Broodwar->self()));

		int friendly_score = 0;
		if (type.isTwoUnitsInOneEgg()) //zerglings and scourges
		{
			while (friendly_score < enemy_score * 2) // add double the units
			{
				MCfap.addUnitPlayer1(std::move(friendly_data.back().convertToFAPUnit()));
				friendly_score += friendly_data.back().pre_score;
				++army_size;
			}
		}
		else
		{
			while (friendly_score < enemy_score)
			{
				MCfap.addUnitPlayer1(std::move(friendly_data.back().convertToFAPUnit()));
				friendly_score += friendly_data.back().pre_score;
				++army_size;
			}
		}

		if (friendly_score < enemy_score - (friendly_data.back().pre_score / 4)) // Add one more unit to friendly sim if scores aren't very even
		{
			MCfap.addUnitPlayer1(std::move(friendly_data.back().convertToFAPUnit()));
			friendly_score += friendly_data.back().pre_score;
			++army_size;
		}
	}

	/// Updates the UnitData's score to post FAP sim values
	void Brawl::setPostRank(const int army_size)
	{
		int i = 0;
		int score = 0;
		// Accumulate total score for the unittype and increment the count;
		for (auto& fu : *MCfap.getState().first)
		{
			double proportion_health = (fu.health + fu.shields) / (double)(fu.maxHealth + fu.maxShields);

			score = static_cast<int>((i * score + (proportion_health * fu.data->pre_score)) / (i + 1));
			++i;
		}
		for (i; i < army_size; ++i) //size discrepancy, these units died in sim
		{
			score = static_cast<int>((i * score) / (i + 1));
		}

		unit_ranks.push_back(std::make_pair(friendly_data.back().type, score));
	}

	/// Sort in descending order
	void Brawl::sortRanks()
	{
		sort(unit_ranks.begin(), unit_ranks.end(), [&](std::pair<BWAPI::UnitType, int>& lhs, std::pair<BWAPI::UnitType, int>& rhs)
		{
			return lhs.second > rhs.second;
		});
	}

	/// Set the optimal unit to the friendly UnitType with the highest score. If scores are tied, chooses more "flexible" UnitType.
	void Brawl::setOptimalUnit()
	{
		int best_sim_score = INT_MIN;
		BWAPI::UnitType res = BWAPI::UnitTypes::None;

		// ranks are sorted so just check scores that are equal to the first unit's score
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
		unit_ranks.reserve(friendly_types.size());
		friendly_data.reserve(friendly_types.size());

		//Optimal is BWAPI::UnitType::None if no simmable friendly UnitData
		if (friendly_types.empty())
		{
			return;
		}
		//Return best initial (economic) score if there are no enemy units to sim against
		else if (enemy_units.empty())
		{
			for (auto& type : friendly_types)
			{
				if (isValidType(type))
				{
					friendly_data.push_back(UnitData(type, BWAPI::Broodwar->self()));
					unit_ranks.push_back(std::make_pair(type, friendly_data.back().pre_score)); // Set the initial ecnomic rank of last UnitData
				}
			}
			sortRanks();
			setOptimalUnit();
			return;
		}
		else
		{
			for (auto& type : friendly_types)  //simming each type against the enemy
			{
				if (isValidType(type))
				{
					addEnemyTypes(enemy_units, type, army_size); //Build enemy unit data and add to sim

					if (valid_enemies)
					{
						addFriendlyType(type, army_size); //Add as many types as enemy score allows for even sim

						MCfap.simulate();

						setPostRank(army_size); // Set the types post FAP-sim score
						MCfap.clear();
					}
				}
			}
			sortRanks();
			setOptimalUnit();
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