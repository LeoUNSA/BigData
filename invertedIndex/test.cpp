#include <bits/stdc++.h>
namespace fs = std::filesystem;
using namespace std;
// Estructura que define un documento
struct Document {
    string path;
    size_t id;
};

// Estructura que almacena las apariciones de un término en un documento
struct Posting {
    size_t docId;
    size_t frequency;
    vector<size_t> positions;
};

// Tipo para el índice invertido parcial de cada hilo
using PartialIndex = unordered_map<string, vector<Posting>>;

// Tipo para el índice invertido global
using InvertedIndex = unordered_map<string, vector<Posting>>;

// Mutex para proteger el acceso al índice global
mutex indexMutex;

// Lista global de documentos
vector<Document> documents;
mutex documentsMutex;

// Contador global para asignar IDs a documentos
atomic<size_t> nextDocId(0);

// Argumentos para la función de los hilos trabajadores
struct ThreadArgs {
    vector<string> filePaths;
    PartialIndex* partialIndex;
    size_t threadId;
};

// Función para normalizar un término (convertir a minúsculas y eliminar signos de puntuación)
string normalizeToken(const string& token) {
    string normalized;
    for (char c : token) {
        if (isalnum(c)) {
            normalized += tolower(c);
        }
    }
    return normalized;
}

// Función para tokenizar un texto en palabras
vector<string> tokenize(const string& text) {
    vector<string> tokens;
    istringstream iss(text);
    string token;
    
    while (iss >> token) {
        token = normalizeToken(token);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

// Función para procesar un archivo y actualizar el índice parcial
void processFile(const string& filePath, PartialIndex& partialIndex) {
    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Error al abrir archivo: " << filePath << endl;
        return;
    }
    
    // Registrar documento y obtener ID
    Document doc;
    doc.path = filePath;
    doc.id = nextDocId.fetch_add(1);
    
    {
        lock_guard<mutex> lock(documentsMutex);
        documents.push_back(doc);
    }
    
    // Leer archivo y construir índice
    string line;
    size_t position = 0;
    
    // Mapa temporal para este documento
    unordered_map<string, Posting> docPostings;
    
    while (getline(file, line)) {
        vector<string> tokens = tokenize(line);
        
        for (const auto& token : tokens) {
            if (token.empty()) continue;
            
            // Si es la primera aparición de este término en el documento, crear nueva posting
            if (docPostings.find(token) == docPostings.end()) {
                docPostings[token] = Posting{doc.id, 1, {position}};
            } else {
                // Incrementar frecuencia y añadir posición
                docPostings[token].frequency++;
                docPostings[token].positions.push_back(position);
            }
            
            position++;
        }
    }
    
    // Actualizar el índice parcial con las apariciones en este documento
    for (const auto& [term, posting] : docPostings) {
        partialIndex[term].push_back(posting);
    }
    
    file.close();
}

// Función para el hilo trabajador
void* workerThread(void* args) {
    ThreadArgs* threadArgs = static_cast<ThreadArgs*>(args);
    PartialIndex& partialIndex = *threadArgs->partialIndex;
    
    for (const auto& filePath : threadArgs->filePaths) {
        processFile(filePath, partialIndex);
    }
    
    return nullptr;
}

// Función para obtener todos los archivos en un directorio y subdirectorios
void getFilesRecursively(const string& directory, vector<string>& files) {
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (fs::is_regular_file(entry) && entry.path().extension() == ".txt") {
            files.push_back(entry.path().string());
        }
    }
}

// Función para unir índices parciales en el índice global
void mergePartialIndices(const vector<PartialIndex>& partialIndices, InvertedIndex& globalIndex) {
    for (const auto& partialIndex : partialIndices) {
        for (const auto& [term, postings] : partialIndex) {
            // Añadir postings al índice global
            for (const auto& posting : postings) {
                globalIndex[term].push_back(posting);
            }
        }
    }
    
    // Opcional: Ordenar las postings por docId para cada término
    for (auto& [term, postings] : globalIndex) {
        sort(postings.begin(), postings.end(), 
                 [](const Posting& a, const Posting& b) { return a.docId < b.docId; });
    }
}

// Función para guardar el índice invertido en un archivo
void saveInvertedIndex(const InvertedIndex& index, const string& outputFile) {
    ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        cerr << "Error al crear archivo de salida: " << outputFile << endl;
        return;
    }
    
    for (const auto& [term, postings] : index) {
        outFile << term << " " << postings.size() << " ";
        for (const auto& posting : postings) {
            outFile << posting.docId << " " << posting.frequency << " ";
            for (size_t pos : posting.positions) {
                outFile << pos << " ";
            }
            outFile << "| ";
        }
        outFile << endl;
    }
    
    outFile.close();
}

