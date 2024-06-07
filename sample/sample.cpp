#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <getopt.h>
#include <torch/torch.h>
#include "../Discretizer.h"
#include "../CPPFImdlp.h"
#include "../BinDisc.h"
#include "../tests/ArffFiles.h"

const string PATH = "tests/datasets/";

/* print a description of all supported options */
void usage(const char* path)
{
    /* take only the last portion of the path */
    const char* basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path;

    std::cout << "usage: " << basename << "[OPTION]" << std::endl;
    std::cout << "  -h, --help\t\t Print this help and exit." << std::endl;
    std::cout
        << "  -f, --file[=FILENAME]\t {all, diabetes, glass, iris, kdd_JapaneseVowels, letter, liver-disorders, mfeat-factors, test}."
        << std::endl;
    std::cout << "  -p, --path[=FILENAME]\t folder where the arff dataset is located, default " << PATH << std::endl;
    std::cout << "  -m, --max_depth=INT\t max_depth pased to discretizer. Default = MAX_INT" << std::endl;
    std::cout
        << "  -c, --max_cutpoints=FLOAT\t percentage of lines expressed in decimal or integer number or cut points. Default = 0 -> any"
        << std::endl;
    std::cout << "  -n, --min_length=INT\t interval min_length pased to discretizer. Default = 3" << std::endl;
}

tuple<string, string, int, int, float> parse_arguments(int argc, char** argv)
{
    string file_name;
    string path = PATH;
    int max_depth = numeric_limits<int>::max();
    int min_length = 3;
    float max_cutpoints = 0;
    const vector<struct option> long_options = {
            {"help",          no_argument,       nullptr, 'h'},
            {"file",          required_argument, nullptr, 'f'},
            {"path",          required_argument, nullptr, 'p'},
            {"max_depth",     required_argument, nullptr, 'm'},
            {"max_cutpoints", required_argument, nullptr, 'c'},
            {"min_length",    required_argument, nullptr, 'n'},
            {nullptr,         no_argument,       nullptr, 0}
    };
    while (true) {
        const auto c = getopt_long(argc, argv, "hf:p:m:c:n:", long_options.data(), nullptr);
        if (c == -1)
            break;
        switch (c) {
            case 'h':
                usage(argv[0]);
                exit(0);
            case 'f':
                file_name = string(optarg);
                break;
            case 'm':
                max_depth = stoi(optarg);
                break;
            case 'n':
                min_length = stoi(optarg);
                break;
            case 'c':
                max_cutpoints = stof(optarg);
                break;
            case 'p':
                path = optarg;
                if (path.back() != '/')
                    path += '/';
                break;
            case '?':
                usage(argv[0]);
                exit(1);
            default:
                abort();
        }
    }
    if (file_name.empty()) {
        usage(argv[0]);
        exit(1);
    }
    return make_tuple(file_name, path, max_depth, min_length, max_cutpoints);
}

