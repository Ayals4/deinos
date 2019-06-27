#ifndef DEINOS_CHESS_H
#define DEINOS_CHESS_H
#include <optional>
#include <array>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <gsl/gsl_assert>

namespace chess {
	enum class Alignment {White, Black};
	Alignment operator!(const Alignment& a);

	enum class GameResult {White, Black, Draw};
	inline double evaluate(GameResult gr) {
		switch (gr) {
			case GameResult::White: return 1.0;
			case GameResult::Black: return 0.0;
			case GameResult::Draw: return 0.5;
			default: return 0.5;
		}
	}
	inline GameResult gmres_victory(Alignment a) {
		return static_cast<GameResult>(a);
	}

	enum class Side {Kingside, Queenside};

	enum class AlRes {White, Black, None}; //Utility to avoid constant Empty checks
	inline bool operator==(AlRes ar, Alignment a) {return (int) ar == (int) a;}
	inline bool operator==(Alignment a, AlRes ar) {return ar == a;}
	inline bool operator!=(AlRes ar, Alignment a) {return !(ar == a);}
	inline bool operator!=(Alignment a, AlRes ar) {return !(ar == a);}
	
	struct Piece {
		enum class Type {Empty, Pawn, Knight, Bishop, Rook, Queen, King};
		
		constexpr Piece() = default;
		constexpr Piece(Alignment a, Type t) : alignment(a), type(t) {}

		inline AlRes almnt_res() const { //None if Empty, otherwise alignment
			if (type == Type::Empty) return AlRes::None;
			return (AlRes) ((int) alignment);
		}
		
		Alignment alignment = Alignment::White;
		Type type = Type::Empty;
	};
	bool operator==(Piece, Piece);
	bool operator!=(Piece, Piece);
	std::ostream& operator<<(std::ostream& os, Piece::Type t);
	std::ostream& operator<<(std::ostream& os, Piece p); //White uppercase, black lowercase

	class Square{
	public:
		constexpr Square() = default;
		constexpr Square(const char* name){ //this must be a zstring
			constexpr std::array<char, 8> files {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
			constexpr std::array<char, 8> ranks {'1', '2', '3', '4', '5', '6', '7', '8'};
			int file = -1;
			int rank = -1;
			for (int i = 0; i < 8; i++){
				if (files[i] == name[0]) file = i;
				if (ranks[i] == name[1]) rank = i;
			}
			assert(file != -1);
			assert(rank != -1);
			m_file = file;
			m_rank = rank;
		}
		
		std::optional<Square> translate(int files, int ranks) const;
		inline int file() const {return m_file;}
		inline int rank() const {return m_rank;}
		explicit operator std::string () const;
	private:
		constexpr Square(int file, int rank) : m_file(file), m_rank(rank) {}
		int m_file = 0; //check ok to use short
		int m_rank = 0;
	};
	bool operator==(Square, Square);
	bool operator!=(Square, Square);
	
	constexpr std::array<Square, 64> all_squares {
		"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
		"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
		"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
		"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
		"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
		"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
		"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
		"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
		};

	class Position;
	
	class Move{
	public:
		constexpr Move() = default; //default constructor
		Move(Square initial, Square final, Piece moved, Piece captured); //normal move
		Move(const Position& pos, const std::string& name); //N.B this constructor does not support castling, promotion or en passant
		void set_castling(); //castling 1.must be a king moving 2.must not capture 3.can only set once
		void set_promotion(Piece::Type); //pawn promotion
		void set_en_passant(); //en_passant
		
		inline Square initial_square() const {return m_initial_square;}
		inline Square final_square() const {return m_final_square;}
		inline Piece moved_piece() const {return m_moved_piece;}
		inline Piece captured_piece() const {return m_captured_piece;}
		inline bool is_en_passant() const {return m_is_en_passant;}
		inline bool is_castling() const {return m_is_castling;}
		inline bool is_promotion() const {return m_promoted_to.type != Piece::Type::Empty;}
		inline Piece promoted_to() const {return m_promoted_to;}

		explicit operator std::string() const;
		friend std::ostream& operator<<(std::ostream& os, const Move& mv);
		std::string to_xboard() const;
	
	private:
		Square m_initial_square;
		Square m_final_square;
		Piece m_moved_piece;
		Piece m_captured_piece;
		Piece m_promoted_to;
		bool m_is_castling = false;
		bool m_is_en_passant = false;

		//std::array<double, 8> pad = {0.0};
	};
	bool operator==(const Move&, const Move&);
	bool operator!=(const Move&, const Move&);
	std::ostream& operator<<(std::ostream& os, const Move& mv);
	std::optional<Move> find_move(const std::string& t_name, const std::vector<Move>& moves);
	
	class Position{
	public:
		constexpr Position() = default;
		Position(const std::string& fen); //construct from FEN representation
		Position(const Position&, const Move&); //generate new position by applying a move TODO
		static Position std_start();

		inline Piece& operator[] (Square s) {return m_board[s.file()][s.rank()];}
		inline Piece operator[] (Square s) const {return m_board[s.file()][s.rank()];}
		inline bool can_castle(Alignment a, Side s) const {return m_castle[(int) a][(int) s];}
		inline bool& mut_castle(Alignment a, Side s){return m_castle[(int) a][(int) s];}

		//inline const std::optional<Move>& prev_move() const {return m_prev_move;}
		//inline void set_prev_move(const std::optional<Move>& mv) {m_prev_move = mv;}
		inline Alignment to_move() const {return m_to_move;}//(m_prev_move ? !(m_prev_move.value().moved_piece().alignment) : chess::Alignment::White);}
		//inline std::optional<GameResult> game_result() const {return m_result;}
		inline std::optional<Square> en_passant_target() const {return m_en_passant_target;}
		inline int hm_clock() const {return m_hm_clock;}
		inline int fm_count() const {return m_fm_count;}

		std::string as_fen() const;
		explicit operator std::string() const;
		friend std::ostream& operator<<(std::ostream& os, const Position& pos);
		friend bool operator==(const Position&, const Position&);

	private:
		//std::optional<Move> m_prev_move = std::nullopt; //previous move
		Alignment m_to_move = Alignment::White;
		std::optional<Square> m_en_passant_target = std::nullopt;
		std::array<std::array<Piece, 8>, 8> m_board = {Piece()};
		std::array<std::array<bool, 2>, 2> m_castle = {{false}}; //"permitted to castle" flags indexed by enum values
		//std::optional<GameResult> m_result = std::nullopt;
		int m_hm_clock = 0;
		int m_fm_count = 1;
	};
	bool operator==(const Position&, const Position&);
	bool operator!=(const Position&, const Position&);
	std::ostream& operator<<(std::ostream& os, const Position& pos);
	std::optional<Move> move_from_fen(const std::string& fen, const Position& curr_pos, const std::vector<Move>& moves);
}
#endif