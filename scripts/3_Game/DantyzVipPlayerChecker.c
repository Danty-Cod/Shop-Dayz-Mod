class DantyzVipPlayerData
{
    string steamId;
    bool isActive;
    string expirationDate;
    int daysRemaining;
    int kitType;
    string activatedDate;
    
    void DantyzVipPlayerData()
    {
        steamId = "";
        isActive = false;
        expirationDate = "";
        daysRemaining = 0;
        kitType = 0;
        activatedDate = "";
    }
}

class DantyzVipPlayerChecker
{
    static bool HasActiveVip(string steamID)
    {
        string playerVipPath = "$profile:SimpleModParty/Store/Vip/PlayersVip/" + steamID + ".json";
        
        if (!FileExist(playerVipPath))
        {
            return false;
        }
        
        DantyzVipPlayerData vipData = LoadPlayerVipData(steamID);
        if (!vipData)
        {
            return false;
        }
        
        bool hasVip = vipData.isActive && vipData.daysRemaining > 0;
        
        return hasVip;
    }
    
    static DantyzVipPlayerData LoadPlayerVipData(string steamID)
    {
        string playerVipPath = "$profile:SimpleModParty/Store/Vip/PlayersVip/" + steamID + ".json";
        
        if (!FileExist(playerVipPath))
        {
            return null;
        }
        
        DantyzVipPlayerData vipData = new DantyzVipPlayerData();
        
        JsonFileLoader<DantyzVipPlayerData>.JsonLoadFile(playerVipPath, vipData);
        
        if (!vipData)
        {
            return null;
        }
        
        // dias restantes sempre pela data de expiracao (o valor salvo era o da compra e nao diminuia)
        if (DantyzVipDate.DaysLeft(vipData.expirationDate) < 0)
        {
            vipData.isActive = false;
            vipData.daysRemaining = 0;
        }
        else
        {
            vipData.daysRemaining = DantyzVipDate.DisplayDays(vipData.expirationDate);
        }
        
        return vipData;
    }
    
    static DantyzVipPlayerData GetVipData(string steamID)
    {
        return LoadPlayerVipData(steamID);
    }
    
    static bool CanClaimVipItem(string steamID, int dayOfMonth, string itemClassname)
    {
        if (!HasActiveVip(steamID))
        {
            return false;
        }
        
        CF_Date currentDate = CF_Date.Now(true);
        int year = currentDate.GetYear();
        int month = currentDate.GetMonth();
        int currentDay = currentDate.GetDay();
        
        int daysInMonth = GetDaysInMonth(month, year);
        int adjustedDay = dayOfMonth;
        
        if (dayOfMonth == 30)
        {
            adjustedDay = daysInMonth;
        }
        else if (dayOfMonth > daysInMonth)
        {
            adjustedDay = daysInMonth;
        }
        
        if (currentDay != adjustedDay)
        {
            return false;
        }
        
        string claimsPath = "$profile:SimpleModParty/Store/Vip/DailyRewardsClaims.json";
        
        if (FileExist(claimsPath))
        {
            array<ref DantyzVipDailyRewardClaim> claims = new array<ref DantyzVipDailyRewardClaim>;
            
            JsonFileLoader<array<ref DantyzVipDailyRewardClaim>>.JsonLoadFile(claimsPath, claims);
            
            int claimsCount = claims.Count();
            for (int i = 0; i < claimsCount; i++)
            {
                DantyzVipDailyRewardClaim claim = claims.Get(i);
                
                if (claim.SteamID == steamID && claim.Year == year && claim.Month == month && DantyzShopUtils.SameClass(claim.ItemClassname, itemClassname))
                {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    static int GetDaysInMonth(int month, int year)
    {
        switch (month)
        {
            case 1:
                return 31;
            case 2:
            {
                if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
                {
                    return 29;
                }
                return 28;
            }
            case 3:
                return 31;
            case 4:
                return 30;
            case 5:
                return 31;
            case 6:
                return 30;
            case 7:
                return 31;
            case 8:
                return 31;
            case 9:
                return 30;
            case 10:
                return 31;
            case 11:
                return 30;
            case 12:
                return 31;
        }
        
        return 30;
    }
}
