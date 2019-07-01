#include "algorithm.h"
#include <memory>
#include <cmath>
#include <sstream>
#include <thread>
#include <chrono>
using namespace std;
using namespace std::chrono_literals;
using namespace chess;
using namespace algorithm;

void calculate_moves(const Position& pos, vector<Move>& output,
	std::array<std::array<uint8_t, 8>, 8>& ctrl_out, Square start)
{
	/*const Piece moved = pos[start];
	const Almnt to_move = moved.almnt();

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

	const auto register_en_passant = [&](Square s_end)
	{
		output.push_back(Move(start, s_end, moved, Piece(!to_move, Piece::Type::Pawn)));
		output.back().set_en_passant();
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
		
	switch (moved.type()) {
		case Piece::Type::Empty: break;
		
		case Piece::Type::Pawn: {
			const int mvdir = (to_move == Almnt::White ? 1 : -1);
			const int init_rnk = (to_move == Almnt::White ? 1 : 6);
			const int pr_rnk = (to_move == Almnt::White ? 6 : 1);
			const bool dbl_mv = start.rank() == init_rnk;
			const bool can_pr = start.rank() == pr_rnk;

			for (int i = -1; i < 2; i++) {
				const auto new_s = start.translate(i,mvdir);
				if (!new_s) continue;
				const auto capt = pos[new_s.value()];
				if (i == 0) {
					if (capt.type() != Piece::Type::Empty) continue;
				}
				else {
					register_control(new_s.value());
					if(pos.en_passant_target() && new_s.value() == pos.en_passant_target().value()) { //handle en_passant
						register_en_passant(new_s.value());
						continue;
					}
					if (capt.type() == Piece::Type::Empty || capt.almnt() == to_move) continue;
				}
				if (can_pr) register_promotion(new_s.value());
				else register_move(new_s.value());

				if (dbl_mv && i == 0){ //double move
					const auto new_s2 = start.translate(i,mvdir * 2).value(); //always valid
					if (pos[new_s2].type() != Piece::Type::Empty) continue;
					register_move(new_s2);
				}
			}
		} break;
		
		case Piece::Type::Knight: apply_tmpl(knight_tmpl1, false); apply_tmpl(knight_tmpl2, false); break;
		case Piece::Type::King: apply_tmpl(strt_tmpl, false); apply_tmpl(diag_tmpl, false); break;
		case Piece::Type::Queen: apply_tmpl(strt_tmpl, true); apply_tmpl(diag_tmpl, true); break;
		case Piece::Type::Rook: apply_tmpl(strt_tmpl, true); break;
		case Piece::Type::Bishop: apply_tmpl(diag_tmpl, true); break;
	}*/
}

void calculate_castling(const AnalysedPosition& apos, vector<Move>& output, Almnt to_move) {
	/*constexpr Piece WKing = Piece(Almnt::White, Piece::Type::King);
	constexpr Piece WRook = Piece(Almnt::White, Piece::Type::Rook);
	constexpr Piece BKing = Piece(Almnt::Black, Piece::Type::King);
	constexpr Piece BRook = Piece(Almnt::Black, Piece::Type::Rook);
	constexpr Piece Empty = Piece();
	constexpr Almnt Wt = Almnt::White;
	constexpr Almnt Bk = Almnt::Black;
	const Position& pos = apos.pos();
	switch (to_move) { //sanitise somehow by factoring? TODO
		case Almnt::White: {
			if (pos.can_castle(Almnt::White, Side::Queenside)) {
				Expects(pos["e1"] == WKing);
				Expects(pos["a1"] == WRook);
				if (pos["b1"] == Empty && pos["c1"] == Empty && pos["d1"] == Empty &&
					apos.ctrl(Bk, "e1") == 0 && apos.ctrl(Bk, "d1") == 0 && apos.ctrl(Bk, "c1") == 0) {
					Move m(Square("e1"), Square("c1"), WKing, Empty);
					m.set_castling();
					output.push_back(m);
				}
			}
			if (pos.can_castle(Almnt::White, Side::Kingside)) {
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
		case Almnt::Black: {
			if (pos.can_castle(Almnt::Black, Side::Queenside)) {
				Expects(pos["e8"] == BKing);
				Expects(pos["a8"] == BRook);
				if (pos["b8"] == Empty && pos["c8"] == Empty && pos["d8"] == Empty &&
					apos.ctrl(Wt, "e8") == 0 && apos.ctrl(Wt, "d8") == 0 && apos.ctrl(Wt, "c8") == 0) {
					Move m(Square("e8"), Square("c8"), BKing, Empty);
					m.set_castling();
					output.push_back(m);
				}
			}
			if (pos.can_castle(Almnt::Black, Side::Kingside)) {
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
	}*/
}

