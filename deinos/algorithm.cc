#include "algorithm.h"
#include <memory>
#include <cmath>
#include <sstream>
using namespace std;
using namespace chess;
using namespace algorithm;

void calculate_moves(const Position& pos, vector<Move>& output,
	std::array<std::array<int, 8>, 8>& ctrl_out, Square start)
{
	const Piece moved = pos[start];
	const Alignment to_move = moved.alignment;

	const auto register_move = [&](Square s_end)
	{
		output.push_back(Move(start, s_end, moved, pos[s_end]));
	};

	const auto register_promotion = [&](Square s_end)
	{
		constexpr array<Piece::Type, 4> promotions = 
			{Piece::Type::Knight, Piece::Type::Bishop, Piece::Type::Rook, Piece::Type::Queen};
		for (Piece::Type p : promotions) {
			output.push_back(Move(start, s_end, moved, pos[s_end])); //emplace?
			output.back().set_promotion(p);
		}
	};

	const auto register_control = [&](Square s_end)
	{
		ctrl_out[s_end.file()][s_end.rank()] += 1;
	}; 

	constexpr array<pair<int, int>, 4> knight_tmpl1 = {
		make_pair(1, 2), make_pair(-1, 2), make_pair(1, -2), make_pair(-1, -2)};
	constexpr array<pair<int, int>, 4> knight_tmpl2 = {
		make_pair(2, 1), make_pair(2, -1), make_pair(-2, 1), make_pair(-2, -1)};
	constexpr array<pair<int, int>, 4> strt_tmpl = {
		make_pair(1,0), make_pair(0,1), make_pair(-1,0), make_pair(0,-1)};
	constexpr array<pair<int, int>, 4> diag_tmpl = {
		make_pair(1,1), make_pair(1,-1), make_pair(-1,1), make_pair(-1,-1)};

	const auto apply_tmpl = [&](const array<pair<int, int>, 4> tmpl, bool repeat)
	{
		for (const auto& p : tmpl) {
			auto new_s = make_optional(start);
			do {
				new_s = new_s.value().translate(p.first, p.second);
				if (!new_s) break;
				const auto capt = pos[new_s.value()].almnt_res();
				if (capt != to_move) {register_move(new_s.value()); register_control(new_s.value());}
				if (capt != AlRes::None) break;
			} while(repeat);
		}
	};
		
	switch (moved.type) {
		case Piece::Type::Empty: break;
		
		case Piece::Type::Pawn: { //TODO: en passant, promotion
			const int mvdir = (to_move == Alignment::White ? 1 : -1);
			const int init_rnk = (to_move == Alignment::White ? 1 : 6);
			const int pr_rnk = (to_move == Alignment::White ? 6 : 1);
			const bool dbl_mv = start.rank() == init_rnk;
			const bool can_pr = start.rank() == pr_rnk;

			for (int i = -1; i < 2; i++) {
				const auto new_s = start.translate(i,mvdir);
				if (!new_s) continue;
				const auto capt = pos[new_s.value()];
				if (i == 0) {
					if (capt.type != Piece::Type::Empty) continue;
				}
				else {
					register_control(new_s.value());
					if (capt.type == Piece::Type::Empty || capt.alignment == to_move) continue;
				}
				if (can_pr) register_promotion(new_s.value());
				else register_move(new_s.value());

				if (dbl_mv && i == 0){ //double move
					const auto new_s2 = start.translate(i,mvdir * 2).value(); //always valid
					if (pos[new_s2].type != Piece::Type::Empty) continue;
					register_move(new_s2);
				}
			}
		} break;
		
		case Piece::Type::Knight: apply_tmpl(knight_tmpl1, false); apply_tmpl(knight_tmpl2, false); break;
		case Piece::Type::King: apply_tmpl(strt_tmpl, false); apply_tmpl(diag_tmpl, false); break;
		case Piece::Type::Queen: apply_tmpl(strt_tmpl, true); apply_tmpl(diag_tmpl, true); break;
		case Piece::Type::Rook: apply_tmpl(strt_tmpl, true); break;
		case Piece::Type::Bishop: apply_tmpl(diag_tmpl, true); break;
	}
}

