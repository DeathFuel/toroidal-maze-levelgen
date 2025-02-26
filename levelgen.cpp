#include <bitset>
#include <iostream>
#include <queue>
#include <limits>
#include <random>
#include <string>

// Comment out the line below to get clean output
#define INFO_PRINT

#ifdef INFO_PRINT
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

using namespace std;

const int w = 27;
const int h = 15;
const int sz = w * h;

const float TARGET_SCORE = 4800;
const int ITERATION_LIMIT = 1<<16;
const int PATTERN_ITER_LIMIT = 1<<14;

const char WALL = '#';
const char SPACE = '-';
const char PLAYER = 'p';
const char EXIT = 'e';
const char UNUSED = 'X';

mt19937 rd = mt19937(random_device{}());

float random_int(int from, int to){
	auto distribution = uniform_int_distribution<int>(from, to);
	return distribution(rd);
}

float random_float(float from, float to){
	auto distribution = uniform_real_distribution<float>(from, to);
	return distribution(rd);
}

// Returns num+offset wrapped around (modulo, not remainder) to fit into the range [min; max]
inline int offset_wrap(int num, int offset, int min, int max) {
	int a = num + offset;
	a -= min;
	int b = max - min + 1;
	a -= (b * floor((float)a / (float)b));
	a += min;
	return a;
}

class LevelGraph {
	public:
		LevelGraph(char* _level) {
			level = _level;
			graph = vector<vector<int>>(sz, vector<int>()); 
			build_graph();
		}

		void set_level(char* _level) {
			level = _level;
			build_graph();
		}

		void build_graph() {
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					// no vertex if it's a wall; otherwise, consider the four cardinal directions
					if (level[x + y * w] == WALL) { continue; }
					for (int dy = -1; dy <= 1; dy += 2) { // vertical
						bool moved = false;
						int cy = offset_wrap(y, dy, 0, h-1);
						int i = 0;
						while (level[x + cy * w] != WALL ) {
							moved = true;
							cy = offset_wrap(cy, dy, 0, h-1);
							if (++i > h) { moved = false; break; } // prevent infinite loop
						}
						if (!moved) { continue; }
						cy = offset_wrap(cy, -dy, 0, h-1);
						graph[x + y * w].push_back(x + cy * w);
					}
					for (int dx = -1; dx <= 1; dx += 2) { // horizontal
						bool moved = false;
						int cx = offset_wrap(x, dx, 0, w-1);
						int i = 0;
						while (level[cx + y * w] != WALL ) {
							moved = true;
							cx = offset_wrap(cx, dx, 0, w-1);
							if (++i > w) { moved = false; break; } // prevent infinite loop
						}
						if (!moved) { continue; }
						cx = offset_wrap(cx, -dx, 0, w-1);
						graph[x + y * w].push_back(cx + y * w);
					}
				}
			}
		}

		void printraw() {
			for (int i = 0; i < sz; i++) {
				unsigned int gsize = graph[i].size();
				if (gsize == 0) { continue; }
				printf("(%d, %d): ", i % w + 1, i / w + 1);
				for (unsigned int j = 0; j < gsize; j++) {
					printf("(%d, %d) ", graph[i][j] % w + 1, graph[i][j] / w + 1);
				}
				cout << endl;
			}
		}

		pair<int,double> score(int from) {
			queue<pair<int, double>> unexplored; // vertex, score
			vector<bool> explored(sz);
			unexplored.emplace(from, 0.0);
			int best_vertex = from;
			double best_score = -1;
			for (; !unexplored.empty(); unexplored.pop()) {
				int v = unexplored.front().first;
				double score = unexplored.front().second; 
				if (score > best_score) { 
					best_score = score;
					best_vertex = v;
				}
				explored[v] = true;
				for (int &c : graph[v]) {
					int cx = c % w;
					int cy = c / w;
					int vx = v % w;
					int vy = v / w;
					int dist = abs(cx - vx + cy - vy);
					double score_delta = pow(dist, 1.5) + 15;
					if (!explored[c]) { unexplored.emplace(c, score + score_delta); }
				}
			}
			return pair<int, double>(best_vertex, best_score); // best-scoring vertex and its score
		}

		bitset<sz> reachable_vertices(int from) {
			queue<int> unexplored;
			bitset<sz> explored;
			unexplored.push(from);
			int v;
			for (; !unexplored.empty(); unexplored.pop()) {
				v = unexplored.front();
				explored[v] = true;
				for (int &c : graph[v]) {
					if (!explored[c]) { unexplored.push(c); }
				}
			}
			return explored;
		}

		// Could have been expressed with reachable_vertices, but performance matters here and this usually returns earlier
		bool path_exists(int from, int goal) {
			queue<int> unexplored;
			vector<bool> explored(sz);
			unexplored.push(from);
			int v;
			for (; !unexplored.empty(); unexplored.pop()) {
				v = unexplored.front();
				if (v == goal) { return true; }
				explored[v] = true;
				for (int &c : graph[v]) {
					if (!explored[c]) { unexplored.push(c); }
				}
			}
			return false;
		}

	private:
		char* level;
		vector<vector<int>> graph;
};

