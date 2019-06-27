#include "gtest/gtest.h"
#include "deinos/chess.h"
#include "deinos/algorithm.h"
using namespace std;
using namespace chess;
using namespace algorithm;

TEST(AnalysedPositionTest, EnPassant)
{
	Position pos("rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
	AnalysedPosition apos(pos);
	optional<Move> mv = find_move("Pe5xd6", apos.moves());
	ASSERT_TRUE(mv != nullopt);
	Position pos2 = Position(pos, mv.value());
	EXPECT_EQ(pos2.as_fen(), "rnbqkbnr/pp2pppp/3P4/2p5/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
}

TEST(AnalysedPositionTest, Check)
{
	
	Position pos ("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
	AnalysedPosition apos(pos);
	EXPECT_FALSE(apos.in_check(Alignment::Black));
	EXPECT_FALSE(apos.in_check(Alignment::White));
	EXPECT_FALSE(apos.legal_check());
	EXPECT_FALSE(apos.illegal_check());
	
	optional<Move> mv = find_move("Bf1-b5", apos.moves());
	ASSERT_TRUE(mv != nullopt);
	pos = Position(pos, mv.value());
	apos = AnalysedPosition(pos);
	EXPECT_TRUE(apos.in_check(Alignment::Black));
	EXPECT_FALSE(apos.in_check(Alignment::White));
	EXPECT_TRUE(apos.legal_check());
	EXPECT_FALSE(apos.illegal_check());

	Position pos2 ("3Kk3/8/8/8/8/8/8/8 b - - 12 30");
	AnalysedPosition apos2(pos2);
	EXPECT_TRUE(apos2.in_check(Alignment::Black));
	EXPECT_TRUE(apos2.in_check(Alignment::White));
	EXPECT_TRUE(apos2.legal_check());
	EXPECT_TRUE(apos2.illegal_check());
}

TEST(TreeTest, Stalemate)
{
	const auto dumb_val = [&] (const AnalysedPosition& ) {return 0.5;};
	const auto dumb_pri = [&] (const AnalysedPosition& ap, const Move&) {return 1.0 / (double) ap.moves().size();};
	
	AnalysedPosition apos(Position("7k/3R4/8/8/4K3/6R1/8/8 b - - 1 1"));
	Tree tree(apos, dumb_val, dumb_pri);
	EXPECT_EQ(tree.base->result(), nullopt);
	tree.search();
	EXPECT_EQ(tree.base->result(), make_optional(GameResult::Draw));
}

TEST(TreeTest, Checkmate)
{
	const auto dumb_val = [&] (const AnalysedPosition& ) {return 0.5;};
	const auto dumb_pri = [&] (const AnalysedPosition& ap, const Move&) {return 1.0 / (double) ap.moves().size();};
	
	AnalysedPosition apos(Position("8/7k/8/7R/4K3/6R1/8/8 b - - 3 2"));
	Tree tree(apos, dumb_val, dumb_pri);
	EXPECT_EQ(tree.base->result(), nullopt);
	tree.search();
	EXPECT_EQ(tree.base->result(), make_optional(GameResult::White));

	AnalysedPosition apos2(Position("8/8/8/8/8/4k3/3q4/3K4 w - - 1 2"));
	Tree tree2(apos2, dumb_val, dumb_pri);
	EXPECT_EQ(tree2.base->result(), nullopt);
	tree2.search();
	EXPECT_EQ(tree2.base->result(), make_optional(GameResult::Black));
}

TEST(TreeTest, FindMateInOne)
{
	const auto dumb_val = [&] (const AnalysedPosition& ) {return 0.5;};
	const auto dumb_pri = [&] (const AnalysedPosition& ap, const Move&) {return 1.0 / (double) ap.moves().size();};

	AnalysedPosition apos(Position("rnbq1rk1/pp1pnppp/2pb4/1B6/3Q4/1P2P3/PBP2PPP/RN2K1NR w KQ - 0 7"));
	Tree tree(apos, dumb_val, dumb_pri);
	for (int i = 0; i < 1000; i++) tree.search();
	Move mv = tree.base->apos->moves()[tree.base->preferred_index()];
	EXPECT_EQ((string) mv, "Qd4xg7");
}

TEST(TreeTest, AvoidMateInOne)
{
	const auto dumb_val = [&] (const AnalysedPosition& ) {return 0.5;};
	const auto dumb_pri = [&] (const AnalysedPosition& ap, const Move&) {return 1.0 / (double) ap.moves().size();};

	AnalysedPosition apos(Position("rnbq1rk1/ppppnppp/8/1B2Q3/8/1P2P3/PBP2PPP/RN2K1NR b KQ - 0 7"));
	Tree tree(apos, dumb_val, dumb_pri);
	for (int i = 0; i < 10000; i++) tree.search();
	const int index = tree.base->preferred_index();
	cerr << tree.base->display();
	cerr << tree.base->child(index)->display();
	Move mv = tree.base->apos->moves()[index];
	const string name = (string) mv;
	const bool success = (name == "Pf7-f6" || name == "Ne7-f5");
	EXPECT_TRUE(success);
}