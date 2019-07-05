#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "deinos/chess.h"
#include "deinos/algorithm.h"
#include "deinos/dsai.h"
using namespace std;
using namespace chess;
using namespace algorithm;
using namespace std::chrono_literals;
using std::chrono::system_clock;

int main() {
	//chess::Position pos = chess::Position::std_start();
	//algorithm::RandomEngine rengine;
	//algorithm::Engine& engine = rengine;

	//algorithm::AnalysedPosition ap(pos);
	//cout << endl << ap << endl;
	/*cout << pos.as_fen() << endl;
	while (true) {
		chess::Move move;
		if (ap.pos().to_move() == chess::Alignment::White) {
			cout << "Please enter a valid move: ";
			char console_input[140];
			cin.getline(console_input, 140);
			string input(console_input);
			cout << "you entered: " << input << endl;
			if (input == "exit") break;

			const auto result = find_move(input, ap.moves(pos.to_move()));
			if (!result) {cout << "Invalid Move" << endl; continue;}
			move = result.value();
		}
		else {
			move = engine.best_move();
		}

		engine.force_move(move);

		pos = chess::Position(pos, move);

		ap = algorithm::AnalysedPosition(pos);
		cout << endl << ap << endl;
		cout << pos.as_fen() << endl;
		
		if (ap.pos().game_result()) {
			switch (ap.pos().game_result().value()) {
				case chess::GameResult::White: cout << "White wins!" <<endl; break;
				case chess::GameResult::Black: cout << "Black wins!" <<endl; break;
				case chess::GameResult::Draw: cout << "It's a draw!" <<endl; break;
			}
			break;
		}
	}*/

	/*chess::Position new_pos;
	new_pos["a2"] = chess::Piece(chess::Alignment::Black, chess::Piece::Type::King);
	new_pos["b7"] = chess::Piece(chess::Alignment::White, chess::Piece::Type::King);
	new_pos["b3"] = chess::Piece(chess::Alignment::White, chess::Piece::Type::Rook);
	new_pos["h7"] = chess::Piece(chess::Alignment::White, chess::Piece::Type::Rook);
	//ap = algorithm::AnalysedPosition(new_pos);
	//ap = algorithm::AnalysedPosition(chess::Position(new_pos, ap.moves()[25]));
	cout << endl << ap << endl;

	const auto dumb = [&](const algorithm::AnalysedPosition& t_ap){
		if (t_ap.pos()["c3"].type == chess::Piece::Type::Knight) return 0.9;
		else return 0.5;
	};
	
	algorithm::Tree tree(ap, dumb);
	for (int i = 0; i < 1000000; i ++) {
		tree.search();
	}
	cout << tree.base->display();
	cout << "best move: " << tree.base->best_move() << endl;
	algorithm::Node* n = tree.base.get();
	cout << n->best_move() << endl;
	for (int i = 0; i < 6; i++) {
		n = get<algorithm::Node*>(n->follow(n->preferred_index(), dumb));
		if (n->result) {
			cout << "#" << endl;
			break;
		}
		cout << n->best_move() << endl;
	}*/
	//cout << "best move: " << tree.base->apos->moves()[tree.base->preferred_index()] << endl;

	Position pos = Position::std_start();
	//pos["b8"] = Piece();
	AnalysedPosition start_pos(pos);
	const auto dumb_val = dsai::material_vf;//= [&] (const AnalysedPosition&) {return 0.5;};
	const auto dumb_pri = [&] (const AnalysedPosition& ap, const Move&) {return 1.0 / (float) ap.moves().size();};
	TreeEngine engine(start_pos, dumb_val, dumb_pri, 0.3);

	cout << dumb_val(start_pos) << endl;

	cout << endl;
	cout << engine.display() << endl;

	//engine.start();
	this_thread::sleep_for(1000ms);
	//bool result = engine.advance_to("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
	//assert(result);
	//this_thread::sleep_for(500ms);

	//while (engine.total_n() < 10000) this_thread::sleep_for(10ms);
	
	cout << engine.display() << endl;

	cout << "sizeof AnPos: " << sizeof(AnalysedPosition) << endl;
	cout << "sizeof Node: " << sizeof(Node) << endl;
	cout << "sizeof Edge: " << sizeof(Edge) << endl;
}
