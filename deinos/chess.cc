#include "chess.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <gsl/gsl_util>
using namespace chess;
using namespace std;

bool chess::operator==(Piece a, Piece b)
{
	return (a.almnt() == b.almnt()) && (a.type() == b.type());
}

bool chess::operator!=(Piece a, Piece b)
{
	return !(a == b);
}

std::ostream& chess::operator<<(std::ostream& os, Piece::Type t)
{
	constexpr array<char, 7> letters {'_', 'P', 'N', 'B', 'R', 'Q', 'K'};
	os << letters[static_cast<uint8_t>(t)];
	return os;
}

std::ostream& chess::operator<<(std::ostream& os, Piece p)
{
	constexpr array<char, 7> white {' ', 'x', 'N', 'B', 'R', 'Q', 'K'};
	constexpr array<char, 7> black {'@', 'o', 'n', 'b', 'r', 'q', 'k'};
	if (p.almnt() == Almnt::White) os << white[static_cast<uint8_t>(p.type())];
	else os << black[static_cast<uint8_t>(p.type())];
	return os;
}

optional<Square> chess::Square::translate(int files,int ranks) const
{
	const int new_file = file() + files;
	if (new_file > 7 || new_file < 0) return nullopt;
	const int new_rank = rank() + ranks;
	if (new_rank > 7 || new_rank < 0) return nullopt;

	return Square(static_cast<uint8_t>(new_file), static_cast<uint8_t>(new_rank));
}

