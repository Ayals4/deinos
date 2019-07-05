#include "algorithm.h"
#include <memory>
#include <cmath>
#include <sstream>
#include <thread>
#include <chrono>
#include <gsl/gsl_util>
#include <list>
using namespace std;
using namespace std::chrono_literals;
using namespace chess;
using namespace algorithm;

void algorithm::AnalysedPosition::append_calculation(Square start)
{
	const auto moved = pos().at(start);
	const auto mvd_t = moved.type();
	const auto mvd_a = moved.almnt();
	auto& moves_out = m_moves[as_index(mvd_a)];
	auto& ctrl_out = m_control[as_index(mvd_a)];

	//TODO: use [=] or [&] in lambdas?
	const auto add_move = [&](Square end) {moves_out.emplace_back(start, end);};
	const auto add_promo = [&](Square end) {for (auto t : MoveRecord::promo_types) moves_out.emplace_back(start, end, t);};
	const auto add_ctrl = [&](Square end) {ctrl_out.set(end.file(), end.rank(), ctrl_out.get(end.file(), end.rank()) + 1);};

	typedef array<pair<int, int>, 4> mv_tmpl;
	constexpr mv_tmpl knight_1 {make_pair(1, 2), make_pair(-1, 2), make_pair(1, -2), make_pair(-1, -2)};
	constexpr mv_tmpl knight_2 {make_pair(2, 1), make_pair(2, -1), make_pair(-2, 1), make_pair(-2, -1)};
	constexpr mv_tmpl orthogonal {make_pair(1,0), make_pair(0,1), make_pair(-1,0), make_pair(0,-1)};
	constexpr mv_tmpl diagonal {make_pair(1,1), make_pair(1,-1), make_pair(-1,1), make_pair(-1,-1)};
	
	const auto apply_tmpl = [&](const mv_tmpl& tmpl, bool repeat)
	{
		for (const auto& p : tmpl) {
			optional<Square> new_s = start;
			do {
				new_s = new_s->translate(p.first, p.second);
				if (!new_s) break;
				const auto capt_almnt = pos().at(*new_s).almnt_res();
				if (capt_almnt != mvd_a) {
					add_move(*new_s);
					add_ctrl(*new_s);
				}
				if (capt_almnt != AlRes::None) break;
			} while (repeat);
		}
	};

	const auto handle_pawn = [&]()
	{
		array<int, 2> start_ranks {1, 6};
		const int mvdir = (mvd_a == Almnt::White ? 1 : -1);
		const bool dbl_mv = start.rank() == start_ranks[as_index(mvd_a)];
		const bool promo = start.rank() == start_ranks[as_index(!mvd_a)];
		for (int i = -1; i <= 1; i++) {
			const auto new_s = start.translate(i,mvdir);
			if (!new_s) continue;
			const auto capt = pos().at(*new_s);
			const auto capt_t = capt.type();
			const auto capt_a = capt.almnt();
			if (i != 0) {
				add_ctrl(*new_s);
				if (!(pos().en_passant_target() && *new_s == *pos().en_passant_target())
					&& (capt_t == Piece::Type::Empty || capt_a == mvd_a)) continue; //capturing moves conditions
			}
			else if (capt.type() != Piece::Type::Empty) continue; //forward moves conditions
			
			if (promo) add_promo(*new_s);
			else add_move(*new_s);
			
			if (dbl_mv && i == 0){ //double move
				const Square dbl_sq = *start.translate(0, mvdir * 2); //translation always valid
				if (pos().at(dbl_sq).type() == Piece::Type::Empty) add_move(dbl_sq);
			}
		}
	};

	switch (mvd_t) {
		case Piece::Type::Empty: break;
		case Piece::Type::Pawn: handle_pawn(); break;
		case Piece::Type::Knight: apply_tmpl(knight_1, false); apply_tmpl(knight_2, false); break;
		case Piece::Type::King: apply_tmpl(orthogonal, false); apply_tmpl(diagonal, false); break;
		case Piece::Type::Queen: apply_tmpl(orthogonal, true); apply_tmpl(diagonal, true); break;
		case Piece::Type::Rook: apply_tmpl(orthogonal, true); break;
		case Piece::Type::Bishop: apply_tmpl(diagonal, true); break;
	}
}

