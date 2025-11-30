#ifndef PARL_LAB2_SENTENCE_SPLITTER_HPP
#define PARL_LAB2_SENTENCE_SPLITTER_HPP
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unordered_set>

namespace util {

class SentenceSplitter {
public:
    SentenceSplitter() = default;
    bool readAndSplit(const std::string &filename);
    bool splitSentences(const std::string &text);
    [[nodiscard]] const std::vector<std::string> &getSentences() const;
    void saveToFile(const std::string &filename) const;

private:
    static std::string cleanSentence(const std::string &sentence);
    static bool isAbbreviation(const std::string &text, size_t pos);

    std::vector<std::string> sentences;
};

}
#endif //PARL_LAB2_SENTENCE_SPLITTER_HPP
