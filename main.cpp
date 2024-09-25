//
//  main.cpp
//  Daniel's Project 4
//
//
#include <iostream>
#include <fstream>
#include <list>
#include <algorithm>
#include <iterator>
#include <utility>
#include <functional>
#include <sstream>  // for istringstream and ostringstream
#include <cassert>
#include <filesystem>
#include <string>
 
using namespace std;

bool finalEnd = false;

class HashTable {
public:
    HashTable(size_t size) {
        m_size = size;
        m_items.resize(size);
    }
    
    int bucketNum(const string& sequence) {
        size_t bucket = hasher(sequence) % m_size;
        return static_cast<int>(bucket);
    }
    
    void insert(const string& sequence, size_t offset) {
        m_items[bucketNum(sequence)].emplace_back(sequence, offset);
    }
    
    bool contains(const string& sequence, size_t& offset) {
        int bucket = bucketNum(sequence);
        auto it = m_items[bucket].begin();
        while (it != m_items[bucket].end()) {
            if (it->first == sequence) {
                offset = it->second;
                return true;
            }
            it++;
        }
        return false;
    }
private:
    vector<list<pair<string, int>>> m_items;
    hash<string> hasher;
    size_t m_size;
};

void readFileContent(istream& fileStream, string& fileContent) {
    string line;
    while (getline(fileStream, line)) {
        fileContent += line + "\n";
    }
}

char findDelimiter(const string& sequence) {
    string delimiters = "/:;|@$%^&*()_+=-[]{}<>?";
    for (char delimiter : delimiters) {
        if (sequence.find(delimiter) == string::npos) {
            return delimiter;
        }
    }
    return '/';
}

void createRevisionSmall(string& oldFileContent, string& newFileContent, ostream& frevision){
    HashTable oldFileTable(1007);
    // Populate hash table with substrings
    for (size_t i = 0; i < oldFileContent.size(); ++i) {
        for (size_t len = 1; len <= oldFileContent.size() - i; ++len) {
            string sub = oldFileContent.substr(i, len);
            oldFileTable.insert(sub, i);
        }
    }

    // Read the new file content

    bool lastCharWasNewline = false;


    // Check if the last character was a newline
    if (!newFileContent.empty() && newFileContent.back() == '\n') {
        lastCharWasNewline = true;
    }

    // Adjust the finalEnd flag based on the presence of the newline at the end
    finalEnd = lastCharWasNewline;

    // Generate revision instructions
    int newFileLength = static_cast<int>(newFileContent.length());
    int offset = 0;

    while (offset < newFileLength) {
        int longestMatchLength = 0;
        int longestMatchOffset = -1;

        // Find the longest match
        for (int len = 1; offset + len <= newFileLength; ++len) {
            string sub = newFileContent.substr(offset, len);
            size_t matchOffset;
            if (oldFileTable.contains(sub, matchOffset)) {
                longestMatchLength = len;
                longestMatchOffset = static_cast<int>(matchOffset);
            } else {
                break;
            }
        }

        if (longestMatchLength > 7) {
            frevision << "#" << longestMatchOffset << "," << longestMatchLength;
            offset += longestMatchLength;
        } else {
            string addSequence;
            while (offset < newFileLength) {
                int matchLength = 0;
                size_t matchOffset = -1;

                for (int len = 1; offset + len <= newFileLength; ++len) {
                    string sub = newFileContent.substr(offset, len);
                    size_t currentMatchOffset;
                    if (oldFileTable.contains(sub, currentMatchOffset)) {
                        matchLength = len;
                        matchOffset = currentMatchOffset;
                    } else {
                        break;
                    }
                }

                if (matchLength > 7) {
                    break;
                } else {
                    addSequence += newFileContent[offset];
                    offset++;
                }
            }

            if (!addSequence.empty()) {
                char delimiter = findDelimiter(addSequence);
                if (addSequence.back() == '\n') {
                    addSequence.pop_back();
                }
                if (!addSequence.empty()) {
                    frevision << "+" << delimiter << addSequence << delimiter;
                }
            }
        }
    }
    
}

