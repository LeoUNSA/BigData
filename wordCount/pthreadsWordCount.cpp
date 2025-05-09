#include <bits/stdc++.h>
using namespace std;
using namespace chrono;
//  Sanitize - Lowercase/Pucntuation
inline string clean_word(const string& word) {
    string cleaned;
    cleaned.reserve(word.length());
    
    for (char c : word) {
        if (isalnum(c)) {
            cleaned += tolower(c);
        }
    }
    
    return cleaned;
}
// Function to count words in a specific chunk of a file
void count_words_in_chunk(
    const string& filename,
    streampos start_pos,
    streampos end_pos,
    unordered_map<string, size_t>& local_counts,
    mutex& map_mutex
) {
    ifstream file(filename, ios::in);
    if (!file) {
        cerr << "Thread could not open file: " << filename << endl;
        return;
    }

    // Start file pointer
    file.seekg(start_pos);

    // Manage partial words if not at the beginning
    if (start_pos > 0) {
        string partial_word;
        file >> partial_word;
    }

    // Read and count words
    string word;
    unordered_map<string, size_t> thread_local_counts;
    thread_local_counts.reserve(1000); // Pre-allocate space

    while (file >> word && file.tellg() <= end_pos) {
        string cleaned = clean_word(word);
        if (!cleaned.empty()) {
            thread_local_counts[cleaned]++;
        }
    }

    // If we're not at the end of the file, read the last partial word
    if (file && file.tellg() < filesystem::file_size(filename)) {
        file >> word;
        string cleaned = clean_word(word);
        if (!cleaned.empty()) {
            thread_local_counts[cleaned]++;
        }
    }

    // Merge local / Shared w mutex
    {
        lock_guard<mutex> lock(map_mutex);
        for (const auto& [word, count] : thread_local_counts) {
            local_counts[word] += count;
        }
    }
}

int main(int argc, char* argv[]) {
    auto start_time = high_resolution_clock::now(); 
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <filename> [num_threads]" << endl;
        return 1;
    }
    
    string filename = argv[1];
    
    // Determine thread number
    unsigned int num_threads = thread::hardware_concurrency();
    if (argc >= 3) {
        try {
            num_threads = stoi(argv[2]);
            if (num_threads == 0) num_threads = 1;
        } catch (...) {
            cerr << "Not enough threads, using default: " << num_threads << endl;
        }
    }
    
    // Get file size
    filesystem::path file_path(filename);
    if (!filesystem::exists(file_path)) {
        cerr << "Error: " << filename << endl;
        return 1;
    }
    
    uintmax_t file_size = filesystem::file_size(file_path);
    cout << "Procesando archivo: " << filename << " (" << file_size << " bytes)" << endl;
    cout << "Usando " << num_threads << " threads" << endl;
    
    // Chunk size
    streampos chunk_size = file_size / num_threads;
    
    // Shared ds
    unordered_map<string, size_t> word_counts;
    word_counts.reserve(10000);
    mutex map_mutex;
    
    // Vector to hold all thread futures
    vector<future<void>> futures;
    
    // Creating threads
    for (unsigned int i = 0; i < num_threads; ++i) {
        streampos start_pos = i * chunk_size;
        streampos end_pos = (i == num_threads - 1) ? streampos(file_size) : start_pos + chunk_size;
        
        futures.push_back(
            async(launch::async, 
                       count_words_in_chunk, 
                       filename, 
                       start_pos, 
                       end_pos, 
                       ref(word_counts), 
                       ref(map_mutex))
        );
    }
    
    for (auto& future : futures) {
        future.wait();
    }
     
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time);
    // ToVector
    vector<pair<string, size_t>> sorted_counts(
        word_counts.begin(), word_counts.end()
    );
    
    // Sort
    sort(sorted_counts.begin(), sorted_counts.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second || (a.second == b.second && a.first < b.first);
        }
    );
    
    cout << "Resultados:" << endl;
    cout << "Word\t\tCount" << endl;
    cout << "------------------------" << endl; 
    for (const auto& [word, count] : sorted_counts) {
        cout << word << "\t\t" << count << endl;
    }
    cout << "\nTotal de palabras distintas: " << word_counts.size() << endl;    
    cout << "Tiempo: " << duration.count() << " milisegundos" << endl; 
    return 0;
}