chess::Square::operator string () const
{
	constexpr array<char, 8> files {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
	constexpr array<char, 8> ranks {'1', '2', '3', '4', '5', '6', '7', '8'};
	return string {files[file()], ranks[rank()]};
}

bool chess::operator==(Square s1, Square s2)
{
	//return (s1.file() == s2.file()) && (s1.rank() == s2.rank());
	return (s1.data == s2.data);
}

bool chess::operator!=(Square s1, Square s2)
{
	return !(s1 == s2);
}

chess::MoveRecord::MoveRecord(Square t_initial, Square t_final, Piece::Type t_promo_type)
	: data1(reinterpret_cast<std::byte&>(t_initial)), data2(reinterpret_cast<std::byte&>(t_final))
{
	for (uint8_t i = 0; i < 4; i++) {
		if (t_promo_type == promo_types[i]){
			data2 |= (std::byte{i} << 6);
			data1 |= std::byte{0x40};
		}
	}
	if (!is_promo()) throw std::invalid_argument("Piece::Type passed is not a valid promotion candidate.");
}

string chess::MoveRecord::to_string() const
{
	stringstream ss;
	ss << (string) initial();
	ss << (string) final();
	if (promo_type()) ss << promo_type().value();
	return ss.str();
}

bool chess::operator==(const MoveRecord& mr1, const MoveRecord& mr2)
{
	return (mr1.initial() == mr2.initial() &&
		mr1.final() == mr2.final() &&
		mr1.promo_type() == mr2.promo_type());
}

bool chess::Move::is_en_passant() const
{
	if (moved().type() != Piece::Type::Pawn) return false;
	return (captured().type() == Piece::Type::Empty) && (initial_sq().file() != final_sq().file());
}

bool chess::Move::is_castling() const
{
	const bool is_king = moved().type() == Piece::Type::King;
	const int file_delta = static_cast<int>(final_sq().file()) - static_cast<int>(initial_sq().file());
	return is_king && abs(file_delta) > 1;
}

string chess::Move::to_xboard() const
{
	stringstream ss;
	ss << (string) initial_sq() << (string) final_sq();
	if (is_promotion()) ss << promo_type().value();
	return ss.str();
}

bool chess::operator==(const Move& m1, const Move& m2)
{
	return (m1.m_pos == m2.m_pos) && (m1.m_record == m2.m_record);
}

bool chess::operator!=(const Move& m1, const Move& m2)
{
	return !(m1 == m2);
}

chess::Move::operator string() const
{
	stringstream ss;
	ss << *this;
	return ss.str();
}

std::ostream& chess::operator<<(std::ostream& os, const Move& mv)
{
	os << mv.moved().type() << (string) mv.initial_sq();
	os << (mv.captured().type() == Piece::Type::Empty ? '-' : 'x');
	os << (string) mv.final_sq();
	if (mv.is_promotion()) os << mv.promo_type().value(); 
	return os;
}

/*optional<Move> chess::find_move(const string& t_name, const AnalysedPosition& apos)
{
	for (const auto& mr : apos.moves()){
		if (mr.to_string() == t_name) return Move(apos.pos(), mr);
	}
	return nullopt;
}*/

chess::Position::Position(const string& fen)
{
	istringstream iss(fen);
	vector<string> words;
	for (string word; iss >> word;) words.push_back(word);

	constexpr array<char, 7> white {'_', 'P', 'N', 'B', 'R', 'Q', 'K'};
	constexpr array<char, 7> black {'_', 'p', 'n', 'b', 'r', 'q', 'k'};
	
	stringstream pieces(words[0]);
	int num_empty = 0;
	for (int r = 7; r >= 0; --r) {
		for (int f = 0; f < 8; ++f) {
			num_empty -= 1;
			if (num_empty > 0) continue;
			
			char next_char;
			pieces >> next_char;
			int loc_w = distance(begin(white), find(begin(white), end(white), next_char));
			if (loc_w < 7) { set(f + r * 8, Piece(Almnt::White, (Piece::Type) loc_w)); continue; }
			int loc_b = distance(begin(black), find(begin(black), end(black), next_char));
			if (loc_b < 7) { set(f + r * 8, Piece(Almnt::Black, (Piece::Type) loc_b)); continue; }
			stringstream(string(1, next_char)) >> num_empty; //read int
		}
		if (r > 0) {
			char discard;
			pieces >> discard;
		}
	}
	
	m_to_move = (words[1] == "w" ? Almnt::White : Almnt::Black);
	
	for (const char& c : words[2]) {
		if (c == 'K') mut_castle(Almnt::White, Side::Kingside) = true;
		if (c == 'Q') mut_castle(Almnt::White, Side::Queenside) = true;
		if (c == 'k') mut_castle(Almnt::Black, Side::Kingside) = true;
		if (c == 'q') mut_castle(Almnt::Black, Side::Queenside) = true;
	}
	
	m_en_passant_target = (words[3] == "-" ? nullopt : make_optional<Square>(words[3].c_str()));
	
	stringstream(words[4]) >> m_hm_clock;
	stringstream(words[5]) >> m_fm_count;
}

void apply_castling(Position& pos, const Move& mv) {
	const Almnt to_move = mv.moved().almnt();
	const Square king_sq = (to_move == Almnt::White ? Square("e1") : Square("e8"));
	const bool kingside = mv.final_sq().file() > mv.initial_sq().file();
	const int king_dir = (kingside ? 1 : -1);
	const Square rook_sq = (kingside ? king_sq.translate(3, 0).value() : king_sq.translate(-4, 0).value());
	pos.set(king_sq, Piece());
	pos.set(rook_sq, Piece());
	pos.set(king_sq.translate(2 * king_dir, 0).value(), Piece(to_move, Piece::Type::King));
	pos.set(king_sq.translate(king_dir, 0).value(), Piece(to_move, Piece::Type::Rook));
}

void apply_en_passant(Position& pos, const Move& mv) {
	pos.set(mv.final_sq(), pos.at(mv.initial_sq()));
	pos.set(mv.initial_sq(), Piece());
	const int capt_dir = (mv.final_sq().file() > mv.initial_sq().file() ? 1 : -1);
	pos.set(mv.initial_sq().translate(capt_dir, 0).value(), Piece());
}

void apply_promotion(Position& pos, const Move& mv) {
	assert(mv.promo_type());
	pos.set(mv.final_sq(), Piece(mv.moved().almnt(), mv.promo_type().value()));
	pos.set(mv.initial_sq(), Piece());
}

chess::Position::Position(const Position& t_pos, const Move& mv)
	: m_board(t_pos.m_board), m_to_move(!mv.moved().almnt()), m_castle(t_pos.m_castle),
	m_hm_clock(t_pos.m_hm_clock + 1), m_fm_count(t_pos.m_fm_count)
{
	//Expects(t_pos.game_result() == nullopt);
	Expects(t_pos.to_move() == mv.moved().almnt());
	Expects(t_pos.at(mv.initial_sq()) == mv.moved());
	if (!mv.is_en_passant()) Expects(t_pos.at(mv.final_sq()) == mv.captured());

	//handle move clocks
	if(to_move() == Almnt::White) m_fm_count += 1;
	if(mv.captured().type() != Piece::Type::Empty) m_hm_clock = 0;
	if(mv.moved().type() == Piece::Type::Pawn) m_hm_clock = 0;

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
		this->set(mv.final_sq(), this->at(mv.initial_sq()));
		this->set(mv.initial_sq(), Piece());
	}

	//check to disable castling
	if (mv.moved().type() == Piece::Type::King && 
		(can_castle(t_pos.to_move(), Side::Kingside) ||
		can_castle(t_pos.to_move(), Side::Queenside))) {
		mut_castle(t_pos.to_move(), Side::Kingside) = false;
		mut_castle(t_pos.to_move(), Side::Queenside) = false;
	}
	if (mv.moved().type() == Piece::Type::Rook) {
		if (can_castle(t_pos.to_move(), Side::Queenside) && mv.initial_sq().file() == 0) {
			mut_castle(t_pos.to_move(), Side::Queenside) = false;
		}
		if (can_castle(t_pos.to_move(), Side::Kingside) && mv.initial_sq().file() == 7) {
			mut_castle(t_pos.to_move(), Side::Kingside) = false;
		}
	}
	if (mv.captured().type() == Piece::Type::Rook) {
		if (can_castle(!t_pos.to_move(), Side::Queenside) && mv.final_sq().file() == 0) {
			mut_castle(!t_pos.to_move(), Side::Queenside) = false;
		}
		if (can_castle(!t_pos.to_move(), Side::Kingside) && mv.final_sq().file() == 7) {
			mut_castle(!t_pos.to_move(), Side::Kingside) = false;
		}
	}

	//create en_passant_target
	if (mv.moved().type() == Piece::Type::Pawn) {
		const int rank_gap = mv.final_sq().rank() - mv.initial_sq().rank();
		if (rank_gap == 2) m_en_passant_target = mv.initial_sq().translate(0,1);
		if (rank_gap == -2) m_en_passant_target = mv.initial_sq().translate(0,-1);
	}
}

