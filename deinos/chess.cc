#include "chess.h"
#include <sstream>
using namespace chess;
using namespace std;

Alignment chess::operator!(const Alignment& a)
{
	int a2 = ((int) a + 1) % 2;
	return (Alignment) a2;
}

bool chess::operator==(Piece a, Piece b)
{
	return (a.alignment == b.alignment) && (a.type == b.type);
}

bool chess::operator!=(Piece a, Piece b)
{
	return !(a == b);
}

std::ostream& chess::operator<<(std::ostream& os, Piece::Type t)
{
	constexpr array<char, 7> letters {'_', 'P', 'N', 'B', 'R', 'Q', 'K'};
	os << letters[(int) t];
	return os;
}

std::ostream& chess::operator<<(std::ostream& os, Piece p)
{
	constexpr array<char, 7> white {' ', 'x', 'N', 'B', 'R', 'Q', 'K'};
	constexpr array<char, 7> black {'@', 'o', 'n', 'b', 'r', 'q', 'k'};
	if (p.alignment == Alignment::White) os << white[(int) p.type];
	else os << black[(int) p.type];
	return os;
}

optional<Square> chess::Square::translate(int files,int ranks) const
{
	const int new_file = m_file + files;
	if (new_file > 7 || new_file < 0) return optional<Square>();
	const int new_rank = m_rank + ranks;
	if (new_rank > 7 || new_rank < 0) return optional<Square>();

	return optional<Square>(Square(new_file, new_rank));
}

