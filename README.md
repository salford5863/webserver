WebChat Application
This is a simple web-based chat application built using C++ and the Boost library. The application allows users to sign up, log in, and send messages to a common chat room, where all messages are visible to all users.
Prerequisites
Before running the application, make sure you have the following installed:

C++ compiler (e.g., g++ for Linux/macOS, or Visual C++ for Windows)
Boost library (version 1.67 or later)

Building the Application

Clone the repository or download the source code.
Navigate to the project directory.
Compile the source code using your C++ compiler and link against the Boost libraries. For example, on Linux or macOS with g++:

g++ -std=c++11 -o webchat main.cpp -lboost_system -lboost_filesystem
On Windows with Visual C++, you may need to configure your project to include the Boost library paths and link against the required Boost libraries.
Running the Application
After successfully compiling the application, you can run it with the following command:
./webchat <port> <doc_root>
Replace <port> with the desired port number (e.g., 8080) and <doc_root> with the directory path where your HTML, CSS, and JavaScript files are located (e.g., /path/to/web/files).
Example:
./webchat 8080 /path/to/web/files
This will start the web server and make the chat application accessible at http://localhost:<port>.
Using the Application

Open a web browser and navigate to http://localhost:<port>.
You will be presented with a sign-up page. Enter a username and password to create a new account.
After successful sign-up, you will be redirected to the login page.
Enter your username and password to log in.
Once logged in, you will be able to see the chat room and any existing messages.
Type your message in the input field and press Enter to send it to the chat room.
All messages sent by you and other users will be displayed in the chat room in real-time.