void algorithm::AnalysedPosition::append_castling() //needs a cancastle check
{
	//cerr << *this << endl;
	typedef pair<Almnt, Side> castle_t;
	constexpr array<castle_t, 4> castle_types {
		make_pair(Almnt::White, Side::Kingside), make_pair(Almnt::White, Side::Queenside),
		make_pair(Almnt::Black, Side::Kingside), make_pair(Almnt::Black, Side::Queenside)
	};

	for (const auto c : castle_types) {
		if (!pos().can_castle(c.first, c.second)) continue;
		const int mvdir = (c.second == Side::Kingside ? 1 : -1);
		const int num_sqs = (c.second == Side::Kingside ? 2 : 3);
		const auto start = Square(c.first == Almnt::White ? "e1" : "e8");
		bool ok = true;
		for (int i = 0; i < 3; i++) if (ctrl(!c.first, *start.translate(i * mvdir, 0)) > 0) ok = false;
		for (int i = 1; i <= num_sqs; i++) if (pos().at(*start.translate(i * mvdir, 0)).type() != Piece::Type::Empty) ok = false;
		if (ok) m_moves[as_index(c.first)].emplace_back(start, *start.translate(2 * mvdir, 0));
	}
}

algorithm::AnalysedPosition::AnalysedPosition(const chess::Position& t_pos)
	: m_position(t_pos)
{
	for (auto& v : m_moves) v.reserve(140);
	for (Square s : all_squares) {
		append_calculation(s);
		const int index = (int) m_position.at(s).almnt();
		if (m_position.at(s).type() == Piece::Type::King) m_king_sq[index] = s;
	}
	append_castling();
	for (auto& v : m_moves) v.shrink_to_fit();
}

void algorithm::AnalysedPosition::advance_by(chess::MoveRecord mr) //en_passant?
{
	Move mv(pos(), mr);
	assert(!(mv.is_en_passant() || mv.is_castling() || mv.is_promotion()));

	const Square start = mr.initial();
	const Square end = mr.final();
	auto info = get_occlusion(mr);
	
	const auto prune_moves = [&]() { //need white and black vector TODO
		vector<MoveRecord> new_w;
		vector<MoveRecord> new_b;
		new_w.reserve(m_moves[0].size());
		new_b.reserve(m_moves[1].size());
		for (const MoveRecord m : m_moves[0]) {
			bool keep = true;
			const Square si {m.initial()};
			const Square sf {m.final()};
			if (si == start) keep = false; //cache m.initial()?
			if (si == end) keep = false; //cull moves of captured piece
			for (int i = 0; i < info.counts[0]; i++) if (si == info.squares[0][i]) keep = false;
			if (pos().at(si).type() != Piece::Type::King) { //strip castling
				if (si.file() == 4 && (sf.file() == 2 || sf.file() == 6)) {
					keep = false;
					continue;
				}
			}
			if (keep) new_w.push_back(m);
			else {
				if (pos().at(si).type() != Piece::Type::Pawn || si.file() != sf.file()) { //is not pawn forward move
					m_control[0].set(sf.file(), sf.rank(), m_control[0].get(sf.file(), sf.rank()) - 1);
				}
			}
		}
		for (const MoveRecord m : m_moves[1]) {
			bool keep = true;
			const Square si {m.initial()};
			const Square sf {m.final()};
			if (si == start) keep = false;
			if (si == end) keep = false; //cull moves of captured piece
			for (int i = 0; i < info.counts[1]; i++) if (si == info.squares[1][i]) keep = false;
			if (pos().at(si).type() != Piece::Type::King) { //strip castling
				if (si.file() == 4 && (sf.file() == 2 || sf.file() == 6)) {
					keep = false;
					continue;
				}
			}
			if (keep) new_b.push_back(m);
			else {
				if (pos().at(si).type() != Piece::Type::Pawn || si.file() != sf.file()) { //is not pawn forward move
					m_control[1].set(sf.file(), sf.rank(), m_control[1].get(sf.file(), sf.rank()) - 1);
				}
			};
		}
		m_moves[0] = move(new_w);
		m_moves[1] = move(new_b);
	};
	
	//get_occlusion(start); //need to handle castling occlusion
	//get_occlusion(end);
	prune_moves();

	m_position = Position(m_position, mv);

	append_calculation(end);
	for (int i = 0;i < info.counts[0]; i++) append_calculation(info.squares[0][i]); //only call as many times as nencessary
	for (int i = 0;i < info.counts[1]; i++) append_calculation(info.squares[1][i]);
	append_castling();

	Piece p = pos().at(start);
	if (p.type() == Piece::Type::King) {
		m_king_sq[as_index(p.almnt())] = end;
	}
}

