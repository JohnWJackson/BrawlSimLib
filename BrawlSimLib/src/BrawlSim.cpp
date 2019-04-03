#include "../../BrawlSimLib/include/BrawlSim.hpp"

using namespace impl;

namespace BrawlSim
{

	namespace
	{

		/// Convert a BWAPI::UnitType to a FAP_Unit and add to the simulation
		template <bool friendly>
		void addToMCFAP(FAP::FastAPproximation<UnitData*>& fap_object, PlayerData<friendly>& player)
		{
			for (const auto& u : player.unit_map_)
			{
				UnitData unit_data(u.first, player);

				auto fap_unit = unit_data.convertToFAPUnit(player);
			
				if (friendly)
				{
					fap_object.addIfCombatUnitPlayer1(std::move(fap_unit)); //std::move rvalue cast
				}

				else
				{
					// Add a fap unit for however many units the enemy has of that unittype
					for (int i = 0; i < u.second; ++i)
					{
						fap_object.addIfCombatUnitPlayer2(std::move(fap_unit)); //std::move rvalue cast
					}
				}
			}
		}

		/// Get the Total FAP Score of units for the player
		template <bool friendly>
		int getTotalFAPScore(FAP::FastAPproximation<UnitData*>& fap) 
		{
			auto lambda = [&](int currentScore, auto FAPunit)
			{
				return static_cast<int>(currentScore + FAPunit.data->pre_sim_score_);
			};

			if (friendly && fap.getState().first && !fap.getState().first->empty())
			{
				return std::accumulate(fap.getState().first->begin(), 
									   fap.getState().first->end(), 
					                   0, 
					                   lambda);
			}
			else if (!friendly && fap.getState().second && !fap.getState().second->empty())
			{
				return std::accumulate(fap.getState().second->begin(),
									   fap.getState().second->end(),
									   0,
									   lambda);
			}
			else return 0;
		}

		/// Updates the FAP Score of players units after sim
		void updateFAPvalue(std::vector<FAP::FAPUnit<UnitData*>>& fap_vector, std::map<BWAPI::UnitType, int>& unit_map)
		{
			for (auto& fu : fap_vector)
			{
				if (fu.data)
				{
					double proportion_health = (fu.health + fu.shields) / static_cast<double>(fu.maxHealth + fu.maxShields);
					fu.data->post_sim_score_ = static_cast<int>(fu.data->pre_sim_score_ * proportion_health);
					unit_map[fu.unitType] = fu.data->post_sim_score_;
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

		updateFAPvalue(*MCfap.getState().first, player.unit_map_);

		int best_sim_score = INT_MIN;
		BWAPI::UnitType build_type = BWAPI::UnitTypes::None;
		for (auto& potential_type : player.unit_map_) 
		{
			// there are several cases where the test return ties, ex: cannot see enemy units and they appear "empty", extremely one-sided combat...
			if (potential_type.second > best_sim_score)
			{
				best_sim_score = potential_type.second;
				build_type = potential_type.first;
			}
			// there are several cases where the test return ties, ex: cannot see enemy units and they appear "empty", extremely one-sided combat...
			else if (potential_type.second == best_sim_score)
			{
				// if the current unit is "flexible" with regard to air and ground units, then keep it and continue to consider the next unit.
				if (build_type.airWeapon() != BWAPI::WeaponTypes::None && build_type.groundWeapon() != BWAPI::WeaponTypes::None)
				{
					continue;
				}
				// if the tying unit is "flexible", then let's use that one.
				else if (potential_type.first.airWeapon() != BWAPI::WeaponTypes::None && potential_type.first.groundWeapon() != BWAPI::WeaponTypes::None)
				{
					build_type = potential_type.first;
				}
			}
		}
		return build_type;
	}

}