#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include "json.hpp"

using json = nlohmann::json;

const std::string SALT = "some\\_random\\_salt"; // Change this to a secure random string
std::vector<std::string> chatMessages; // Global vector to store all messages

std::string hashPassword(const std::string& password) {
    std::string hashedPassword;
    for (char c : password + SALT) {
        hashedPassword.push_back(c ^ 0x5A);
    }
    return hashedPassword;
}

void handleSignup(const std::string& username, const std::string& password) {
    // Load existing user data from JSON file
    json userData;
    std::ifstream userFile("users.json");
    if (userFile.is_open()) {
        userFile >> userData;
        userFile.close();
    }

    // Check if the username already exists
    if (userData.count(username) == 0) {
        // Hash the password with salt
        std::string hashedPassword = hashPassword(password);

        // Add the new user to the user data
        json newUser;
        newUser["username"] = username;
        newUser["password"] = hashedPassword;
        userData.push_back(newUser);

        // Save the updated user data to the JSON file
        std::ofstream userFileOut("users.json");
        if (userFileOut.is_open()) {
            userFileOut << std::setw(4) << userData << std::endl;
            userFileOut.close();
            std::cout << "Signup successful for user: " << username << std::endl;
        } else {
            std::cerr << "Error writing to users.json" << std::endl;
        }
    } else {
        std::cerr << "Username already exists: " << username << std::endl;
    }
}

bool handleLogin(const std::string& username, const std::string& password) {
    // Load existing user data from JSON file
    json userData;
    std::ifstream userFile("users.json");
    if (userFile.is_open()) {
        userFile >> userData;
        userFile.close();
    }

    // Check if the username and password are correct
    for (const auto& user : userData) {
        if (user["username"] == username && user["password"] == hashPassword(password)) {
            std::cout << "Login successful for user: " << username << std::endl;
            chatMessages.push_back("Welcome, " + username + "!");
            return true;
        }
    }

    std::cerr << "Invalid username or password for user: " << username << std::endl;
    return false;
}

void sendMessage(const std::string& username, const std::string& message) {
    chatMessages.push_back(username + ": " + message);
}

void displayMessages() {
    std::cout << "Messages:" << std::endl;
    for (const auto& message : chatMessages) {
        std::cout << message << std::endl;
    }
}

int main() {
    std::string username, password, message;

    std::cout << "Enter username: ";
    std::getline(std::cin, username);
    std::cout << "Enter password: ";
    std::getline(std::cin, password);

    if (handleLogin(username, password)) {
        std::cout << "Logged in as " << username << std::endl;

        while (true) {
            std::cout << "Enter message (or 'exit' to quit): ";
            std::getline(std::cin, message);

            if (message == "exit") {
                break;
            }

            sendMessage(username, message);
            displayMessages();
        }
    }

    return 0;
}