// Función para guardar el mapeo de docId a ruta del archivo
void saveDocumentMapping(const vector<Document>& docs, const string& outputFile) {
    ofstream outFile(outputFile);
    if (!outFile.is_open()) {
        cerr << "Error al crear archivo de mapeo: " << outputFile << endl;
        return;
    }
    
    for (const auto& doc : docs) {
        outFile << doc.id << " " << doc.path << endl;
    }
    
    outFile.close();
}

// Función para buscar términos en el índice
void searchIndex(const InvertedIndex& index, const vector<Document>& docs, const string& query) {
    vector<string> queryTerms = tokenize(query);
    
    // Documentos que contienen todos los términos de la consulta
    set<size_t> resultDocs;
    bool firstTerm = true;
    
    for (const auto& term : queryTerms) {
        if (index.find(term) == index.end()) {
            // Si algún término no está en el índice, no hay resultados
            resultDocs.clear();
            break;
        }
        
        // Conjunto de documentos que contienen este término
        set<size_t> termDocs;
        for (const auto& posting : index.at(term)) {
            termDocs.insert(posting.docId);
        }
        
        if (firstTerm) {
            resultDocs = termDocs;
            firstTerm = false;
        } else {
            // Intersección con términos anteriores
            set<size_t> intersection;
            set_intersection(
                resultDocs.begin(), resultDocs.end(),
                termDocs.begin(), termDocs.end(),
                inserter(intersection, intersection.begin())
            );
            resultDocs = intersection;
        }
    }
    
    // Mostrar resultados
    cout << "Resultados para: " << query << endl;
    cout << "Documentos encontrados: " << resultDocs.size() << endl;
    
    for (size_t docId : resultDocs) {
        for (const auto& doc : docs) {
            if (doc.id == docId) {
                cout << "- " << doc.path << endl;
                break;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso: " << argv[0] << " <directorio_datos> <num_hilos>" << endl;
        return 1;
    }
    
    string dataDirectory = argv[1];
    int numThreads = stoi(argv[2]);
    
    auto startTime = chrono::high_resolution_clock::now();
    
    // Obtener todos los archivos de texto en el directorio
    vector<string> allFiles;
    getFilesRecursively(dataDirectory, allFiles);
    
    cout << "Encontrados " << allFiles.size() << " archivos para indexar." << endl;
    
    // Ajustar el número de hilos si hay menos archivos que hilos
    numThreads = min(numThreads, static_cast<int>(allFiles.size()));
    
    // Distribuir archivos entre hilos
    vector<ThreadArgs> threadArgs(numThreads);
    vector<PartialIndex> partialIndices(numThreads);
    
    size_t filesPerThread = allFiles.size() / numThreads;
    size_t remainingFiles = allFiles.size() % numThreads;
    
    size_t fileIndex = 0;
    for (int i = 0; i < numThreads; i++) {
        size_t numFilesForThisThread = filesPerThread + (i < remainingFiles ? 1 : 0);
        
        threadArgs[i].threadId = i;
        threadArgs[i].partialIndex = &partialIndices[i];
        
        // Asignar archivos a este hilo
        for (size_t j = 0; j < numFilesForThisThread; j++) {
            if (fileIndex < allFiles.size()) {
                threadArgs[i].filePaths.push_back(allFiles[fileIndex++]);
            }
        }
    }
    
    // Crear los hilos
    vector<pthread_t> threads(numThreads);
    for (int i = 0; i < numThreads; i++) {
        int rc = pthread_create(&threads[i], nullptr, workerThread, &threadArgs[i]);
        if (rc) {
            cerr << "Error al crear hilo: " << rc << endl;
            return 1;
        }
    }
    
    // Esperar a que todos los hilos terminen
    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], nullptr);
    }
    
    // Unir índices parciales en el índice global
    InvertedIndex globalIndex;
    mergePartialIndices(partialIndices, globalIndex);
    
    auto endTime = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = endTime - startTime;
    
    cout << "Indexación completada en " << elapsed.count() << " segundos." << endl;
    cout << "Términos únicos: " << globalIndex.size() << endl;
    cout << "Documentos procesados: " << documents.size() << endl;
    
    // Guardar el índice y mapeo de documentos
    saveInvertedIndex(globalIndex, "inverted_index.idx");
    saveDocumentMapping(documents, "document_mapping.txt");
    
    cout << "Índice guardado en inverted_index.idx" << endl;
    cout << "Mapeo de documentos guardado en document_mapping.txt" << endl;
    
    // Modo interactivo de búsqueda (opcional)
    string query;
    cout << "Ingrese una consulta (o 'salir' para terminar): ";
    getline(cin, query);
    
    while (query != "salir") {
        searchIndex(globalIndex, documents, query);
        cout << "\nIngrese una consulta (o 'salir' para terminar): ";
        getline(cin, query);
    }
    
    return 0;
}
