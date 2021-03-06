#ifndef DEINOS_ALGORITHM_H
#define DEINOS_ALGORITHM_H
#include "chess.h"
#include <vector>
#include <array>
#include <gsl/pointers>
#include <mutex>
#include <variant>
#include <functional>
#include <string>
#include <thread>
#include <future>
#include <condition_variable>

//memory consmption ideas:
//can halve Move size by removing piece type tracking and putting boold inside promotion piece
//can reduce AnalysedPosition size by doubling up control inside one byte
//perhaps dynamically delete parts of the tree e.g. only positions and regenerate if necessary
//deletion thread?

//Other
//use gsl::index in for loops?

namespace algorithm {
	class AnalysedPosition{
	public:
		constexpr AnalysedPosition() = default;
		explicit AnalysedPosition(const chess::Position&); //generate from scratch

		void advance_by(chess::MoveRecord);
		struct occlusion_info {
			std::array<std::array<chess::Square, 16>, 2> squares;
			std::array<int, 2> counts = {0};
		};
		AnalysedPosition::occlusion_info get_occlusion(chess::MoveRecord);

		inline const chess::Position& pos() const {return m_position;}
		inline uint8_t ctrl(chess::Almnt a, chess::Square s) const {return m_control[chess::as_index(a)].get(s.file(), s.rank());}
		inline const std::vector<chess::MoveRecord>& moves(chess::Almnt a) const {return m_moves[chess::as_index(a)];}
		inline const std::vector<chess::MoveRecord>& moves() const {return moves(pos().to_move());}
		inline chess::Move get_move(int index) const {return chess::Move(pos(), moves().at(index));}
		inline const chess::Square king_sq(chess::Almnt a) const {return m_king_sq[chess::as_index(a)];}
		inline bool in_check(chess::Almnt a) const {return ctrl(!a, king_sq(a)) > 0;}
		inline bool legal_check() const {return in_check(pos().to_move());}
		inline bool illegal_check() const {return in_check(!pos().to_move());}
		std::optional<chess::Move> find_record(const std::string& name) const;
		friend std::ostream& operator<<(std::ostream& os, const AnalysedPosition& ap);
		
	private:
		void append_calculation(chess::Square start); //calculate data associated with this square and append to state (for initialisation)
		void append_castling();
		std::array<chess::Square, 2> m_king_sq;
		chess::Position m_position;
		std::array<chess::HalfByteBoard, 2> m_control;
		std::array<std::vector<chess::MoveRecord>, 2> m_moves = {};
	};
	std::ostream& operator<<(std::ostream& os, const AnalysedPosition& ap);

	class Node;
	
	struct Edge {
		//std::mutex ptr_mutex;
		float total_value;
		int visits = 0;
		//bool legal = true;
		inline bool legal() const {return visits != -1;}
		std::unique_ptr<Node> node = nullptr;
	};

	struct EdgeDatum {
		float total_value = 0.0;
		int visits = 0;
		bool legal = true;
	};

	struct EdgeData {
		EdgeData(int moves_num) : edges(std::vector<EdgeDatum>(moves_num)) {}
		int total_n = 1;
		std::vector<EdgeDatum> edges;
	};

	class Node { //TODO
	public:
		explicit Node(std::unique_ptr<const AnalysedPosition> t_apos);
		explicit Node(const AnalysedPosition& t_apos);
		int preferred_index();
		std::unique_ptr<Node>* find_child(const std::string& fen);
		std::unique_ptr<Node>* find_child(const chess::Move& mv);
		inline Node* child(int index) const {return edges[index].node.get();}
		const chess::Move best_move() {return chess::Move(apos->pos(), apos->moves()[preferred_index()]);}
		inline std::optional<chess::GameResult> result() const {return m_result;}
		//inline int res_dist() const {return m_res_dist;} //TODO
		std::string display() const;

		inline int total_n() const {return m_total_n;} //synchronise?
		
		const std::unique_ptr<const AnalysedPosition> apos;
		const bool white_to_play;
		
	private:
		void update(int index, float t_value);
		void increment_n();
		void set_illegal(int index);
	
		std::optional<chess::GameResult> m_result = std::nullopt;
		//int m_res_dist = 0; //TODO
		//std::mutex data_mutex2;
		//EdgeData data;
		int m_total_n = 1;
		std::vector<Edge> edges;
		//std::vector<Edge> edges2;
		std::mutex data_mutex; //40B

		std::array<uint8_t, 3> node_prefetch = {0};
		
		friend class Tree;
	};

	class Tree {
	public:
		Tree(const AnalysedPosition& base_apos, std::function<float(const AnalysedPosition&)> t_value_fn,
			std::function<float(const AnalysedPosition&, const chess::Move&)> t_prior_fn, float t_expl_c = 0.2)			
			: base(std::make_unique<Node>(base_apos)), value_fn(t_value_fn), prior_fn(t_prior_fn), expl_c(t_expl_c) {}
		void search(bool update_prefetch = false);
		std::unique_ptr<Node> base;
		std::function<float(const AnalysedPosition&)> value_fn;
		std::function<float(const AnalysedPosition&, const chess::Move&)> prior_fn;
		float expl_c = 0.2; //exploration coefficient

	private:
		float evaluate_node(Node& node); //used in search() for recursion
		void update_node(int index, float t_value);
		std::optional<int> edge_to_search(Node& node);
		void update_prefetch(Node& node);
	};

	class TreeEngine {
	public:
		TreeEngine(
			const AnalysedPosition& initial_position,
			std::function<float(const AnalysedPosition&)> t_value_fn,
			std::function<float(const AnalysedPosition&, const chess::Move&)> t_prior_fn,
			float exploration_coefficient);

		~TreeEngine() {
			halt_promise.set_value();
			for (auto& t : m_threads) t.join();
		}

		TreeEngine(const TreeEngine&) = delete;
		TreeEngine& operator=(const TreeEngine&) = delete;
		TreeEngine(TreeEngine&&) = delete;
		TreeEngine& operator=(TreeEngine&&) = delete;

		//void start();
		const chess::Move choose_move(); //maybe add exploration?
		bool advance_to(const std::string& fen);
		bool advance_by(const chess::Move& mv); //TODO
		std::string display() const; //TODO

		inline int total_n() const {return m_tree.base->total_n();};
		
	private:
		bool should_pause();
		void pause(); //blocks until all threads are paused
		void resume(); //blocks until all threads are resumed
	
		Tree m_tree;
		std::promise<void> halt_promise;
		std::mutex pause_mx;
		std::condition_variable pause_cv;
		bool pause_bool = false;
		int pause_count = 0;
		std::vector<std::thread> m_threads;
	};
}
#endif