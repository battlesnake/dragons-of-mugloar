#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <getopt.h>

#include "AnsiCodes.hpp"

/* Zero-based index of first "optional feature" column */
static constexpr size_t feature_col = 12;

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

/*
 * Calculate cost of operation based on changes.
 *
 * This controls how we prioritise different objectives (score, lives, etc).
 */
static float costfunction(const vector<float>::const_iterator& v)
{
	float cost = 0;
	/* Death = huge loss */
	cost += v[0] * -3000;
	/* Each life = moderate value */
	cost += v[1] > 0 ? v[1] * 100 : v[1] * 1000;
	/* Score = 1 per point */
	cost += v[2];
	/* Reputation = 100 per point */
	cost += v[3] * 100;
	cost += v[4] * 100;
	cost += v[5] * 100;

	return cost;
}

/* Structure to hold training data */
struct Dataset
{
	/* Mapping of tag strings to column indices in feature matrix */
	unordered_map<string, size_t> tags;
	unordered_map<size_t, string> tags_r;

	/* Header entries */
	vector<string> header;

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

/* Build the dataset from the string lists read from the input file */
Dataset build_dataset(const vector<string>& header, const vector<vector<string>>& data)
{
	cerr << "Building dataset..." << endl;
	Dataset out;

	out.header = header;

	/* Assign each tag a unique number, used as column number to create the feature matrix */
	out.tags.reserve(data.size() * 3);
	out.tags.max_load_factor(10);
	out.tags_r.reserve(data.size() * 3);
	out.tags_r.max_load_factor(10);
	for (const auto& line : data) {
		for (auto it = line.begin() + feature_col, end = line.end(); it != end; ++it) {
			auto [tag, is_new] = out.tags.try_emplace(*it, out.tags.size() + feature_col);
			if (is_new) {
				out.tags_r[tag->second] = tag->first;
			}
		}
	}

	/* Set matrix geometry */
	cerr << "Tags: " << out.tags.size() << endl;
	cerr << "Rows: " << data.size() << endl;
	out.cols = out.tags.size() + feature_col;
	out.rows = data.size();
	out.data.resize(out.cols * out.rows);
	std::fill(out.data.begin(), out.data.end(), 0.0f);

	/* Build the feature matrix */
	size_t row = 0;
	for (const auto& line : data) {
		size_t col = 0;
		for (const auto& field : line) {
			if (col < feature_col) {
				/* Copy value verbatim */
				out(row, col) = std::stod(field);
			} else {
				/* Lookup tag's column and set to 1 */
				out(row, out.tags[field]) = 1;
			}
			++col;
		}
		++row;
	}
	return out;
}

/* Read the file line-by-line, splitting each column by tab-terminator */
static pair<vector<string>, vector<vector<string>>> read_file(const string& in)
{
	cerr << "Reading file " << in << "..." << endl;
	ifstream f(in);
	vector<string> header;
	vector<vector<string>> data;
	data.reserve(10000);
	/* Read file line-by-line */
	string line;
	while (getline(f, line)) {
		string_view sv(line);
		string_view::size_type end;
		/* Split line into fields by tab-terminator */
		auto& fields = data.emplace_back();
		fields.reserve(50);
		while ((end = sv.find('\t')) != string_view::npos) {
			fields.push_back(string(sv.substr(0, end)));
			sv.remove_prefix(end + 1);
		}
		/* If first token is not numeric, drop row (header rows) */
		try {
			std::stod(fields[0]);
		} catch (std::invalid_argument e) {
			auto last = data.end() - 1;
			if (header.empty()) {
				header = std::move(*last);
			}
			data.erase(last);
		}
	}
	return { std::move(header), std::move(data) };
}

static vector<float> calc_row_costs(const Dataset& dataset)
{
	cerr << "Calculating costs for each action..." << endl;

	vector<float> row_cost;

	row_cost.resize(dataset.rows);

	for (size_t row = 0; row < dataset.rows; ++row) {
		row_cost[row] = costfunction(dataset.row_begin(row));
	}

	return row_cost;
}

static vector<pair<float, size_t>> calc_feature_costs(const Dataset& dataset, const vector<float>& row_cost)
{
	cerr << "Accumulating costs for each feature (no cross-correlation)" << endl;

	vector<pair<float, size_t>> feature_cost;
	feature_cost.resize(dataset.cols);
	std::fill(feature_cost.begin(), feature_cost.end(), make_pair(0.0f, size_t(0)));

	for (size_t row = 0; row < dataset.rows; ++row) {
		/* Calculate number of non-zero "presence" entries in this row */
		size_t nonzero = 0;
		for (size_t col = feature_col; col < dataset.cols; ++col) {
			const auto& input = dataset(row, col);
			if (input != 0) {
				nonzero++;
			}
		}
		/* Accumulate cost per-feature */
		for (size_t col = 0; col < dataset.cols && nonzero > 0; ++col) {
			auto& [total_cost, samples] = feature_cost[col];
			const auto& cost = row_cost[row];
			const auto& input = dataset(row, col);
			if (input != 0) {
				if (col < feature_col) {
					total_cost += cost * input;
				} else {
					/* Scale boolean feature cost by number of boolean features */
					total_cost += cost * input / nonzero;
				}
				samples++;
			}
		}
	}

	for (auto& [value, samples] : feature_cost) {
		if (samples > 0) {
			value /= samples;
		}
	}

	return feature_cost;
}

static void save_result(const Dataset& dataset, const vector<pair<float, size_t>>& feature_cost, const string& filename)
{
	cerr << "Saving result to file " << filename << endl;

	ofstream f(filename);

	for (size_t col = 0; col < dataset.cols; ++col) {
		const auto& header = col < feature_col ? dataset.header[col] : dataset.tags_r.at(col);
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

	const auto [header, cells] = read_file(infilename);

	const auto dataset = build_dataset(header, cells);

	const auto row_cost = calc_row_costs(dataset);

	const auto feature_cost = calc_feature_costs(dataset, row_cost);

	save_result(dataset, feature_cost, outfilename);

}
