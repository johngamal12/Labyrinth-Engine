#include "DatabaseManager.hpp"

// Constructor: Initializes the pointer to null
DatabaseManager::DatabaseManager() : m_db(nullptr) {}

// Destructor: Ensures the database connection is closed when the engine shuts down
DatabaseManager::~DatabaseManager() {
    disconnect();
}

// ---------------------------------------------------------
// 1. Connection Management
// ---------------------------------------------------------
bool DatabaseManager::connect(const std::string& dbPath) {
    // Open the connection to the SQLite file
    int exitCode = sqlite3_open(dbPath.c_str(), &m_db);
    
    if (exitCode != SQLITE_OK) {
        std::cerr << "Database Error: Failed to open database at " << dbPath 
                  << ". Reason: " << sqlite3_errmsg(m_db) << "\n";
        return false;
    }

    // THE WHY: SQLite disables Foreign Key constraints by default for backwards compatibility.
    // We MUST execute this PRAGMA command to enforce our relational schema.
    
    char* errMsg = nullptr;
    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
    if (errMsg) {
        std::cerr << "Database Error: Failed to enable foreign keys: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }

    std::cout << "Database connected successfully.\n";
    return true;
}

void DatabaseManager::disconnect() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
        std::cout << "Database disconnected safely.\n";
    }
}

// ---------------------------------------------------------
// 2. ACID Transactions (REQ-3.2.3)
// ---------------------------------------------------------
void DatabaseManager::beginTransaction() {
    sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
}

void DatabaseManager::commit() {
    sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
}

void DatabaseManager::rollback() {
    sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
}