void process_file(const string& path, const string& file_name, bool class_last, int max_depth, int min_length,
    float max_cutpoints)
{
    ArffFiles file;

    file.load(path + file_name + ".arff", class_last);
    const auto attributes = file.getAttributes();
    const auto items = file.getSize();
    std::cout << "Number of lines: " << items << std::endl;
    std::cout << "Attributes: " << std::endl;
    for (auto attribute : attributes) {
        std::cout << "Name: " << get<0>(attribute) << " Type: " << get<1>(attribute) << std::endl;
    }
    std::cout << "Class name: " << file.getClassName() << std::endl;
    std::cout << "Class type: " << file.getClassType() << std::endl;
    std::cout << "Data: " << std::endl;
    std::vector<mdlp::samples_t>& X = file.getX();
    mdlp::labels_t& y = file.getY();
    for (int i = 0; i < 5; i++) {
        for (auto feature : X) {
            std::cout << fixed << setprecision(1) << feature[i] << " ";
        }
        std::cout << y[i] << std::endl;
    }
    auto test = mdlp::CPPFImdlp(min_length, max_depth, max_cutpoints);
    size_t total = 0;
    for (auto i = 0; i < attributes.size(); i++) {
        auto min_max = minmax_element(X[i].begin(), X[i].end());
        std::cout << "Cut points for feature " << get<0>(attributes[i]) << ": [" << setprecision(3);
        test.fit(X[i], y);
        auto cut_points = test.getCutPoints();
        for (auto item : cut_points) {
            std::cout << item;
            if (item != cut_points.back())
                std::cout << ", ";
        }
        total += test.getCutPoints().size();
        std::cout << "]" << std::endl;
        std::cout << "Min: " << *min_max.first << " Max: " << *min_max.second << std::endl;
        std::cout << "--------------------------" << std::endl;
    }
    std::cout << "Total cut points ...: " << total << std::endl;
    std::cout << "Total feature states: " << total + attributes.size() << std::endl;
    std::cout << "Version ............: " << test.version() << std::endl;
    std::cout << "Transformed data (vector)..: " << std::endl;
    test.fit(X[0], y);
    auto data = test.transform(X[0]);
    for (int i = 130; i < 135; i++) {
        std::cout << std::fixed << std::setprecision(1) << X[0][i] << " " << data[i] << std::endl;
    }
    auto Xt = torch::tensor(X[0], torch::kFloat32);
    auto yt = torch::tensor(y, torch::kInt64);
    //test.fit_t(Xt, yt);
    auto result = test.fit_transform_t(Xt, yt);
    std::cout << "Transformed data (torch)...: " << std::endl;
    for (int i = 130; i < 135; i++) {
        std::cout << std::fixed << std::setprecision(1) << Xt[i].item<float>() << " " << result[i].item<int64_t>() << std::endl;
    }
    auto disc = mdlp::BinDisc(3);
    auto res_v = disc.fit_transform(X[0], y);
    disc.fit_t(Xt, yt);
    auto res_t = disc.transform_t(Xt);
    std::cout << "Transformed data (BinDisc)...: " << std::endl;
    for (int i = 130; i < 135; i++) {
        std::cout << std::fixed << std::setprecision(1) << Xt[i].item<float>() << " " << res_v[i] << " " << res_t[i].item<int64_t>() << std::endl;
    }
}

void process_all_files(const map<string, bool>& datasets, const string& path, int max_depth, int min_length,
    float max_cutpoints)
{
    std::cout << "Results: " << "Max_depth: " << max_depth << "  Min_length: " << min_length << "  Max_cutpoints: "
        << max_cutpoints << std::endl << std::endl;
    printf("%-20s %4s %4s\n", "Dataset", "Feat", "Cuts Time(ms)");
    printf("==================== ==== ==== ========\n");
    for (const auto& dataset : datasets) {
        ArffFiles file;
        file.load(path + dataset.first + ".arff", dataset.second);
        auto attributes = file.getAttributes();
        std::vector<mdlp::samples_t>& X = file.getX();
        mdlp::labels_t& y = file.getY();
        size_t timing = 0;
        size_t cut_points = 0;
        for (auto i = 0; i < attributes.size(); i++) {
            auto test = mdlp::CPPFImdlp(min_length, max_depth, max_cutpoints);
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            test.fit(X[i], y);
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            timing += std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
            cut_points += test.getCutPoints().size();
        }
        printf("%-20s %4lu %4zu %8zu\n", dataset.first.c_str(), attributes.size(), cut_points, timing);
    }
}


int main(int argc, char** argv)
{
    std::map<std::string, bool> datasets = {
            {"diabetes",           true},
            {"glass",              true},
            {"iris",               true},
            {"kdd_JapaneseVowels", false},
            {"letter",             true},
            {"liver-disorders",    true},
            {"mfeat-factors",      true},
            {"test",               true}
    };
    std::string file_name;
    std::string path;
    int max_depth;
    int min_length;
    float max_cutpoints;
    tie(file_name, path, max_depth, min_length, max_cutpoints) = parse_arguments(argc, argv);
    if (datasets.find(file_name) == datasets.end() && file_name != "all") {
        std::cout << "Invalid file name: " << file_name << std::endl;
        usage(argv[0]);
        exit(1);
    }
    if (file_name == "all")
        process_all_files(datasets, path, max_depth, min_length, max_cutpoints);
    else {
        process_file(path, file_name, datasets[file_name], max_depth, min_length, max_cutpoints);
        std::cout << "File name ....: " << file_name << std::endl;
        std::cout << "Max depth ....: " << max_depth << std::endl;
        std::cout << "Min length ...: " << min_length << std::endl;
        std::cout << "Max cutpoints : " << max_cutpoints << std::endl;
    }
    return 0;
}