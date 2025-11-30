#include "sentence_splitter.hpp"

#include <vector>
#include <sstream>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>

using json = nlohmann::json;

int main(int argc, char **argv)
{
    size_t chunk_size = 50;
    util::SentenceSplitter splitter;
    splitter.readAndSplit("/home/asgrim/school/lab2-Asgriim/cpp/moby_dick.txt");
    fmt::println("Sentence count {}", splitter.getSentences().size());

    auto chunks = splitter.getSentences() | std::views::chunk(chunk_size) | std::views::enumerate;
    auto len = std::ranges::size(chunks);

    auto *loop = ev_default_loop(0);
    AMQP::LibEvHandler handler(loop);

    AMQP::TcpConnection connection(&handler, AMQP::Address("localhost", 5672, AMQP::Login("guest", "guest"), "/"));
    AMQP::TcpChannel channel(&connection);

    auto id = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                  .count();

    channel.declareQueue("task_queue", AMQP::durable)
        .onSuccess([&](const std::string &name, uint32_t messagecount, uint32_t consumercount) {
            for (auto &&[i, chunk] : chunks) {
                fmt::println("({}/{}) -> {}", i + 1, len, chunk.size());
                json mes;
                mes["id"] = id;
                mes["chunk"] = i;
                mes["total"] = len;
                mes["sentences"] = chunk;

                channel.publish("", "task_queue", mes.dump());
            }

            connection.close();
        });

    ev_run(loop, 0);

    return 0;
}
