#include "gtest/gtest.h"
#include "deinos/chess.h"
#include <string>
#include <optional>
using namespace std;

TEST(AlignmentTest, First)
{
	auto w = chess::Alignment::White;
	auto b = chess::Alignment::Black;
	EXPECT_EQ(!w, b);
	EXPECT_EQ(!b, w);
}

TEST(PieceTest, DefaultConstructor)
{
	chess::Piece default_piece;
	chess::Piece intended {chess::Alignment::White, chess::Piece::Type::Empty};
	EXPECT_EQ(default_piece, intended);
}

TEST(SquareTest, DefaultConstructor)
{
	chess::Square s;
	EXPECT_EQ(s.file(), 0);
	EXPECT_EQ(s.rank(), 0);
}

TEST(SquareTest, ConversionToString)
{
	chess::Square sq;
	EXPECT_EQ((string) sq, (string) "a1");
}

TEST(SquareTest, ConstructFromZString)
{
	chess::Square sq("c4");
	EXPECT_EQ(sq.file(), 2);
	EXPECT_EQ(sq.rank(), 3);
}

TEST(SquareTest, Translate)
{
	chess::Square sq("c4");
	auto sq2 = sq.translate(2, -3).value();
	EXPECT_EQ(sq2, chess::Square("e1"));
}

TEST(SquareTest, TranslateOffBoard)
{
	chess::Square sq("c4");
	auto sq2 = sq.translate(-1, 5);
	EXPECT_EQ(sq2, nullopt);
}

TEST(SquareTest, AllSquares)
{
	EXPECT_EQ(chess::all_squares[0], chess::Square("a1"));
	EXPECT_EQ(chess::all_squares[7], chess::Square("h1"));
	EXPECT_EQ(chess::all_squares[56], chess::Square("a8"));
	EXPECT_EQ(chess::all_squares[63], chess::Square("h8"));
}

TEST(MoveTestDeathTest, DefaultConstructor)
{
	chess::Move defmv;
	EXPECT_EQ(defmv.initial_square(), chess::Square("a1"));
	EXPECT_EQ(defmv.final_square(), chess::Square("a1"));
	EXPECT_EQ(defmv.moved_piece(), chess::Piece());
	EXPECT_EQ(defmv.captured_piece(), chess::Piece());
	EXPECT_FALSE(defmv.is_en_passant());
	EXPECT_FALSE(defmv.is_castling());
	EXPECT_FALSE(defmv.is_promotion());
}

TEST(MoveTestDeathTest, SetCastling)
{
	chess::Move mv(chess::Square("a1"), chess::Square("a2"),
		chess::Piece {chess::Alignment::White, chess::Piece::Type::King}, chess::Piece());
	EXPECT_FALSE(mv.is_castling()); //initially false
	mv.set_castling();
	EXPECT_TRUE(mv.is_castling()); //now true
	EXPECT_DEATH(mv.set_castling(), ""); //cannot set again

	chess::Move mv2(chess::Square("a1"), chess::Square("a2"),
		chess::Piece {chess::Alignment::White, chess::Piece::Type::Rook}, chess::Piece());
	EXPECT_DEATH(mv2.set_castling(), ""); //must be a king

	chess::Move mv3(chess::Square("a1"), chess::Square("a2"),
			chess::Piece {chess::Alignment::White, chess::Piece::Type::King},
			chess::Piece {chess::Alignment::Black, chess::Piece::Type::Pawn});
		EXPECT_DEATH(mv3.set_castling(), ""); //cannot capture
}

