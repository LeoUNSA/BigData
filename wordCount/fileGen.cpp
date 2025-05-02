#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <chrono>
#include <sstream>
using namespace std;
using namespace chrono;
vector<string> readWords(const string& csvFilename) {
    vector<string> words;
    ifstream file(csvFilename);
    
    if (!file.is_open()) {
        cerr << "Error opening the file: " << csvFilename << endl;
        return {"the", "be", "to", "of", "and", "a", "in", "that", "have", "it", "for"};
    } 
    string line;
    while (getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);    
        if (!line.empty()) {
            words.push_back(line);
        }
    } 
    file.close(); 
    if (words.empty()) {
        cerr << "Empty csv file." << endl;
        return {"the", "be", "to", "of", "and", "a", "in", "that", "have", "it", "for"};
    } 
    return words;
}

void generateRandomText(const string& filename, size_t targetSizeGB, const vector<string>& wordList) {
    const size_t GB = 1024 * 1024 * 1024;
    const size_t targetSizeBytes = targetSizeGB * GB;
    // Output
    ofstream outFile(filename, ios::binary);
    if (!outFile) {
        cerr << "Error opening output file: " << filename << endl;
        return;
    }
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> wordDist(0, wordList.size() - 1);
    size_t bytesWritten = 0;
    size_t lastReportedPercent = 0;
    size_t reportInterval = GB / 10;
    auto startTime = high_resolution_clock::now();
    const size_t bufferSize = 8192; // Buffer size for writing
    string buffer;
    buffer.reserve(bufferSize * 2);
    
    while (bytesWritten < targetSizeBytes) {
        // Fill buffer with random words
        buffer.clear();
        while (buffer.size() < bufferSize && bytesWritten + buffer.size() < targetSizeBytes) {
            string word = wordList[wordDist(gen)];
            buffer += word + " "; 
            // Occasionally add line breaks
            if (gen() % 15 == 0) {
                buffer += "\n";
            }
        }
        
        // Write buffer to file
        outFile.write(buffer.data(), buffer.size());
        bytesWritten += buffer.size(); 
        // Report progress
        if (bytesWritten / reportInterval > lastReportedPercent) {
            lastReportedPercent = bytesWritten / reportInterval;
            auto currentTime = high_resolution_clock::now();
            auto elapsed = duration_cast<seconds>(currentTime - startTime).count(); 
            double percentComplete = (static_cast<double>(bytesWritten) / targetSizeBytes) * 100.0;
            double mbWritten = bytesWritten / (1024.0 * 1024.0);
            double mbPerSecond = elapsed > 0 ? mbWritten / elapsed : 0;
            cout << "\rProgress: " << percentComplete << "% (" << mbWritten << " MB, " 
                      << mbPerSecond << " MB/s)";
            cout.flush();
        }
    } 
    outFile.close();
    cout << "\nFile generation complete: " << filename << " (" << bytesWritten << " bytes)" << endl;
}
int main(int argc, char* argv[]) {
    string outputFilename = "random_words.txt";
    string wordListFilename = "words.csv";
    size_t sizeGB = 20; 
    // Parse command line arguments
    if (argc >= 2) {
        sizeGB = stoi(argv[1]); if (sizeGB > 9) {
            cout << "Warning: Limiting size to 9 GB" << endl;
            sizeGB = 9;
        }
    } 
    if (argc >= 3) {
        outputFilename = argv[2];
    } 
    if (argc >= 4) {
        wordListFilename = argv[3];
    } 
    // Read words from CSV file
    vector<string> wordList = readWords(wordListFilename); 
    cout << "Generating " << sizeGB << " GB of random words to " << outputFilename << endl; 
    // Record start time
    auto startTime = high_resolution_clock::now(); 
    // Generate the file
    generateRandomText(outputFilename, sizeGB, wordList); 
    // Calculate and display elapsed time
    auto endTime = high_resolution_clock::now();
    auto elapsed = duration_cast<seconds>(endTime - startTime).count();
    cout << "Time taken: " << elapsed << " seconds" << endl; 
    return 0;
}