// ---------------------------------------------------------
// 3. Schema Initialization
// ---------------------------------------------------------
void DatabaseManager::initSchema() {
    if (!m_db) return;

    // THE WHY: We use C++11 Raw String Literals R"(...)" to write multi-line SQL safely.
    // The order of creation is CRITICAL. Independent tables (like Factions, Rarities) 
    // MUST be created before the tables that depend on them (like Enemy_Profiles, Items).
    
    const char* schemaSQL = R"(
        -- =====================================
        -- CATEGORY 1: CORE ENTITIES & SYSTEMS
        -- =====================================
        CREATE TABLE IF NOT EXISTS Factions (
            FactionID INTEGER PRIMARY KEY AUTOINCREMENT,
            FactionName TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS Base_Classes (
            ClassID INTEGER PRIMARY KEY AUTOINCREMENT,
            ClassName TEXT NOT NULL,
            StartingHealth INTEGER,
            BaseSpeed REAL,
            BaseDamage INTEGER,
            BaseArmor INTEGER
        );

        CREATE TABLE IF NOT EXISTS Level_Thresholds (
            LevelID INTEGER PRIMARY KEY,
            XPRequired INTEGER,
            StatMultiplier REAL
        );

        CREATE TABLE IF NOT EXISTS Damage_Types (
            TypeID INTEGER PRIMARY KEY AUTOINCREMENT,
            TypeName TEXT NOT NULL
        );

        CREATE TABLE IF NOT EXISTS Enemy_Profiles (
            EnemyID INTEGER PRIMARY KEY AUTOINCREMENT,
            Codename TEXT UNIQUE NOT NULL,
            Name TEXT,
            BaseHealth INTEGER,
            FactionID INTEGER,
            FOREIGN KEY(FactionID) REFERENCES Factions(FactionID)
        );

        CREATE TABLE IF NOT EXISTS NPC_Profiles (
            NPCID INTEGER PRIMARY KEY AUTOINCREMENT,
            Name TEXT,
            FactionID INTEGER,
            IsMerchant BOOLEAN,
            FOREIGN KEY(FactionID) REFERENCES Factions(FactionID)
        );

        CREATE TABLE IF NOT EXISTS Enemy_Resistances (
            EnemyID INTEGER,
            TypeID INTEGER,
            DamageMultiplier REAL,
            FOREIGN KEY(EnemyID) REFERENCES Enemy_Profiles(EnemyID),
            FOREIGN KEY(TypeID) REFERENCES Damage_Types(TypeID),
            PRIMARY KEY (EnemyID, TypeID)
        );

        -- =====================================
        -- CATEGORY 2: THE LOOT ENGINE
        -- =====================================
        CREATE TABLE IF NOT EXISTS Rarities (
            RarityID INTEGER PRIMARY KEY AUTOINCREMENT,
            RarityName TEXT NOT NULL,
            DropWeight REAL
        );

        CREATE TABLE IF NOT EXISTS Status_Effects (
            EffectID INTEGER PRIMARY KEY AUTOINCREMENT,
            EffectName TEXT NOT NULL,
            DamagePerSecond INTEGER,
            DurationSeconds REAL
        );

        CREATE TABLE IF NOT EXISTS Item_Modifiers (
            ModifierID INTEGER PRIMARY KEY AUTOINCREMENT,
            PrefixName TEXT,
            StatBoost INTEGER
        );

        CREATE TABLE IF NOT EXISTS Items (
            ItemID INTEGER PRIMARY KEY AUTOINCREMENT,
            Name TEXT NOT NULL,
            Category TEXT,
            RarityID INTEGER,
            FOREIGN KEY(RarityID) REFERENCES Rarities(RarityID)
        );

        CREATE TABLE IF NOT EXISTS Weapons (
            WeaponID INTEGER PRIMARY KEY AUTOINCREMENT,
            ItemID INTEGER UNIQUE,
            TypeID INTEGER,
            DamageValue INTEGER,
            FireRate REAL,
            EffectID INTEGER,
            ModifierID INTEGER,
            FOREIGN KEY(ItemID) REFERENCES Items(ItemID),
            FOREIGN KEY(TypeID) REFERENCES Damage_Types(TypeID),
            FOREIGN KEY(EffectID) REFERENCES Status_Effects(EffectID),
            FOREIGN KEY(ModifierID) REFERENCES Item_Modifiers(ModifierID)
        );

        CREATE TABLE IF NOT EXISTS Armor (
            ArmorID INTEGER PRIMARY KEY AUTOINCREMENT,
            ItemID INTEGER UNIQUE,
            DefenseValue INTEGER,
            EquipSlot TEXT,
            FOREIGN KEY(ItemID) REFERENCES Items(ItemID)
        );

        CREATE TABLE IF NOT EXISTS Consumables (
            ConsumableID INTEGER PRIMARY KEY AUTOINCREMENT,
            ItemID INTEGER UNIQUE,
            HealAmount INTEGER,
            IsStackable BOOLEAN,
            FOREIGN KEY(ItemID) REFERENCES Items(ItemID)
        );

        CREATE TABLE IF NOT EXISTS Loot_Drop_Tables (
            DropID INTEGER PRIMARY KEY AUTOINCREMENT,
            EnemyID INTEGER,
            ItemID INTEGER,
            DropChance REAL,
            FOREIGN KEY(EnemyID) REFERENCES Enemy_Profiles(EnemyID),
            FOREIGN KEY(ItemID) REFERENCES Items(ItemID)
        );

        -- =====================================
        -- CATEGORY 3 & 4: PROGRESSION & SAVES
        -- =====================================
        CREATE TABLE IF NOT EXISTS Biome_Types (
            BiomeID INTEGER PRIMARY KEY AUTOINCREMENT,
            BiomeName TEXT NOT NULL,
            DifficultyMultiplier REAL
        );

        CREATE TABLE IF NOT EXISTS Room_Templates (
            TemplateID INTEGER PRIMARY KEY AUTOINCREMENT,
            YamlFilePath TEXT,
            BiomeID INTEGER,
            IsBossRoom BOOLEAN,
            FOREIGN KEY(BiomeID) REFERENCES Biome_Types(BiomeID)
        );

        CREATE TABLE IF NOT EXISTS Achievements (
            AchievementID INTEGER PRIMARY KEY AUTOINCREMENT,
            Title TEXT,
            Description TEXT
        );

        CREATE TABLE IF NOT EXISTS PlayerSaves (
            SaveID INTEGER PRIMARY KEY AUTOINCREMENT,
            ClassID INTEGER,
            CurrentLevel INTEGER,
            CurrentHealth INTEGER,
            CurrentXP INTEGER,
            DungeonFloor INTEGER,
            PlayerX REAL,
            PlayerY REAL,
            FOREIGN KEY(ClassID) REFERENCES Base_Classes(ClassID),
            FOREIGN KEY(CurrentLevel) REFERENCES Level_Thresholds(LevelID)
        );

        CREATE TABLE IF NOT EXISTS Player_Inventory_Bridge (
            SaveID INTEGER,
            ItemID INTEGER,
            Quantity INTEGER,
            FOREIGN KEY(SaveID) REFERENCES PlayerSaves(SaveID),
            FOREIGN KEY(ItemID) REFERENCES Items(ItemID),
            PRIMARY KEY (SaveID, ItemID)
        );
    )";

    char* errMsg = nullptr;
    int exitCode = sqlite3_exec(m_db, schemaSQL, nullptr, nullptr, &errMsg);

    if (exitCode != SQLITE_OK) {
        std::cerr << "Schema Initialization Error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        std::cout << "Database Schema initialized successfully.\n";
    }
}
// ---------------------------------------------------------
// 4. Data Fetching APIs (Prepared Statements)
// ---------------------------------------------------------
EnemyStats DatabaseManager::queryEnemyProfile(const std::string& codename) {
    EnemyStats stats;
    
    // SQL Query with a placeholder '?' for the codename
    const char* query = "SELECT EnemyID, Name, BaseHealth, FactionID FROM Enemy_Profiles WHERE Codename = ?;";
    sqlite3_stmt* stmt = nullptr;

    // 1. Prepare: Compiles the SQL query safely
    if (sqlite3_prepare_v2(m_db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL Error (Prepare): " << sqlite3_errmsg(m_db) << "\n";
        return stats;
    }

    // 2. Bind: Safely inserts the string into the '?' placeholder
    sqlite3_bind_text(stmt, 1, codename.c_str(), -1, SQLITE_STATIC);

    // 3. Step: Execute the query and read the returned row
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        stats.enemyID = sqlite3_column_int(stmt, 0);
        
        // Handle text carefully (it might be NULL in the database)
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        stats.name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        
        stats.baseHealth = sqlite3_column_int(stmt, 2);
        stats.factionID = sqlite3_column_int(stmt, 3);
    } else {
        std::cerr << "Warning: Enemy codename '" << codename << "' not found in DB.\n";
    }

    // 4. Finalize: Delete the statement from memory to prevent leaks
    sqlite3_finalize(stmt);
    
    return stats;
}
bool DatabaseManager::savePlayerState(const PlayerSaveData& data) {
    if (!m_db) return false;

    beginTransaction(); // Start ACID Transaction

    try {
        // 1. Update PlayerSaves table (using INSERT OR REPLACE for easy updates)
        const char* playerSQL = R"(
            INSERT OR REPLACE INTO PlayerSaves (SaveID, ClassID, CurrentLevel, CurrentHealth, CurrentXP, DungeonFloor, PlayerX, PlayerY)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?);
        )";

        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(m_db, playerSQL, -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, data.saveID);
        sqlite3_bind_int(stmt, 2, data.classID);
        sqlite3_bind_int(stmt, 3, data.currentLevel);
        sqlite3_bind_int(stmt, 4, data.currentHealth);
        sqlite3_bind_int(stmt, 5, data.currentXP);
        sqlite3_bind_int(stmt, 6, data.dungeonFloor);
        sqlite3_bind_double(stmt, 7, data.playerX);
        sqlite3_bind_double(stmt, 8, data.playerY);