algorithm::AnalysedPosition::AnalysedPosition(const chess::Position& t_pos)
	: m_position(t_pos)
{
	for (Square s : all_squares) {
		const int index = (int) m_position.at(s).almnt();
		calculate_moves(t_pos, m_moves[index], m_control[index], s);
		if (m_position.at(s).type() == Piece::Type::King) m_king_sq[index] = s;
	}
	calculate_castling(*this, m_moves[0], Almnt::White);
	calculate_castling(*this, m_moves[1], Almnt::Black);
}

ostream& algorithm::operator<<(ostream& os, const AnalysedPosition& ap)
{
	const auto w = Almnt::White;
	const auto b = Almnt::Black;
	const auto k = Side::Kingside;
	const auto q = Side::Queenside;
	os << "Cast: " << (ap.pos().can_castle(w,k) ? "Y" : "N") << " ";
	os << "" << (ap.pos().can_castle(w,q) ? "Y" : "N") << " ";
	os << "" << (ap.pos().can_castle(b,k) ? "Y" : "N") << " ";
	os << "" << (ap.pos().can_castle(b,q) ? "Y" : "N") << " ";
	os << "    Prev: ";
	//if (ap.pos().prev_move()) os << ap.pos().prev_move().value();
	os << "???   ";
	os << "     To Move: " << (ap.pos().to_move() == Almnt::White ? "White" : "Black") << endl;
	
	constexpr const char* hline = "-------------------------------------------------";
	os << hline <<endl;
	for (int i = 7; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			os << "|  " << ap.m_position.at(chess::all_squares[j + i * 8]) << "  ";
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

algorithm::Node::Node(unique_ptr<const AnalysedPosition> t_apos)
	: apos(move(t_apos)),
	white_to_play(apos->pos().to_move() == Almnt::White),
	data(apos->moves().size()),
	edges(vector<Edge>(apos->moves().size()))
	{}

algorithm::Node::Node(const AnalysedPosition& t_apos)
	: apos(make_unique<AnalysedPosition>(t_apos)),
	white_to_play(apos->pos().to_move() == Almnt::White),
	data(apos->moves().size()),
	edges(vector<Edge>(apos->moves().size()))
	{}

void algorithm::Node::increment_n() {
	data_mutex.lock();
	data.total_n += 1;
	data_mutex.unlock();
}

int algorithm::Node::preferred_index()
{
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

bool equal_bar_ep (const string & fen1, const string & fen2)
{
	stringstream ss1(fen1);
	stringstream ss2(fen2);

	for (int i = 0; i < 6; i++) {
		string word1;
		ss1 >> word1;
		string word2;
		ss2 >> word2;
		if (i == 3) continue;
		if (word1 != word2) return false;
	}
	return true;
}

unique_ptr<Node>* algorithm::Node::find_child(const string& fen)
{
	for (auto& edge : edges) {
		if (edge.node) {
			//cerr << edge.node->apos->pos().as_fen() << " --VS-- " << fen << endl;
			if (equal_bar_ep(edge.node->apos->pos().as_fen(), fen)) {
				//assert(equal_bar_ep(edge.node->apos->pos().as_fen(), fen)); //verify new function
				return &edge.node;
			}
		}
	}
	return nullptr;
}

unique_ptr<Node>* algorithm::Node::find_child(const Move& mv)
{
	for (int i = 0; i < (int) edges.size(); i++) {
		if (edges[i].node) {
			if (apos->moves()[i] == mv) {
				//assert(equal_bar_ep(edge.node->apos->pos().as_fen(), fen)); //verify new function
				return &edges[i].node;
			}
		}
	}
	return nullptr;
}

optional<int> algorithm::Tree::edge_to_search(Node& node) { //need to make change with almnt TODO
	node.data_mutex.lock();
	//hacky code to fix search impotence while winning
	float node_avg = 0.0; 
	for (const auto& ed : node.data.edges) {
		if (!ed.legal) continue;
		node_avg += ed.total_value;
	}
	node_avg /= node.data.total_n;
	float margin = 0.5 - abs(node_avg - 0.5);
	if (margin == 0.0) margin = 0.5;
	//cerr << node_avg << " <:> " << margin << endl;

	const auto evaluate = [&](const EdgeDatum& ed) {
		if (!ed.legal) return -1.0f;
		const float avg_val = ed.total_value / ((float) ed.visits + 0.01f);
		const float oriented_val = (node.white_to_play ? avg_val : 1.0f - avg_val);
		const float expl = expl_c * sqrt(node.data.total_n) / (1 + ed.visits) * 2.0 * margin;
		return oriented_val + expl;
	};

	
	const auto& vec = node.data.edges;
	unsigned int i = 0;
	unsigned int max_pos = i;
	float max_eval = evaluate(vec[i]);
	i++;
	for (; i < vec.size(); i++) {
		const float new_eval = evaluate(vec[i]);
		if (new_eval > max_eval) {
			max_pos = i;
			max_eval = new_eval;
		}
	}
	node.data_mutex.unlock();
	if (max_eval == -1.0) return nullopt;
	return make_optional((int) max_pos);
}

void algorithm::Node::update(int index, float t_value) {
	data_mutex.lock();
	data.total_n += 1;
	data.edges[index].visits += 1;
	data.edges[index].total_value += t_value;
	data_mutex.unlock();
}

void algorithm::Node::set_illegal(int index) {
	data_mutex.lock();
	data.edges[index].legal = false;
	data_mutex.unlock();
}

string algorithm::Node::display() const
{ //use GameResult TODO
	stringstream output;
	output << apos->pos().as_fen() << endl;
	//output << "Victor: ";
	//if (result) {
	//	if (result.value() == 1.0) output << "White" << endl;
	//	else if (result.value() == 0.0) output << "Black" << endl;
	//	else output << "ERROR" << endl;
	//}
	//else output << "none" << endl;
	output << "Total N: " << data.total_n << endl;
	for (unsigned int i = 0; i < data.edges.size(); i++) {
		output << apos->moves()[i] << ": ";
		output << data.edges[i].visits << " ";
		if (data.edges[i].visits > 0) {
			output << data.edges[i].total_value / (float) data.edges[i].visits << endl;
		}
		else {
			output << "unknown" << endl;
		}
	}
	return output.str();
}

float algorithm::Tree::evaluate_node(Node& node)
{
	//if result determined return
	if (node.result()) {
		node.increment_n();
		return evaluate(node.result().value());
	}

	//choose edge to search & check for end of game
	const auto search_res = edge_to_search(node);
	if (!search_res) {
		if (node.apos->legal_check()) {
			node.m_result = make_optional<GameResult>(victory(!node.apos->pos().to_move()));
		}
		else {
			node.m_result = make_optional<GameResult>(GameResult::Draw);
		}
		return evaluate_node(node);
	}
	const int index = search_res.value();
	
	float evaluation;
	//node.edges[index].ptr_mutex.lock();
	node.data_mutex.lock();
	auto& node_ptr = node.edges[index].node;
	if (node_ptr) {
		//node.edges[index].ptr_mutex.unlock();
		node.data_mutex.unlock();
		evaluation = evaluate_node(*node_ptr);
	}
	else {
		auto new_apos = make_unique<AnalysedPosition>(Position(node.apos->pos(), node.apos->moves()[index]));
		if (new_apos->illegal_check()) {
			//node.edges[index].ptr_mutex.unlock();
			node.data_mutex.unlock();
			node.set_illegal(index);
			return evaluate_node(node);
		}
		else {
			node_ptr = make_unique<Node>(move(new_apos));;
			//node.edges[index].ptr_mutex.unlock();
			node.data_mutex.unlock();
			evaluation = value_fn(*node_ptr->apos);
		}
	}
	node.update(index, evaluation);
	return evaluation;
}

void algorithm::Tree::search()
{
	if (base) evaluate_node(*base);
}

algorithm::TreeEngine::TreeEngine(
	const AnalysedPosition& t_apos,
	function<float(const AnalysedPosition&)> t_value_fn,
	function<float(const AnalysedPosition&, const Move&)> t_prior_fn,
	float t_expl_c
)	: m_tree(Tree(t_apos, t_value_fn, t_prior_fn, t_expl_c))
{
	shared_future<void> halt_future(halt_promise.get_future());

	const auto constant_search = [&, halt_future] () {
		while (halt_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout) {
			if (should_pause()) {
				unique_lock<mutex> lk(pause_mx);
				pause_count++;
				pause_cv.notify_all();
				while (pause_bool) pause_cv.wait(lk);
				pause_count--;
				pause_cv.notify_all();
			}
			if (total_n() > 2000000) { //hacky "solution" to avoid running out of ram
				this_thread::sleep_for(1ms);
				continue;
			}
			for (int i = 0; i < 1000; i++) m_tree.search(); //should be 1000
		}
	};

	m_threads.emplace_back(constant_search);
	m_threads.emplace_back(constant_search);
	m_threads.emplace_back(constant_search);
	m_threads.emplace_back(constant_search);

	cerr << "Initial fen: " << m_tree.base->apos->pos().as_fen() << endl;
}

const Move& algorithm::TreeEngine::choose_move()
{
	return m_tree.base->best_move();
}

bool algorithm::TreeEngine::advance_to(const string& fen)
{
	pause();
	unique_ptr<Node>* next_ptr = m_tree.base->find_child(fen);
	if (next_ptr) {
		m_tree.base = move(*next_ptr);
		resume();
		return true;
	}
	else {
		resume();
		return false;
	}
}

bool algorithm::TreeEngine::advance_by(const Move& mv)
{
	pause();
	unique_ptr<Node>* next_ptr = m_tree.base->find_child(mv);
	if (next_ptr) {
		m_tree.base = move(*next_ptr);
		resume();
		return true;
	}
	else {
		resume();
		return false;
	}
}
string algorithm::TreeEngine::display() const
{
	stringstream output;
	output << "FEN: " << m_tree.base->apos->pos().as_fen() << endl;
	output << (string) m_tree.base->apos->pos();
	output << "Best move: " << m_tree.base->best_move() << endl;
	output << endl;
	output << m_tree.base->display();
	return output.str();
}

bool algorithm::TreeEngine::should_pause()
{
	lock_guard<mutex> lk(pause_mx);
	return pause_bool;
}

void algorithm::TreeEngine::pause()
{
	unique_lock<mutex> lk(pause_mx);
	pause_bool = true;
	pause_count = 0;
	//pause_cv.notify_all();
	while (pause_count < (int) m_threads.size()) pause_cv.wait(lk);
}

void algorithm::TreeEngine::resume()
{
	unique_lock<mutex> lk(pause_mx);
	pause_bool = false;
	assert(pause_count == (int) m_threads.size());
	pause_cv.notify_all();
	while (pause_count > 0) pause_cv.wait(lk);
}