void calculate_castling(const AnalysedPosition& apos, vector<Move>& output, Alignment to_move) {
	constexpr Piece WKing = Piece(Alignment::White, Piece::Type::King);
	constexpr Piece WRook = Piece(Alignment::White, Piece::Type::Rook);
	constexpr Piece BKing = Piece(Alignment::Black, Piece::Type::King);
	constexpr Piece BRook = Piece(Alignment::Black, Piece::Type::Rook);
	constexpr Piece Empty = Piece();
	constexpr Alignment Wt = Alignment::White;
	constexpr Alignment Bk = Alignment::Black;
	const Position& pos = apos.pos();
	switch (to_move) { //sanitise somehow by factoring? TODO
		case Alignment::White: {
			if (pos.can_castle(Alignment::White, Side::Queenside)) {
				Expects(pos["e1"] == WKing);
				Expects(pos["a1"] == WRook);
				if (pos["b1"] == Empty && pos["c1"] == Empty && pos["d1"] == Empty &&
					apos.ctrl(Bk, "e1") == 0 && apos.ctrl(Bk, "d1") == 0 && apos.ctrl(Bk, "c1") == 0) {
					Move m(Square("e1"), Square("c1"), WKing, Empty);
					m.set_castling();
					output.push_back(m);
				}
			}
			if (pos.can_castle(Alignment::White, Side::Kingside)) {
				Expects(pos["e1"] == WKing);
				Expects(pos["h1"] == WRook);
				if (pos["f1"] == Empty && pos["g1"] == Empty &&
					apos.ctrl(Bk, "e1") == 0 && apos.ctrl(Bk, "f1") == 0 && apos.ctrl(Bk, "g1") == 0) {
					Move m(Square("e1"), Square("g1"), WKing, Empty);
					m.set_castling();
					output.push_back(m);
				}
			}
		} break;
		case Alignment::Black: {
			if (pos.can_castle(Alignment::Black, Side::Queenside)) {
				Expects(pos["e8"] == BKing);
				Expects(pos["a8"] == BRook);
				if (pos["b8"] == Empty && pos["c8"] == Empty && pos["d8"] == Empty &&
					apos.ctrl(Wt, "e8") == 0 && apos.ctrl(Wt, "d8") == 0 && apos.ctrl(Wt, "c8") == 0) {
					Move m(Square("e8"), Square("c8"), BKing, Empty);
					m.set_castling();
					output.push_back(m);
				}
			}
			if (pos.can_castle(Alignment::Black, Side::Kingside)) {
				Expects(pos["e8"] == BKing);
				Expects(pos["h8"] == BRook);
				if (pos["f8"] == Empty && pos["g8"] == Empty &&
					apos.ctrl(Wt, "e8") == 0 && apos.ctrl(Wt, "f8") == 0 && apos.ctrl(Wt, "g8") == 0) {
					Move m(Square("e8"), Square("g8"), BKing, Empty);
					m.set_castling();
					output.push_back(m);
				}
			}
		} break;
	}
}

void handle_en_passant(const Position& pos, vector<Move>& output, std::array<std::array<int, 8>, 8>& ctrl_out) {
	if (!pos.prev_move()) return;
	const Move& prev_move = pos.prev_move().value();
	if (prev_move.moved_piece().type != Piece::Type::Pawn) return;
	const int delta_rank = prev_move.final_square().rank() - prev_move.initial_square().rank();
	if (delta_rank == 1 || delta_rank == -1) return;
	const Alignment to_move = pos.to_move();
	const int capt_dir = (to_move == Alignment::White ? 1 : -1);
	const auto start1 = prev_move.final_square().translate(1, 0);
	if (start1 && pos[start1.value()] == Piece(to_move, Piece::Type::Pawn)) {
		const Square s_end = prev_move.final_square().translate(0, capt_dir).value();
		output.push_back(Move(
			start1.value(), s_end,
			Piece(to_move, Piece::Type::Pawn),
			Piece(!to_move, Piece::Type::Pawn)));
		output.back().set_en_passant();
		ctrl_out[s_end.file()][s_end.rank()] += 1;
	}
	const auto start2 = prev_move.final_square().translate(-1, 0);
	if (start2 && pos[start2.value()] == Piece(to_move, Piece::Type::Pawn)) {
		const Square s_end = prev_move.final_square().translate(0, capt_dir).value();
		output.push_back(Move(
			start2.value(), s_end,
			Piece(to_move, Piece::Type::Pawn),
			Piece(!to_move, Piece::Type::Pawn)));
		output.back().set_en_passant();
		ctrl_out[s_end.file()][s_end.rank()] += 1;
	}
}

