#pragma once
#include <sqlite3.h>
#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>

// ==========================================
// DTOs (Data Transfer Objects)
// ==========================================
struct EnemyStats {
    int enemyID = 0;
    std::string name = "";
    int baseHealth = 0;
    float moveSpeed = 0.0f;
    float attackRange = 0.0f;
    int factionID = 0;
};

struct WeaponStats {
    int weaponID = 0;
    int itemID = 0;
    int damageValue = 0;
    float fireRate = 0.0f;
    int effectID = 0; 
    bool hasEffect = false;
};

// DTO to retrieve player save data
struct PlayerSaveData {
    int saveID = 0;
    int classID = 0;
    int currentLevel = 0;
    int currentHealth = 0;
    int currentXP = 0;
    int dungeonFloor = 0;
    double playerX = 0.0f;
    double playerY = 0.0f;
    std::vector<int> inventoryItemIDs; // Array of items from the Bridge table
};

// ==========================================
// DatabaseManager (Singleton)
// ==========================================
class DatabaseManager {
private:
    sqlite3* m_db = nullptr;
    
    DatabaseManager();
    ~DatabaseManager();

public:
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    // --- Connection & Schema Management ---
    bool connect(const std::string& dbPath);
    void disconnect();
    void initSchema(); 

    // --- ACID Transaction Control ---
    void beginTransaction();
    void commit();
    void rollback();

    // --- Data Fetching APIs ---
    EnemyStats queryEnemyProfile(const std::string& codename);
    WeaponStats queryWeaponStats(int weaponID);

    // --- Persistence & State APIs ---
    bool savePlayerState(const PlayerSaveData& data);
    PlayerSaveData loadPlayerState(int saveID);
    
    // Getter for ImGui
    sqlite3* getDBPointer() const { return m_db; }
};