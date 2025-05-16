// inverted_index.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <dirent.h>
#include <pthread.h>
#include <mutex>
#include <algorithm>

using namespace std;

mutex global_mutex;
unordered_map<string, unordered_set<string>> global_index;

struct ThreadData {
    vector<string> files;
};

void to_lowercase(string& word) {
    transform(word.begin(), word.end(), word.begin(), ::tolower);
}

vector<string> tokenize(const string& line) {
    vector<string> tokens;
    istringstream iss(line);
    string word;
    while (iss >> word) {
        to_lowercase(word);
        // Opcional: eliminar signos de puntuación
        word.erase(remove_if(word.begin(), word.end(), ::ispunct), word.end());
        tokens.push_back(word);
    }
    return tokens;
}

void* process_files(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    unordered_map<string, unordered_set<string>> local_index;

    for (const auto& file : data->files) {
        ifstream infile(file);
        if (!infile.is_open()) {
            cerr << "No se pudo abrir " << file << endl;
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

    // Combinar con índice global
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
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".txt") {
            files.push_back(dir_path + "/" + filename);
        }
    }
    closedir(dir);
    return files;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso: " << argv[0] << " <directorio_de_textos> <num_hilos>" << endl;
        return 1;
    }

    string dir_path = argv[1];
    int num_threads = stoi(argv[2]);

    vector<string> files = list_text_files(dir_path);
    if (files.empty()) {
        cerr << "No hay archivos .txt en el directorio." << endl;
        return 1;
    }

    vector<pthread_t> threads(num_threads);
    vector<ThreadData> thread_data(num_threads);

    for (size_t i = 0; i < files.size(); ++i) {
        thread_data[i % num_threads].files.push_back(files[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, process_files, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Guardar índice invertido en archivo
    ofstream out("indice_invertido.txt");
    for (const auto& [word, file_set] : global_index) {
        out << word << ": ";
        for (const auto& file : file_set) {
            out << file << " ";
        }
        out << "\n";
    }
    out.close();

    cout << "Índice invertido creado con éxito.\n";
    return 0;
}

