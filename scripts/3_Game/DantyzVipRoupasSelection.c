
class DantyzVipRoupasSelection
{
    string SteamID;
    // Tipo de personagem escolhido (ex: "SurvivorM_Mirek"). "" = padrao do jogo.
    string CharacterType;
    // Classname escolhido por categoria ("" = nao escolheu / usar nada nesse slot)
    string Tops;
    string Pants;
    string Boots;
    string Gloves;
    string Helmets;
    string Vests;
    string Backpacks;
    string Belts;

    void DantyzVipRoupasSelection()
    {
        SteamID = "";
        CharacterType = "";
        Tops = "";
        Pants = "";
        Boots = "";
        Gloves = "";
        Helmets = "";
        Vests = "";
        Backpacks = "";
        Belts = "";
    }

    static string GetPath(string steamID)
    {
        return "$profile:SimpleModParty/Store/Vip/PlayersRoupas/" + steamID + ".json";
    }

    static bool HasSelection(string steamID)
    {
        return FileExist(GetPath(steamID));
    }

    static ref DantyzVipRoupasSelection Load(string steamID)
    {
        DantyzVipRoupasSelection sel = new DantyzVipRoupasSelection();
        sel.SteamID = steamID;

        string path = GetPath(steamID);
        if (FileExist(path))
            JsonFileLoader<DantyzVipRoupasSelection>.JsonLoadFile(path, sel);

        return sel;
    }

    void Save()
    {
        string dir = "$profile:SimpleModParty/Store/Vip/PlayersRoupas/";
        if (!FileExist(dir))
            MakeDirectory(dir);

        JsonFileLoader<DantyzVipRoupasSelection>.JsonSaveFile(GetPath(SteamID), this);
    }

    void SetByCategory(string category, string classname)
    {
        if (category == "Tops")           Tops = classname;
        else if (category == "Pants")     Pants = classname;
        else if (category == "Boots")     Boots = classname;
        else if (category == "Gloves")    Gloves = classname;
        else if (category == "Helmets")   Helmets = classname;
        else if (category == "Vests")     Vests = classname;
        else if (category == "Backpacks") Backpacks = classname;
        else if (category == "Belts")     Belts = classname;
    }

    // Retorna true se ha pelo menos uma peca escolhida.
    bool HasAny()
    {
        return (Tops != "" || Pants != "" || Boots != "" || Gloves != "" || Helmets != "" || Vests != "" || Backpacks != "" || Belts != "");
    }

    // Serializacao manual pra passar por RPC (DayZ exige OnSend/OnRecieve).
    void OnSend(ParamsWriteContext ctx)
    {
        ctx.Write(SteamID);
        ctx.Write(CharacterType);
        ctx.Write(Tops);
        ctx.Write(Pants);
        ctx.Write(Boots);
        ctx.Write(Gloves);
        ctx.Write(Helmets);
        ctx.Write(Vests);
        ctx.Write(Backpacks);
        ctx.Write(Belts);
    }

    bool OnRecieve(ParamsReadContext ctx)
    {
        if (!ctx.Read(SteamID)) return false;
        if (!ctx.Read(CharacterType)) return false;
        if (!ctx.Read(Tops)) return false;
        if (!ctx.Read(Pants)) return false;
        if (!ctx.Read(Boots)) return false;
        if (!ctx.Read(Gloves)) return false;
        if (!ctx.Read(Helmets)) return false;
        if (!ctx.Read(Vests)) return false;
        if (!ctx.Read(Backpacks)) return false;
        if (!ctx.Read(Belts)) return false;
        return true;
    }
}
