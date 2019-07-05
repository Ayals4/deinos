#include "gtest/gtest.h"
#include "deinos/chess.h"
#include "deinos/algorithm.h"
using namespace std;
using namespace chess;
using namespace algorithm;

TEST(AnalysedPositionTest, EnPassant)
{
	Position pos("rnbqkbnr/pp2pppp/8/2ppP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
	//cerr << pos;
	AnalysedPosition apos(pos);
	//for (auto mr : apos.moves()) cerr << mr << endl;
	//cerr << apos.find_record("e5d6").value() << endl;
	Position pos2 = apos.find_record("e5d6").value().apply();
	//cerr << pos2;
	EXPECT_EQ(pos2.as_fen(), "rnbqkbnr/pp2pppp/3P4/2p5/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 3");
}

TEST(AnalysedPositionTest, CastlingOpen)
{
	Position pos("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 3");
	AnalysedPosition apos(pos);
	EXPECT_TRUE(apos.find_record("e1g1"));
	EXPECT_TRUE(apos.find_record("e1c1"));
	Position pos2("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R b KQkq - 0 3");
	AnalysedPosition apos2(pos2);
	EXPECT_TRUE(apos2.find_record("e8g8"));
	EXPECT_TRUE(apos2.find_record("e8c8"));
}

TEST(AnalysedPositionTest, CastlingBlocked)
{
	Position pos("r3k2r/pqppn1pp/bpn2p2/4p3/1bBPP3/NP3N2/PBP1QPPP/R3K2R w KQkq - 0 1");
	AnalysedPosition apos(pos);
	EXPECT_FALSE(apos.find_record("e1g1"));
	EXPECT_FALSE(apos.find_record("e1c1"));
	Position pos2("r3k2r/pqppn1pp/bpn2p2/4p3/1bBPP3/NP3N2/PBP1QPPP/R3K2R b KQkq - 0 1");
	AnalysedPosition apos2(pos2);
	EXPECT_FALSE(apos2.find_record("e8g8"));
	EXPECT_TRUE(apos2.find_record("e8c8"));
}

TEST(AnalysedPositionTest, Check)
{
	
	Position pos ("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
	AnalysedPosition apos(pos);
	EXPECT_FALSE(apos.in_check(Almnt::Black));
	EXPECT_FALSE(apos.in_check(Almnt::White));
	EXPECT_FALSE(apos.legal_check());
	EXPECT_FALSE(apos.illegal_check());
	
	pos = apos.find_record("f1b5").value().apply();//Position(pos, mv.value());
	apos = AnalysedPosition(pos);
	EXPECT_TRUE(apos.in_check(Almnt::Black));
	EXPECT_FALSE(apos.in_check(Almnt::White));
	EXPECT_TRUE(apos.legal_check());
	EXPECT_FALSE(apos.illegal_check());

	Position pos2 ("3Kk3/8/8/8/8/8/8/8 b - - 12 30");
	AnalysedPosition apos2(pos2);
	EXPECT_TRUE(apos2.in_check(Almnt::Black));
	EXPECT_TRUE(apos2.in_check(Almnt::White));
	EXPECT_TRUE(apos2.legal_check());
	EXPECT_TRUE(apos2.illegal_check());
}

TEST(AnalysedPositionTest, Occlusion1)
{
	AnalysedPosition apos(Position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"));
	MoveRecord mr("g1", "f3");
	auto info = apos.get_occlusion(mr);
	//EXPECT_EQ(info.counts[0], 4);
	for (int i = 0; i < info.counts[0]; i++) cerr << (string) info.squares[0][i] << " ";
	cerr << endl;
	//EXPECT_EQ(info.counts[1], 1);
	for (int i = 0; i < info.counts[1]; i++) cerr << (string) info.squares[1][i] << " ";
	cerr << endl;
	//cerr << apos;
}

TEST(AnalysedPositionTest, Occlusion2)
{
	AnalysedPosition apos(Position("7k/8/3p4/2p5/1b1B1R2/4n3/1Q1P4/8 w - - 0 1"));
	MoveRecord mr("d4", "f6");
	auto info = apos.get_occlusion(mr);
	EXPECT_EQ(info.counts[0], 5);
	for (int i = 0; i < info.counts[0]; i++) cerr << (string) info.squares[0][i] << " ";
	cerr << endl;
	EXPECT_EQ(info.counts[1], 1);
	for (int i = 0; i < info.counts[1]; i++) cerr << (string) info.squares[1][i] << " ";
	cerr << endl;
	//cerr << apos;
}

TEST(AnalysedPositionTest, Occlusion3)
{
	AnalysedPosition apos(Position("rnbqkbnr/pppppppp/8/8/6Q1/8/PPPPPPPP/RNB1KBNR w KQkq - 0 1"));
	MoveRecord mr("g4", "h3");
	auto info = apos.get_occlusion(mr);
	EXPECT_EQ(info.counts[0], 4);
	for (int i = 0; i < info.counts[0]; i++) cerr << (string) info.squares[0][i] << " ";
	cerr << endl;
	EXPECT_EQ(info.counts[1], 0);
	for (int i = 0; i < info.counts[1]; i++) cerr << (string) info.squares[1][i] << " ";
	cerr << endl;
	//cerr << apos;
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
	Move mv = tree.base->best_move();
	EXPECT_EQ((string) mv, "Qd4xg7");
}

TEST(TreeTest, AvoidMateInOne)
{
	const auto dumb_val = [&] (const AnalysedPosition& ) {return 0.5;};
	const auto dumb_pri = [&] (const AnalysedPosition& ap, const Move&) {return 1.0 / (double) ap.moves().size();};

	AnalysedPosition apos(Position("rnbq1rk1/ppppnppp/8/1B2Q3/8/1P2P3/PBP2PPP/RN2K1NR b KQ - 0 7"));
	Tree tree(apos, dumb_val, dumb_pri);
	for (int i = 0; i < 10000; i++) tree.search();
	//const int index = tree.base->preferred_index();
	//cerr << tree.base->display();
	//cerr << tree.base->child(index)->display();
	Move mv = tree.base->best_move();
	const string name = (string) mv;
	const bool success = (name == "Pf7-f6" || name == "Ne7-f5");
	EXPECT_TRUE(success);
}