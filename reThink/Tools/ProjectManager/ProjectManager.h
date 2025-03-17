// ProjectManager.h
#pragma once
#include <string>
#include <fstream>

class ProjectManager {
public:
    void newProject();
    void saveProject(const std::string& path);
    void loadProject(const std::string& path);
};