TEST(MoveTestDeathTest, SetPromotion)
{
	chess::Move mv(chess::Square("a2"), chess::Square("a1"),
		chess::Piece {chess::Alignment::Black, chess::Piece::Type::Pawn}, chess::Piece());
	EXPECT_FALSE(mv.is_promotion()); //initially false
	EXPECT_DEATH(mv.set_promotion(chess::Piece::Type::King), ""); //cannot promote to king
	EXPECT_DEATH(mv.set_promotion(chess::Piece::Type::Pawn), ""); //cannot promote to pawn
	mv.set_promotion(chess::Piece::Type::Knight);
	chess::Piece expected {chess::Alignment::Black, chess::Piece::Type::Knight};
	EXPECT_EQ(mv.promoted_to(), expected);
	EXPECT_DEATH(mv.set_promotion(chess::Piece::Type::Knight), ""); //cannot set again

	chess::Move mv2(chess::Square("a2"), chess::Square("a1"),
		chess::Piece {chess::Alignment::Black, chess::Piece::Type::Bishop}, chess::Piece());
	EXPECT_DEATH(mv2.set_promotion(chess::Piece::Type::Knight), ""); //can only promote a pawn

	chess::Move mv3(chess::Square("a2"), chess::Square("a1"),
		chess::Piece {chess::Alignment::White, chess::Piece::Type::Pawn}, chess::Piece());
	EXPECT_DEATH(mv3.set_promotion(chess::Piece::Type::Knight), ""); //white promotes on rank 8

	chess::Move mv4(chess::Square("a3"), chess::Square("a2"),
		chess::Piece {chess::Alignment::Black, chess::Piece::Type::Pawn}, chess::Piece());
	EXPECT_DEATH(mv4.set_promotion(chess::Piece::Type::Knight), ""); //black promotes on rank 1
}

TEST(MoveTestDeathTest, SetEnPassant)
{
	chess::Move mv(chess::Square("e5"), chess::Square("f6"),
		chess::Piece {chess::Alignment::White, chess::Piece::Type::Pawn},
		chess::Piece {chess::Alignment::Black, chess::Piece::Type::Pawn});
	EXPECT_FALSE(mv.is_en_passant()); //initially false
	mv.set_en_passant();
	EXPECT_TRUE(mv.is_en_passant()); //now true
	EXPECT_DEATH(mv.set_en_passant(), ""); //cannot set again

	chess::Move mv2(chess::Square("e5"), chess::Square("f6"),
		chess::Piece {chess::Alignment::White, chess::Piece::Type::Knight},
		chess::Piece {chess::Alignment::Black, chess::Piece::Type::Pawn});
	EXPECT_DEATH(mv2.set_en_passant(), ""); //must be a pawn

	chess::Move mv3(chess::Square("e5"), chess::Square("f6"),
		chess::Piece {chess::Alignment::White, chess::Piece::Type::Pawn},
		chess::Piece {chess::Alignment::Black, chess::Piece::Type::Rook});
	EXPECT_DEATH(mv3.set_en_passant(), ""); //must capture a pawn

	chess::Move mv4(chess::Square("e5"), chess::Square("f6"),
		chess::Piece {chess::Alignment::White, chess::Piece::Type::Pawn},
		chess::Piece());
	EXPECT_DEATH(mv4.set_en_passant(), ""); //must capture a pawn
}

TEST(PositionTest, PrevMove)
{
	chess::Position defpos;
	EXPECT_EQ(defpos.prev_move(), nullopt);
	EXPECT_EQ(defpos.to_move(), chess::Alignment::White);
	chess::Move mv(chess::Square("e2"), chess::Square("e4"),
		chess::Piece {chess::Alignment::White, chess::Piece::Type::Pawn}, chess::Piece());
	defpos.set_prev_move(make_optional<chess::Move>(mv));
	EXPECT_EQ(defpos.to_move(), chess::Alignment::Black);
}

TEST(PositionTest, Indexing)
{
	chess::Position pos;
	EXPECT_EQ(pos["c3"], chess::Piece());
	chess::Piece new_p {chess::Alignment::White, chess::Piece::Type::King};
	pos["c3"] = new_p;
	const auto pos2 = pos;
	EXPECT_EQ(pos2["c3"], new_p);
}

TEST(PositionTest, CastleFlags)
{
	chess::Position pos;
	auto a = chess::Alignment::White;
	auto s = chess::Side::Kingside;
	EXPECT_FALSE(pos.can_castle(a, s));
	pos.mut_castle(a, s) = true;
	EXPECT_TRUE(pos.can_castle(a, s));
}