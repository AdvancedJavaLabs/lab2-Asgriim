#ifndef PAR_LAB2_WORDS_UTIL_HPP
#define PAR_LAB2_WORDS_UTIL_HPP

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <set>

namespace util {

std::pair<size_t, size_t> analyzeSentiment(const std::vector<std::string> &sentences);
std::vector<std::string> splitIntoWords(const std::string &text);
std::vector<std::pair<std::string, size_t>> topNWords(const std::vector<std::string> &sentences, size_t n);
std::vector<std::string> sortSentencesByLength(const std::vector<std::string> &sentences);
std::vector<std::string> replaceNames(const std::vector<std::string> &sentences, const std::string &replacement);
std::vector<std::pair<std::string, size_t>> mergeTopWords(
        const std::vector<std::vector<std::pair<std::string, size_t>>> &all_top_words, size_t n);

std::vector<std::string> mergeAndSortSentences(const std::vector<std::vector<std::string>> &all_sentences);


} // namespace util

#endif //PAR_LAB2_WORDS_UTIL_HPP
