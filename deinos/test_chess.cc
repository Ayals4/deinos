#include "gtest/gtest.h"
#include "deinos/chess.h"
#include <string>
#include <optional>
using namespace std;
using namespace chess;

TEST(AlignmentTest, Inversion)
{
	auto w = Almnt::White;
	auto b = Almnt::Black;
	EXPECT_EQ(!w, b);
	EXPECT_EQ(!b, w);
}

TEST(AlResTest, AlmntComp)
{
	EXPECT_EQ(Almnt::White, AlRes::White);
	EXPECT_NE(Almnt::Black, AlRes::White);
	EXPECT_NE(Almnt::Black, AlRes::None);
}

TEST(GameResultTest, Evaluation)
{
	EXPECT_EQ(evaluate(GameResult::White), 1.0f);
	EXPECT_EQ(evaluate(GameResult::Black), 0.0f);
	EXPECT_EQ(evaluate(GameResult::Draw), 0.5f);
}

TEST(PieceTest, DefaultConstructor)
{
	Piece default_piece;
	Piece intended {Almnt::White, Piece::Type::Empty};
	EXPECT_EQ(default_piece, intended);
	EXPECT_EQ(default_piece.almnt_res(), AlRes::None);
}

TEST(PieceTest, Construction)
{
	Piece w_bishop(Almnt::White, Piece::Type::Bishop);
	EXPECT_EQ(w_bishop.type(), Piece::Type::Bishop);
	EXPECT_EQ(w_bishop.almnt(), Almnt::White);
	EXPECT_EQ(w_bishop.almnt_res(), AlRes::White);
	Piece b_king(Almnt::Black, Piece::Type::King);
	EXPECT_EQ(b_king.type(), Piece::Type::King);
	EXPECT_EQ(b_king.almnt(), Almnt::Black);
	EXPECT_EQ(b_king.almnt_res(), AlRes::Black);
}

TEST(SquareTest, DefaultConstructor)
{
	Square s;
	EXPECT_EQ(s.file(), 0);
	EXPECT_EQ(s.rank(), 0);
}

TEST(SquareTest, ConversionToString)
{
	Square sq;
	EXPECT_EQ((string) sq, (string) "a1");
}

TEST(SquareTest, ConstructFromZString)
{
	Square sq("c4");
	EXPECT_EQ(sq.file(), 2);
	EXPECT_EQ(sq.rank(), 3);
}

TEST(SquareTest, Translate)
{
	Square sq("c4");
	auto sq2 = sq.translate(2, -3).value();
	EXPECT_EQ(sq2, Square("e1"));
}

TEST(SquareTest, TranslateOffBoard)
{
	Square sq("c4");
	auto sq2 = sq.translate(-1, 5);
	EXPECT_EQ(sq2, nullopt);
}

TEST(SquareTest, AllSquares)
{
	EXPECT_EQ(all_squares[0], Square("a1"));
	EXPECT_EQ(all_squares[7], Square("h1"));
	EXPECT_EQ(all_squares[56], Square("a8"));
	EXPECT_EQ(all_squares[63], Square("h8"));
}

TEST(MoveRecordTest, Normal)
{
	MoveRecord mr("e2", "e4");
	EXPECT_EQ(mr.initial(), Square("e2"));
	EXPECT_EQ(mr.final(), Square("e4"));
	EXPECT_FALSE(mr.is_promo());
	EXPECT_EQ(mr.promo_type(), nullopt);
}

TEST(MoveRecordTest, Promotion)
{
	MoveRecord mr("c7", "c8", Piece::Type::Rook);
	EXPECT_EQ(mr.initial(), Square("c7"));
	EXPECT_EQ(mr.final(), Square("c8"));
	EXPECT_TRUE(mr.is_promo());
	EXPECT_EQ(mr.promo_type().value(), Piece::Type::Rook);
	EXPECT_THROW(MoveRecord("h7", "h8", Piece::Type::King), std::invalid_argument);
	EXPECT_THROW(MoveRecord("h7", "h8", Piece::Type::Pawn), std::invalid_argument);
}

TEST(HalfByteBoardTest, Indexing)
{
	HalfByteBoard hbb;
	EXPECT_EQ(hbb.get(0), 0);
	EXPECT_EQ(hbb.get(62), 0);
	EXPECT_EQ(hbb.get(63), 0);
	hbb.set(6, 7, 15);
	EXPECT_EQ(hbb.get(62), 15);
	EXPECT_EQ(hbb.get(63), 0);
	hbb.set(63, 6);
	EXPECT_EQ(hbb.get(62), 15);
	EXPECT_EQ(hbb.get(63), 6);
}

TEST(PositionTest, FenConstruction)
{
	Position pos("r3kb1r/1bq2ppp/p1nppn2/8/1p1NP3/P1N1BP2/1PPQB1PP/2KR3R w kq - 0 12");
	cerr << pos << endl;
	EXPECT_EQ(pos.as_fen(), "r3kb1r/1bq2ppp/p1nppn2/8/1p1NP3/P1N1BP2/1PPQB1PP/2KR3R w kq - 0 12");
}

TEST(PositionTest, ToMove)
{
	Position defpos = Position::std_start();
	EXPECT_EQ(defpos.to_move(), Almnt::White);
	MoveRecord mr{"e2", "e4"};
	Move mv(defpos, mr);
	defpos = Position(defpos, mv);
	EXPECT_EQ(defpos.to_move(), Almnt::Black);
}

TEST(PositionTest, Indexing)
{
	Position pos;
	EXPECT_EQ(pos.at("c3"), Piece());
	Piece new_p {Almnt::White, Piece::Type::King};
	pos.set("c3", new_p);
	const auto pos2 = pos;
	EXPECT_EQ(pos2.at("c3"), new_p);
}