algorithm::AnalysedPosition::AnalysedPosition(const chess::Position& t_pos)
	: m_position(t_pos)
{
	for (Square s : all_squares) {
		const int index = (int) m_position[s].alignment;
		calculate_moves(t_pos, m_moves[index], m_control[index], s);
		if (m_position[s].type == Piece::Type::King) m_king_sq[index] = s;
	}
	const int index = (int) t_pos.to_move();
	handle_en_passant(t_pos, m_moves[index], m_control[index]);
	calculate_castling(*this, m_moves[0], Alignment::White);
	calculate_castling(*this, m_moves[1], Alignment::Black);
}

ostream& algorithm::operator<<(ostream& os, const AnalysedPosition& ap)
{
	const auto w = Alignment::White;
	const auto b = Alignment::Black;
	const auto k = Side::Kingside;
	const auto q = Side::Queenside;
	os << "Cast: " << (ap.pos().can_castle(w,k) ? "Y" : "N") << " ";
	os << "" << (ap.pos().can_castle(w,q) ? "Y" : "N") << " ";
	os << "" << (ap.pos().can_castle(b,k) ? "Y" : "N") << " ";
	os << "" << (ap.pos().can_castle(b,q) ? "Y" : "N") << " ";
	os << "    Prev: ";
	if (ap.pos().prev_move()) os << ap.pos().prev_move().value();
	else os << "None  ";
	os << "     To Move: " << (ap.pos().to_move() == Alignment::White ? "White" : "Black") << endl;
	
	constexpr const char* hline = "-------------------------------------------------";
	os << hline <<endl;
	for (int i = 7; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			os << "|  " << ap.m_position[chess::all_squares[j + i * 8]] << "  ";
		}
		os << "|" << endl;
		for (int j = 0; j < 8; j++) {
			os << "|" << ap.m_control[0][j][i] << "   " << ap.m_control[1][j][i];
		}
		os << "|" << endl << hline << endl;
	}

	for (int j = 0; j < 2; j++){
		os << (j == 0 ? "White:" : "Black:") << endl;
		int i = 0;
		for (auto m : ap.m_moves[j]) {
			os << m << " ";
			i++;
			if (i >= 7) {os << endl; i = 0;}
		}
		if (i != 0) os << endl;
	}
	
	return os;
}

optional<double> get_result(const AnalysedPosition& apos) {
	const Alignment to_move = apos.pos().to_move();
	const Square enemy_king_sq = apos.king_sq(!to_move);
	if (apos.ctrl(to_move, enemy_king_sq) > 0) {
		return make_optional<double>(to_move == Alignment::White ? 1.0 : 0.0);
	}
	else return nullopt;
}

algorithm::Node::Node(AnalysedPosition&& t_apos)
	: apos(make_unique<AnalysedPosition>(t_apos)),
	result(get_result(*apos)),
	white_to_play(apos->pos().to_move() == Alignment::White),
	data(apos->moves().size()),
	edges(vector<Edge>(apos->moves().size()))
	{}

algorithm::Node::Node(const AnalysedPosition& t_apos)
	: apos(make_unique<AnalysedPosition>(t_apos)),
	result(get_result(*apos)),
	white_to_play(apos->pos().to_move() == Alignment::White),
	data(apos->moves().size()),
	edges(vector<Edge>(apos->moves().size()))
	{
	}

