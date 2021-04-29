#include <iostream>
#include <string_view>
#include <string>
#include <stdexcept>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "config.hh"


std::string
mimeType
(std::string_view path_to_file) {
    auto extension = [&path_to_file]{
        std::string path{path_to_file.data()};
        auto idx = path.rfind('.');
        if (idx != std::string::npos)
            return path.substr(idx);
        else
            throw std::invalid_argument("File must contain extesion!");
    }();
    auto compare = [](const std::string& file_ext, const char* ext){
        return file_ext.compare(ext) == 0;
    };
    if(compare(extension, ".htm"))  return "text/html";
    if(compare(extension, ".html")) return "text/html";
    if(compare(extension, ".php"))  return "text/html";
    if(compare(extension, ".css"))  return "text/css";
    if(compare(extension, ".txt"))  return "text/plain";
    if(compare(extension, ".js"))   return "application/javascript";
    if(compare(extension, ".json")) return "application/json";
    if(compare(extension, ".xml"))  return "application/xml";
    if(compare(extension, ".swf"))  return "application/x-shockwave-flash";
    if(compare(extension, ".flv"))  return "video/x-flv";
    if(compare(extension, ".png"))  return "image/png";
    if(compare(extension, ".jpe"))  return "image/jpeg";
    if(compare(extension, ".jpeg")) return "image/jpeg";
    if(compare(extension, ".jpg"))  return "image/jpeg";
    if(compare(extension, ".gif"))  return "image/gif";
    if(compare(extension, ".bmp"))  return "image/bmp";
    if(compare(extension, ".ico"))  return "image/vnd.microsoft.icon";
    if(compare(extension, ".tiff")) return "image/tiff";
    if(compare(extension, ".tif"))  return "image/tiff";
    if(compare(extension, ".svg"))  return "image/svg+xml";
    if(compare(extension, ".svgz")) return "image/svg+xml";
    return "application/text";
}

template <typename Stream>
struct sendLambda final {
    Stream& stream_;
    bool& close_;
    boost::beast::error_code& ec_;

    explicit sendLambda
    (Stream& stream, bool& close, boost::beast::error_code& ec)
        :  stream_(stream),
           close_(close),
           ec_(ec){}

    template<bool isRequest, class Body, class Fields>
    void
    operator()
    (boost::beast::http::message<isRequest, Body, Fields> msg) {
        close_ = msg.need_eof();
        boost::beast::http::serializer<isRequest, Body, Fields> serizer{msg};
        boost::beast::http::write(stream_, serizer);
    }
};

void reportFailure(std::string_view what, boost::beast::error_code& ec){
    std::cerr << what.data() << " : " << ec.message();
}

template<typename Body, typename Send>
void
handleRequest
(boost::beast::http::request<Body> req, Send send_lambda) {
    namespace http = boost::beast::http;

    auto bad_request = [&req](std::string_view why){
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    std::cout << "Got request! " << req.target() << '\n';
    if (req.method() != http::verb::get)
        return send_lambda(bad_request("Only get requests allowed!"));
    if (req.target().empty() || req.target() == "/")
        return send_lambda(bad_request("You MUST provide filename!"));

    auto make_path = [](std::string_view filename) {
        return std::string(Config::template_dir) + filename.data();
    };

    auto path = make_path(std::string(req.target()));
    boost::beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);
    if (ec == boost::beast::errc::no_such_file_or_directory)
        return send_lambda(bad_request("File you requested was not found!"));
    if(ec)
        return send_lambda(bad_request("Something went wrong of file opening!"));

    auto const sz = body.size();
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())
            };

    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mimeType(path));
    res.content_length(sz);
    res.keep_alive(req.keep_alive());
    return send_lambda(std::move(res));
}


void runSession(boost::asio::ip::tcp::socket& sock) {
    using boost::asio::ip::tcp;
    namespace http = boost::beast::http;
    boost::beast::flat_buffer buff;
    boost::beast::error_code ec;
    bool is_closed = false;
    sendLambda<tcp::socket> send_lambda{sock, is_closed, ec};

    for(;;){
        http::request<http::string_body> req;
        http::read(sock, buff, req, ec);
        if (ec == http::error::end_of_stream)
            break;
        if (ec)
            reportFailure("Read request error", ec);
        handleRequest(req, send_lambda);
        if (ec)
            return reportFailure("Writing response and something went wrong!", ec);
        if (is_closed)
            break;
    }
    sock.shutdown(tcp::socket::shutdown_send);
}


void runServer(){
    using boost::asio::ip::tcp;
    boost::asio::io_context ioc;
    tcp::acceptor actor{ioc};
    std::cout << "Created acceptor.\n";
    tcp::endpoint ep{boost::asio::ip::make_address("0.0.0.0"), (unsigned short)6969};
    std::cout << "Created endpoint.\n";
    actor.open(tcp::v4());
    actor.bind(ep);
    actor.listen();
    std::cout << "Binded acceptor to endpoint.\n";
    for(auto i = 0; i < 5; ++i) {
        tcp::socket sock{ioc};
        actor.accept(sock);
        std::cout << "Got socket from " << sock.remote_endpoint().address().to_string() << ":" << sock.remote_endpoint().port() << '\n';
        std::thread{std::bind(runSession, std::move(sock))}.detach();
    }
}

#ifdef DEBUG
#include <cassert>
#define assert_explained(expr, msg) assert(((void)msg, expr))

void
testMimeType
() {
    auto compare = [](const std::string& file_ext, const char* ext){
        return file_ext.compare(ext) == 0;
    };
    assert_explained(compare(mimeType("index.html"), "text/html"), "index.html failed!");
    assert_explained(compare(mimeType("meme.jpg"), "image/jpeg"), "meme.jpg failed!");
    assert_explained(compare(mimeType("file.dot.txt"), "text/plain"), "file.dot.txt failed!");

    bool is_throwed = false;
    try {
        mimeType("index"); // MUST THROW

    }
    catch (std::invalid_argument& e){
        is_throwed = true;
    }
    assert_explained(is_throwed, "index failed!");
}

void
runTests
() {
    testMimeType();
}
#endif


int main(int argc, char *argv[]) {

    runTests();

    runServer();

    return EXIT_SUCCESS;
}
