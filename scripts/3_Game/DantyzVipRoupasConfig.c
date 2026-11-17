class DantyzRoupaItem
{
    string Name;
    string Classname;
}

class DantyzVipRoupasConfig
{
    string LogoImage;
    string BackgroundImage;
    string VipIcon;
    ref array<ref DantyzRoupaItem> Tops;
    ref array<ref DantyzRoupaItem> Pants;
    ref array<ref DantyzRoupaItem> Boots;
    ref array<ref DantyzRoupaItem> Gloves;
    ref array<ref DantyzRoupaItem> Helmets;
    ref array<ref DantyzRoupaItem> Vests;
    ref array<ref DantyzRoupaItem> Backpacks;
    ref array<ref DantyzRoupaItem> Belts;
    
    void DantyzVipRoupasConfig()
    {
        Tops = new array<ref DantyzRoupaItem>();
        Pants = new array<ref DantyzRoupaItem>();
        Boots = new array<ref DantyzRoupaItem>();
        Gloves = new array<ref DantyzRoupaItem>();
        Helmets = new array<ref DantyzRoupaItem>();
        Vests = new array<ref DantyzRoupaItem>();
        Backpacks = new array<ref DantyzRoupaItem>();
        Belts = new array<ref DantyzRoupaItem>();
    }
    
    static ref DantyzVipRoupasConfig Load()
    {
        string path = "$profile:SimpleModParty/Store/Vip/VipRoupasConfig.json";
        DantyzVipRoupasConfig config = new DantyzVipRoupasConfig();

        string dir = "$profile:SimpleModParty/Store/Vip/";
        if (!FileExist(dir))
            MakeDirectory(dir);

        // Se nao existe, copia o template do mod.
        if (!FileExist(path))
        {
            string modPath = "DantyzShop/Store/Vip/VipRoupasConfig.json";
            if (FileExist(modPath))
                CopyFile(modPath, path);
        }

        if (!FileExist(path))
        {
            return config;
        }

        // Le manualmente (JsonSerializer NAO crasha em JSON invalido, so' retorna
        // false). Isso evita a Virtual Machine Exception que derrubava o server.
        FileHandle file = OpenFile(path, FileMode.READ);
        if (file == 0)
        {
            return config;
        }

        string jsonContent = "";
        string line = "";
        while (FGets(file, line) >= 0)
            jsonContent += line;
        CloseFile(file);

        JsonSerializer js = new JsonSerializer();
        string err = "";
        if (!js.ReadFromString(config, jsonContent, err))
        {
            // Retorna config vazio mas valido (nao crasha). Corrija o JSON.
            return new DantyzVipRoupasConfig();
        }

        return config;
    }

    // Serializacao manual pra passar por RPC (DayZ exige OnSend/OnRecieve).
    void OnSend(ParamsWriteContext ctx)
    {
        WriteList(ctx, Tops);
        WriteList(ctx, Pants);
        WriteList(ctx, Boots);
        WriteList(ctx, Gloves);
        WriteList(ctx, Helmets);
        WriteList(ctx, Vests);
        WriteList(ctx, Backpacks);
        WriteList(ctx, Belts);
    }

    bool OnRecieve(ParamsReadContext ctx)
    {
        if (!ReadList(ctx, Tops)) return false;
        if (!ReadList(ctx, Pants)) return false;
        if (!ReadList(ctx, Boots)) return false;
        if (!ReadList(ctx, Gloves)) return false;
        if (!ReadList(ctx, Helmets)) return false;
        if (!ReadList(ctx, Vests)) return false;
        if (!ReadList(ctx, Backpacks)) return false;
        if (!ReadList(ctx, Belts)) return false;
        return true;
    }

    void WriteList(ParamsWriteContext ctx, array<ref DantyzRoupaItem> list)
    {
        int n = 0;
        if (list)
            n = list.Count();
        ctx.Write(n);
        for (int i = 0; i < n; i++)
        {
            ctx.Write(list.Get(i).Name);
            ctx.Write(list.Get(i).Classname);
        }
    }

    bool ReadList(ParamsReadContext ctx, array<ref DantyzRoupaItem> list)
    {
        if (!list)
            return false;
        list.Clear();
        int n = 0;
        if (!ctx.Read(n)) return false;
        for (int i = 0; i < n; i++)
        {
            DantyzRoupaItem it = new DantyzRoupaItem();
            if (!ctx.Read(it.Name)) return false;
            if (!ctx.Read(it.Classname)) return false;
            list.Insert(it);
        }
        return true;
    }
}