chess::Square::operator string () const
{
	constexpr array<char, 8> files {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
	constexpr array<char, 8> ranks {'1', '2', '3', '4', '5', '6', '7', '8'};
	return string {files[m_file], ranks[m_rank]};
}

bool chess::operator==(Square s1, Square s2)
{
	return (s1.file() == s2.file()) && (s1.rank() == s2.rank());
}

bool chess::operator!=(Square s1, Square s2)
{
	return !(s1 == s2);
}

chess::Move::Move(Square initial, Square final, Piece moved, Piece captured)
	: m_initial_square(initial), m_final_square(final), m_moved_piece(moved), m_captured_piece(captured)
{
	Expects(m_moved_piece.type != Piece::Type::Empty);
	Expects(m_captured_piece.type == Piece::Type::Empty ||
		m_captured_piece.alignment != m_moved_piece.alignment);
}

void chess::Move::set_castling() {
	Expects(m_moved_piece.type == Piece::Type::King);
	Expects(m_captured_piece.type == Piece::Type::Empty);
	Expects(m_is_castling == false);
	m_is_castling = true;
}

void chess::Move::set_promotion(Piece::Type promo_type) {
	Expects(m_moved_piece.type == Piece::Type::Pawn); //must be a pawn
	Expects(this->is_promotion() == false); //cannot already be a promotion
	Expects(promo_type == Piece::Type::Knight ||
		promo_type == Piece::Type::Bishop ||
		promo_type == Piece::Type::Rook ||
		promo_type == Piece::Type::Queen); //do not promote to King, Pawn or Empty
	Expects((m_moved_piece.alignment == Alignment::White && m_final_square.rank() == 7)
		|| (m_moved_piece.alignment == Alignment::Black && m_final_square.rank() == 0)); //promote on the right rank
	
	m_promoted_to = Piece {m_moved_piece.alignment, promo_type};
}

void chess::Move::set_en_passant() {
	Expects(m_moved_piece.type == Piece::Type::Pawn);
	Expects(m_captured_piece.type == Piece::Type::Pawn);
	Expects(m_is_en_passant == false);
	m_is_en_passant = true;
}

string chess::Move::to_xboard() const {
	stringstream ss;
	ss << (string) m_initial_square << (string) m_final_square;
	if (is_promotion()) ss << m_promoted_to.type;
	return ss.str();
}

bool chess::operator==(const Move& m1, const Move& m2)
{
	return (m1.initial_square() == m2.initial_square() &&
		m1.final_square() == m2.final_square() &&
		m1.moved_piece() == m2.moved_piece() &&
		m1.captured_piece() == m2.captured_piece() &&
		m1.promoted_to() == m2.promoted_to() &&
		m1.is_castling() == m2.is_castling() &&
		m1.is_en_passant() == m2.is_en_passant());
}

bool chess::operator!=(const Move& m1, const Move& m2)
{
	return !(m1 == m2);
}

std::ostream& chess::operator<<(std::ostream& os, const Move& mv)
{
	os << mv.moved_piece().type << (string) mv.initial_square();
	os << (mv.captured_piece().type == Piece::Type::Empty ? '-' : 'x');
	os << (string) mv.final_square();
	if (mv.promoted_to().type != Piece::Type::Empty) os << mv.promoted_to().type; 
	return os;
}

optional<Move> chess::find_move(const string& t_name, const vector<Move>& moves)
{
	for (const auto& m : moves){
		stringstream ss;
		ss << m;
		string name = ss.str();
		if (name == t_name) return make_optional(m);
	}
	return nullopt;
}

void apply_castling(Position& pos, const Move& mv) {
	const Alignment to_move = mv.moved_piece().alignment;
	const Square king_sq = (to_move == Alignment::White ? Square("e1") : Square("e8"));
	const bool kingside = mv.final_square().file() > mv.initial_square().file();
	const int king_dir = (kingside ? 1 : -1);
	const Square rook_sq = (kingside ? king_sq.translate(3, 0).value() : king_sq.translate(-4, 0).value());
	pos[king_sq] = Piece();
	pos[rook_sq] = Piece();
	pos[king_sq.translate(2 * king_dir, 0).value()] = Piece(to_move, Piece::Type::King);
	pos[king_sq.translate(king_dir, 0).value()] = Piece(to_move, Piece::Type::Rook);
}

void apply_en_passant(Position& pos, const Move& mv) {
	pos[mv.final_square()] = pos[mv.initial_square()];
	pos[mv.initial_square()] = Piece();
	const int capt_dir = (mv.final_square().file() > mv.initial_square().file() ? 1 : -1);
	pos[mv.initial_square().translate(capt_dir, 0).value()] = Piece();
}

void apply_promotion(Position& pos, const Move& mv) {
	pos[mv.final_square()] = mv.promoted_to();
	pos[mv.initial_square()] = Piece();
}

chess::Position::Position(const Position& t_pos, const Move& mv)
	: m_prev_move(optional<Move>(mv)), m_board(t_pos.m_board), m_castle(t_pos.m_castle)
{
	Expects(t_pos.game_result() == nullopt);
	Expects(t_pos.to_move() == mv.moved_piece().alignment);
	Expects(t_pos[mv.initial_square()] == mv.moved_piece());
	if (!mv.is_en_passant()) Expects(t_pos[mv.final_square()] == mv.captured_piece());

	if (mv.is_castling()) {
		apply_castling(*this, mv);
	}
	else if (mv.is_promotion()) {
		apply_promotion(*this, mv);
	}
	else if (mv.is_en_passant()) {
		apply_en_passant(*this, mv);
	}
	else {
		(*this)[mv.final_square()] = (*this)[mv.initial_square()];
		(*this)[mv.initial_square()] = Piece();
	}

	//check for end of game
	if (mv.captured_piece().type == Piece::Type::King){
		if (t_pos.to_move() == Alignment::White) m_result = GameResult::White;
		else m_result = GameResult::Black;
	}

	//check to disable castling
	if (mv.moved_piece().type == Piece::Type::King && 
		(can_castle(t_pos.to_move(), Side::Kingside) ||
		can_castle(t_pos.to_move(), Side::Queenside))) {
		mut_castle(t_pos.to_move(), Side::Kingside) = false;
		mut_castle(t_pos.to_move(), Side::Queenside) = false;
	}
	if (mv.moved_piece().type == Piece::Type::Rook) {
		if (can_castle(t_pos.to_move(), Side::Queenside) && mv.initial_square().file() == 0) {
			mut_castle(t_pos.to_move(), Side::Queenside) = false;
		}
		if (can_castle(t_pos.to_move(), Side::Kingside) && mv.initial_square().file() == 7) {
			mut_castle(t_pos.to_move(), Side::Kingside) = false;
		}
	}
	if (mv.captured_piece().type == Piece::Type::Rook) {
		if (can_castle(!t_pos.to_move(), Side::Queenside) && mv.final_square().file() == 0) {
			mut_castle(!t_pos.to_move(), Side::Queenside) = false;
		}
		if (can_castle(!t_pos.to_move(), Side::Kingside) && mv.final_square().file() == 7) {
			mut_castle(!t_pos.to_move(), Side::Kingside) = false;
		}
	}
}

Position chess::Position::std_start()
{
	Position pos;
	constexpr array<Piece::Type, 8> backrank
		{Piece::Type::Rook, Piece::Type::Knight, Piece::Type::Bishop, Piece::Type::Queen,
		Piece::Type::King, Piece::Type::Bishop, Piece::Type::Knight, Piece::Type::Rook};
	for (int i = 0; i < 8; i++) {
		pos.m_board[i][0] = Piece(Alignment::White, backrank[i]);
		pos.m_board[i][1] = Piece(Alignment::White, Piece::Type::Pawn); 
		pos.m_board[i][6] = Piece(Alignment::Black, Piece::Type::Pawn);
		pos.m_board[i][7] = Piece(Alignment::Black, backrank[i]);
	}
	pos.m_castle = {true, true, true, true};
	return pos;
}

string chess::Position::as_fen() const
{
	constexpr array<char, 7> white {' ', 'P', 'N', 'B', 'R', 'Q', 'K'};
	constexpr array<char, 7> black {'@', 'p', 'n', 'b', 'r', 'q', 'k'};
	
	string output = "";
	for (int r = 7; r >= 0; r--) {
		int empty_counter = 0;
		for (int f = 0; f < 8; f++) {
			const Piece p = m_board[f][r];
			if (p.type == Piece::Type::Empty) {
				empty_counter += 1;
				continue;
			}
			if (empty_counter != 0) {
				output += to_string(empty_counter);
				empty_counter = 0;
			}
			output += (p.alignment == Alignment::White ? white[(int) p.type] : black[(int) p.type]);
		}
		if (empty_counter != 0) {
			output += to_string(empty_counter);
			empty_counter = 0;
		}
		if (r != 0) output += '/';
	}
	
	return output;
}

bool chess::operator==(const Position& p1, const Position& p2)
{
	return (p1.m_board == p2.m_board &&
		p1.m_prev_move == p2.m_prev_move &&
		p1.m_castle == p2.m_castle);
}

bool chess::operator!=(const Position& p1, const Position& p2)
{
	return !(p1 == p2);
}

std::ostream& chess::operator<<(std::ostream& os, const Position& pos)
{
	constexpr const char* hline = "---------------------------------";
	os << hline <<endl;
	for (int i = 7; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			os << "| " << pos.m_board[j][i] << " ";
		}
		os << "|" << endl << hline << endl;
	}
	return os;
}

optional<Move> chess::move_from_fen(const string& fen, const Position& curr_pos, const vector<Move>& moves) {
	for (const auto& m : moves){
		Position new_pos(curr_pos, m);
		if (new_pos.as_fen() == fen) return make_optional(m);
	}
	return nullopt;
}