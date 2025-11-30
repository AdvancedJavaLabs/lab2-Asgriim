#include "sentence_splitter.hpp"

namespace util {
namespace {
std::unordered_set<std::string> abbreviations{
        "т.д.",  "т.п.",  "т.е.",  "т.к.", "т.н.",     "т.о.",  "пр.",   "др.",   "г.",    "см.",
        "стр.",  "рис.",  "ул.",   "пер.", "д.",       "кв.",   " Mr.",  " Mrs.", " Dr.",  " Prof.",
        " etc.", " i.e.", " e.g.", " vs.", " approx.", " max.", " min.", " no.",  " vol.", " fig."};
}
std::string SentenceSplitter::cleanSentence(const std::string &sentence)
{
    std::string cleaned;
    bool lastWasSpace = false;
    bool hasContent = false;

    for (char c : sentence) {
        if (std::isspace(c)) {
            if (!lastWasSpace && hasContent) {
                cleaned += ' ';
                lastWasSpace = true;
            }
        } else {
            cleaned += c;
            lastWasSpace = false;
            hasContent = true;
        }
    }

    size_t start = cleaned.find_first_not_of(" \t\n\r");
    size_t end = cleaned.find_last_not_of(" \t\n\r");

    if (start == std::string::npos) {
        return "";
    }

    return cleaned.substr(start, end - start + 1);
}
bool SentenceSplitter::isAbbreviation(const std::string &text, size_t pos)
{
    size_t start = pos;
    while (start > 0 && !std::isspace(text[start - 1]) && text[start - 1] != '.') {
        --start;
    }

    std::string potentialAbbr = text.substr(start, pos - start + 1);

    if (abbreviations.find(potentialAbbr) != abbreviations.end()) {
        return true;
    }

    if (potentialAbbr.length() == 2 && std::isalpha(potentialAbbr[0]) && potentialAbbr[1] == '.') {
        return true;
    }

    return false;
}
bool SentenceSplitter::readAndSplit(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("SentenceSplitter::readAndSplit: Could not open " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    return splitSentences(content);
}
bool SentenceSplitter::splitSentences(const std::string &text)
{
    sentences.clear();
    std::string currentSentence;

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        currentSentence += c;

        if (c == '.' || c == '!' || c == '?') {
            bool isSentenceEnd = true;

            if (i + 1 < text.length()) {
                char nextChar = text[i + 1];

                if (c == '.' && isAbbreviation(text, i)) {
                    isSentenceEnd = false;
                }
                else if (c == '.' && i + 2 < text.length() && text[i + 1] == '.' && text[i + 2] == '.') {
                    isSentenceEnd = false;
                    currentSentence += text[++i]; // Вторая точка
                    currentSentence += text[++i]; // Третья точка
                }
                else if (nextChar == '\"' || nextChar == '\'' || nextChar == '»' || nextChar == '”') {
                    currentSentence += nextChar;
                    i++;
                }
                else if (std::islower(nextChar)) {
                    isSentenceEnd = false;
                }
            }

            if (isSentenceEnd) {
                std::string cleaned = cleanSentence(currentSentence);
                if (!cleaned.empty()) {
                    sentences.push_back(cleaned);
                }
                currentSentence.clear();
            }
        }
    }

    // Последнее предложение
    if (!currentSentence.empty()) {
        std::string cleaned = cleanSentence(currentSentence);
        if (!cleaned.empty()) {
            sentences.push_back(cleaned);
        }
    }

    return true;
}
const std::vector<std::string> &SentenceSplitter::getSentences() const
{
    return sentences;
}
void SentenceSplitter::saveToFile(const std::string &filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("SentenceSplitter::saveToFile: Could not open " + filename);
    }

    for (size_t i = 0; i < sentences.size(); ++i) {
        file << "[" << (i + 1) << "] " << sentences[i] << "\n";
        if (i < sentences.size() - 1) {
            file << "---\n";
        }
    }

    file.close();
}
}