#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP
#include<sstream>
#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<tuple>
#include "../typesFImdlp.h"

template <typename T>
void show_vector(const std::vector<T>& data, std::string title)
{
    std::cout << title << ": ";
    std::string sep = "";
    for (const auto& d : data) {
        std::cout << sep << d;
        sep = ", ";
    }
    std::cout << std::endl;
}
enum class experiment_t {
    RANGE,
    VECTOR
};
class Experiment {
public:
    Experiment(float from_, float to_, float step_, int n_bins, std::string strategy, std::vector<int> data_discretized, std::vector<mdlp::precision_t> cutpoints) :
        from_{ from_ }, to_{ to_ }, step_{ step_ }, n_bins_{ n_bins }, strategy_{ strategy }, discretized_data_{ data_discretized }, cutpoints_{ cutpoints }, type_{ experiment_t::RANGE }
    {
        validate_strategy();

    }
    Experiment(std::vector<mdlp::precision_t> dataset, int n_bins, std::string strategy, std::vector<int> data_discretized, std::vector<mdlp::precision_t> cutpoints) :
        n_bins_{ n_bins }, strategy_{ strategy }, dataset_{ dataset }, discretized_data_{ data_discretized }, cutpoints_{ cutpoints }, type_{ experiment_t::VECTOR }
    {
        validate_strategy();
    }
    void validate_strategy()
    {
        if (strategy_ != "Q" && strategy_ != "U") {
            throw std::invalid_argument("Invalid strategy " + strategy_);
        }
    }
    float from_;
    float to_;
    float step_;
    int n_bins_;
    std::string strategy_;
    std::vector<mdlp::precision_t> dataset_;
    std::vector<int> discretized_data_;
    std::vector<mdlp::precision_t> cutpoints_;
    experiment_t type_;
};
class Experiments {
public:
    Experiments(const std::string filename) : filename{ filename }
    {
        test_file.open(filename);
        if (!test_file.is_open()) {
            throw std::runtime_error("File " + filename + " not found");
        }
        exp_end = false;
    }
    ~Experiments()
    {
        test_file.close();
    }
    bool end() const
    {
        return exp_end;
    }
    bool is_next()
    {
        while (std::getline(test_file, line) && line[0] == '#');
        if (test_file.eof()) {
            exp_end = true;
            return false;
        }
        return true;
    }
    Experiment next()
    {
        return parse_experiment(line);
    }
private:
    std::tuple<float, float, float, int, std::string> parse_header(const std::string& line)
    {
        std::istringstream iss(line);
        std::string from_, to_, step_, n_bins, strategy;
        iss >> from_ >> to_ >> step_ >> n_bins >> strategy;
        return { std::stof(from_), std::stof(to_), std::stof(step_), std::stoi(n_bins), strategy };
    }
    template <typename T>
    std::vector<T> parse_vector(const std::string& line)
    {
        std::istringstream iss(line);
        std::vector<T> data;
        std::string d;
        while (iss >> d) {
            data.push_back(std::is_same<T, float>::value ? std::stof(d) : std::stoi(d));
        }
        return data;
    }
    Experiment parse_experiment(std::string& line)
    {
        // Read experiment lines
        std::string experiment, data, cuts, strategy;
        std::getline(test_file, experiment);
        std::getline(test_file, data);
        std::getline(test_file, cuts);
        // split data into variables
        float from_, to_, step_;
        int n_bins;
        std::vector<mdlp::precision_t> dataset;
        auto data_discretized = parse_vector<int>(data);
        auto cutpoints = parse_vector<mdlp::precision_t>(cuts);
        if (line == "RANGE") {
            tie(from_, to_, step_, n_bins, strategy) = parse_header(experiment);
            return Experiment{ from_, to_, step_, n_bins, strategy, data_discretized, cutpoints };
        }
        strategy = experiment.substr(0, 1);
        n_bins = std::stoi(experiment.substr(1, 1));
        data = experiment.substr(3, experiment.size() - 4);
        dataset = parse_vector<mdlp::precision_t>(data);
        return Experiment(dataset, n_bins, strategy, data_discretized, cutpoints);
    }
    std::ifstream test_file;
    std::string filename;
    std::string line;
    bool exp_end;
};
#endif