#include <bits/stdc++.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
using namespace std;
using namespace chrono;

mutex global_mutex;
unordered_map<string, unordered_set<string>> global_index;

struct ThreadData {
    vector<string> files;
};

void to_lowercase(string& word) {
    for (char& c : word) c = tolower(c);
}

vector<string> tokenize(const string& line) {
    vector<string> tokens;
    string word;
    for (char c : line) {
        if (isalnum(c)) {
            word += tolower(c);
        } else if (!word.empty()) {
            tokens.push_back(word);
            word.clear();
        }
    }
    if (!word.empty()) tokens.push_back(word);
    return tokens;
}

void* process_files(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    unordered_map<string, unordered_set<string>> local_index;
    local_index.reserve(100000);  // Ajusta si tienes una idea del número de palabras

    for (const string& file : data->files) {
        ifstream infile(file);
        if (!infile.is_open()) {
            cerr << "No se pudo abrir: " << file << endl;
            continue;
        }

        string line;
        while (getline(infile, line)) {
            vector<string> words = tokenize(line);
            for (const string& word : words) {
                if (!word.empty()) {
                    local_index[word].insert(file);
                }
            }
        }
        infile.close();
    }

    // Unir resultados al índice global (una sola vez al final del hilo)
    lock_guard<mutex> lock(global_mutex);
    for (const auto& [word, files] : local_index) {
        global_index[word].insert(files.begin(), files.end());
    }

    return nullptr;
}

vector<string> list_text_files(const string& dir_path) {
    vector<string> files;
    DIR* dir = opendir(dir_path.c_str());
    if (!dir) {
        cerr << "No se puede abrir el directorio: " << dir_path << endl;
        return files;
    }

    dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        string filename = entry->d_name;
        if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".txt") {
            files.push_back(dir_path + "/" + filename);
        }
    }
    closedir(dir);
    sort(files.begin(), files.end());  // Opcional
    return files;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso: " << argv[0] << " <directorio> <num_hilos>" << endl;
        return 1;
    }

    string dir_path = argv[1];
    int num_threads = stoi(argv[2]);

    vector<string> files = list_text_files(dir_path);
    if (files.empty()) {
        cerr << "No se encontraron archivos .txt" << endl;
        return 1;
    }

    vector<pthread_t> threads(num_threads);
    vector<ThreadData> thread_data(num_threads);

    // Distribuir archivos equitativamente
    for (size_t i = 0; i < files.size(); ++i) {
        thread_data[i % num_threads].files.push_back(files[i]);
    }

    auto start = high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, process_files, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    auto end = high_resolution_clock::now();
    duration<double> elapsed = end - start;
    cout << "Indice invertido creado en " << elapsed.count() << " segundos.\n";

    ofstream out("indice_invertido.txt");
    for (const auto& [word, file_set] : global_index) {
        out << word << ": ";
        for (const string& file : file_set) {
            out << file << " ";
        }
        out << "\n";
    }
    out.close();

    cout << "Indice guardado en 'indice_invertido.txt'.\n";
    return 0;
}