// --- THE FIX: Read the C-API return code! ---
        int rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "CRITICAL DB ERROR: " << sqlite3_errmsg(m_db) << "\n";
            sqlite3_finalize(stmt);
            rollback();
            return false; // Stop the save process!
        }
        sqlite3_finalize(stmt);

        // 2. Clear old inventory for this save and insert new items (The Bridge)
        sqlite3_exec(m_db, ("DELETE FROM Player_Inventory_Bridge WHERE SaveID = " + std::to_string(data.saveID)).c_str(), nullptr, nullptr, nullptr);

        const char* bridgeSQL = "INSERT INTO Player_Inventory_Bridge (SaveID, ItemID, Quantity) VALUES (?, ?, 1);";
        for (int itemID : data.inventoryItemIDs) {
            if (itemID == -1) continue;
            sqlite3_prepare_v2(m_db, bridgeSQL, -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, data.saveID);
            sqlite3_bind_int(stmt, 2, itemID);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }

        commit(); // All good![cite: 2]
        std::cout << "Auto-Save successful for SaveID: " << data.saveID << "\n";
        return true;
    } catch (...) {
        rollback(); // Something went wrong, revert changes[cite: 2]
        return false;
    }
}
PlayerSaveData DatabaseManager::loadPlayerState(int saveID) {
    PlayerSaveData data{}; // Initialize empty 
    if (!m_db) return data;

    
    const char* playerSQL = "SELECT ClassID, CurrentLevel, CurrentHealth, CurrentXP, DungeonFloor, PlayerX, PlayerY FROM PlayerSaves WHERE SaveID = ?;";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(m_db, playerSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, saveID); // Bind the SaveID
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            data.saveID = saveID;
            data.classID = sqlite3_column_int(stmt, 0);       // Column 0
            data.currentLevel = sqlite3_column_int(stmt, 1);  // Column 1
            data.currentHealth = sqlite3_column_int(stmt, 2); // Column 2
            data.currentXP = sqlite3_column_int(stmt, 3);     // Column 3
            data.dungeonFloor = sqlite3_column_int(stmt, 4);  // Column 4
            data.playerX = sqlite3_column_double(stmt, 5);    // Column 5
            data.playerY = sqlite3_column_double(stmt, 6);    // Column 6
        }
        sqlite3_finalize(stmt);
    }

    // 2. Rebuild the Inventory Array from the Bridge Table

    const char* bridgeSQL = "SELECT ItemID FROM Player_Inventory_Bridge WHERE SaveID = ?;";
    if (sqlite3_prepare_v2(m_db, bridgeSQL, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, saveID);
        
        // Loop through all items linked to this SaveID
    
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            data.inventoryItemIDs.push_back(sqlite3_column_int(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }

    return data;
}