int mr_dir(MoveRecord mr)
{
	const Square sqi = mr.initial();
	const Square sqf = mr.final();
	const int file_delta = (int) sqf.file() - (int) sqi.file();// cerr << "fd: " << file_delta << endl;
	const int rank_delta = (int) sqf.rank() - (int) sqi.rank();// cerr << "rd: " << rank_delta << endl;

	constexpr array<int, 9> lookup {
		7, 0, 1,
		6, -1, 2,
		5, 4, 3
	};

	int result = lookup[(clamp(file_delta, -1, 1) + 1) + 3 * (1 - clamp(rank_delta, -1 , 1))];
	assert (result != -1);
	return result;
}

//remember to delete moves of captured piece
AnalysedPosition::occlusion_info algorithm::AnalysedPosition::get_occlusion(MoveRecord mr)
{
	//bool dump = false;
	AnalysedPosition::occlusion_info out;
	//const int mvdir = mr_dir(mr);
	//cerr << "mvdir :" << mvdir << endl;
	const Square start = mr.initial();
	const Square end = mr.final();
	//const Piece::Type moved_type = pos().at(start).type();
	const Almnt moved_almnt = pos().at(start).almnt();
	bool check_ep = false;
	if (pos().en_passant_target()) {
		auto ir = start.rank();
		auto fr = end.rank();
		if (ir == 1 && fr == 3) check_ep = true;
		if (ir == 6 && fr == 4) check_ep = true;
	}

	typedef pair<int, int> trans;
	constexpr array<trans, 8> adjacencies {
	make_pair(0, 1), make_pair(1, 1), make_pair(1, 0), make_pair(1, -1),
	make_pair(0, -1), make_pair(-1, -1), make_pair(-1, 0), make_pair(-1, 1)
	};

	const auto check_dir = [&](Square sq, int dir)
	{
		assert(dir >= 0);
		assert(dir < 8);
		const trans t = adjacencies[dir];
		auto s = sq.translate(t.first, t.second);
		int dist = 1;
		while (s) { //check s isn't start
			if (*s != start) { //ignore own start square
				const Piece p {pos().at(*s)};
				const Piece::Type p_t {p.type()};
				const Almnt p_a {p.almnt()};

				const auto mark = [&]()
				{
					assert(out.counts[as_index(p_a)] < 16);
					out.squares[as_index(p_a)][out.counts[as_index(p_a)]] = *s;
					out.counts[as_index(p_a)] += 1;
				};
				
				switch(p_t) {
					case Piece::Type::Empty: break; //continue while loop until hit piece or board edge
					case Piece::Type::Knight: return;
					case Piece::Type::Bishop: if (dir % 2 == 1) mark(); return;
					case Piece::Type::Rook: if(dir % 2 == 0) mark(); return;
					case Piece::Type::Queen: mark(); return;
					case Piece::Type::King: if(dist <= 1) mark(); return;
					case Piece::Type::Pawn: {
						//if (moved_almnt == Almnt::White && moved_type == Piece::Type::Queen && start == Square("h5") && end == Square("h3") && pos().at("h2").type() == Piece::Type::Pawn) dump = true;
						int dist_thresh = (s->rank() != 1 && s->rank() != 6) ? 1 : 2;
						if (dist > dist_thresh) return; //too far away
						//if (dump && *s == Square("h2")) cerr << "noted: ";
						if (!check_ep && dir % 4 == 2) return; //horizontal movement not possible TODO: What about en_passant?
						if (dist == 2 && dir % 4 != 0) return; //double move is vertical
						bool in_front = sq.rank() > s->rank();
						if (p_a == Almnt::White && !in_front) return; //white pawns move forward
						if (p_a == Almnt::Black && in_front) return; //black pawns move backward
						mark();
						//if (dump && *s == Square("h2")) cerr << "noted: ";
						return;
					}
				}
			}
			s = s->translate(t.first, t.second);
			dist++;
		}
	};

	const auto check_knights = [&](Square sq)
	{
		constexpr array<trans, 8> jumps = {
			make_pair(1, 2), make_pair(-1, 2), make_pair(1, -2), make_pair(-1, -2),
			make_pair(2, 1), make_pair(2, -1), make_pair(-2, 1), make_pair(-2, -1)
		};
		for (trans t : jumps) {
			auto s = sq.translate(t.first, t.second);
			if (!s) continue;
			if (*s == start) continue;
			const Piece p {pos().at(*s)};
			const Piece::Type p_t {p.type()};
			const Almnt p_a {p.almnt()};

			if (p_t != Piece::Type::Knight) continue;
			if (p_a != moved_almnt) continue;
			assert(out.counts[as_index(p_a)] < 16);
			out.squares[as_index(p_a)][out.counts[as_index(p_a)]] = *s;
			out.counts[as_index(p_a)] += 1;
		}
	};
	
	for (int i = 0; i < 8; i++) check_dir(start, i); //check in all directions from start square
	check_knights(start);
	for (int i = 0; i < 8; i++) {
		//if ((i % 4 == mvdir % 4) && moved_type != Piece::Type::Knight) continue; //would be a repeat TODO: is this efficient/necessary?
		check_dir(end, i);
	}
	check_knights(end);
//	if (dump) {
//		cerr << "Move: " << Move(pos(), mr);
//		for (int i = 0;i < out.counts[0]; i++) cerr << " " << (string) out.squares[0][i];
//		cerr << endl;
//	}
	return out;
}

