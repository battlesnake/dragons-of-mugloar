#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <getopt.h>

#include "Locale.hpp"
#include "AnsiCodes.hpp"

using std::string;
using std::string_view;
using std::vector;
using std::unordered_map;
using std::ifstream;
using std::ofstream;
using std::pair;
using std::make_pair;
using std::cerr;
using std::endl;
using std::getline;

/* Structure to hold training data */
struct Dataset
{
	/* Mapping of tag strings to column indices in feature matrix */
	unordered_map<string, size_t> tags;
	unordered_map<size_t, string> tags_r;

	size_t score_tag;
	size_t lives_tag;
	size_t gold_tag;
	size_t rep_people_tag;
	size_t rep_state_tag;
	size_t rep_underworld_tag;
	size_t level_tag;

	/* Feature matrix size */
	size_t cols = 0;
	size_t rows = 0;

	/* Feature matrix */
	vector<float> data;

	size_t size() const
	{
		return data.size();
	}

	/* Accessors */

	auto row_begin(int row)
	{
		return data.begin() + (row * cols);
	}

	auto row_end(int row)
	{
		return row_begin(row + 1);
	}

	float& operator () (int row, int col)
	{
		return row_begin(row)[col];
	}

	auto row_begin(int row) const
	{
		return data.begin() + (row * cols);
	}

	auto row_end(int row) const
	{
		return row_begin(row + 1);
	}

	const float& operator () (int row, int col) const
	{
		return row_begin(row)[col];
	}

};

static float asym(float x, float cost_neg, float cost_pos)
{
	return x >= 0 ? x * cost_pos : x * cost_neg;
}

/*
 * Calculate cost of operation based on changes.
 *
 * This controls how we prioritise different objectives (score, lives, etc).
 */
static float costfunction(const Dataset& ds, const vector<float>::const_iterator& v)
{
	float cost = 0;
	/* Each life = moderate value (gain), high value (loss) */
	cost += asym(v[ds.lives_tag], 150, 30);
	/* Score = 0.1 per point */
	cost += v[ds.score_tag] * 0.1f;
	/* Reputation = 10 per loss, 20 per gain */
	cost += asym(v[ds.rep_people_tag], 10, 20);
	cost += asym(v[ds.rep_state_tag], 10, 20);
	cost += asym(v[ds.rep_underworld_tag], 10, 20);
	/* Level = 500 per level */
	cost += v[ds.level_tag] * 500;

	return cost;
}

/* Build the dataset from the string lists read from the input file */
Dataset build_dataset(const vector<vector<string>>& data)
{
	cerr << "Building dataset..." << endl;
	Dataset out;

	auto foreach_line = [&] (auto callback) {
		size_t row = 0;
		for (const auto& line : data) {
			/* Skip lines with invalid number of fields */
			if (line.empty() || (line.size() & 1) == 0) {
				cerr << "Invalid line ignored" << endl;
				continue;
			}
			for (auto it = line.begin() + 1, end = line.end(); it != end; it += 2) {
				callback(row, it);
			}
			++row;
		}
	};

	/* Assign each tag a unique number, used as column number to create the feature matrix */
	out.tags.reserve(data.size() * 3);
	out.tags.max_load_factor(10);
	out.tags_r.reserve(data.size() * 3);
	out.tags_r.max_load_factor(10);
	foreach_line([&] (auto, auto col_it) {
		auto [tag, is_new] = out.tags.try_emplace(*col_it, out.tags.size());
		if (is_new) {
			/* Add reverse mapping */
			out.tags_r[tag->second] = tag->first;
		}
	});

	/* Lookup and cache column numbers for specific features */
	out.score_tag = out.tags["diff:score"];
	out.lives_tag = out.tags["diff:lives"];
	out.gold_tag = out.tags["diff:gold"];
	out.rep_people_tag = out.tags["diff:rep_people"];
	out.rep_state_tag = out.tags["diff:rep_state"];
	out.rep_underworld_tag = out.tags["diff:rep_underworld"];
	out.level_tag = out.tags["diff:level"];

	/* Set matrix geometry */
	cerr << "Tags: " << out.tags.size() << endl;
	cerr << "Rows: " << data.size() << endl;
	cerr << "Size: " << (out.tags.size() * data.size() * 4 / 1048576) << " MB" << endl;
	out.cols = out.tags.size();
	out.rows = data.size();
	out.data.resize(out.cols * out.rows);
	std::fill(out.data.begin(), out.data.end(), 0.0f);

	/* Build the feature matrix */
	foreach_line([&] (auto row, auto col_it) {
		/* Lookup tag's column and set value */
		out(row, out.tags[*col_it]) = std::stof(col_it[1]);
	});
	return out;
}

