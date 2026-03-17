#ifndef CONFIG_H
#define CONFIG_H

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

class ConfigHander
{
  json settings;
  std::filesystem::path app_folder;
  std::filesystem::path config_file;

  json getDefaultSettings()
  {
    return {{"database",
             {{"path", (app_folder / "vault.db").string()},
              {"backup_path", (app_folder / "backups").string()},
              {"connection_timeout", 5}}},
            {"encryption",
             {{"algorithm", "AES-256-GCM"},
              {"key_length", 32},
              {"iterations", 3},
              {"memory_cost", 64},
              {"parallelism", 4},
              {"hash_length", 32}}},
            {"user", {{"is_first_run", true}}}};
  }

  void saveSettings()
  {
    try
    {
      std::ofstream file(config_file);
      if (!file.is_open())
      {
        std::cerr << "Cannot open config file for writing: " << config_file
                  << std::endl;
        return;
      }
      file << settings.dump(4);
      file.close();
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error saving settings: " << e.what() << std::endl;
    }
  }

  void loadSettings()
  {
    try
    {
      if (std::filesystem::exists(config_file))
      {
        std::ifstream file(config_file);
        if (!file.is_open())
        {
          std::cerr << "Cannot open config file for reading: " << config_file
                    << std::endl;
          settings = getDefaultSettings();
          return;
        }

        try
        {
          file >> settings;
        }
        catch (const json::parse_error &e)
        {
          std::cerr << "JSON parse error: " << e.what() << std::endl;
          settings = getDefaultSettings();
          saveSettings();
        }
      }
      else
      {
        settings = getDefaultSettings();
        saveSettings();
      }
    }
    catch (const std::exception &e)
    {
      std::cerr << "Error loading settings: " << e.what() << std::endl;
      settings = getDefaultSettings();
    }
  }

  // Безопасный доступ к JSON полям
  template <typename T>
  T getValue(const std::string &section, const std::string &key,
             const T &defaultValue)
  {
    try
    {
      if (settings.contains(section) && settings[section].is_object() &&
          settings[section].contains(key))
      {
        return settings[section][key].get<T>();
      }
    }
    catch (...)
    {
      // Игнорируем ошибки
    }
    return defaultValue;
  }

public:
  ConfigHander()
  {
#ifdef _WIN32
    const char *home = std::getenv("USERPROFILE");
    app_folder = home ? std::filesystem::path(home) / ".cryptosafe"
                      : std::filesystem::path(".cryptosafe");
#else
    const char *home = std::getenv("HOME");
    app_folder = home ? std::filesystem::path(home) / ".cryptosafe"
                      : std::filesystem::path(".cryptosafe");
#endif

    config_file = app_folder / "config.json";

    try
    {
      if (!std::filesystem::exists(app_folder))
      {
        std::filesystem::create_directories(app_folder);
      }
    }
    catch (const std::exception &e)
    {
      std::cerr << "Failed to create app folder: " << e.what() << std::endl;
    }

    loadSettings();
  }

  std::string getDatabasePath()
  {
    return getValue<std::string>("database", "path",
                                 (app_folder / "vault.db").string());
  }

  std::string getBackupPath()
  {
    return getValue<std::string>("database", "backup_path",
                                 (app_folder / "backups").string());
  }

  int getConnectionTimeout()
  {
    return getValue<int>("database", "connection_timeout", 5);
  }

  bool isFirstRun() { return getValue<bool>("user", "is_first_run", true); }

  // Сеттеры
  void setDatabasePath(const std::string &path)
  {
    settings["database"]["path"] = path;
    saveSettings();
  }

  void setFirstRun(bool value)
  {
    settings["user"]["is_first_run"] = value;
    saveSettings();
  }

  void setArgon2TimeCost(int iterations)
  {
    settings["encryption"]["iterations"] = iterations;
    saveSettings();
  }

  void setArgon2MemoryCost(int mb)
  {
    settings["encryption"]["memory_cost"] = mb;
    saveSettings();
  }

  void setArgon2Parallelism(int threads)
  {
    settings["encryption"]["parallelism"] = threads;
    saveSettings();
  }

  void setArgon2HashLength(int bytes)
  {
    settings["encryption"]["hash_length"] = bytes;
    saveSettings();
  }

  // Геттеры для encryption параметров
  std::string getEncryptionAlgorithm()
  {
    return getValue<std::string>("encryption", "algorithm", "AES-256-GCM");
  }

  int getKeyLength() { return getValue<int>("encryption", "key_length", 32); }

  int getArgon2TimeCost()
  {
    return getValue<int>("encryption", "iterations", 3);
  }

  int getArgon2MemoryCost()
  {
    return getValue<int>("encryption", "memory_cost", 64);
  }

  int getArgon2Parallelism()
  {
    return getValue<int>("encryption", "parallelism", 4);
  }

  int getArgon2HashLength()
  {
    return getValue<int>("encryption", "hash_length", 32);
  }
};

#endif