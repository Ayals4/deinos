#include <iostream>
#include <sstream>
#include <vector>
#include "deinos/chess.h"
#include "deinos/algorithm.h"
using namespace std;
using namespace chess;
using namespace algorithm;

int main() {
	RandomEngine reng;
	Engine& engine = reng;
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
		stringstream ss(input);
		vector<string> words;
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
		}
		if (input == "go") {
			//cerr << "GO ACKNOWLEDGED" << endl;
			Move to_make = engine.best_move();
			cout << "move " << to_make.to_xboard() << endl;
			engine.force_move(to_make);
		}
	}
}