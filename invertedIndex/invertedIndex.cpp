#include <bits/stdc++.h>
#include <dirent.h>
#include <pthread.h>
using namespace std;
using namespace chrono;

const size_t BUFFER_SIZE = 1 << 16; // 20 = 1MB

mutex global_mutex;
unordered_map<string, unordered_set<string>> global_index;

struct ThreadData {
    vector<string> files;
};

void to_lowercase(string& word) {
    transform(word.begin(), word.end(), word.begin(), ::tolower);
}

vector<string> tokenize(const string& text, string& carry) {
    vector<string> tokens;
    size_t start = 0, end = 0;
    string chunk = carry + text;
    carry.clear();

    while (end < chunk.size()) {
        while (end < chunk.size() && !isspace(chunk[end])) ++end;
        string word = chunk.substr(start, end - start);
        to_lowercase(word);
        word.erase(remove_if(word.begin(), word.end(), ::ispunct), word.end());
        if (!word.empty()) tokens.push_back(word);

        // saltar espacios en blanco
        while (end < chunk.size() && isspace(chunk[end])) ++end;
        start = end;
    }

    // Si la ultima palabra esta incompleta, guardar
    if (!chunk.empty() && !isspace(chunk.back())) {
        carry = tokens.empty() ? chunk.substr(start) : tokens.back();
        if (!tokens.empty()) tokens.pop_back(); // quitar ultima incompleta
    }

    return tokens;
}

void* process_files(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    unordered_map<string, unordered_set<string>> local_index;
    char buffer[BUFFER_SIZE];

    for (const string& file : data->files) {
        ifstream infile(file, ios::binary);
        if (!infile.is_open()) {
            cerr << "No se pudo abrir " << file << endl;
            continue;
        }

        string carry; // para manejar palabras cortadas
        while (infile.read(buffer, BUFFER_SIZE) || infile.gcount() > 0) {
            size_t bytes_read = infile.gcount();
            string chunk(buffer, bytes_read);
            vector<string> words = tokenize(chunk, carry);

            for (const string& word : words) {
                local_index[word].insert(file);
            }
        }

        if (!carry.empty()) {
            to_lowercase(carry);
            carry.erase(remove_if(carry.begin(), carry.end(), ::ispunct), carry.end());
            local_index[carry].insert(file);
        }

        infile.close();
    }

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

    auto start_time = high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, process_files, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    auto end_time = high_resolution_clock::now();
    duration<double> elapsed = end_time - start_time;
    cout << "Índice invertido creado en " << elapsed.count() << " segundos\n";

    ofstream out("indice_invertido.txt");
    for (const auto& [word, file_set] : global_index) {
        out << word << ": ";
        for (const auto& file : file_set) {
            out << file << " ";
        }
        out << "\n";
    }
    out.close();

    cout << "Índice invertido guardado como 'indice_invertido.txt'.\n";
    return 0;
}

