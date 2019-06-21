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
		std::mutex ptr_mutex;
		std::unique_ptr<Node> node = nullptr;
	};

	struct EdgeDatum {
		double total_value = 0.0;
		int visits = 0;
	};

	struct EdgeData {
		EdgeData(int moves_num) : edges(std::vector<EdgeDatum>(moves_num)) {}
		int total_n = 1;
		std::vector<EdgeDatum> edges;
	};

	class Node {
	public:
		explicit Node(AnalysedPosition&& t_apos);
		explicit Node(const AnalysedPosition& t_apos);
		std::variant<Node*, double> follow(int index, std::function<double(const AnalysedPosition&)> value_fn);
		void update(int index, double t_value);
		void increment_n();
		int preferred_index();
		const chess::Move& best_move(); //TODO
		int to_search(); //later with exploration coefficient

		std::string display() const;

		const std::unique_ptr<const AnalysedPosition> apos;
		const std::optional<double> result; //use GameResult TODO
		const bool white_to_play;
	private:
		std::mutex data_mutex;
		EdgeData data;
		std::vector<Edge> edges;
	};

	struct Tree {
	public:
		Tree(const AnalysedPosition& base_apos, std::function<double(const AnalysedPosition&)> t_value_fn)
			: base(std::make_unique<Node>(base_apos)), value_fn(t_value_fn) {}
		void search(); //TODO
		std::unique_ptr<Node> base;
		std::function<double(const AnalysedPosition&)> value_fn;
	};

	class Engine {
	public:
		virtual ~Engine() = default;
		virtual void force_move(const chess::Move&) = 0;
		virtual const chess::Move& best_move() const = 0;

		virtual const chess::Position& pos() const = 0;
		virtual const AnalysedPosition& apos() const = 0;
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
}
#endif