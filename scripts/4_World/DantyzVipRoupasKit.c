class DantyzVipRoupasClothingItem
{
    string SlotName;
    string Classname;
    string DisplayName;
}

class DantyzVipRoupasBonusItem
{
    string Classname;
    int Quantity;
    string DisplayName;
}

class DantyzVipRoupasKit
{
    int KitID;
    string KitName;
    string Description;
    ref array<ref DantyzVipRoupasClothingItem> ClothingItems;
    ref array<ref DantyzVipRoupasBonusItem> BonusItems;
    
    void DantyzVipRoupasKit()
    {
        ClothingItems = new array<ref DantyzVipRoupasClothingItem>();
        BonusItems = new array<ref DantyzVipRoupasBonusItem>();
    }
}

class DantyzVipRoupasKitManager
{
    private static ref array<ref DantyzVipRoupasKit> m_Kits;
    
    static void Init()
    {
        LoadKits();
    }
    
    static void LoadKits()
    {
        m_Kits = new array<ref DantyzVipRoupasKit>();
        
        // Lê do VipRoupasKits.json
        string path = "$profile:SimpleModParty/Store/Vip/VipRoupasKits.json";
        
        if (!FileExist(path))
        {
            string modPath = "DantyzShop/Store/Vip/VipRoupasKits.json";
            CopyFile(modPath, path);
        }
        
        JsonFileLoader<array<ref DantyzVipRoupasKit>>.JsonLoadFile(path, m_Kits);
        
        
        for (int i = 0; i < m_Kits.Count(); i++)
        {
            DantyzVipRoupasKit kit = m_Kits.Get(i);
        }
    }
    
    static DantyzVipRoupasKit GetKitByID(int kitID)
    {
        if (!m_Kits)
        {
            LoadKits();
        }
        
        for (int i = 0; i < m_Kits.Count(); i++)
        {
            DantyzVipRoupasKit kit = m_Kits.Get(i);
            if (kit.KitID == kitID)
            {
                return kit;
            }
        }
        
        return null;
    }
    
    static void EquipKitOnPlayer(PlayerBase player, int kitID)
    {
        if (!player)
        {
            return;
        }
        
        DantyzVipRoupasKit kit = GetKitByID(kitID);
        
        if (!kit)
        {
            return;
        }
        
        
        // Remove roupas atuais
        RemovePlayerClothes(player);
        
        // Equipa cada roupa
        for (int i = 0; i < kit.ClothingItems.Count(); i++)
        {
            DantyzVipRoupasClothingItem clothingItem = kit.ClothingItems.Get(i);
            
            EntityAI entity = player.GetInventory().CreateInInventory(clothingItem.Classname);
            
            if (entity)
            {
            }
            else
            {
            }
        }
        
        // Adiciona itens bônus
        GiveBonusItems(player, kitID);
        
    }

    // Entrega SO' os itens bonus do kit (sem mexer nas roupas). Usado tanto
    // pelo kit fixo quanto quando o player usa a selecao propria de roupas.
    static void GiveBonusItems(PlayerBase player, int kitID)
    {
        if (!player)
            return;

        DantyzVipRoupasKit kit = GetKitByID(kitID);
        if (!kit)
            return;

        for (int j = 0; j < kit.BonusItems.Count(); j++)
        {
            DantyzVipRoupasBonusItem bonusItem = kit.BonusItems.Get(j);
            for (int q = 0; q < bonusItem.Quantity; q++)
            {
                EntityAI bonusEntity = player.GetInventory().CreateInInventory(bonusItem.Classname);
            }
        }
    }
    
    static void RemovePlayerClothes(PlayerBase player)
    {
        
        // Lista de slots de roupa
        array<string> clothingSlots = {
            "Body",
            "Legs",
            "Feet",
            "Gloves",
            "Headgear",
            "Vest",
            "Back",
            "Hips",
            "Eyewear",
            "Mask"
        };
        
        for (int i = 0; i < clothingSlots.Count(); i++)
        {
            EntityAI item = player.FindAttachmentBySlotName(clothingSlots.Get(i));
            if (item)
            {
                player.GetInventory().LocalDestroyEntity(item);
            }
        }
    }
}

class DantyzVipRoupasSelectionHelper
{
   
    static bool IsAllowed(DantyzVipRoupasConfig config, string category, string classname)
    {
        if (classname == "")
            return true;
        if (!config)
            return false;

        array<ref DantyzRoupaItem> list = GetCategoryList(config, category);
        if (!list)
            return false;

        for (int i = 0; i < list.Count(); i++)
        {
            // Sem caixa: o classname vem do cliente/JSON e o DayZ nao diferencia
            // maiuscula de minuscula em classname.
            if (DantyzShopUtils.SameClass(list.Get(i).Classname, classname))
                return true;
        }
        return false;
    }

    static array<ref DantyzRoupaItem> GetCategoryList(DantyzVipRoupasConfig config, string category)
    {
        if (category == "Tops")           return config.Tops;
        else if (category == "Pants")     return config.Pants;
        else if (category == "Boots")     return config.Boots;
        else if (category == "Gloves")    return config.Gloves;
        else if (category == "Helmets")   return config.Helmets;
        else if (category == "Vests")     return config.Vests;
        else if (category == "Backpacks") return config.Backpacks;
        else if (category == "Belts")     return config.Belts;
        return null;
    }

    // Valida a selecao inteira contra o config (filtra o que nao for permitido).
    static void Sanitize(DantyzVipRoupasSelection sel, DantyzVipRoupasConfig config)
    {
        if (!IsAllowed(config, "Tops", sel.Tops))           sel.Tops = "";
        if (!IsAllowed(config, "Pants", sel.Pants))         sel.Pants = "";
        if (!IsAllowed(config, "Boots", sel.Boots))         sel.Boots = "";
        if (!IsAllowed(config, "Gloves", sel.Gloves))       sel.Gloves = "";
        if (!IsAllowed(config, "Helmets", sel.Helmets))     sel.Helmets = "";
        if (!IsAllowed(config, "Vests", sel.Vests))         sel.Vests = "";
        if (!IsAllowed(config, "Backpacks", sel.Backpacks)) sel.Backpacks = "";
        if (!IsAllowed(config, "Belts", sel.Belts))         sel.Belts = "";
    }

    // Equipa a selecao no player (server-side). Remove as roupas atuais e veste
    // as escolhidas. Usa CreateInInventory que coloca direto no slot certo.
    static void EquipSelection(PlayerBase player, DantyzVipRoupasSelection sel)
    {
        if (!player || !sel)
            return;

        DantyzVipRoupasKitManager.RemovePlayerClothes(player);

        EquipOne(player, sel.Tops);
        EquipOne(player, sel.Pants);
        EquipOne(player, sel.Boots);
        EquipOne(player, sel.Gloves);
        EquipOne(player, sel.Helmets);
        EquipOne(player, sel.Vests);
        EquipOne(player, sel.Backpacks);
        EquipOne(player, sel.Belts);
    }

    static void EquipOne(PlayerBase player, string classname)
    {
        if (classname == "")
            return;

        EntityAI ent = player.GetInventory().CreateInInventory(classname);
    }
}
