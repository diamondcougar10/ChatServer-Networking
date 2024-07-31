#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <unordered_map>
#include <string>
#include <unordered_set>

class UserManager 
{
    public:
    void addUser(const std::string& username, const std::string& password);
    bool userExists(const std::string& username);
    bool authenticateUser(const std::string& username, const std::string& password);
    bool isLoggedIn(const std::string& username);
    void loginUser(const std::string& username);
    void logoutUser(const std::string& username);
    std::unordered_set<std::string> getLoggedInUsers();
    private:
    std::unordered_map<std::string, std::string> users;
    std::unordered_set<std::string> loggedInUsers;
};

#endif
