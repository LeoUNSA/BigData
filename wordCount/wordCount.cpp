#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <vector>
#include <chrono>
using namespace std;
using namespace chrono;
// Helper function to clean a word (remove punctuation, convert to lowercase)
string clean_word(const string& word) {
    string cleaned;
    cleaned.reserve(word.length());
    
    for (char c : word) {
        if (isalnum(c)) {
            cleaned += tolower(c);
        }
    }
    
    return cleaned;
}

int main(int argc, char* argv[]) {
    // Start timing
    auto start_time = high_resolution_clock::now();
    
    // Check if filename is provided
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <filename>" << endl;
        return 1;
    }
    
    string filename = argv[1];
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Error: " << filename << endl;
        return 1;
    }
    
    // Use unordered_map for O(1) lookups and inserts
    unordered_map<string, size_t> word_counts;
    
    // Reserve space to avoid rehashing (improves performance if we have a rough estimate)
    word_counts.reserve(10000);
    
    string word;
    // Read word by word
    while (file >> word) {
        string cleaned = clean_word(word);
        if (!cleaned.empty()) {
            word_counts[cleaned]++;
        }
    }

    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time);

    // Convert to vector for sorting by frequency
    vector<pair<string, size_t>> sorted_counts(
        word_counts.begin(), word_counts.end()
    );
    
    // Sort by frequency (descending)
    sort(sorted_counts.begin(), sorted_counts.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        }
    );
    
    // Print results
    cout << "Resultados (ordenados por frecuencia):" << endl;
    cout << "Word\t\tCount" << endl;
    cout << "------------------------" << endl;
    
    for (const auto& [word, count] : sorted_counts) {
        cout << word << "\t\t" << count << endl;
    }
    
    // Print total unique words
    cout << "\nNumero de palabras distintas: " << word_counts.size() << endl;
     
    cout << "Tiempo de ejecucion: " << duration.count() << " milisegundos" << endl;
    
    return 0;
}