void createRevisionLarge(string& oldFileContent, string& newFileContent, ostream& frevision){
    
    const size_t numBuckets = oldFileContent.size() + 1; //size of oldfile + 1 for good measure
    HashTable table (numBuckets);
    
    //N is the length of sequences considered for matching during the revision process
    size_t N = 16;

    //insert every N len sequence into the hash
    for ( size_t i = 0; i <= oldFileContent.size() - N; i++) {
        table.insert(oldFileContent.substr(i, N), i);
    }
    
    // Use a stringstream to build the initial revision content
        stringstream tempRevision;

    //process new file content!
    size_t j = 0;
    //loop until newfile is done
    while (j < newFileContent.size()) {
        //make sure not to extend past end
        if(j <= newFileContent.size() - N) {
            string newSeq = newFileContent.substr(j, N);
            //new variable that will hold the offset index of where the matching sequence starts in the old file
            size_t matchStart = 0;
            if(table.contains(newSeq, matchStart)){
                size_t matchLength = N;
                
                //extend matchlength if true
                while(j + matchLength < newFileContent.size() && matchStart + matchLength < oldFileContent.size() &&
                      newFileContent[j + matchLength] == oldFileContent[matchStart + matchLength] /*|| oldFileContent[matchStart + matchLength] == '\n')*/) {
                    ++matchLength;
                }
                tempRevision << "#" << matchStart << "," << matchLength;
                j += matchLength;
            } else {
                // Start of addition sequence
                size_t addStart = j;
                                
                // Find the best delimiter
                char delimiter = findDelimiter(string(1, newFileContent[j]));
                                
                // Loop to find characters not in old sequence and add them to combinedCharacters
                string combinedCharacters;
                size_t lookForCharNotPlus = addStart;
                                
                while (lookForCharNotPlus < newFileContent.size() &&
                !table.contains(newFileContent.substr(lookForCharNotPlus, N), matchStart)) {
                        combinedCharacters += newFileContent[lookForCharNotPlus++];
                    }
                j = lookForCharNotPlus; // Move j to where lookForCharNotPlus left off
                tempRevision << "+" << delimiter << combinedCharacters << delimiter;
            }
        }
        else {
            char delimiter = findDelimiter(std::string(1, newFileContent[j]));
            tempRevision << "+" << delimiter << newFileContent[j] << delimiter;
            j++;
        }
    }

    
    frevision << tempRevision.str();
}

void createRevision(istream& fold, istream& fnew, ostream& frevision) {
    //string oldFileContent;
    //string line;

    // Read old file and populate hash table
    //readFileContent(fold, oldFileContent);
    string oldFileContent ((istreambuf_iterator<char>(fold)), istreambuf_iterator<char>());
    string newFileContent ((istreambuf_iterator<char>(fnew)), istreambuf_iterator<char>());
    
    
    if (oldFileContent.size() < 4200){
        createRevisionSmall(oldFileContent, newFileContent, frevision);
    } else{
        createRevisionLarge(oldFileContent, newFileContent, frevision);
    }

}

bool revise(istream& fold, istream& frevision, ostream& fnew) {
    string old_file_string = "";
    string string_to_copy;
    char ch;

    // Read the old file into a string
    while (getline(fold, string_to_copy)) {
        old_file_string += string_to_copy + "\n";
    }
    
    stringstream newFileContent;

    istringstream oldFileStream(old_file_string);

    while (frevision.get(ch)) {
        string instruction;
        if (ch == '+') {
            while (frevision.get(ch)) {
                if (ch != '+') {
                    char delimiter = ch;
                    getline(frevision, instruction, delimiter);
                    cout << instruction;
                    newFileContent << instruction;
                    break;
                }
            }
        } else if (ch == '#') {
            while (frevision.get(ch)) {
                if (ch != '+' && ch != '#') {
                    instruction += ch;
                } else {
                    frevision.unget();
                    break;
                }
            }
            size_t commaPos = instruction.find(',');
            if (commaPos == string::npos) {
                cout << instruction;
                newFileContent << instruction;
            } else {
                int offset = stoi(instruction.substr(0));
                int length = stoi(instruction.substr(commaPos + 1));
                if (offset < 0 || offset + length > old_file_string.size()) {
                    return false; // Invalid range
                }
                cout << old_file_string.substr(offset, length);
                newFileContent << old_file_string.substr(offset, length);
            }
        }
    }

    string newContent = newFileContent.str();
    for (size_t i = 0; i < newContent.size(); i++) {
        if (newContent[i] != '\0') {
            fnew << newContent[i];
        }
    }
    
    // Only add a new line if finalEnd is true and the new content doesn't already end with a newline
    if (finalEnd && !newContent.empty() && newContent.back() != '\n') {
        fnew << '\n';
    }
    
    return true;
}

