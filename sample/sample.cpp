#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <getopt.h>
#include "../CPPFImdlp.h"
#include "../tests/ArffFiles.h"

using namespace std;
using namespace mdlp;

const string PATH = "../../tests/datasets/";

/* print a description of all supported options */
void usage(const char* path)
{
    /* take only the last portion of the path */
    const char* basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path;

    cout << "usage: " << basename << "[OPTION]" << endl;
    cout << "  -h, --help\t\t Print this help and exit." << endl;
    cout << "  -f, --file[=FILENAME]\t {all, glass, iris, kdd_JapaneseVowels, letter, liver-disorders, mfeat-factors, test}." << endl;
    cout << "  -p, --path[=FILENAME]\t folder where the arff dataset is located, default " << PATH << endl;
    cout << "  -m, --max_depth=INT\t max_depth pased to discretizer. Default = MAX_INT" << endl;
    cout << "  -n, --min_length=INT\t interval min_length pased to discretizer. Default = 3" << endl;
}

tuple<string, string, int, int> parse_arguments(int argc, char** argv)
{
    string file_name;
    string path = PATH;
    int max_depth = numeric_limits<int>::max();
    int min_length = 3;
    static struct option long_options[] = {
            { "help", no_argument, 0, 'h' },
            { "file", required_argument, 0, 'f' },
            { "path", required_argument, 0, 'p' },
            { "max_depth", required_argument, 0, 'm' },
            { "min_length", required_argument, 0, 'n' },
            { 0, 0, 0, 0 }
    };
    while (1) {
        auto c = getopt_long(argc, argv, "hf:p:m:n:", long_options, 0);
        if (c == -1)
            break;
        switch (c) {
            case 'h':
                usage(argv[0]);
                exit(0);
            case 'f':
                file_name = optarg;
                break;
            case 'm':
                max_depth = atoi(optarg);
                break;
            case 'n':
                min_length = atoi(optarg);
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
    return make_tuple(file_name, path, max_depth, min_length);
}

void process_file(string path, string file_name, bool class_last, int max_depth, int min_length)
{
    ArffFiles file;

    file.load(path + file_name + ".arff", class_last);
    auto attributes = file.getAttributes();
    int items = file.getSize();
    cout << "Number of lines: " << items << endl;
    cout << "Attributes: " << endl;
    for (auto attribute : attributes) {
        cout << "Name: " << get<0>(attribute) << " Type: " << get<1>(attribute) << endl;
    }
    cout << "Class name: " << file.getClassName() << endl;
    cout << "Class type: " << file.getClassType() << endl;
    cout << "Data: " << endl;
    vector<samples_t>& X = file.getX();
    labels_t& y = file.getY();
    for (int i = 0; i < 5; i++) {
        for (auto feature : X) {
            cout << fixed << setprecision(1) << feature[i] << " ";
        }
        cout << y[i] << endl;
    }
    mdlp::CPPFImdlp test = mdlp::CPPFImdlp(min_length, max_depth);
    auto total = 0;
    for (auto i = 0; i < attributes.size(); i++) {
        auto min_max = minmax_element(X[i].begin(), X[i].end());
        cout << "Cut points for " << get<0>(attributes[i]) << endl;
        cout << "Min: " << *min_max.first << " Max: " << *min_max.second << endl;
        cout << "--------------------------" << setprecision(3) << endl;
        test.fit(X[i], y);
        for (auto item : test.getCutPoints()) {
            cout << item << endl;
        }
        total += test.getCutPoints().size();
    }
    cout << "Total cut points ...: " << total << endl;
    cout << "Total feature states: " << total + attributes.size() << endl;
}

void process_all_files(map<string, bool> datasets, string path, int max_depth, int min_length)
{
    cout << "Results: " << "Max_depth: " << max_depth << "  Min_length: " << min_length << endl << endl;
    printf("%-20s %4s %4s\n", "Dataset", "Feat", "Cuts Time(s)");
    printf("==================== ==== ==== =======\n");
    for (auto dataset : datasets) {
        ArffFiles file;
        file.load(path + dataset.first + ".arff", dataset.second);
        auto attributes = file.getAttributes();
        vector<samples_t>& X = file.getX();
        labels_t& y = file.getY();
        float timing = 0;
        int cut_points = 0;
        for (auto i = 0; i < attributes.size(); i++) {
            mdlp::CPPFImdlp test = mdlp::CPPFImdlp(min_length, max_depth);
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            test.fit(X[i], y);
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            timing += std::chrono::duration_cast<std::chrono::seconds>(end - begin).count();
            cut_points += test.getCutPoints().size();
        }
        printf("%-20s %4lu %4d %7.2f\n", dataset.first.c_str(), attributes.size(), cut_points, timing);
    }
}


int main(int argc, char** argv)
{
    map<string, bool> datasets = {
            {"glass",              true},
            {"iris",               true},
            {"kdd_JapaneseVowels", false},
            {"letter",             true},
            {"liver-disorders",    true},
            {"mfeat-factors",      true},
            {"test",               true}
    };
    string file_name, path;
    int max_depth, min_length;
    tie(file_name, path, max_depth, min_length) = parse_arguments(argc, argv);
    if (datasets.find(file_name) == datasets.end() && file_name != "all") {
        cout << "Invalid file name: " << file_name << endl;
        usage(argv[0]);
        exit(1);
    }
    if (file_name == "all")
        process_all_files(datasets, path, max_depth, min_length);
    else {
        process_file(path, file_name, datasets[file_name], max_depth, min_length);
        cout << "File name: " << file_name << endl;
        cout << "Max depth: " << max_depth << endl;
        cout << "Min length: " << min_length << endl;
    }
    return 0;
}