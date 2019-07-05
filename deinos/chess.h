#ifndef DEINOS_CHESS_H
#define DEINOS_CHESS_H
#include <optional>
#include <array>
#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <gsl/gsl_assert>
#include <cstddef>

namespace chess {
	enum class Almnt : uint8_t;
	enum class AlRes : uint8_t;
	enum class GameResult : uint8_t;
	enum class Side : uint8_t;
	struct Piece;
	class Square;
	struct MoveRecord;
	class Move;
	class Position;
	
	//Alignment (i.e. White or Black)
	enum class Almnt : uint8_t {White = 0, Black = 1};
	constexpr Almnt operator!(const Almnt& a) {return (a == Almnt::White ? Almnt::Black : Almnt::White);}
	inline uint8_t as_index(Almnt a) {return static_cast<uint8_t>(a);}
	
	//Alignment Result (for cases where "None" is an option)
	enum class AlRes : uint8_t {White = 0, Black = 1, None = 2}; //Utility to avoid constant Empty checks
	inline bool operator==(AlRes ar, Almnt a) {return static_cast<uint8_t>(ar) == static_cast<uint8_t>(a);}
	inline bool operator==(Almnt a, AlRes ar) {return ar == a;}
	inline bool operator!=(AlRes ar, Almnt a) {return !(ar == a);}
	inline bool operator!=(Almnt a, AlRes ar) {return !(ar == a);}

	//Game Result
	enum class GameResult : uint8_t {White = 0, Black = 1, Draw = 2};
	inline float evaluate(GameResult gr) {
		switch (gr) {
			case GameResult::White: return 1.0f;
			case GameResult::Black: return 0.0f;
			case GameResult::Draw: return 0.5f;
			default: return 0.5f;
		}
	}
	inline GameResult victory(Almnt a) {return static_cast<GameResult>(a);}
	inline AlRes victor(GameResult gr) {return static_cast<AlRes>(gr);}

	//Side of Board
	enum class Side : uint8_t {Kingside = 0, Queenside = 1};

	//The contents of a square (i.e. a chess piece or empty)
	struct Piece {
		enum class Type : uint8_t {Empty, Pawn, Knight, Bishop, Rook, Queen, King};
		
		constexpr Piece() = default;
		constexpr Piece(Almnt a, Type t) : data((static_cast<std::byte>(t) << 1) | static_cast<std::byte>(a)) {}

		inline AlRes almnt_res() const {return (type() == Type::Empty ? AlRes::None : static_cast<AlRes>(almnt()));}
		inline Almnt almnt() const {return static_cast<Almnt>(data & std::byte{0x01});}
		inline Type type() const {return static_cast<Type>(data >> 1);}

	private:
		constexpr Piece(uint8_t d) : data(std::byte{d}) {}
		inline uint8_t raw() const {return static_cast<uint8_t>(data);}
		std::byte data = std::byte{0};
		friend class Position;
	};
	
	bool operator==(Piece, Piece);
	bool operator!=(Piece, Piece);
	std::ostream& operator<<(std::ostream& os, Piece::Type t);
	std::ostream& operator<<(std::ostream& os, Piece p); //White uppercase, black lowercase

