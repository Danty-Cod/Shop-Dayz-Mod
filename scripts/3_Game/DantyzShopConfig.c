class DantyzShopCategory
{
    string CategoryName;
    string FileName;
    string CategoryIcon;
    bool Enabled;
}

// Local do test drive (Config.json -> TestDriveSpots)
class DantyzTestDriveSpot
{
    vector Position;
    float Heading;
}

class DantyzShopConfig
{
    string StoreName;
    string CoinName;
    int StartCurrency;
    int InsuranceWaitTimeSeconds;
    int InsuranceTimeToExpire;
    int InsuranceCostPercent;int InsuranceCostPercentPreviwMap;
    int TrackDurationSeconds;
    int InsuranceReturnPercent;
    int RefundInsuranceOnDestroyed;
    int VehicleRecallDelaySeconds;
    int VehicleRecallCooldownSeconds;
    int EnableMessages;
    int PlaytimeRewardEnabled;
    int PlaytimeRewardMinutes;
    int PlaytimeRewardCoins;
    ref array<string> AdminSteamIDs;
    ref array<ref DantyzShopCategory> Categories;

    // TEST DRIVE (gratis) - botao TESTAR nos carros da loja
    //   TestDriveEnabled         1 = liga / 0 = desliga o botao
    //   TestDriveSeconds         tempo do teste (900 = 15 min)
    //   TestDriveCooldownSeconds      espera para o proximo teste quando usou o TEMPO TODO (conta do fim do teste)
    //   TestDriveEarlyCooldownSeconds espera quando SAIU DO CARRO antes do tempo (conta de quando voltou)
    //   TestDriveSpots           locais onde o carro nasce: Position "x y z" e Heading (direcao 0-360).
    //                            Com mais de um, a loja usa o primeiro que estiver livre.
    int TestDriveEnabled;
    int TestDriveSeconds;
    int TestDriveCooldownSeconds;
    int TestDriveEarlyCooldownSeconds;
    ref array<ref DantyzTestDriveSpot> TestDriveSpots;
    
    void DantyzShopConfig()
    {
        AdminSteamIDs = new array<string>();
        Categories = new array<ref DantyzShopCategory>();
        TestDriveSpots = new array<ref DantyzTestDriveSpot>();
        TestDriveEnabled = 1;
        TestDriveSeconds = 600;
        TestDriveCooldownSeconds = 180;
        TestDriveEarlyCooldownSeconds = 60;
        InsuranceWaitTimeSeconds = 60;
        InsuranceTimeToExpire = 172800;
        InsuranceCostPercent = 25;
        InsuranceCostPercentPreviwMap = 10;
        TrackDurationSeconds = 1800;
        InsuranceReturnPercent = 100;
        RefundInsuranceOnDestroyed = 1;
        VehicleRecallDelaySeconds = 0;
        VehicleRecallCooldownSeconds = 3600;
        EnableMessages = 1;
        PlaytimeRewardEnabled = 1;
        PlaytimeRewardMinutes = 30;
        PlaytimeRewardCoins = 2;
    }
    

    static ref DantyzShopConfig m_Cached;

    static ref DantyzShopConfig Load()
    {
        if (!m_Cached)
            m_Cached = LoadFromDisk();

        return m_Cached;
    }
    static ref DantyzShopConfig Reload()
    {
        m_Cached = LoadFromDisk();
        return m_Cached;
    }
    static void InvalidateCache()
    {
        m_Cached = null;
    }

    protected static ref DantyzShopConfig LoadFromDisk()
    {
        DantyzShopFileSystem.EnsureDirectoriesExist();
        
        string path = "$profile:SimpleModParty/Store/Config.json";
        
        
        DantyzShopConfig config;
        
        if (!FileExist(path))
        {
            config = new DantyzShopConfig();
            config.StoreName = "Dantyz Shop";
            config.CoinName = "ZCoins";
            config.StartCurrency = 15;
            config.InsuranceWaitTimeSeconds = 60;
            config.InsuranceCostPercent = 25;
            config.InsuranceReturnPercent = 100;
            config.AdminSteamIDs = new array<string>();
            config.Categories = new array<ref DantyzShopCategory>();
            return config;
        }
        
        config = new DantyzShopConfig();
        
        JsonFileLoader<DantyzShopConfig>.JsonLoadFile(path, config);
        
        DantyzShopMsg.Enabled = config.EnableMessages;
        
        if (config && config.Categories && config.Categories.Count() > 0)
        {
            
            for (int i = 0; i < config.Categories.Count(); i++)
            {
                DantyzShopCategory cat = config.Categories.Get(i);
                string enabledText;
                if (cat.Enabled)
                    enabledText = "ATIVADA";
                else
                    enabledText = "DESATIVADA";
            }
        }
        else
        {
        }
        
        
        return config;
    }
    
