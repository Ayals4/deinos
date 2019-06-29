#include <iostream>
#include <sstream>
#include <vector>
#include "deinos/chess.h"
#include "deinos/algorithm.h"
#include "deinos/dsai.h"
using namespace std;
using namespace chess;
using namespace algorithm;
using namespace std::chrono_literals;
using std::chrono::system_clock;

int main() {
	//const auto dumb_val = [&] (const AnalysedPosition&) {return 0.5;};
	//const auto dumb_pri = [&] (const AnalysedPosition& ap, const Move&) {return 1.0 / (double) ap.moves().size();};
	const auto dumb_val = dsai::material_vf;
	const auto dumb_pri = dsai::uniform_pf;

	AnalysedPosition apos(Position::std_start());
	auto engine = make_unique<TreeEngine>(apos, dumb_val, dumb_pri, 0.5);
	while (true) {
		//cerr << engine.apos();
		string input;
		getline(cin, input);
		//cerr << input << endl;
		if (input == "ping 123") {
			//cerr << "PING ACKNOWLEDGED" << endl;
			cout << "pong 123" << endl;
		}
		if (input == "quit") {
			break;
		}
		if (input == "go") {
			//cerr << "GO ACKNOWLEDGED" << endl;
			this_thread::sleep_for(5000ms);
			//cerr << engine->display();
			Move to_make = engine->choose_move();
			cerr << engine->display();
			cout << "move " << to_make.to_xboard() << endl;
			if(!engine->advance_by(to_make)) cerr << "ERROR: Could not advance position after move" << endl;
		}
		
		stringstream ss(input);
		string word1;
		ss >> word1;
		if (word1 == "setboard") {
			string fen;
			getline(ss, fen);
			fen.erase(fen.begin());
			//cerr << "FEN: " << fen << endl;
			if (!engine->advance_to(fen)) {
				engine.reset(new TreeEngine(AnalysedPosition(Position(fen)), dumb_val, dumb_pri, 0.2));
				cerr << "ENGINE RESET: position not recognised" << endl;
			}
		}
		/*vector<string> words;
		string word;
		while(std::getline(ss, word, ' ')) {
			words.push_back(word);
		}
		//for (string w : words) {
		//	cerr << w << ' ' << endl;
		//}
		if (words.size() >= 2 && words[0] == "setboard") {
			cerr << "SETBOARD: " << words[1];
			const optional<Move> to_make = move_from_fen(words[1], engine.pos(), engine.moves());
			if (to_make) {
				cerr << " : " << to_make.value() << endl;
				engine.force_move(to_make.value());
			}
			else cerr << " : " << "MOVE NOT FOUND" << endl;
		}*/
	}
}