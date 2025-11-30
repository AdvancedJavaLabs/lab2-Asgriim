#include "words_util.hpp"
#include <regex>
#include <algorithm>

namespace util {
namespace {
const std::set<std::string> POSITIVE_WORDS = {
        "good", "great", "excellent", "amazing", "wonderful", "fantastic",   "awesome", "happy",     "joy",
        "love", "nice",  "beautiful", "perfect", "brilliant", "outstanding", "superb",  "marvelous", "delightful"};

const std::set<std::string> NEGATIVE_WORDS = {
        "bad",     "terrible", "awful",   "horrible",   "sad",  "unhappy", "hate", "worst",    "boring",
        "dislike", "angry",    "hateful", "disgusting", "ugly", "stupid",  "dumb", "annoying", "disappointing"};
}

std::vector<std::string> splitIntoWords(const std::string &text)
{
    std::vector<std::string> words;
    std::istringstream iss(text);
    std::string word;

    while (iss >> word) {
        if (!word.empty() && word.back() == '.') {
            word.pop_back();
        }

        if (!word.empty()) {
            words.push_back(word);
        }
    }

    return words;
}

std::vector<std::pair<std::string, size_t>> topNWords(const std::vector<std::string> &sentences, size_t n)
{
    std::map<std::string, size_t> word_count;

    for (const auto &sentence : sentences) {
        auto words = util::splitIntoWords(sentence);
        for (const auto &word : words) {
            std::string lower_word = word;
            std::transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
            word_count[lower_word]++;
        }
    }

    std::vector<std::pair<std::string, size_t>> sorted_words(word_count.begin(), word_count.end());
    std::sort(sorted_words.begin(), sorted_words.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    if (sorted_words.size() > n) {
        sorted_words.resize(n);
    }

    return sorted_words;
}

std::vector<std::string> sortSentencesByLength(const std::vector<std::string> &sentences)
{
    std::vector<std::string> sorted = sentences;
    std::sort(sorted.begin(), sorted.end(), [](const std::string &a, const std::string &b) {
        return a.length() > b.length(); // по убыванию длины
    });
    return sorted;
}

std::vector<std::string> replaceNames(const std::vector<std::string> &sentences, const std::string &replacement)
{
    std::vector<std::string> result;
    std::regex name_regex(R"(\b[A-Z][a-z]*\b)"); // Слова, начинающиеся с заглавной буквы

    for (const auto &sentence : sentences) {
        std::string modified = std::regex_replace(sentence, name_regex, replacement);
        result.push_back(modified);
    }

    return result;
}

std::vector<std::pair<std::string, size_t>> mergeTopWords(
        const std::vector<std::vector<std::pair<std::string, size_t>>> &all_top_words, size_t n)
{

    std::map<std::string, size_t> merged_count;

    for (const auto &chunk_words : all_top_words) {
        for (const auto &[word, count] : chunk_words) {
            merged_count[word] += count;
        }
    }

    std::vector<std::pair<std::string, size_t>> merged_sorted(merged_count.begin(), merged_count.end());
    std::sort(merged_sorted.begin(), merged_sorted.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    if (merged_sorted.size() > n) {
        merged_sorted.resize(n);
    }

    return merged_sorted;
}

std::vector<std::string> mergeAndSortSentences(const std::vector<std::vector<std::string>> &all_sentences)
{

    std::vector<std::string> all_merged;

    for (const auto &chunk_sentences : all_sentences) {
        all_merged.insert(all_merged.end(), chunk_sentences.begin(), chunk_sentences.end());
    }

    std::sort(all_merged.begin(), all_merged.end(),
              [](const std::string &a, const std::string &b) { return a.length() > b.length(); });

    return all_merged;
}

std::pair<size_t, size_t> analyzeSentiment(const std::vector<std::string> &sentences)
{
    size_t positive_count = 0;
    size_t negative_count = 0;

    for (const auto &sentence : sentences) {
        auto words = splitIntoWords(sentence);
        for (const auto &word : words) {
            std::string lower_word = word;
            std::ranges::transform(lower_word, lower_word.begin(), ::tolower);

            if (POSITIVE_WORDS.contains(lower_word)) {
                positive_count++;
            } else if (NEGATIVE_WORDS.contains(lower_word)) {
                negative_count++;
            }
        }
    }

    return {positive_count, negative_count};
}

}
