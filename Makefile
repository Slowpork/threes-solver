threes-solver: threes-solver.cc
	$(CXX) -Wall -Wextra -DNDEBUG -g -std=c++11 -O3 -flto -o $@ $^

threes-solver-main.js: threes-solver.cc
	emcc -Wall -Wextra -O3 -flto -g4 -o $@ -std=c++11 -s RESERVED_FUNCTION_POINTERS=1 -s EXPORTED_FUNCTIONS="['_get_next_move','_create_board','_free_board', '_create_worker', '_serialize_board', '_make_computers_move', '_shift_board']" $^

threes-solver-worker.js: threes-solver.cc
	emcc -Wall -Wextra -O3 -flto -s BUILD_AS_WORKER=1 -g4 -o $@ -std=c++11 -s EXPORTED_FUNCTIONS="['_web_worker']" $^