Position chess::Position::std_start()
{
	Position pos;
	constexpr array<Piece::Type, 8> backrank
		{Piece::Type::Rook, Piece::Type::Knight, Piece::Type::Bishop, Piece::Type::Queen,
		Piece::Type::King, Piece::Type::Bishop, Piece::Type::Knight, Piece::Type::Rook};
	for (int i = 0; i < 8; i++) {
		pos.set(i + 8 * 0, Piece(Almnt::White, backrank[i]));
		pos.set(i + 8 * 1, Piece(Almnt::White, Piece::Type::Pawn)); 
		pos.set(i + 8 * 6, Piece(Almnt::Black, Piece::Type::Pawn));
		pos.set(i + 8 * 7, Piece(Almnt::Black, backrank[i]));
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
			const Piece p = at(f + r * 8);
			if (p.type() == Piece::Type::Empty) {
				empty_counter += 1;
				continue;
			}
			if (empty_counter != 0) {
				output += to_string(empty_counter);
				empty_counter = 0;
			}
			output += (p.almnt() == Almnt::White ? white[(int) p.type()] : black[(int) p.type()]);
		}
		if (empty_counter != 0) {
			output += to_string(empty_counter);
			empty_counter = 0;
		}
		if (r != 0) output += '/';
	}

	output += (to_move() == Almnt::White ? " w" : " b");

	output += ' ';
	bool no_castling = true;
	if (can_castle(Almnt::White, Side::Kingside)) {output += 'K'; no_castling = false;}
	if (can_castle(Almnt::White, Side::Queenside)) {output += 'Q'; no_castling = false;}
	if (can_castle(Almnt::Black, Side::Kingside)) {output += 'k'; no_castling = false;}
	if (can_castle(Almnt::Black, Side::Queenside)) {output += 'q'; no_castling = false;}
	if (no_castling) output += '-';

	output += ' ';
	if(en_passant_target()) output += (string) en_passant_target().value();
	else output += '-';

	output += ' ';
	output += to_string(hm_clock());
	output += ' ';
	output += to_string(fm_count());
	
	return output;
}

bool chess::operator==(const Position& p1, const Position& p2)
{
	return (p1.m_board == p2.m_board &&
		p1.m_to_move == p2.m_to_move &&
		p1.m_en_passant_target == p2.m_en_passant_target &&
		p1.m_castle == p2.m_castle);
}

bool chess::operator!=(const Position& p1, const Position& p2)
{
	return !(p1 == p2);
}

chess::Position::operator string() const
{
	stringstream ss;
	ss << *this;
	return ss.str();
}

std::ostream& chess::operator<<(std::ostream& os, const Position& pos)
{
	os << "To move: " << (pos.to_move() == Almnt::White ? "white" : "black");
	os << "      hm: " << pos.hm_clock() << " fm: " << pos.fm_count() << endl;
	constexpr const char* hline = "---------------------------------";
	os << hline <<endl;
	for (int r = 7; r >= 0; r--) {
		for (int f = 0; f < 8; f++) {
			os << "| " << pos.at(f + r * 8) << " ";
		}
		os << "|" << endl << hline << endl;
	}
	return os;
}

/*optional<Move> chess::move_from_fen(const string& fen, const Position& curr_pos, const vector<Move>& moves) {
	for (const auto& m : moves){
		Position new_pos(curr_pos, m);
		if (new_pos.as_fen() == fen) return make_optional(m);
	}
	return nullopt;
}*/