void pattern_tile_probabilities(float* density) {
	float* last_density = new float[sz];
	for (int i = 0; i < sz; i++) { last_density[i] = density[i]; }
	float score = numeric_limits<float>::lowest();
	float last_score = score;
	// Each iteration, swap a few random pairs of nearby values and see if the score improves
	for (int iter = 0; iter < PATTERN_ITER_LIMIT; iter++) {
		float temp = 1 - (float)iter/PATTERN_ITER_LIMIT;
		temp *= 16 * temp;
		for (int s = 0; s < temp; s++) {
			int x = random_int(1, w-2);
			int y = random_int(1, h-2);
			int dx = 0; int dy = 0;
			while (dx == 0 && dy == 0) {
				dx = random_int(-1, 1);
				dy = random_int(-1, 1);
			}
			int pos1 = y * w + x;
			int pos2 = (y+dy) * w + (x+dx);
			swap(density[pos1], density[pos2]);
		}
		score = 0;
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				for (int dy = -3; dy <= 3; dy++) {
					for (int dx = -3; dx <= 3; dx++) {
						int py = y + dy;
						int px = x + dx;
						if ((dy == 0 && dx == 0) || py < 0 || px < 0 || py >= h || py >= w) { continue; }
						int pos1 = y * w + x;
						int pos2 = (y+dy) * w + (x+dx);
						float absdiff = abs(density[pos1] - density[pos2]);
						int dist = dx * dx + dy * dy;
						score += (8 - dist) * absdiff;
					}
				}
			}
		}

			if (iter % (PATTERN_ITER_LIMIT/16) == 0) { LOG("Tile pattern with score %10.2f at iteration %7d\n", score, iter); }
		if (score > last_score) {
			for (int i = 0; i < sz; i++) { last_density[i] = density[i]; }
			last_score = score;

		} else {
			for (int i = 0; i < sz; i++) { density[i] = last_density[i]; }
			score = last_score;
		}
	}
	LOG("Tile pattern with score %10.2f after %d iterations\n", score, PATTERN_ITER_LIMIT);
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			int pos = j + w * i;
			LOG("%.2f ", density[pos]);
		}
		LOG("\n");
	}
}

void distribute_tile_probabilities(float* density, float avg_density) {
	bool patterns_enabled = random_int(0, 1);
	if (patterns_enabled) { LOG("Patterns ON\n"); }
	// Assign a "density" (probability of a wall appearing) to every tile, with some noise
	float max_noise = 0.125 + patterns_enabled * random_int(1, 8);
	for (int i = 0; i < sz; i++) {
		density[i] = avg_density + random_float(-max_noise, max_noise);
	}

	// Possibly add patterns via simulated annealing
	if (patterns_enabled) { pattern_tile_probabilities(density); }

	// Clamp to sensible values
	for (int i = 0; i < sz; i++) {
		density[i] = (density[i] > 0.875) ? 0.875 : density[i];
		density[i] = (density[i] < 0) ? 0 : density[i];
	}

	// Potentially increase outer wall density
	float wall_density = random_float(0, 1);
	for (int i = 0; i < w; i++) {
		density[i] += wall_density;
		density[i+w*h-w] += wall_density;
	}
	for (int i = 1; i < h-1; i++) {
		density[w*i] += wall_density;
		density[w*i+w-1] += wall_density;
	}
}


/* Level generation process:
 * 1. For each tile, determine the likelihood of a wall being placed there.
 *    Patterns (random, striped, checkerboard, etc) can be inserted at this stage.
 * 2. Randomly create a level in accordance with the probabilities.
 * 3. Iteratively improve upon the level by rerolling tiles with the aforementioned probabilities.
 * 4. Along the way, save the best-scoring beatable level where it is impossible to get stuck.
 */

