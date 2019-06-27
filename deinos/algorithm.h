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

namespace algorithm {
	class AnalysedPosition{
	public:
		constexpr AnalysedPosition() = default;
		explicit AnalysedPosition(const chess::Position&); //generate from scratch

		inline const chess::Position& pos() const {return m_position;}
		inline int ctrl(chess::Alignment a, chess::Square s) const {return m_control[(int) a][s.file()][s.rank()];}
		inline const std::vector<chess::Move>& moves(chess::Alignment a) const {return m_moves[(int) a];}
		inline const std::vector<chess::Move>& moves() const {return moves(pos().to_move());}
		inline const chess::Square king_sq(chess::Alignment a) const {return m_king_sq[(int) a];}
		inline bool in_check(chess::Alignment a) const {return ctrl(!a, king_sq(a)) > 0;}
		inline bool legal_check() const {return in_check(pos().to_move());}
		inline bool illegal_check() const {return in_check(!pos().to_move());}
		friend std::ostream& operator<<(std::ostream& os, const AnalysedPosition& ap);
		
	private:
		std::array<chess::Square, 2> m_king_sq;
		chess::Position m_position;
		std::array<std::array<std::array<int, 8>, 8>, 2> m_control {0};
		std::array<std::vector<chess::Move>, 2> m_moves = {};
	};
	std::ostream& operator<<(std::ostream& os, const AnalysedPosition& ap);

	class Node;
	
	struct Edge {
		//std::mutex ptr_mutex;
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
		inline Node* child(int index) const {return edges[index].node.get();}
		const chess::Move& best_move() {return apos->moves()[preferred_index()];}
		inline std::optional<chess::GameResult> result() const {return m_result;}
		//inline int res_dist() const {return m_res_dist;} //TODO
		std::string display() const;

		inline int total_n() const {return data.total_n;} //synchronise?
		
		const std::unique_ptr<const AnalysedPosition> apos;
		const bool white_to_play;
		
	private:
		void update(int index, float t_value);
		void increment_n();
		void set_illegal(int index);
	
		std::optional<chess::GameResult> m_result = std::nullopt;
		//int m_res_dist = 0; //TODO
		std::mutex data_mutex;
		EdgeData data;
		std::vector<Edge> edges;
		
		friend class Tree;
	};

	class Tree {
	public:
		Tree(const AnalysedPosition& base_apos, std::function<float(const AnalysedPosition&)> t_value_fn,
			std::function<float(const AnalysedPosition&, const chess::Move&)> t_prior_fn, float t_expl_c = 0.2)			
			: base(std::make_unique<Node>(base_apos)), value_fn(t_value_fn), prior_fn(t_prior_fn), expl_c(t_expl_c) {}
		void search();
		std::unique_ptr<Node> base;
		std::function<float(const AnalysedPosition&)> value_fn;
		std::function<float(const AnalysedPosition&, const chess::Move&)> prior_fn;
		float expl_c = 0.2; //exploration coefficient

	private:
		float evaluate_node(Node& node); //used in search() for recursion
		void update_node(int index, float t_value);
		std::optional<int> edge_to_search(Node& node);
	};

	/*class Engine {
	public:
		virtual ~Engine() = default;
		virtual void force_move(const chess::Move&) = 0;
		virtual const chess::Move& best_move() const = 0;

		//virtual const chess::Position& pos() const = 0;
		//virtual const AnalysedPosition& apos() const = 0;
		virtual const std::vector<chess::Move>& moves() const = 0;
	};

	class RandomEngine : public Engine {
	public:
		constexpr RandomEngine() = default;
		void force_move(const chess::Move& mv) override;
		const chess::Move& best_move() const override;

		inline const chess::Position& pos() const override {return m_apos.pos();}
		inline const AnalysedPosition& apos() const override {return m_apos;}
		inline const std::vector<chess::Move>& moves() const override {return m_apos.moves();}
		
	private:
		AnalysedPosition m_apos = AnalysedPosition(chess::Position::std_start());
	};

	class TreeEngine : public Engine {
	public:
		TreeEngine()
		void force_move(const chess::Move& mv) override;
		const chess::Move& best_move() const override;

		
	}*/

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
		const chess::Move& choose_move(); //maybe add exploration?
		bool advance_to(const std::string& fen);
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