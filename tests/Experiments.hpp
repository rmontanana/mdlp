#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP
#include<sstream>
#include<iostream>
#include<string>
#include<fstream>
#include<vector>
#include<tuple>
#include "../typesFImdlp.h"
class Experiment {
public:
    Experiment(float from_, float to_, float step_, int n_bins, std::string strategy, std::vector<int> data_discretized, std::vector<float> cutpoints) :
        from_{ from_ }, to_{ to_ }, step_{ step_ }, n_bins_{ n_bins }, strategy_{ strategy }, discretized_data_{ data_discretized }, cutpoints_{ cutpoints }
    {
        if (strategy != "Q" && strategy != "U") {
            throw std::invalid_argument("Invalid strategy " + strategy);
        }
    }
    float from_;
    float to_;
    float step_;
    int n_bins_;
    std::string strategy_;
    std::vector<int> discretized_data_;
    std::vector<float> cutpoints_;
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
        if (line == "RANGE") {
            std::getline(test_file, line);
            auto [from_, to_, step_, n_bins, strategy] = parse_header(line);
        } else {
            std::getline(test_file, line);

        }
        std::getline(test_file, line);
        auto data_discretized = parse_vector<int>(line);
        std::getline(test_file, line);
        auto cutpoints = parse_vector<float>(line);
        return Experiment{ from_, to_, step_, n_bins, strategy, data_discretized, cutpoints };
    }
    std::ifstream test_file;
    std::string filename;
    std::string line;
    bool exp_end;
};
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
#endif