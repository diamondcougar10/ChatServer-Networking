#include "UserManager.h"

void UserManager::addUser(const std::string& username, const std::string& password) 
{
    users[username] = password;
}

bool UserManager::userExists(const std::string& username) 
{
    return users.find(username) != users.end();
}

bool UserManager::authenticateUser(const std::string& username, const std::string& password) 
{
    auto it = users.find(username);
    return it != users.end() && it->second == password;
}

bool UserManager::isLoggedIn(const std::string& username) 
{
    return loggedInUsers.find(username) != loggedInUsers.end();
}

void UserManager::loginUser(const std::string& username)
{
    loggedInUsers.insert(username);
}

void UserManager::logoutUser(const std::string& username) 
{
    loggedInUsers.erase(username);
}

std::unordered_set<std::string> UserManager::getLoggedInUsers()
{
    return loggedInUsers;
}
