#include <iostream>
#include <string>
#include "deinos/chess.h"
#include "deinos/algorithm.h"
using namespace std;

int main() {
	chess::Position pos = chess::Position::std_start();
	algorithm::RandomEngine rengine;
	algorithm::Engine& engine = rengine;

	algorithm::AnalysedPosition ap(pos);
	cout << endl << ap << endl;
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

	chess::Position new_pos;
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
	}
	//cout << "best move: " << tree.base->apos->moves()[tree.base->preferred_index()] << endl;
}