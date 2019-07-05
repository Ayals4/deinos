#include "deinos/chess.h"
#include "deinos/algorithm.h"
#include "deinos/dsai.h"
using namespace std;
using namespace chess;
using namespace algorithm;

int main() {
	auto test_pos = Position::std_start();
	Tree tree(AnalysedPosition(test_pos), dsai::material_vf, dsai::uniform_pf, 0.3);
	for (int j = 0; j < 10; j++) {
		for (int k = 0; k < 10; k++) {
			for (int i = 0; i < 999; i++) tree.search();
			tree.search(true);
		}
		cerr << "*";
	}
	cerr << endl;
	//cout << tree.base->display() << endl;
}