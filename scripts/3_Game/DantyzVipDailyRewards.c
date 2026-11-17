class DantyzVipDailyRewardClaim
{
    string SteamID;
    int Year;
    int Month;
    int Day;
    string ItemClassname;
    
    void DantyzVipDailyRewardClaim(string steamID, int year, int month, int day, string itemClass)
    {
        SteamID = steamID;
        Year = year;
        Month = month;
        Day = day;
        ItemClassname = itemClass;
    }
}

class DantyzVipDailyRewards
{
    private static ref array<ref DantyzVipDailyRewardClaim> m_Claims;
    private static const string CLAIMS_FILE = "$profile:SimpleModParty/Store/Vip/DailyRewardsClaims.json";
    
    static void Init()
    {
        if (!m_Claims)
        {
            m_Claims = new array<ref DantyzVipDailyRewardClaim>();
            Load();
        }
    }
    
    static void Load()
    {
        if (!FileExist(CLAIMS_FILE))
        {
            m_Claims = new array<ref DantyzVipDailyRewardClaim>();
            Save();
            return;
        }
        
        JsonFileLoader<array<ref DantyzVipDailyRewardClaim>>.JsonLoadFile(CLAIMS_FILE, m_Claims);
        
        CleanOldClaims();
    }
    
    static void Save()
    {
        JsonFileLoader<array<ref DantyzVipDailyRewardClaim>>.JsonSaveFile(CLAIMS_FILE, m_Claims);
    }
    
    static void CleanOldClaims()
    {
        CF_Date currentDate = CF_Date.Now(true);
        int currentYear = currentDate.GetYear();
        int currentMonth = currentDate.GetMonth();
        
        int currentMonthTotal = currentYear * 12 + currentMonth;
        
        for (int i = m_Claims.Count() - 1; i >= 0; i--)
        {
            DantyzVipDailyRewardClaim claim = m_Claims.Get(i);
            int claimMonthTotal = claim.Year * 12 + claim.Month;
            
            if (currentMonthTotal - claimMonthTotal >= 2)
            {
                m_Claims.Remove(i);
            }
        }
    }
    
    static bool CanClaimToday(string steamID, int dayOfMonth, string itemClassname)
    {
        Init();
        
        CF_Date currentDate = CF_Date.Now(true);
        int year = currentDate.GetYear();
        int month = currentDate.GetMonth();
        int day = currentDate.GetDay();
        
        // AJUSTA O DIA DO ITEM BASEADO NO MÊS ATUAL
        int adjustedDay = GetAdjustedDayForItem(dayOfMonth);
        
        
        // LÓGICA RESTRITIVA: APENAS NO DIA EXATO
        if (day != adjustedDay)
        {
            if (day < adjustedDay)
            {
            }
            else
            {
            }
            return false;
        }
        
        // Verifica se já pegou este item neste mês (usando classname original para evitar duplicatas)
        for (int i = 0; i < m_Claims.Count(); i++)
        {
            DantyzVipDailyRewardClaim claim = m_Claims.Get(i);
            
            if (claim.SteamID == steamID && claim.Year == year && claim.Month == month && DantyzShopUtils.SameClass(claim.ItemClassname, itemClassname))
            {
                return false;
            }
        }
        
        return true;
    }
    
    static void RegisterClaim(string steamID, string itemClassname)
    {
        Init();
        
        CF_Date currentDate = CF_Date.Now(true);
        int year = currentDate.GetYear();
        int month = currentDate.GetMonth();
        int day = currentDate.GetDay();
        
        DantyzVipDailyRewardClaim claim = new DantyzVipDailyRewardClaim(steamID, year, month, day, itemClassname);
        m_Claims.Insert(claim);
        
        Save();
        
    }
    
    static bool HasActiveVip(string steamID)
    {
        
        // USA O NOVO SISTEMA OTIMIZADO - Lê diretamente da pasta PlayersVip
        bool hasVip = DantyzVipPlayerChecker.HasActiveVip(steamID);
        
        if (hasVip)
        {
            DantyzVipPlayerData vipData = DantyzVipPlayerChecker.GetVipData(steamID);
            if (vipData)
            {
            }
        }
        else
        {
        }
        
        return hasVip;
    }
    
    static int GetCurrentDayOfMonth()
    {
        CF_Date currentDate = CF_Date.Now(true);
        return currentDate.GetDay();
    }
    
    // Retorna quantos dias tem o mês atual
    static int GetDaysInCurrentMonth()
    {
        CF_Date currentDate = CF_Date.Now(true);
        int month = currentDate.GetMonth();
        int year = currentDate.GetYear();
        
        return GetDaysInMonth(month, year);
    }
    
    // Calcula quantos dias tem um mês específico
    static int GetDaysInMonth(int month, int year)
    {
        switch (month)
        {
            case 1: case 3: case 5: case 7: case 8: case 10: case 12:
                return 31;
            case 4: case 6: case 9: case 11:
                return 30;
            case 2:
                // Verifica ano bissexto
                if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
                    return 29;
                else
                    return 28;
        }
        return 30; // Fallback
    }
    
    // Ajusta o dia do item baseado no mês atual
    static int GetAdjustedDayForItem(int originalDay)
    {
        int daysInMonth = GetDaysInCurrentMonth();
        
        // PRÊMIO ESPECIAL (dia 30) sempre vai para o ÚLTIMO DIA DO MÊS
        if (originalDay == 30)
        {
            return daysInMonth;
        }
        
        // Para outros itens, se o mês tem menos dias que o item original, ajusta
        if (originalDay > daysInMonth)
        {
            return daysInMonth;
        }
        
        return originalDay;
    }
    
    // Verifica se o item já expirou (dia passou)
    static bool IsItemExpired(int dayOfMonth)
    {
        int currentDay = GetCurrentDayOfMonth();
        int adjustedDay = GetAdjustedDayForItem(dayOfMonth);
        
        return (currentDay > adjustedDay);
    }
}