optional<Move> algorithm::AnalysedPosition::find_record(const string& name) const
{
	for (unsigned int i = 0; i < moves().size(); i++) if (moves()[i].to_string() == name) return Move(pos(), moves()[i]);
	return nullopt;
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
			os << "|" << (int) ap.m_control[0].get(j, i) << "   " << (int) ap.m_control[1].get(j, i);
		}
		os << "|" << endl << hline << endl;
	}

	for (int j = 0; j < 2; j++){
		os << (j == 0 ? "White:" : "Black:") << endl;
		int i = 0;
		for (auto mr : ap.m_moves[j]) {
			os << Move(ap.pos(), mr) << " ";
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
	//data(apos->moves().size()),
	edges(vector<Edge>(apos->moves().size()))
	{}

algorithm::Node::Node(const AnalysedPosition& t_apos)
	: apos(make_unique<AnalysedPosition>(t_apos)),
	white_to_play(apos->pos().to_move() == Almnt::White),
	//data(apos->moves().size()),
	edges(vector<Edge>(apos->moves().size()))
	{}

void algorithm::Node::increment_n() {
	data_mutex.lock();
	//data.total_n += 1;
	m_total_n += 1;
	data_mutex.unlock();
}

int algorithm::Node::preferred_index()
{
	unsigned int i = 0;
	int max_n = edges[0].visits;
	unsigned int max_loc = 0;
	i++;
	for (; i < edges.size(); i++) {
		if (edges[i].visits > max_n) {
			max_n = edges[i].visits;
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
			if (apos->moves()[i] == mv.record()) {
				//assert(equal_bar_ep(edge.node->apos->pos().as_fen(), fen)); //verify new function
				return &edges[i].node;
			}
		}
	}
	return nullptr;
}

optional<int> algorithm::Tree::edge_to_search(Node& node)
{
	node.data_mutex.lock();
	//hacky code to fix search impotence while winning !!this cuts performance by ~10%!!
	float node_avg = 0.0; 
	for (const auto& ed : node.edges) {
		if (!ed.legal()) continue;
		node_avg += ed.total_value;
	}
	node_avg /= node.m_total_n;
	float margin = 0.5 - abs(node_avg - 0.5);
	if (margin == 0.0) margin = 0.5;
	//cerr << node_avg << " <:> " << margin << endl;

	//disable margin
//	float margin = 0.5;

	const auto evaluate = [&](const Edge& ed) {
		if (!ed.legal()) return -1.0f;
		const float avg_val = (ed.visits == 0 ? 0.0 : ed.total_value / ed.visits);//ed.total_value / ((float) ed.visits + 0.01f);
		const float oriented_val = (node.white_to_play ? avg_val : 1.0f - avg_val);
		const float expl = expl_c * sqrt(node.m_total_n) / (1 + ed.visits) * 2.0 * margin;
		return oriented_val + expl;
	};

	
	const auto& vec = node.edges;
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
	return (int) max_pos;
}

void algorithm::Tree::update_prefetch(Node& node) //this algorithm is broken if the first move is best
{
	node.node_prefetch = {0};
	assert(node.edges.size() < 256);
	assert(node.node_prefetch.size() < 256);
	for (uint8_t j = 0; j < node.edges.size(); j++) {
		int to_compare = node.edges[j].visits;
		uint8_t new_index = j;
		for (uint8_t i = 0; i < node.node_prefetch.size(); i++) {
			const uint8_t pfi = node.node_prefetch[i];
			//if (new_index == pfi) break;
			if (to_compare > node.edges[pfi].visits) {
				to_compare = node.edges[pfi].visits;
				uint8_t temp = node.node_prefetch[i]; //use std::swap()?
				node.node_prefetch[i] = new_index;
				new_index = temp;
			}
		}
	}

	//for (uint8_t pfi : node.node_prefetch) assert(pfi < node.edges.size());
}

void algorithm::Node::update(int index, float t_value) {
	data_mutex.lock();
	m_total_n += 1;
	edges.at(index).visits += 1;
	edges.at(index).total_value += t_value;
	data_mutex.unlock();
}

void algorithm::Node::set_illegal(int index) {
	data_mutex.lock();
	//cerr << edges[index].visits << " ";
	assert(edges[index].visits <= 0);
	edges[index].visits = -1;
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
	output << "Total N: " << m_total_n << endl;
	for (unsigned int i = 0; i < edges.size(); i++) {
		output << apos->get_move(i) << ": ";
		output << edges[i].visits << " ";
		if (edges[i].visits > 0) {
			output << edges[i].total_value / (float) edges[i].visits << endl;
		}
		else {
			output << "unknown" << endl;
		}
	}

	output << "prefetch:";
	for (uint8_t index : node_prefetch) output << " " << static_cast<int>(index);
	output << endl;
	
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
	int total = 0;
	for (Edge& ed : node.edges) total += ed.visits;
	
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
		Move mv = node.apos->get_move(index);
		//cerr << node.apos->get_move(index) << endl;
		//Position pos2(node.apos->pos(), mv);
		unique_ptr<AnalysedPosition> new_apos;
		if (true || mv.is_en_passant() || mv.is_castling() || mv.is_promotion()) { //"true" to disable experimental move calculation
			new_apos = make_unique<AnalysedPosition>(node.apos->get_move(index).apply());
		}
		else {
			new_apos = make_unique<AnalysedPosition>(*node.apos);
			new_apos->advance_by(mv.record());
		}
		//auto new_apos = make_unique<AnalysedPosition>(node.apos->get_move(index).apply());//(Position(node.apos->pos(), node.apos->moves()[index]));
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

//void algorithm::Tree::search()
//{
//	if (base) evaluate_node(*base);
//}

// This function appears to be the limiting factor for speed of execution. Large quantities of cache misses occur when
// fetching the next node from ram. Prefetching is not viable since most cache misses occur when the algorithm searches
// a rare node.
void algorithm::Tree::search(bool t_update_prefetch)
{
	vector<Node*> nodes;
	vector<int> indices;

	int depth = 0;
	nodes.push_back(base.get());
	float evaluation = 0.5;

	//int diagnostic = 0;
	//for (Edge& ed : base->edges) diagnostic += ed.visits;

	while (true) {
		Node& node = *nodes.at(depth);
		
		if (node.result()) {
			node.increment_n(); //update without evaluation
			evaluation = evaluate(*node.result());
			depth -= 1; //do not further update node after break
			break;
		}
		
		//prefetch edges vector of likely next node candidates
		//for (uint8_t index : node.node_prefetch) {
			//const Edge* const ptr = (node.edges.at(index).node ? node.edges.at(index).node->edges.data() : nullptr);
			//__builtin_prefetch(ptr, 0, 1);
			//__builtin_prefetch(node.edges.at(index).node.get(), 0, 1);
			//const Edge* ptr = (node.edges.at(index).node ? node.edges.at(index).node->edges.data() : nullptr);
			//__builtin_prefetch(ptr, 0, 1);
			//if (ptr) total += ptr->visits;

			//cause cache misses on edges data for likely next nodes
			//if (node.edges.at(index).node) for (Edge& ed : node.edges.at(index).node->edges) diagnostic += ed.visits;
		//}

		//cause cache misses on edge data for this node
		//for (Edge& ed : node.edges) diagnostic += ed.visits;
		
		const auto search_res = edge_to_search(node);

		//was the next node predicted successfully?
		//bool predicted = false;
		//if (search_res) for (auto prediction : node.node_prefetch) if (*search_res == (int) prediction) predicted = true;
		
		if (!search_res) {
			if (node.apos->legal_check()) {
				node.m_result = make_optional<GameResult>(victory(!node.apos->pos().to_move()));
			}
			else {
				node.m_result = make_optional<GameResult>(GameResult::Draw);
			}
			continue; //evaluate then break
		}
		const int index = *search_res;
		indices.push_back(index); 
		node.data_mutex.lock();

		//if (t_update_prefetch) {
		//	update_prefetch(node);
		//}

		//check prediction accuracy
		//if (predicted) {
		//	if (search_res && node.edges[*search_res].node) for (Edge& ed : node.edges[*search_res].node->edges) diagnostic += ed.visits;
		//} else {
		//	if (search_res && node.edges[*search_res].node) for (Edge& ed : node.edges[*search_res].node->edges) diagnostic += ed.visits;
		//}
		
		auto& node_ptr = node.edges[index].node;
		if (node_ptr) {
			node.data_mutex.unlock();
			depth += 1;
			nodes.push_back(node_ptr.get());
			continue;
		}
		else {
			Move mv = node.apos->get_move(index);
			unique_ptr<AnalysedPosition> new_apos;
			if (false || mv.is_en_passant() || mv.is_castling() || mv.is_promotion()) { //true to disable experimental move generation
				new_apos = make_unique<AnalysedPosition>(node.apos->get_move(index).apply());
			}
			else {
				new_apos = make_unique<AnalysedPosition>(*node.apos);
				new_apos->advance_by(mv.record());
			}
			if (new_apos->illegal_check()) {
				node.data_mutex.unlock();
				node.set_illegal(index);
				indices.pop_back();
				continue;
			}
			else {
				node_ptr = make_unique<Node>(move(new_apos));;
				node.data_mutex.unlock();
				evaluation = value_fn(*node_ptr->apos);
				break;
			}
		}
	}

	for (int i = depth; i >= 0; i--) {
		nodes[i]->update(indices.at(i), evaluation);
	}

	//if (diagnostic > 1000000000) cerr << "wow"; //check unlikely condition to prevent optimising out
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
			if (total_n() > 10000000) { //hacky "solution" to avoid running out of ram
				this_thread::sleep_for(1ms);
				continue;
			}
			for (int i = 0; i < 999; i++) m_tree.search(); //should be 1000
			m_tree.search(true);
		}
	};

	m_threads.emplace_back(constant_search);
	m_threads.emplace_back(constant_search);
	m_threads.emplace_back(constant_search);
	m_threads.emplace_back(constant_search);

	cerr << "Initial fen: " << m_tree.base->apos->pos().as_fen() << endl;
}

const Move algorithm::TreeEngine::choose_move()
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