/* Read the file line-by-line, splitting each column by tab-terminator */
static vector<vector<string>> read_file(const string& in)
{
	cerr << "Reading file " << in << "..." << endl;
	ifstream f(in);
	vector<vector<string>> data;
	data.reserve(100000);

	/* Read file line-by-line */
	string line;
	while (getline(f, line)) {
		string_view sv(line);
		string_view::size_type end;
		/* Split line into fields by tab-terminator */
		auto& fields = data.emplace_back();
		fields.reserve(1000);
		while ((end = sv.find('\t')) != string_view::npos) {
			fields.push_back(string(sv.substr(0, end)));
			sv.remove_prefix(end + 1);
		}
	}
	return data;
}

static vector<float> calc_row_costs(const Dataset& dataset)
{
	cerr << "Calculating costs for each event..." << endl;

	vector<float> row_cost;

	row_cost.reserve(dataset.rows);

	for (size_t row = 0; row < dataset.rows; ++row) {
		row_cost.push_back(costfunction(dataset, dataset.row_begin(row)));
	}

	return row_cost;
}

static vector<pair<float, size_t>> calc_feature_costs(const Dataset& dataset, const vector<float>& row_cost)
{
	cerr << "Accumulating costs for each feature (no cross-correlation)" << endl;

	vector<pair<float, size_t>> feature_cost;
	feature_cost.reserve(dataset.cols);

	/* Accumulate cost per-feature */
	for (size_t col = 0; col < dataset.cols; ++col) {
		auto& [total_cost, samples] = feature_cost.emplace_back(0.0f, size_t(0));
		for (size_t row = 0; row < dataset.rows; ++row) {
			const auto& cost = row_cost[row];
			const auto& input = dataset(row, col);
			if (input != 0) {
				total_cost += cost;
				samples++;
			}
		}
	}

	for (auto& [value, samples] : feature_cost) {
		/* We only weakly consider features that we haven't sampled much */
		value /= (samples + 20);
	}

	return feature_cost;
}

static void save_result(const Dataset& dataset, const vector<pair<float, size_t>>& feature_cost, const string& filename)
{
	cerr << "Saving result to file " << filename << endl;

	ofstream f(filename);

	for (size_t col = 0; col < dataset.cols; ++col) {
		const auto& header = dataset.tags_r.at(col);
		const auto& [value, samples] = feature_cost[col];
		f << value << "\t" << samples << "\t" << header << "\t" << endl;
	}
}

static void help()
{
	cerr << "Arguments:" << endl;
	cerr << "  -i input-filename" << endl;
	cerr << "  -o output-filename" << endl;
}

int main(int argc, char *argv[])
{
	init_locale();

	const char *infilename = nullptr;
	const char *outfilename = nullptr;
	char c;
	while ((c = getopt(argc, argv, "hi:o:")) != -1) {
		switch (c) {
		case 'h': help(); return 1;
		case 'i': infilename = optarg; break;
		case 'o': outfilename = optarg; break;
		case '?': help(); return 1;
		}
	}

	if (!infilename || !outfilename || optind != argc) {
		help();
		return 1;
	}

	const auto cells = read_file(infilename);

	const auto dataset = build_dataset(cells);

	const auto row_cost = calc_row_costs(dataset);

	const auto feature_cost = calc_feature_costs(dataset, row_cost);

	save_result(dataset, feature_cost, outfilename);

}
