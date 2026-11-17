class DantyzVipFilaPlayerData
{
    string steamId;
    bool isActive;
    string expirationDate;   // ISO 8601 - "2026-06-01T23:59:59Z"
    int daysRemaining;
    string activatedDate;    // ISO 8601

    void DantyzVipFilaPlayerData()
    {
        steamId = "";
        isActive = false;
        expirationDate = "";
        daysRemaining = 0;
        activatedDate = "";
    }
}

class DantyzVipFilaManager
{

    private static const string FOLDER_PLAYERS = "$profile:SimpleModParty/Store/Vip/PlayersVipFila";
    private static const string CONFIG_PATH    = "$profile:SimpleModParty/Store/Vip/VipFilaConfig.json";
    private static const string DEFAULT_PRIORITY_PATH = "$profile:priority.txt";
    private static bool   m_Initialized = false;
    private static string m_PriorityFilePath;

    static void Init()
    {
        if (m_Initialized)
        {
            return;
        }


        EnsureDirectories();
        LoadOrCreateConfig();
        CleanExpiredAndSync();

        // Verifica a cada 1 hora
        GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(
            DantyzVipFilaManager.CleanExpiredAndSync, 3600 * 1000, true);

        m_Initialized = true;
    }

    private static void EnsureDirectories()
    {
        if (!FileExist("$profile:SimpleModParty"))
            MakeDirectory("$profile:SimpleModParty");
        if (!FileExist("$profile:SimpleModParty/Store"))
            MakeDirectory("$profile:SimpleModParty/Store");
        if (!FileExist("$profile:SimpleModParty/Store/Vip"))
            MakeDirectory("$profile:SimpleModParty/Store/Vip");
        if (!FileExist(FOLDER_PLAYERS))
            MakeDirectory(FOLDER_PLAYERS);
    }
    private static void LoadOrCreateConfig()
    {
        DantyzVipFilaConfig cfg = new DantyzVipFilaConfig();

        if (FileExist(CONFIG_PATH))
        {
            JsonFileLoader<DantyzVipFilaConfig>.JsonLoadFile(CONFIG_PATH, cfg);
        }
        else
        {
            cfg.PriorityFilePath = DEFAULT_PRIORITY_PATH;
            JsonFileLoader<DantyzVipFilaConfig>.JsonSaveFile(CONFIG_PATH, cfg);
        }

        if (cfg.PriorityFilePath == "")
            cfg.PriorityFilePath = DEFAULT_PRIORITY_PATH;

        m_PriorityFilePath = cfg.PriorityFilePath;
    }
    static bool HasActiveVipFila(string steamID)
    {
        DantyzVipFilaPlayerData data = LoadPlayerData(steamID);
        if (!data) return false;
        if (!data.isActive) return false;
        if (data.daysRemaining <= 0) return false;
        if (IsExpired(data.expirationDate)) return false;
        return true;
    }

    static DantyzVipFilaPlayerData GetVipFilaData(string steamID)
    {
        return LoadPlayerData(steamID);
    }

    private static DantyzVipFilaPlayerData LoadPlayerData(string steamID)
    {
        string path = FOLDER_PLAYERS + "/" + steamID + ".json";
        if (!FileExist(path)) return null;

        DantyzVipFilaPlayerData data = new DantyzVipFilaPlayerData();
        JsonFileLoader<DantyzVipFilaPlayerData>.JsonLoadFile(path, data);
        return data;
    }

    private static void SavePlayerData(DantyzVipFilaPlayerData data)
    {
        string path = FOLDER_PLAYERS + "/" + data.steamId + ".json";
        JsonFileLoader<DantyzVipFilaPlayerData>.JsonSaveFile(path, data);
    }

    private static void DeletePlayerData(string steamID)
    {
        string path = FOLDER_PLAYERS + "/" + steamID + ".json";
        if (FileExist(path))
            DeleteFile(path);
    }
    static void AddVipFila(string steamID, int days)
    {
        if (!m_Initialized) Init();


        DantyzVipFilaPlayerData existing = LoadPlayerData(steamID);
        int totalDays = days;

        if (existing && existing.isActive && !IsExpired(existing.expirationDate))
        {
            totalDays = existing.daysRemaining + days;
        }
        else
        {
        }

        DantyzVipFilaPlayerData newData = new DantyzVipFilaPlayerData();
        newData.steamId        = steamID;
        newData.isActive       = true;
        newData.daysRemaining  = totalDays;
        newData.activatedDate  = NowIsoDate(false);
        newData.expirationDate = AddDaysIso(totalDays);

        SavePlayerData(newData);

        // Atualiza priority.txt imediatamente
        SyncPriorityFile();

    }

