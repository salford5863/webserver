#include <iostream>
#include <string>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

using boost::asio::ip::tcp;
namespace fs = boost::filesystem;

class WebServer {
public:
    WebServer(short port, const std::string& doc_root) : acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)), doc_root_(doc_root) {
        startAccept();
    }

    void run() {
        io_service_.run();
    }

private:
    void startAccept() {
        tcp::socket* newSocket = new tcp::socket(io_service_);
        acceptor_.async_accept(*newSocket, [this, newSocket](const boost::system::error_code& error) {
            if (!error) {
                handleRequest(newSocket);
            } else {
                delete newSocket;
            }
            startAccept();
        });
    }

    void handleRequest(tcp::socket* socket) {
        boost::asio::streambuf buffer;
        boost::asio::read_until(*socket, buffer, "\r\n\r\n");

        std::istream stream(&buffer);
        std::string request;
        std::getline(stream, request);

        // Parse the request
        std::istringstream request_stream(request);
        std::string method, path, protocol;
        request_stream >> method >> path >> protocol;

        // Construct the full path to the requested file
        std::string full_path = doc_root_ + path;

        // Check if the file exists
        if (fs::exists(full_path) && fs::is_regular_file(full_path)) {
            // Read the file contents
            std::ifstream file(full_path.c_str(), std::ios::binary);
            std::string file_contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            // Generate the response headers
            std::string response_headers = "HTTP/1.1 200 OK\r\n";
            response_headers += "Content-Type: text/html\r\n";
            response_headers += "Content-Length: " + std::to_string(file_contents.length()) + "\r\n";
            response_headers += "Connection: close\r\n\r\n";

            // Send the response headers and file contents
            boost::asio::write(*socket, boost::asio::buffer(response_headers));
            boost::asio::write(*socket, boost::asio::buffer(file_contents));
        } else {
            // Generate a 404 Not Found response
            std::string response_headers = "HTTP/1.1 404 Not Found\r\n";
            response_headers += "Content-Type: text/plain\r\n";
            response_headers += "Connection: close\r\n\r\n";
            std::string response_body = "404 Not Found";

            boost::asio::write(*socket, boost::asio::buffer(response_headers));
            boost::asio::write(*socket, boost::asio::buffer(response_body));
        }

        delete socket;
    }

    boost::asio::io_service io_service_;
    tcp::acceptor acceptor_;
    std::string doc_root_;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <doc_root>\n";
        return 1;
    }

    short port = std::atoi(argv[1]);
    std::string doc_root = argv[2];

    try {
        WebServer server(port, doc_root);
        server.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
