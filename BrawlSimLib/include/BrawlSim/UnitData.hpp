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


		UnitData(const BWAPI::UnitType& u, const BWAPI::Player& p)
			: type(u)
			, player(p)
			, pre_sim_score(initialUnitScore(type))
		{
		}

	private:
		/// Return a initial score for a unittype based on economic value
		inline int initialUnitScore(const BWAPI::UnitType& ut) const
		{
			return static_cast<int>(ut.mineralPrice() + (1.25 * ut.gasPrice()) + (25 * (ut.supplyRequired() / 2)));
		}
	};

}