variant<Node*, double> algorithm::Node::follow(int index, function<double(const AnalysedPosition&)> value_fn) {
	edges[index].ptr_mutex.lock();
	if (edges[index].node != nullptr) {
		edges[index].ptr_mutex.unlock();
		return edges[index].node.get();
	}
	else {
		const Move& m = apos->moves()[index];
		AnalysedPosition new_apos(Position(apos->pos(), m));
		const double value = value_fn(new_apos);
		edges[index].node = make_unique<Node>(move(new_apos));
		edges[index].ptr_mutex.unlock();
		return value;
	}
}

void algorithm::Node::update(int index, double t_value) {
	data_mutex.lock();
	data.total_n += 1;
	data.edges[index].visits += 1;
	data.edges[index].total_value += t_value;
	data_mutex.unlock();
}

void algorithm::Node::increment_n() {
	data_mutex.lock();
	data.total_n += 1;
	data_mutex.unlock();
}

int algorithm::Node::preferred_index() {
	unsigned int i = 0;
	int max_n = data.edges[0].visits;
	unsigned int max_loc = 0;
	i++;
	for (; i < data.edges.size(); i++) {
		if (data.edges[i].visits > max_n) {
			max_n = data.edges[i].visits;
			max_loc = i;
		}
	}
	return (int) max_loc;
}

const Move& algorithm::Node::best_move() {
	return apos->moves()[preferred_index()];
}

int algorithm::Node::to_search() { //need to make change with alignment TODO
	const auto evaluate = [&](const EdgeDatum& ed) {
		const double avg_val = ed.total_value / ((double) ed.visits + 0.01);
		const double oriented_val = (white_to_play ? avg_val : 1.0 - avg_val);
		const double expl = 0.5 * sqrt(data.total_n) / (1 + ed.visits);
		return oriented_val + expl;
	};

	data_mutex.lock();
	const auto& vec = data.edges;
	unsigned int i = 0;
	unsigned int max_pos = i;
	double max_eval = evaluate(vec[i]);
	i++;
	for (; i < data.edges.size(); i++) {
		const double new_eval = evaluate(data.edges[i]);
		if (new_eval > max_eval) {
			max_pos = i;
			max_eval = new_eval;
		}
	}
	data_mutex.unlock();
	return (int) max_pos;
}

string algorithm::Node::display() const { //use GameResult TODO
	stringstream output;
	output << apos->pos().as_fen() << endl;
	output << "Victor: ";
	if (result) {
		if (result.value() == 1.0) output << "White" << endl;
		else if (result.value() == 0.0) output << "Black" << endl;
		else output << "ERROR" << endl;
	}
	else output << "none" << endl;
	output << "Total N: " << data.total_n << endl;
	for (unsigned int i = 0; i < data.edges.size(); i++) {
		output << apos->moves()[i] << ": ";
		output << data.edges[i].visits << " ";
		output << data.edges[i].total_value / (double) data.edges[i].visits << endl;
	}
	return output.str();
}

double search_node(Node* node, const function<double(const AnalysedPosition&)>& value_fn) {
	if (node->result) {
		node->increment_n();
		return node->result.value();
	}
	Expects(node->apos->pos().game_result() == nullopt);
	const int index_to_search = node->to_search();
	const auto follow_result = node->follow(index_to_search, value_fn);
	const double value = (follow_result.index() == 1 ?
		get<double>(follow_result) : search_node(get<Node*>(follow_result), value_fn));
	node->update(index_to_search, value);
	return value;
}

void algorithm::Tree::search() {
	search_node(base.get(), value_fn);
}

void algorithm::RandomEngine::force_move(const chess::Move& mv) {
	m_apos = AnalysedPosition(Position(m_apos.pos(), mv));
}

const Move& algorithm::RandomEngine::best_move() const {
	Expects(!m_apos.moves().empty());
	for (const Move& m : m_apos.moves()) {
		AnalysedPosition new_apos(Position(m_apos.pos(), m));
		if (new_apos.ctrl(new_apos.pos().to_move(), new_apos.king_sq(m_apos.pos().to_move())) <= 0) return m;
	}
	return m_apos.moves().front();
}