string levelgen(int px, int py, float avg_density, int& end_pos) {
	LOG("Level generation started with ppos = (%d, %d) and target density = %f\n", px + 1, py + 1, avg_density);

	// 1.
	float* density = new float[sz];
	distribute_tile_probabilities(density, avg_density);

	// 2.
	int player_pos = px + py * w;
	char* level = new char[sz];
	for (int i = 0; i < sz; i++) {
		if (density[i] == 1) {
			level[i] = WALL;
		} else {
			level[i] = (density[i] <= random_float(0, 1)) ? SPACE : WALL;
		}
	}
	level[player_pos] = SPACE;

	// 3.
	double score = numeric_limits<float>::lowest();
	double best_score = score;
	double last_score = LevelGraph(level).score(player_pos).second; // save rating of initial solution
	char* last_level = new char[sz];
	char* best_level = new char[sz+1]; best_level[0] = UNUSED; best_level[sz] = '\0';
	int best_end_pos = 0;
	int best_iteration = 0;
	int iteration = 0;
	bool stuck = true;
	bool generation_failed = false;
	int iters_since_update = 0;

	for (int i = 0; i < sz; i++) { last_level[i] = level[i]; }

	while (stuck || score < TARGET_SCORE) {
		if (++iteration > ITERATION_LIMIT) {
			generation_failed = (best_level[0] == UNUSED);
			LOG("Iteration limit reached\n");
			break;
		}

		for (int i = 0; i < sz; i++) {
			if (density[i] != 1 && random_float(0,1) < 0.02) {
				level[i] = (density[i] <= random_float(0, 1)) ? SPACE : WALL;
			}
		}
		level[player_pos] = SPACE;

		LevelGraph lg = LevelGraph(level);
		pair<int, double> best = lg.score(player_pos);
		end_pos = best.first;
		score = best.second;

		// Stuck spot check: does there exist a reachable vertex which can not reach the exit?
		// Computationally expensive (double quantifier) so we're putting it off until near the end
		if (score > 0.875 * TARGET_SCORE || iteration > 0.875 * ITERATION_LIMIT) {
			stuck = false;
			bitset<sz> reachable = lg.reachable_vertices(player_pos);
			for (int i = 0; i < sz; i++) {
				if (reachable.test(i) && !lg.path_exists(i, end_pos)) {
					stuck = true;
					break;
				}
			}
		}

		// 4.
		if (!stuck && score > best_score && lg.path_exists(player_pos, end_pos)) {
			best_score = score;
			best_end_pos = end_pos;
			best_iteration = iteration;
			for (int i = 0; i < sz; i++) {
				best_level[i] = level[i];
			}
		}


		bool acceptable = (score >= last_score);

		iters_since_update++; // cosmetic, limits console spam
		if (acceptable) {
			for (int i = 0; i < sz; i++) { last_level[i] = level[i]; }
			last_score = score;

			if (iters_since_update >= ITERATION_LIMIT/64) {
				iters_since_update = 0;
				LOG("\nScore: %7.2f Target: %7.2f Iteration %d\n", score, TARGET_SCORE, iteration);
				for (int i = 0; i < h; i++) {
					for (int j = 0; j < w; j++) {
						int pos = j + w * i;
						if (pos == end_pos) { LOG("%c", EXIT); } 
						else if (pos == player_pos) { LOG("%c", PLAYER); } 
						else { LOG("%c", level[pos]); }
					}
					LOG("\n");
				}
			}
		} else {
			for (int i = 0; i < sz; i++) { level[i] = last_level[i]; }
			score = last_score;
		}
	}
	end_pos = best_end_pos;

	best_level[player_pos] = PLAYER;
	best_level[end_pos] = EXIT;
	string lv;
	if (generation_failed) {
		lv = "";
		LOG("Level generation failed\n");
	} else {
		lv = string(best_level);
#ifdef INFO_PRINT
		LOG("\nBest recorded level with score: %7.2f/%7.2f at iteration %d:\n", score, TARGET_SCORE, best_iteration);
		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				LOG("%c", best_level[j + w * i]);
			}
			LOG("\n");
		}
		LevelGraph lg = LevelGraph(best_level);

		bool ok = true;
		LOG("End reachable: ");
		if (lg.path_exists(player_pos, end_pos)) { LOG("YES\n"); } else { LOG("NO (!)\n"); ok = false; }

		LOG("Can get stuck: ");
		stuck = false;
		bitset<sz> reachable = lg.reachable_vertices(player_pos);
		for (int i = 0; i < sz; i++) {
			if (reachable.test(i) && !lg.path_exists(i, end_pos)) {
				stuck = true;
				last_level[i] = WALL;
				break;
			}
		}
		if (stuck) { LOG("YES (!)\n"); ok = false; } else { LOG("NO\n"); }
		if (!ok) { LOG("Something's wrong - consider opening an issue with the output attached\n"); }
		LOG("\nLevel string:\n");
#endif
	}
	delete[] level;
	delete[] last_level;
	delete[] best_level;
	delete[] density;
	return lv;
}

int main() {
	if (w < 5 || h < 5) { printf("Bad size parameters\n"); return -1; };
	int end_pos = w + 1;
	while (true) {
		float d = random_float(0.125, 0.5);
		float target_density = 4 * d * d * d; // prefer sparser levels; the range is [1/128; 1/2] 
		string level = levelgen(end_pos % w, end_pos / w, target_density, end_pos);
		cout << level << endl;
#ifdef INFO_PRINT 
		LOG("Press any key to generate another level.\n");
		getchar();
#endif
	}
	return 0;
}
