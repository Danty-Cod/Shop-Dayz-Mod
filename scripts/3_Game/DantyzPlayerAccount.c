class DantyzPlayerAccount
{
    string steamid;
    int coins;
    
    void DantyzPlayerAccount()
    {
        coins = 0;
    }
    
    static ref DantyzPlayerAccount Load(string steamID)
    {
        DantyzShopFileSystem.EnsureDirectoriesExist();
        
        string path = "$profile:SimpleModParty/Store/Players/" + steamID + ".json";
        
        DantyzPlayerAccount account = new DantyzPlayerAccount();
        
        if (!FileExist(path))
        {
            account.steamid = steamID;
            
            DantyzShopConfig config = DantyzShopConfig.Load();
            account.coins = config.StartCurrency;
            
            account.Save();
            return account;
        }
        
        JsonFileLoader<DantyzPlayerAccount>.JsonLoadFile(path, account);
        

        return account;
    }
    
    void Save()
    {
        string path = "$profile:SimpleModParty/Store/Players/" + steamid + ".json";
        JsonFileLoader<DantyzPlayerAccount>.JsonSaveFile(path, this);

    }
    
    bool CanAfford(int price)
    {
        return coins >= price;
    }
    
    bool Purchase(int price)
    {
        if (!CanAfford(price))
        {
            return false;
        }
        
        coins = coins - price;
        Save();
        return true;
    }
    
    void AddCoins(int amount)
    {
        coins = coins + amount;
        Save();
    }
}