    // Remove um player especifico (usado em comandos admin / expiracao)
    static void RemoveVipFila(string steamID)
    {
        DeletePlayerData(steamID);
        SyncPriorityFile();
    }
    static void CleanExpiredAndSync()
    {

        if (!FileExist(FOLDER_PLAYERS))
        {
            SyncPriorityFile(); // garante priority.txt vazio
            return;
        }

        string fileName;
        FileAttr attr;
        FindFileHandle h = FindFile(FOLDER_PLAYERS + "/*.json", fileName, attr, FindFileFlags.ALL);
        bool found = (h != 0);

        while (found)
        {
            if (fileName != "" && fileName != "." && fileName != "..")
            {
                // Pega o steamID a partir do nome do arquivo (remove .json)
                string steamID = fileName;
                int dot = steamID.IndexOf(".");
                if (dot > 0) steamID = steamID.Substring(0, dot);

                DantyzVipFilaPlayerData data = LoadPlayerData(steamID);
                if (data && IsExpired(data.expirationDate))
                {
                    DeletePlayerData(steamID);
                }
            }
            found = FindNextFile(h, fileName, attr);
        }

        if (h != 0) CloseFindFile(h);

        SyncPriorityFile();
    }
    static void SyncPriorityFile()
    {
        if (m_PriorityFilePath == "")
            m_PriorityFilePath = DEFAULT_PRIORITY_PATH;

        array<string> ids = new array<string>();

        if (FileExist(FOLDER_PLAYERS))
        {
            string fileName;
            FileAttr attr;
            FindFileHandle h = FindFile(FOLDER_PLAYERS + "/*.json", fileName, attr, FindFileFlags.ALL);
            bool found = (h != 0);

            while (found)
            {
                if (fileName != "" && fileName != "." && fileName != "..")
                {
                    string steamID = fileName;
                    int dot = steamID.IndexOf(".");
                    if (dot > 0) steamID = steamID.Substring(0, dot);

                    DantyzVipFilaPlayerData data = LoadPlayerData(steamID);
                    if (data && data.isActive && !IsExpired(data.expirationDate) && data.daysRemaining > 0)
                    {
                        ids.Insert(steamID);
                    }
                }
                found = FindNextFile(h, fileName, attr);
            }

            if (h != 0) CloseFindFile(h);
        }
        string content = "";
        for (int i = 0; i < ids.Count(); i++)
        {
            if (i > 0) content += ";";
            content += ids[i];
        }
        FileHandle fh = OpenFile(m_PriorityFilePath, FileMode.WRITE);
        if (fh == 0)
        {

            string fallback = "$profile:priority.txt";
            fh = OpenFile(fallback, FileMode.WRITE);
            if (fh == 0)
            {
                return;
            }
            FPrint(fh, content);
            CloseFile(fh);
            return;
        }
        FPrint(fh, content);
        CloseFile(fh);

    }
    private static string NowIsoDate(bool endOfDay)
    {
        int year, month, day;
        GetYearMonthDayUTC(year, month, day);
        return BuildIso(year, month, day, endOfDay);
    }

    private static string AddDaysIso(int days)
    {
        int year, month, day;
        GetYearMonthDayUTC(year, month, day);
        day += days;
        while (day > 30)
        {
            day -= 30;
            month++;
            if (month > 12)
            {
                month = 1;
                year++;
            }
        }
        return BuildIso(year, month, day, true);
    }

    private static string BuildIso(int year, int month, int day, bool endOfDay)
    {
        string s = year.ToString() + "-";
        if (month < 10) s += "0";
        s += month.ToString() + "-";
        if (day < 10) s += "0";
        s += day.ToString();
        if (endOfDay) s += "T23:59:59Z";
        else          s += "T00:00:00Z";
        return s;
    }

    private static bool IsExpired(string isoDate)
    {
        if (isoDate == "") return true;

        // Pega so a parte da data: "YYYY-MM-DD"
        int tIdx = isoDate.IndexOf("T");
        string datePart = isoDate;
        if (tIdx > 0) datePart = isoDate.Substring(0, tIdx);

        array<string> parts = new array<string>();
        datePart.Split("-", parts);
        if (parts.Count() < 3) return true;

        int expY = parts[0].ToInt();
        int expM = parts[1].ToInt();
        int expD = parts[2].ToInt();

        int y, m, d;
        GetYearMonthDayUTC(y, m, d);

        if (y > expY) return true;
        if (y == expY && m > expM) return true;
        if (y == expY && m == expM && d > expD) return true;
        return false;
    }
}

class DantyzVipFilaConfig
{
    string PriorityFilePath;

    void DantyzVipFilaConfig()
    {
        PriorityFilePath = "";
    }
}
