#include "dsai.h"
#include <cmath>
using namespace std;
using namespace chess;
using namespace algorithm;
using namespace dsai;

float dsai::material_vf(const AnalysedPosition& ap)
{
	const auto& pos = ap.pos();

	constexpr array<float, 7> values {0.0f, 1.0f, 2.5f, 3.0f, 5.0f, 9.0f, 0.0f};
	array<float, 2> totals {0.0f, 0.0f};
	float ctrl_dif = 0.0f;

	for(Square s : all_squares) {
		const Piece p = pos.at(s);
		totals[static_cast<uint8_t>(p.almnt())] += values[static_cast<uint8_t>(p.type())];
		ctrl_dif += ap.ctrl(Almnt::White, s);
		ctrl_dif -= ap.ctrl(Almnt::Black, s);
	}

	float king_ctrl_adj = 0.0;
	const float total_dif = totals[0] - totals[1];
	if (total_dif > 10.0f) king_ctrl_adj += king_ctrl_score(ap, Almnt::White);
	if (total_dif < -10.0f) king_ctrl_adj -= king_ctrl_score(ap, Almnt::Black);

	constexpr float gain = 0.3f;
	const float weight = totals[0] - totals[1] + ctrl_dif * 0.05f + king_ctrl_adj;

	const float result = 0.5f * (tanhf(gain * 0.5f * weight) + 1.0f);

	return result;
}

/*float dsai::material_score(const AnalysedPosition& ap)
{
	const auto& pos = ap.pos();
	
	constexpr array<float, 7> values {0.0f, 1.0f, 2.5f, 3.0f, 5.0f, 9.0f, 0.0f};
	array<float, 2> totals {0.0f, 0.0f};

	for(Square s : all_squares) {
		const Piece p = pos[s];
		totals[static_cast<uint8_t>(p.alignment())] += values[static_cast<uint8_t>(p.type())];
	}

	return totals[0] - totals[1];
}*/

float dsai::king_ctrl_score(const AnalysedPosition& ap, const Almnt a)
{
	//const auto& pos = ap.pos();
	const Square ksq = ap.king_sq(!a);
	
	constexpr array<pair<int, int>, 8> trans {
		make_pair<int>(0,1),make_pair<int>(1,0),make_pair<int>(0,-1),make_pair<int>(-1,0),
		make_pair<int>(1,1),make_pair<int>(1,-1),make_pair<int>(-1,1),make_pair<int>(-1,-1)
	};
	float total {0.0f};

	for (const auto& t : trans) {
		const auto target = ksq.translate(t.first, t.second);
		if (target) total += 0.1 * ap.ctrl(a, target.value());
	}

	return total;
}

/*float dsai::test_vf(const AnalysedPosition& ap)
{
	float weight = material_score(ap);
	weight += king_ctrl_score(ap);

	constexpr float gain = 0.3f; //advantage of 5pts ~0.8 win
	
	const float result = 0.5f * (tanhf(gain * 0.5f * weight) + 1.0f);
	
	return result;
}*/