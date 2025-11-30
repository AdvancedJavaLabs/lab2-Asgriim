#include "words_util.hpp"

#include <vector>
#include <sstream>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <nlohmann/json.hpp>

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>

using json = nlohmann::json;

int main()
{
    auto *loop = ev_default_loop(0);
    AMQP::LibEvHandler handler(loop);

    AMQP::TcpConnection connection(&handler, AMQP::Address("localhost", 5672, AMQP::Login("guest", "guest"), "/"));
    AMQP::TcpChannel channel(&connection);

    channel.declareQueue("task_queue", AMQP::durable)
        .onSuccess([](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
            fmt::println("Waiting for messages. To exit press CTRL+C");
        });

    channel.declareQueue("agg_queue", AMQP::durable);

    channel.consume("task_queue").onReceived([&](const AMQP::Message &message, uint64_t deliveryTag, bool redelivered) {
        std::string content(message.body(), message.bodySize());
        json data(json::parse(content));
        auto &&id = data["id"].get<uint64_t>();
        auto &&i = data["chunk"].get<uint64_t>();
        auto &&total = data["total"].get<uint64_t>();
        auto &&sentences = data["sentences"].get<std::vector<std::string>>();
        fmt::println("Got ID={} ({}/{}) {}", id, i, total, sentences.size());

        json ser;
        ser["id"] = data["id"];
        ser["chunk"] = data["chunk"];
        ser["total"] = data["total"];
        size_t word_count = 0;
        for (auto &&sentence : sentences) {
            word_count += util::splitIntoWords(sentence).size();
        }
        ser["word_count"] = word_count;

        ser["top_words"] = util::topNWords(sentences, 10);

        auto [positive_count, negative_count] = util::analyzeSentiment(sentences);
        ser["sentiment"] = {
            {"positive", positive_count}, {"negative", negative_count}, {"score", positive_count - negative_count}};

        auto sentences_with_replaced_names = util::replaceNames(sentences, "ASSGRIM");
        ser["sentences_with_replaced_names"] = sentences_with_replaced_names;

        ser["sorted_sentences"] = util::sortSentencesByLength(sentences);

        channel.publish("", "agg_queue", ser.dump());

        channel.ack(deliveryTag);
    });

    // Запускаем цикл событий
    ev_run(loop, 0);
    return 0;
}
