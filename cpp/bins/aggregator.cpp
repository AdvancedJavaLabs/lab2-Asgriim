#include "words_util.hpp"

#include <map>
#include <vector>
#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>

using json = nlohmann::json;

struct TaskResult {
    uint64_t id;
    uint64_t total_chunks;
    std::vector<uint64_t> word_counts;
    std::vector<std::vector<std::pair<std::string, size_t>>> top_words_per_chunk;
    std::vector<std::vector<std::string>> sorted_sentences_per_chunk;
    std::vector<std::vector<std::string>> sentences_with_replaced_names_per_chunk;
    std::vector<int> sentiment_scores;
    std::vector<size_t> positive_counts;
    std::vector<size_t> negative_counts;
    std::vector<bool> received_chunks;
};

std::map<uint64_t, TaskResult> results;

int main()
{
    auto *loop = ev_default_loop(0);
    AMQP::LibEvHandler handler(loop);

    AMQP::TcpConnection connection(&handler, AMQP::Address("localhost", 5672, AMQP::Login("guest", "guest"), "/"));
    AMQP::TcpChannel channel(&connection);

    channel.declareQueue("agg_queue", AMQP::durable)
        .onSuccess([](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
            fmt::println("Aggregator started. Waiting for results in queue '{}'", name);
        });

    channel.consume("agg_queue").onReceived([&](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::string content(message.body(), message.bodySize());
        json data(json::parse(content));

        uint64_t id = data["id"].get<uint64_t>();
        uint64_t chunk_num = data["chunk"].get<uint64_t>();
        uint64_t total_chunks = data["total"].get<uint64_t>();
        uint64_t word_count = data["word_count"].get<uint64_t>();
        auto top_words = data["top_words"].get<std::vector<std::pair<std::string, size_t>>>();
        auto sorted_sentences = data["sorted_sentences"].get<std::vector<std::string>>();
        auto sentiment = data["sentiment"];
        auto sentences_with_replaced_names = data["sentences_with_replaced_names"].get<std::vector<std::string>>();

        size_t positive_count = sentiment["positive"].get<size_t>();
        size_t negative_count = sentiment["negative"].get<size_t>();
        int sentiment_score = sentiment["score"].get<int>();

        fmt::println("Received result for ID={}, chunk {}/{}: {} words, sentiment={:+}", id, chunk_num, total_chunks,
                     word_count, sentiment_score);

        if (results.find(id) == results.end()) {
            results[id] = TaskResult{
                .id = id,
                .total_chunks = total_chunks,
                .word_counts = std::vector<uint64_t>(total_chunks, 0),
                .top_words_per_chunk = std::vector<std::vector<std::pair<std::string, size_t>>>(total_chunks),
                .sorted_sentences_per_chunk = std::vector<std::vector<std::string>>(total_chunks),
                .sentences_with_replaced_names_per_chunk = std::vector<std::vector<std::string>>(total_chunks),
                .sentiment_scores = std::vector<int>(total_chunks, 0),
                .positive_counts = std::vector<size_t>(total_chunks, 0),
                .negative_counts = std::vector<size_t>(total_chunks, 0),
                .received_chunks = std::vector<bool>(total_chunks, false)};
        }

        auto &result = results[id];
        result.word_counts[chunk_num] = word_count;
        result.top_words_per_chunk[chunk_num] = top_words;
        result.sorted_sentences_per_chunk[chunk_num] = sorted_sentences;
        result.sentences_with_replaced_names_per_chunk[chunk_num] = sentences_with_replaced_names;
        result.sentiment_scores[chunk_num] = sentiment_score;
        result.positive_counts[chunk_num] = positive_count;
        result.negative_counts[chunk_num] = negative_count;
        result.received_chunks[chunk_num] = true;

        bool all_received = true;
        for (bool received : result.received_chunks) {
            if (!received) {
                all_received = false;
                break;
            }
        }

        if (all_received) {
            uint64_t total_words = 0;
            for (uint64_t count : result.word_counts) {
                total_words += count;
            }

            auto global_top_words = util::mergeTopWords(result.top_words_per_chunk, 10);

            auto global_sorted_sentences = util::mergeAndSortSentences(result.sorted_sentences_per_chunk);

            std::vector<std::string> all_replaced_sentences;
            for (const auto &chunk : result.sentences_with_replaced_names_per_chunk) {
                all_replaced_sentences.insert(all_replaced_sentences.end(), chunk.begin(), chunk.end());
            }

            size_t total_positive = 0;
            size_t total_negative = 0;
            int total_sentiment = 0;
            for (size_t i = 0; i < result.total_chunks; ++i) {
                total_positive += result.positive_counts[i];
                total_negative += result.negative_counts[i];
                total_sentiment += result.sentiment_scores[i];
            }

            fmt::println("\n=== AGGREGATED RESULT ===");
            fmt::println("Task ID: {}", id);
            fmt::println("Total chunks: {}", total_chunks);
            fmt::println("Word counts per chunk: {}", result.word_counts);
            fmt::println("TOTAL WORDS: {}", total_words);
            fmt::println("DURATION IS: {}", std::chrono::duration_cast<std::chrono::milliseconds>(
                                                std::chrono::system_clock::now().time_since_epoch())
                                                    .count() -
                                                id);

            fmt::println("\nSENTIMENT ANALYSIS:");
            fmt::println("Positive words: {}", total_positive);
            fmt::println("Negative words: {}", total_negative);
            fmt::println("Overall sentiment score: {:+}", total_sentiment);
            fmt::println("Sentiment: {}", total_sentiment > 0   ? "POSITIVE"
                                          : total_sentiment < 0 ? "NEGATIVE"
                                                                : "NEUTRAL");

            fmt::println("\nTOP 10 WORDS:");
            for (size_t i = 0; i < global_top_words.size(); ++i) {
                const auto &[word, count] = global_top_words[i];
                fmt::println("  {}. {}: {}", i + 1, word, count);
            }

            fmt::println("\nLONGEST 5 SENTENCES:");
            for (size_t i = 0; i < std::min(size_t(5), global_sorted_sentences.size()); ++i) {
                fmt::println("  {}. {} chars: {}", i + 1, global_sorted_sentences[i].length(),
                             global_sorted_sentences[i].substr(0, 100) +
                                 (global_sorted_sentences[i].length() > 100 ? "..." : ""));
            }

            fmt::println("\nEXAMPLES WITH NAMES REPLACED:");
            for (size_t i = 0; i < std::min(size_t(3), all_replaced_sentences.size()); ++i) {
                fmt::println("  {}. {}", i + 1,
                             all_replaced_sentences[i].substr(0, 100) +
                                 (all_replaced_sentences[i].length() > 100 ? "..." : ""));
            }

            fmt::println("=========================\n");

            results.erase(id);
        }

        channel.ack(deliveryTag);
    });

    // Запускаем цикл событий
    ev_run(loop, 0);
    return 0;
}