TEST(PositionTest, CastleFlags)
{
	Position pos;
	auto a = Almnt::White;
	auto s = Side::Kingside;
	EXPECT_FALSE(pos.can_castle(a, s));
	pos.mut_castle(a, s) = true;
	EXPECT_TRUE(pos.can_castle(a, s));
}

TEST(PositionTest, AsFen)
{
	auto pos = Position::std_start();
	EXPECT_EQ(pos.as_fen(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

TEST(PositionTest, AsFenOpening)
{
	auto pos = Position::std_start();
	EXPECT_EQ(pos.as_fen(), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	MoveRecord mr1{"e2", "e4"};
	Move mv1(pos, mr1);
	pos = Position(pos, mv1);
	EXPECT_EQ(pos.as_fen(), "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
	MoveRecord mr2{"c7", "c5"};
	Move mv2(pos, mr2);
	pos = Position(pos, mv2);
	EXPECT_EQ(pos.as_fen(), "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2");
	MoveRecord mr3{"g1", "f3"};
	Move mv3(pos, mr3);
	pos = Position(pos, mv3);
	EXPECT_EQ(pos.as_fen(), "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2");
}

TEST(MoveTest, Normal)
{
	auto pos = Position::std_start();
	MoveRecord mr {"e2", "e4"};
	Move mv {pos, mr};
	EXPECT_EQ(mv.initial_sq(), "e2");
	EXPECT_EQ(mv.final_sq(), "e4");
	EXPECT_EQ(mv.moved(), Piece(Almnt::White, Piece::Type::Pawn));
	EXPECT_EQ(mv.captured(), Piece());
	EXPECT_FALSE(mv.is_en_passant());
	EXPECT_FALSE(mv.is_castling());
	EXPECT_FALSE(mv.is_promotion());
}

TEST(MoveTest, PawnCapture)
{
	Position pos{"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2"};
	MoveRecord mr {"e4", "d5"};
	Move mv {pos, mr};
	EXPECT_EQ(mv.initial_sq(), "e4");
	EXPECT_EQ(mv.final_sq(), "d5");
	EXPECT_EQ(mv.moved(), Piece(Almnt::White, Piece::Type::Pawn));
	EXPECT_EQ(mv.captured(), Piece(Almnt::Black, Piece::Type::Pawn));
	EXPECT_FALSE(mv.is_en_passant());
	EXPECT_FALSE(mv.is_castling());
	EXPECT_FALSE(mv.is_promotion());
}

TEST(MoveTest, PieceCapture)
{
	Position pos{"rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2"};
	MoveRecord mr {"d8", "d5"};
	Move mv {pos, mr};
	EXPECT_EQ(mv.initial_sq(), "d8");
	EXPECT_EQ(mv.final_sq(), "d5");
	EXPECT_EQ(mv.moved(), Piece(Almnt::Black, Piece::Type::Queen));
	EXPECT_EQ(mv.captured(), Piece(Almnt::White, Piece::Type::Pawn));
	EXPECT_FALSE(mv.is_en_passant());
	EXPECT_FALSE(mv.is_castling());
	EXPECT_FALSE(mv.is_promotion());
}

TEST(MoveTest, EnPassant)
{
	Position pos{"rnbqkbnr/pp2pppp/8/2pP4/8/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 3"};
	MoveRecord mr {"d5", "c6"};
	Move mv {pos, mr};
	EXPECT_EQ(mv.initial_sq(), "d5");
	EXPECT_EQ(mv.final_sq(), "c6");
	EXPECT_EQ(mv.moved(), Piece(Almnt::White, Piece::Type::Pawn));
	EXPECT_EQ(mv.captured(), Piece());
	EXPECT_TRUE(mv.is_en_passant());
	EXPECT_FALSE(mv.is_castling());
	EXPECT_FALSE(mv.is_promotion());
}

TEST(MoveTest, Promotion)
{
	Position pos{"r2qkbnr/pP2pppp/2n5/5b2/8/8/PPPP1PPP/RNBQKBNR w KQkq - 1 5"};
	MoveRecord mr {"b7", "a8", Piece::Type::Queen};
	Move mv {pos, mr};
	EXPECT_EQ(mv.initial_sq(), "b7");
	EXPECT_EQ(mv.final_sq(), "a8");
	EXPECT_EQ(mv.moved(), Piece(Almnt::White, Piece::Type::Pawn));
	EXPECT_EQ(mv.captured(), Piece(Almnt::Black, Piece::Type::Rook));
	EXPECT_FALSE(mv.is_en_passant());
	EXPECT_FALSE(mv.is_castling());
	EXPECT_TRUE(mv.is_promotion());
	EXPECT_EQ(mv.promo_type().value(), Piece::Type::Queen);
}

TEST(MoveTest, Castling)
{
	Position pos{"r2qkbnr/pp1n1ppp/3pp3/2p5/4P3/3P1N2/PPP2PPP/RNBQK2R w KQkq - 0 6"};
	MoveRecord mr {"e1", "g1"};
	Move mv {pos, mr};
	EXPECT_EQ(mv.initial_sq(), "e1");
	EXPECT_EQ(mv.final_sq(), "g1");
	EXPECT_EQ(mv.moved(), Piece(Almnt::White, Piece::Type::King));
	EXPECT_EQ(mv.captured(), Piece());
	EXPECT_FALSE(mv.is_en_passant());
	EXPECT_TRUE(mv.is_castling());
	EXPECT_FALSE(mv.is_promotion());
}