	//A bounds checked index into a chess position
	class Square{
	public:
		constexpr Square() = default;
		constexpr Square(const char* name){ //this must be a zstring TODO
			constexpr std::array<char, 8> files {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
			constexpr std::array<char, 8> ranks {'1', '2', '3', '4', '5', '6', '7', '8'};
			uint8_t file = 255;
			uint8_t rank = 255;
			for (int i = 0; i < 8; i++) {
				if (files[i] == name[0]) file = i;
				if (ranks[i] == name[1]) rank = i;
			}
			assert(file != 255);
			assert(rank != 255);
			data = (std::byte{rank} << 3) | std::byte{file};
		}
		
		std::optional<Square> translate(int files, int ranks) const; //TODO consider changing type to int8_t
		inline uint8_t file() const {return std::to_integer<uint8_t>(data & std::byte{0b00000111});}
		inline uint8_t rank() const {return std::to_integer<uint8_t>((data & std::byte{0b00111000}) >> 3);}
		explicit operator std::string () const;
	private:
		constexpr Square(uint8_t file, uint8_t rank) : data((std::byte{rank} << 3) | std::byte{file}) {
			assert(file < 8);
			assert(rank < 8);
		}
		constexpr Square(std::byte d) : data(d) {} //construct from raw data
		std::byte data = std::byte{0};
		friend class MoveRecord;
		friend bool operator==(Square, Square);
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

	//stores information necessary to reconstruct a move from a position
	struct MoveRecord {
	private:
		
		inline uint8_t promo_index() const {return static_cast<uint8_t>(data2) >> 6;}		
	public:
		constexpr static std::array<Piece::Type, 4> promo_types
			{Piece::Type::Knight, Piece::Type::Bishop, Piece::Type::Rook, Piece::Type::Queen};
		
		static_assert(sizeof(Square) == 1);
		constexpr MoveRecord(Square t_initial, Square t_final)
			: data1(reinterpret_cast<std::byte&>(t_initial)), data2(reinterpret_cast<std::byte&>(t_final)) {}
		MoveRecord(Square t_initial, Square t_final, Piece::Type t_promo_type);
		
		inline Square initial() const {return Square(data1 & std::byte{0x3F});}
		inline Square final() const {return Square(data2 & std::byte{0x3F});}
		inline bool is_promo() const {return (data1 & std::byte{0b11000000}) != std::byte{0};}
		inline std::optional<Piece::Type> promo_type() const
		{
			return is_promo() ? std::make_optional<Piece::Type>(promo_types[promo_index()]) : std::nullopt;
		};
		std::string to_string() const;
	private:
		std::byte data1 = std::byte{0}; //initial square and promotion existence flag
		std::byte data2 = std::byte{0}; //final square and promotion type flag
	};
	bool operator==(const MoveRecord&, const MoveRecord&);
	inline bool operator!=(const MoveRecord& mr1, const MoveRecord& mr2) {return !(mr1 == mr2);}
	inline std::ostream& operator<<(std::ostream& os, const MoveRecord& mr) {os << mr.to_string(); return os;}

	//64 half-byte uints stored in 32B
	struct HalfByteBoard {
	private:
		inline uint8_t get_front(int i) const {return data[i] >> 4;}
		inline uint8_t get_back(int i) const {return data[i] & 0x0F;}
		inline void set_front(int i, uint8_t val) {data[i] = (val << 4) | (data[i] & 0x0F);}
		inline void set_back(int i, uint8_t val) {data[i] = val | (data[i] & 0xF0);}
	public:
		inline uint8_t get(int i) const
		{
			assert(i >= 0);
			assert(i < 64);
			if (i % 2 == 0) return get_front(i / 2);
			else return get_back(i / 2);
		}
		inline uint8_t get(int file, int rank) const {return get(rank * 8 + file);}
		inline void set(int i, uint8_t val)
		{
			assert(i >= 0);
			assert(i < 64);
			assert(val <= 0x0F);
			if (i % 2 == 0) set_front(i / 2, val);
			else set_back(i / 2, val);
		}
		inline void set(int file, int rank, uint8_t val) {set(rank * 8 + file, val);}
	private:
		std::array<uint8_t, 32> data = {0};
		friend bool operator==(const HalfByteBoard&, const HalfByteBoard&);
	};
	inline bool operator==(const HalfByteBoard& hbb1, const HalfByteBoard& hbb2) {return hbb1.data == hbb2.data;}
	inline bool operator!=(const HalfByteBoard& hbb1, const HalfByteBoard& hbb2) {return !(hbb1 == hbb2);}

	//Equivalent to a chess position recorded in Forsyth-Edwards Notaion
	class Position {
	public:
		constexpr Position() = default;
		Position(const std::string& fen); //construct from FEN representation
		Position(const Position&, const Move&); //generate new position by applying a move
		static Position std_start();

		//inline Piece& operator[] (Square s) {return m_board[s.file()][s.rank()];}
		//inline Piece operator[] (Square s) const {return m_board[s.file()][s.rank()];}
		inline Piece at(Square s) const {return Piece(m_board.get(s.file(), s.rank()));}
		//inline Piece at(Square s) const {return m_board[s.file()][s.rank()];}
		inline Piece at(int index) const {return Piece(m_board.get(index));}
		//inline Piece at(int index) const {return m_board[index % 8][index / 8];}
		inline void set(Square s, Piece p) {m_board.set(s.file(), s.rank(), p.raw());}
		//inline void set(Square s, Piece p) {m_board[s.file()][s.rank()] = p;}
		inline void set(int index, Piece p) {m_board.set(index, p.raw());}
		//inline void set(int index, Piece p) {m_board[index % 8][index / 8] = p;}
		inline bool can_castle(Almnt a, Side s) const {return m_castle[(int) a][(int) s];} //TODO use as_index
		inline bool& mut_castle(Almnt a, Side s){return m_castle[(int) a][(int) s];}
		inline Almnt to_move() const {return m_to_move;}
		inline std::optional<Square> en_passant_target() const {return m_en_passant_target;}
		inline int hm_clock() const {return m_hm_clock;}
		inline int fm_count() const {return m_fm_count;}

		std::string as_fen() const;
		explicit operator std::string() const;
		friend std::ostream& operator<<(std::ostream& os, const Position& pos);
		friend bool operator==(const Position&, const Position&);

	private:
		static_assert(sizeof(Piece) == 1);
		HalfByteBoard m_board;
		//std::array<std::array<Piece, 8>, 8> m_board = {Piece()};
		Almnt m_to_move = Almnt::White;
		std::array<std::array<bool, 2>, 2> m_castle = {{false}}; //"permitted to castle" flags indexed by enum values
		//std::optional<GameResult> m_result = std::nullopt;
		std::optional<Square> m_en_passant_target = std::nullopt;
		short m_hm_clock = 0;
		short m_fm_count = 1;
	};
	bool operator==(const Position&, const Position&);
	bool operator!=(const Position&, const Position&);
	std::ostream& operator<<(std::ostream& os, const Position& pos);

	
	//A wrapper for a position and move record, representing a single move
	class Move { //TODO: test code
	public:
		//constexpr Move() = default; //default constructor
		//Move(Square initial, Square final, Piece moved, Piece captured); //normal move
		//Move(const Position& pos, const std::string& name); //N.B this constructor does not support castling, promotion or en passant
		//void set_castling(); //castling 1.must be a king moving 2.must not capture 3.can only set once
		//void set_promotion(Piece::Type); //pawn promotion
		//void set_en_passant(); //en_passant
		constexpr Move(const Position& t_pos, const MoveRecord& t_record) : m_pos(t_pos), m_record(t_record) {
			if (moved().type() == Piece::Type::Empty) {std::cerr << *this << std::endl << t_pos << std::endl; throw std::exception();}
			if (captured().almnt_res() == moved().almnt()) {std::cerr << *this << " capt: " << captured() << " mr: " << m_record << std::endl << t_pos << std::endl; throw std::exception();}
			assert(moved().type() != Piece::Type::Empty);
			assert(captured().almnt_res() != moved().almnt());
		}
		//inline Square initial_square() const {return m_initial_square;}
		inline Square initial_sq() const {return m_record.initial();}
		//inline Square final_square() const {return m_final_square;}
		inline Square final_sq() const {return m_record.final();}
		//inline Piece moved_piece() const {return m_moved_piece;}
		inline Piece moved() const {return m_pos.at(initial_sq());}
		//inline Piece captured_piece() const {return m_captured_piece;}
		inline Piece captured() const {return m_pos.at(final_sq());} //returns empty piece if en passant
		//inline bool is_en_passant() const {return m_is_en_passant;}
		bool is_en_passant() const;
		//inline bool is_castling() const {return m_is_castling;}
		bool is_castling() const;
		//inline bool is_promotion() const {return m_promoted_to.type() != Piece::Type::Empty;}
		inline bool is_promotion() const {return m_record.is_promo();}
		//inline Piece promoted_to() const {return m_promoted_to;}
		inline std::optional<Piece::Type> promo_type() const {return m_record.promo_type();}
		inline const MoveRecord& record() const {return m_record;}
		inline Position apply() const {return Position(m_pos, *this);}

		explicit operator std::string() const;
		friend std::ostream& operator<<(std::ostream& os, const Move& mv);
		std::string to_xboard() const;
	
	private:
		//Square m_initial_square;
		//Square m_final_square;
		//Piece m_moved_piece;
		//Piece m_captured_piece;
		//Piece m_promoted_to;
		//bool m_is_castling = false;
		//bool m_is_en_passant = false;
		const Position& m_pos;
		const MoveRecord& m_record;
		friend bool operator==(const Move&, const Move&);
	};
	bool operator==(const Move&, const Move&);
	bool operator!=(const Move&, const Move&);
	std::ostream& operator<<(std::ostream& os, const Move& mv);
	//std::optional<Move> find_move(const std::string& t_name, const AnalysedPosition& apos);

	//std::optional<Move> move_from_fen(const std::string& fen, const Position& curr_pos, const std::vector<Move>& moves);
}
#endif