#include "ArffFiles.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include "../CPPFImdlp.h"

using namespace std;
using namespace mdlp;

tuple<precision_t, size_t> getCutPoint(samples_t& X, labels_t& y, size_t start, size_t cut, size_t end)
{
    size_t idxPrev = cut - 1;
    precision_t previous, next, actual;
    previous = X[idxPrev];
    next = actual = X[cut];
    // definition 2 of the paper => X[t-1] < X[t]
    while (idxPrev-- > start && actual == previous) {
        previous = X[idxPrev];
    }
    // get the last equal value of X in the interval
    while (actual == X[cut++] && cut < end);
    if (previous == actual && cut < end)
        actual = X[cut];
    cut--;
    return make_tuple((previous + actual) / 2, cut);
}

void show_points(samples_t& X, labels_t& y, size_t start, size_t end)
{
    cout << "Interval: " << start << " - " << end << endl;
    tuple<precision_t, size_t> cutPoint;
    size_t cut = start + 1;
    if (start >= end) {
        return;
    }
    while (y[cut - 1] == y[cut] && cut < end)
        cut++;
    if (cut != end) {
        cutPoint = getCutPoint(X, y, start, cut, end);
        cout << cut << ": " << fixed << setprecision(1) << X[cut] << " " << y[cut] << endl;
        cout << "Cut point: " << get<0>(cutPoint) << " at " << get<1>(cutPoint) << endl;
        show_points(X, y, start, get<1>(cutPoint));
        show_points(X, y, get<1>(cutPoint), end);
    }

}

int main(int argc, char** argv)
{
    ArffFiles file;
    vector<string> lines;
    string path = "../tests/";
    map<string, bool > datasets = {
        {"01", true},
        {"02", true},
        {"03", true},
        {"04", true}
    };
    if (argc != 2 || datasets.find(argv[1]) == datasets.end()) {
        cout << "Usage: " << argv[0] << " {01, 02, 03, 04}" << endl;
        return 1;
    }

    file.load(path + argv[1] + ".arff", datasets[argv[1]]);
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
    for (int i = 0; i < y.size(); i++) {
        for (auto feature : X) {
            cout << i << ": " << fixed << setprecision(1) << feature[i] << " ";
        }
        cout << y[i] << endl;
    }
    mdlp::CPPFImdlp test = mdlp::CPPFImdlp(0);
    for (auto i = 0; i < attributes.size(); i++) {
        cout << "Cut points for " << get<0>(attributes[i]) << endl;
        cout << "--------------------------" << setprecision(3) << endl;
        test.fit(X[i], y);
        for (auto item : test.getCutPoints()) {
            cout << item << endl;
        }
    }
    cout << "Function test" << endl;
    show_points(X[0], y, 0, items);
    return 0;
}
