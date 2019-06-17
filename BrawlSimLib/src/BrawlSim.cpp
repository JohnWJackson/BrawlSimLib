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
			type == BWAPI::UnitTypes::Terran_Nuclear_Missile ||
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
					UnitData temp = UnitData(u->getType(), BWAPI::Broodwar->enemy());
					enemy_data[temp]++;
					enemy_score += temp.eco_score;
					++unit_total;
				}
			}
			if (army_size != -1) // if army_size == -1 no scaling
			{
				for (const auto& ut : enemy_data)
				{
					double percent = ut.second / (double)unit_total;
					int scaled_count = std::lround(percent * army_size);

					enemy_data[ut.first] = scaled_count;
				}
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

		army_size = 0;
		if (type.isTwoUnitsInOneEgg()) //zerglings and scourges
		{
			while (friendly_score < enemy_score) // add double the units
			{
				for (int i = 0; i < 2; i++)
				{
					MCfap.addUnitPlayer1(std::move(friendly_data.back().convertToFAPUnit()));
				}
				friendly_score += friendly_data.back().eco_score;
				++army_size;
			}
		}
		else
		{
			while (friendly_score < enemy_score)
			{
				MCfap.addUnitPlayer1(std::move(friendly_data.back().convertToFAPUnit()));
				friendly_score += friendly_data.back().eco_score;
				++army_size;
			}
		}

		if (friendly_score < enemy_score - (friendly_data.back().eco_score / 4)) // Add one more unit to friendly sim if scores aren't very even
		{
			MCfap.addUnitPlayer1(std::move(friendly_data.back().convertToFAPUnit()));
			friendly_score += friendly_data.back().eco_score;
			++army_size;
		}
	}

	void Brawl::checkAliveUnits(std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>>& friendly_pre_units, std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>>& enemy_pre_units)
	{
		checkAliveFriendly(friendly_pre_units);
		checkAliveEnemy(enemy_pre_units);
	}

	void Brawl::checkAliveFriendly(std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>>& friendly_pre_units)
	{
		size_t i = 0, j = 0;
		while (i < MCfap.getState().first->size())
		{
			// Unit died
			if (MCfap.getState().first->at(i).data != friendly_pre_units.at(j).first.data)
			{
				friendly_score -= friendly_pre_units.at(j).first.data->eco_score;
				friendly_pre_units.at(j).second = 0;
				++j;
			}
			// Unit alive
			else
			{
				++i;
				++j;
			}
		}
		for (j; j < friendly_pre_units.size(); ++j)
		{
			friendly_score -= friendly_pre_units.at(j).first.data->eco_score;
		}
	}

	void Brawl::checkAliveEnemy(std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>>& enemy_pre_units)
	{
		size_t i = 0, j = 0;
		while (i < MCfap.getState().second->size())
		{
			// Unit died
			if (MCfap.getState().second->at(i).data != enemy_pre_units.at(j).first.data)
			{
				enemy_score -= enemy_pre_units.at(j).first.data->eco_score;
				enemy_pre_units.at(j).second = 0;
				++j;
			}
			// Unit alive
			else
			{
				++i;
				++j;
			}
		}
		for (j; j < enemy_pre_units.size(); ++j)
		{
			enemy_score -= enemy_pre_units.at(j).first.data->eco_score;
		}
	}

	/// Updates the UnitData's score to post FAP sim values
	void Brawl::setPostRank(const int scoring_type, const int army_size)
	{
		double initial_score = 0;
		switch (scoring_type)
		{
		case 0:
			initial_score = friendly_data.back().survival_rate;
			break;
		case 1:
			initial_score = friendly_data.back().eco_score;
			break;
		case 2:
			initial_score = 1;
			break;
		}

		double res_score = 0;
		int i = 0;
		// Accumulate total score for the unittype and increment the count;
		for (auto& fu : *MCfap.getState().first)
		{
			double proportion_health = (fu.health + fu.shields) / (double)(fu.maxHealth + fu.maxShields);

			res_score = (i * res_score + (proportion_health * initial_score)) / (double)(i + 1);
			++i;
		}
		for (i; i < army_size; ++i) //size discrepancy, these units died in sim
		{
			res_score = (i * res_score) / (double)(i + 1);
		}
		unit_ranks.push_back(std::make_pair(friendly_data.back().type, res_score));
	}

	/// Sort in descending order
	void Brawl::sortRanks()
	{
		sort(unit_ranks.begin(), unit_ranks.end(), [&](std::pair<BWAPI::UnitType, double>& lhs, std::pair<BWAPI::UnitType, double>& rhs)
		{
			return lhs.second > rhs.second;
		});
	}

	/// Set the optimal unit to the friendly UnitType with the highest score. If scores are tied, chooses more "flexible" UnitType.
	void Brawl::setOptimalUnit()
	{
		double best_sim_score = INT_MIN;
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
	void Brawl::simulateEach(const BWAPI::UnitType::set& friendly_types, const BWAPI::Unitset& enemy_units, const int scoring_type, int army_size, const int sims)
	{
		resetFlags();
		unit_ranks.reserve(friendly_types.size());
		friendly_data.reserve(friendly_types.size());

		//Optimal is BWAPI::UnitType::None if no simmable friendly UnitData
		if (friendly_types.empty())
		{
			return;
		}
		//Return best initial score if there are no enemy units to sim against
		else if (enemy_units.empty())
		{
			for (auto& type : friendly_types)
			{
				if (isValidType(type) && type.maxGroundHits()) //Dont consider units that can only shoot air initially
				{
					friendly_data.push_back(UnitData(type, BWAPI::Broodwar->self()));

					switch (scoring_type)
					{
					case 0:
						unit_ranks.push_back(std::make_pair(type, friendly_data.back().survival_rate));
						break;
					case 1:
						unit_ranks.push_back(std::make_pair(type, friendly_data.back().eco_score));
						break;
					case 2:
						unit_ranks.push_back(std::make_pair(type, 1));
						break;
					}
				}
				sortRanks();
				setOptimalUnit();
			}
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

						setPostRank(scoring_type, army_size); // Set the types post FAP-sim score
					}
					MCfap.clear();
				}
			}
			sortRanks();
			setOptimalUnit();
		}
		simEachFlag = true;
	}

	/// Simulate an entire friendly force against an entire enemy force
	void Brawl::simulateForces(const BWAPI::Unitset& friendly_units, const BWAPI::Unitset& enemy_units, const int sims)
	{
		resetFlags();
		unit_ranks.reserve(friendly_units.size());
		friendly_data.reserve(enemy_units.size());

		// Invalid Simulation - one of the sides doesn't have any units to simulate against
		if (friendly_units.empty() || enemy_units.empty())
		{
			return;
		}
		// Valid Simulation
		else
		{
			// Setup friendly units
			for (auto& unit : friendly_units)
			{
				if (isValidType(unit->getType()))
				{
					friendly_data.push_back(UnitData(unit->getType(), BWAPI::Broodwar->self()));
					friendly_score += friendly_data.back().eco_score;

					if (unit->getType().isTwoUnitsInOneEgg())
					{
						for (int i = 0; i < 2; i++)
						{
							MCfap.addUnitPlayer1(std::move(friendly_data.back().convertToFAPUnit()));
						}
					}
					else
					{
						MCfap.addUnitPlayer1(std::move(friendly_data.back().convertToFAPUnit()));
					}
				}
			}
			// Take a copy of friendly pre simulation units
			std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>> friendly_pre_units;
			for (auto& u : *MCfap.getState().first)
			{
				auto temp = std::make_pair(u, 1);
				friendly_pre_units.push_back(temp);
			}

			// Setup enemy units
			for (auto& unit : enemy_units)
			{
				if (isValidType(unit->getType()))
				{
					UnitData temp = UnitData(unit->getType(), BWAPI::Broodwar->enemy());
					enemy_data[temp]++;
					enemy_score += temp.eco_score;

					if (unit->getType().isTwoUnitsInOneEgg())
					{
						for (int i = 0; i < 2; i++)
						{
							MCfap.addUnitPlayer2(std::move(temp.convertToFAPUnit()));
						}
					}
					else
					{
						MCfap.addUnitPlayer2(std::move(temp.convertToFAPUnit()));
					}
				}
			}
			// Take a copy of enemy pre simulation units
			std::vector<std::pair<FAP::FAPUnit<UnitData*>, int>> enemy_pre_units;
			for (auto& u : *MCfap.getState().second)
			{
				auto temp = std::make_pair(u, 1);
				enemy_pre_units.push_back(temp);
			}

			MCfap.simulate();

			checkAliveUnits(friendly_pre_units, enemy_pre_units);

			MCfap.clear();
		}
		simForcesFlag = true;
	}

	/// Return top scored friendly unittype of the sim
	BWAPI::UnitType Brawl::getOptimalUnit() const
	{
		if (simEachFlag)
		{
			return optimal_unit;
		}
		else
		{
			BWAPI::Broodwar->sendText("Invalid use of getOptimalUnit()");
		}
	}

	/// Return BWAPI::UnitType and score of each unit in sim
	std::vector<std::pair<BWAPI::UnitType, double>> Brawl::getUnitRanks() const
	{
		if (simEachFlag)
		{
			return unit_ranks;
		}
		else
		{
			BWAPI::Broodwar->sendText("Invalid use of getUnitRanks()");
		}
	}

	/// Return the force with the highest score
	std::pair<BWAPI::Player, int> Brawl::getBestForce() const
	{
		if (simForcesFlag)
		{
			if (friendly_score > enemy_score)
			{
				return std::make_pair(BWAPI::Broodwar->self(), friendly_score);
			}
			else if (friendly_score < enemy_score)
			{
				return std::make_pair(BWAPI::Broodwar->enemy(), enemy_score);
			}
			else
			{
				return std::make_pair(BWAPI::Broodwar->self(), NULL);
			}
		}
		else
		{
			BWAPI::Broodwar->sendText("Invalid use of getBestForces()");
			return std::make_pair(BWAPI::Broodwar->self(), NULL);
		}
	}

	/// Draw the winning force and score in a unit vs unit simulation
	void Brawl::drawBestForce(const int x, const int y) const
	{
		if (simForcesFlag)
		{
			BWAPI::Broodwar->drawTextScreen(x, y, "Force: %s", getBestForce().first);
			BWAPI::Broodwar->drawTextScreen(x + 100, y, "Score: %s", getBestForce().second);
		}
		else
		{
			BWAPI::Broodwar->sendText("Invalid use of drawBestForce()");
		}
	}

	/// Draw the OVERALL most optimal unit to the desired coordinates
	void Brawl::drawOptimalUnit(const int x, const int y) const
	{
		if (simEachFlag)
		{
			BWAPI::Broodwar->drawTextScreen(x, y, "Brawl Unit: %s", getOptimalUnit().c_str());
		}
		else
		{
			BWAPI::Broodwar->sendText("Invalid use of drawOptimalUnit()");
		}
	}
	void Brawl::drawOptimalUnit(const BWAPI::Position& pos) const
	{
		if (simEachFlag)
		{
			BWAPI::Broodwar->drawTextScreen(pos, "Brawl Unit: %s", getOptimalUnit().c_str());
		}
		else
		{
			BWAPI::Broodwar->sendText("Invalid use of drawOptimalUnit()");
		}
	}

	/// Draw the ranked unit list to the screen
	void Brawl::drawUnitRank(const int x, const int y) const
	{
		if (simEachFlag)
		{
			int spacing = 0;
			for (const auto& u : getUnitRanks())
			{
				BWAPI::Broodwar->drawTextScreen(x, y + spacing, u.first.c_str());
				BWAPI::Broodwar->drawTextScreen(x + 200, y + spacing, std::to_string(u.second).c_str());
				spacing += 12;
			}
		}
		else
		{
			BWAPI::Broodwar->sendText("Invalid use of drawUnitRank()");
		}
	}

	void Brawl::resetFlags()
	{
		simEachFlag = false;
		simForcesFlag = false;
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