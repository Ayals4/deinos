#ifndef DEINOS_DSAI_H
#define DEINOS_DSAI_H
#include "chess.h"
#include "algorithm.h"

//ideas:
//incentivise exchanging off pieces when ahead
//incentivise pawn pushing
//incentivise controlling squares near enemy king

namespace dsai {
	inline float uniform_vf(const algorithm::AnalysedPosition&) {return 0.5;}
	inline float uniform_pf(const algorithm::AnalysedPosition& ap, const chess::Move&) {return 1.0f / (float) ap.moves().size();};

	float material_vf(const algorithm::AnalysedPosition& ap);
	float material_score(const algorithm::AnalysedPosition& ap);
	float king_ctrl_score(const algorithm::AnalysedPosition& ap, chess::Alignment a);

	float test_vf(const algorithm::AnalysedPosition& ap);
}
#endif