void runtest(string oldtext, string newtext)
{
    istringstream oldFile(oldtext);
    istringstream newFile(newtext);
    ostringstream revisionFile;
    createRevision(oldFile, newFile, revisionFile);
    string result = revisionFile.str();
    cout << "The revision file length is " << result.size()
         << " and its text is " << endl;
    cout << result << endl;


    oldFile.clear();   // clear the end of file condition
    oldFile.seekg(0);  // reset back to beginning of the stream
    istringstream revisionFile2(result);
    ostringstream newFile2;
    assert(revise(oldFile, revisionFile2, newFile2));
    cout << newtext << endl;
    cout << newFile2.str() << endl;
    assert(newtext == newFile2.str());
}



int main() {
    // Paths to the files
//        std::string oldFilePath = "/Users/danielluan/Desktop/p4test/strange1.txt";
//        std::string newFilePath = "/Users/danielluan/Desktop/p4test/strange2.txt";
//        std::string revisionPath = "/Users/danielluan/Desktop/p4test/strangeRevision.txt";
//        std::string revisedPath = "/Users/danielluan/Desktop/p4test/newStrange.txt";
        
//        std::string oldFilePath = "/Users/danielluan/Desktop/p4test/warandpeace1.txt";
//        std::string newFilePath = "/Users/danielluan/Desktop/p4test/warandpeace2.txt";
//        std::string revisionPath = "/Users/danielluan/Desktop/warandpeaceRevision.txt";
//        std::string revisedPath = "/Users/danielluan/Desktop/newWarandpeace.txt";
//        
        std::string oldFilePath = "/Users/danielluan/Desktop/greeneggs1.txt";
        std::string newFilePath = "/Users/danielluan/Desktop/greeneggs2.txt";
        std::string revisionPath = "/Users/danielluan/Desktop/greeneggsRevision.txt";
        std::string revisedPath = "/Users/danielluan/Desktop/newGreeneggs.txt";
//
//        std::string oldFilePath = "/Users/danielluan/Desktop/p4test/mallmart1.txt";
//        std::string newFilePath = "/Users/danielluan/Desktop/p4test/mallmart2.txt";
//        std::string revisionPath = "/Users/danielluan/Desktop/mallmartRevision.txt";
//        std::string revisedPath = "/Users/danielluan/Desktop/newMallmart.txt";
    
//            std::string oldFilePath = "/Users/danielluan/Desktop/p4test/large1.txt";
//            std::string newFilePath = "/Users/danielluan/Desktop/p4test/large2.txt";
//            std::string revisionPath = "/Users/danielluan/Desktop/largeRevision.txt";
//            std::string revisedPath = "/Users/danielluan/Desktop/newLarge.txt";
////
////    // Create input file streams
    std::ifstream fold(oldFilePath);
    std::ifstream fnew(newFilePath);
    std::ofstream ofrevision(revisionPath);

    if (!fold || !fnew || !ofrevision) {
        std::cerr << "Error opening input or output file!" << std::endl;
        return 1;
    }

    // Create the revision file
    createRevision(fold, fnew, ofrevision);

    // Close the streams
    fold.close();
    fnew.close();
    ofrevision.close();

    // Re-open the necessary streams for revising the file
    fold.open(oldFilePath);
    std::ifstream ifrevision(revisionPath);
    std::ofstream revised(revisedPath);

    if (!fold || !ifrevision || !revised) {
        std::cerr << "Error reopening input or output file!" << std::endl;
        return 1;
    }

    // Apply the revision to create the new file
    if (!revise(fold, ifrevision, revised)) {
        std::cerr << "Failed to apply revision." << std::endl;
        return 1;
    }

    revise(fold, ifrevision, revised);

    // Close the streams
    fold.close();
    ifrevision.close();
    revised.close();
//
    return 0;
//    runtest("There's a bathroom on the right.",
//            "There's a bad moon on the rise.");
//    runtest("ABCDEFGHIJBLAHPQRSTUVPQRSTUV",
//            "XYABCDEFGHIJBLETCHPQRSTUVPQRSTQQ/OK");
//    cout << "All tests passed" << endl;

}
//+!66284,Screwdriver,1000,!#0,23+!5!#24,27+!490,Bedspread,87!#75,29+!,40411,Hair Spray,380!
//+/66284,Screwdriver,1000,/#0,23+/5/#24,27+/490,Bedspread,87/#75,29+/,40411,Hair Spray,380/

