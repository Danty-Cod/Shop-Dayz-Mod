
class DantyzShopUtils
{

    static bool SameClass(string a, string b)
    {
        string la = a;
        string lb = b;
        la.ToLower();
        lb.ToLower();
        return la == lb;
    }
}

class DantyzShopFileSystem
{
    static bool m_DirectoriesChecked = false;

    static void EnsureDirectoriesExist()
    {
        if (m_DirectoriesChecked)
            return;

        
        if (!FileExist("$profile:SimpleModParty"))
        {
            MakeDirectory("$profile:SimpleModParty");
        }
        
        if (!FileExist("$profile:SimpleModParty/Store"))
        {
            MakeDirectory("$profile:SimpleModParty/Store");
        }
        
        if (!FileExist("$profile:SimpleModParty/Store/Categories"))
        {
            MakeDirectory("$profile:SimpleModParty/Store/Categories");
        }
        
        if (!FileExist("$profile:SimpleModParty/Store/Players"))
        {
            MakeDirectory("$profile:SimpleModParty/Store/Players");
        }
        
        if (!FileExist("$profile:SimpleModParty/Store/Insurance"))
        {
            MakeDirectory("$profile:SimpleModParty/Store/Insurance");
        }
        
        if (!FileExist("$profile:SimpleModParty/Store/PlayerVehicles"))
        {
            MakeDirectory("$profile:SimpleModParty/Store/PlayerVehicles");
        }
        
        // Pastas do sistema VIP
        if (!FileExist("$profile:SimpleModParty/Store/Vip"))
        {
            MakeDirectory("$profile:SimpleModParty/Store/Vip");
        }
        
        if (!FileExist("$profile:SimpleModParty/Store/Vip/PlayersVip"))
        {
            MakeDirectory("$profile:SimpleModParty/Store/Vip/PlayersVip");
        }
        
        if (!FileExist("$profile:SimpleModParty/Store/Vip/PlayersVipFreeOnce"))
        {
            MakeDirectory("$profile:SimpleModParty/Store/Vip/PlayersVipFreeOnce");
        }
        
        // Pasta do controle de estoque limitado
        if (!FileExist("$profile:SimpleModParty/Store/Stock"))
        {
            MakeDirectory("$profile:SimpleModParty/Store/Stock");
        }
        
        m_DirectoriesChecked = true;
    }
}
