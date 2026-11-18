class DantyzVipData
{
    string SteamID;
    string ExpirationDate; // Formato: DD/MM/YYYY
    int DaysRemaining;
    int KitID; // ID do kit de roupas
    
    void DantyzVipData(string steamID, int days, int kitID = 1)
    {
        SteamID = steamID;
        DaysRemaining = days;
        KitID = kitID;
        
        // Calcula data de expiração (calendario real)
        int year, month, day;
        DantyzVipDate.AddDays(days, year, month, day);
        
        ExpirationDate = day.ToString() + "/" + month.ToString() + "/" + year.ToString();
    }
    
    bool IsExpired()
    {
        int year, month, day, hour, minute, second;
        GetYearMonthDayUTC(year, month, day);
        
        // Parse da data de expiração
        array<string> parts = new array<string>();
        ExpirationDate.Split("/", parts);
        
        if (parts.Count() != 3)
            return true;
        
        int expDay = parts.Get(0).ToInt();
        int expMonth = parts.Get(1).ToInt();
        int expYear = parts.Get(2).ToInt();
        
        // Compara datas
        if (year > expYear)
            return true;
        if (year == expYear && month > expMonth)
            return true;
        if (year == expYear && month == expMonth && day > expDay)
            return true;
        
        return false;
    }
}

class DantyzVipManager
{
    static ref array<ref DantyzVipData> m_VipList;  // Tornado público para debug
    private static string VIP_FILE_PATH = "$profile:SimpleModParty/Store/Vip/VipRoupas.json";
    private static bool m_Initialized = false;  // Flag para evitar múltiplas inicializações
    
    static void Init()
    {
        if (m_Initialized)
        {
            return;
        }
        
        
        m_VipList = new array<ref DantyzVipData>();
        Load();
        m_Initialized = true;
    }
    
    static void Load()
    {
        
        if (!FileExist(VIP_FILE_PATH))
        {
            // NÃO limpa a lista nem salva - deixa vazio até alguém comprar VIP
            return;
        }
        
        JsonFileLoader<array<ref DantyzVipData>>.JsonLoadFile(VIP_FILE_PATH, m_VipList);
        
        // Debug: Lista todos os VIPs carregados
        for (int i = 0; i < m_VipList.Count(); i++)
        {
            DantyzVipData vip = m_VipList.Get(i);
        }
        
        
        // Remove VIPs expirados
        CleanExpiredVips();
    }
    
    static void Save()
    {
        JsonFileLoader<array<ref DantyzVipData>>.JsonSaveFile(VIP_FILE_PATH, m_VipList);
    }
    
    static void CleanExpiredVips()
    {
        for (int i = m_VipList.Count() - 1; i >= 0; i--)
        {
            DantyzVipData vip = m_VipList.Get(i);
            if (vip.IsExpired())
            {
                m_VipList.Remove(i);
            }
        }
        Save();
    }
    
    static bool HasActiveVip(string steamID)
    {
        for (int i = 0; i < m_VipList.Count(); i++)
        {
            DantyzVipData vip = m_VipList.Get(i);
            if (vip.SteamID == steamID && !vip.IsExpired())
            {
                return true;
            }
        }
        return false;
    }
    
    static void AddVip(string steamID, int days, int kitID = 1)
    {
        // Verifica se já tem VIP ativo
        DantyzVipData existingVip = null;
        int existingIndex = -1;
        DantyzVipData newVip;
        
        for (int i = 0; i < m_VipList.Count(); i++)
        {
            DantyzVipData vip = m_VipList.Get(i);
            if (vip.SteamID == steamID)
            {
                existingVip = vip;
                existingIndex = i;
                break;
            }
        }
        
        // Se já tem VIP ativo (não expirado), SOMA os dias
        if (existingVip && !existingVip.IsExpired())
        {
            // renovando: soma ao que ainda falta de verdade (DaysRemaining salvo era o da compra)
            int left = DantyzVipDate.DaysLeft(existingVip.ExpirationDate);
            if (left < 0)
                left = 0;
            int totalDays = left + days;
            
            
            // Remove o VIP antigo
            m_VipList.Remove(existingIndex);
            
            // Cria novo VIP com dias somados e atualiza o KitID para o maior
            int finalKitID = kitID;
            if (existingVip.KitID > kitID)
            {
                finalKitID = existingVip.KitID;
            }
            
            newVip = new DantyzVipData(steamID, totalDays, finalKitID);
            m_VipList.Insert(newVip);
            Save();
            
            // SALVA TAMBÉM NO NOVO FORMATO (PlayersVip)
            SavePlayerVipFile(steamID, totalDays, finalKitID);
            
        }
        else
        {
            // Se VIP expirado ou não existe, remove o antigo e cria novo
            if (existingIndex >= 0)
            {
                m_VipList.Remove(existingIndex);
            }
            
            // Adiciona novo VIP
            newVip = new DantyzVipData(steamID, days, kitID);
            m_VipList.Insert(newVip);
            Save();
            
            // SALVA TAMBÉM NO NOVO FORMATO (PlayersVip)
            SavePlayerVipFile(steamID, days, kitID);
            
        }
    }
    
    // NOVO MÉTODO: Salva VIP no formato novo (PlayersVip)
    static void SavePlayerVipFile(string steamID, int days, int kitID)
    {
        // Calcula data de expiração no formato ISO 8601
        int year, month, day;
        DantyzVipDate.AddDays(days, year, month, day);
        
        // Formata data ISO 8601
        string expirationDate = year.ToString() + "-";
        if (month < 10) expirationDate += "0";
        expirationDate += month.ToString() + "-";
        if (day < 10) expirationDate += "0";
        expirationDate += day.ToString() + "T23:59:59Z";
        
        // Data de ativação (hoje)
        int actYear, actMonth, actDay;
        GetYearMonthDayUTC(actYear, actMonth, actDay);
        string activatedDate = actYear.ToString() + "-";
        if (actMonth < 10) activatedDate += "0";
        activatedDate += actMonth.ToString() + "-";
        if (actDay < 10) activatedDate += "0";
        activatedDate += actDay.ToString() + "T00:00:00Z";
        
        // Cria objeto VIP no novo formato
        DantyzVipPlayerData vipData = new DantyzVipPlayerData();
        vipData.steamId = steamID;
        vipData.isActive = true;
        vipData.expirationDate = expirationDate;
        vipData.daysRemaining = days;
        vipData.kitType = kitID;
        vipData.activatedDate = activatedDate;
        
        // Salva arquivo individual do player
        string playerVipPath = "$profile:SimpleModParty/Store/Vip/PlayersVip/" + steamID + ".json";
        JsonFileLoader<DantyzVipPlayerData>.JsonSaveFile(playerVipPath, vipData);
        
    }
    
    static DantyzVipData GetVipData(string steamID)
    {
        
        for (int i = 0; i < m_VipList.Count(); i++)
        {
            DantyzVipData vip = m_VipList.Get(i);
            
            if (vip.SteamID == steamID)
            {
                bool expired = vip.IsExpired();
                
                if (!expired)
                {
                    return vip;
                }
                else
                {
                }
            }
        }
        
        return null;
    }
}