    void Save()
    {
        string path = "$profile:SimpleModParty/Store/Config.json";
        JsonFileLoader<DantyzShopConfig>.JsonSaveFile(path, this);
    }
    
    
    bool IsAdmin(string steamID)
    {
        for (int i = 0; i < AdminSteamIDs.Count(); i++)
        {
            if (AdminSteamIDs.Get(i) == steamID)
            {
                return true;
            }
        }
        return false;
    }
    
    void OnSend(ParamsWriteContext ctx)
    {
        ctx.Write(StoreName);
        ctx.Write(CoinName);
        ctx.Write(StartCurrency);
        ctx.Write(InsuranceWaitTimeSeconds);
        ctx.Write(InsuranceTimeToExpire);
        ctx.Write(InsuranceCostPercent);
        ctx.Write(InsuranceReturnPercent);
        ctx.Write(RefundInsuranceOnDestroyed);
        ctx.Write(VehicleRecallDelaySeconds);
        ctx.Write(VehicleRecallCooldownSeconds);
        ctx.Write(EnableMessages);
        ctx.Write(AdminSteamIDs.Count());
        for (int i = 0; i < AdminSteamIDs.Count(); i++)
        {
            ctx.Write(AdminSteamIDs.Get(i));
        }
        ctx.Write(Categories.Count());
        for (int j = 0; j < Categories.Count(); j++)
        {
            DantyzShopCategory cat = Categories.Get(j);
            ctx.Write(cat.CategoryName);
            ctx.Write(cat.FileName);
            ctx.Write(cat.CategoryIcon);
            ctx.Write(cat.Enabled);
        }
    }
    
    bool OnRecieve(ParamsReadContext ctx)
    {
        if (!ctx.Read(StoreName)) return false;
        if (!ctx.Read(CoinName)) return false;
        if (!ctx.Read(StartCurrency)) return false;
        if (!ctx.Read(InsuranceWaitTimeSeconds)) return false;
        if (!ctx.Read(InsuranceTimeToExpire)) return false;
        if (!ctx.Read(InsuranceCostPercent)) return false;
        if (!ctx.Read(InsuranceReturnPercent)) return false;
        if (!ctx.Read(RefundInsuranceOnDestroyed)) return false;
        if (!ctx.Read(VehicleRecallDelaySeconds)) return false;
        if (!ctx.Read(VehicleRecallCooldownSeconds)) return false;
        if (!ctx.Read(EnableMessages)) return false;
        DantyzShopMsg.Enabled = EnableMessages;
        
        int adminCount;
        if (!ctx.Read(adminCount)) return false;
        AdminSteamIDs.Clear();
        for (int i = 0; i < adminCount; i++)
        {
            string adminId;
            if (!ctx.Read(adminId)) return false;
            AdminSteamIDs.Insert(adminId);
        }
        
        int catCount;
        if (!ctx.Read(catCount)) return false;
        Categories.Clear();
        for (int j = 0; j < catCount; j++)
        {
            DantyzShopCategory cat = new DantyzShopCategory();
            if (!ctx.Read(cat.CategoryName)) return false;
            if (!ctx.Read(cat.FileName)) return false;
            if (!ctx.Read(cat.CategoryIcon)) return false;
            if (!ctx.Read(cat.Enabled)) return false;
            Categories.Insert(cat);
        }
        
        return true;
    }
}

class DantyzShopMsg
{
    static int Enabled = 1;

    // Atalho: retorna true se pode mostrar mensagem do mod.
    static bool On()
    {
        return Enabled != 0;
    }
}
