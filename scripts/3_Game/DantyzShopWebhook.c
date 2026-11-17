class DantyzShopWebhook
{

    static const string URL_COMPRAS = "https://discord.com/api/webhooks/1518431799158771743/n62V0Nfbxsp5k0o9qDpKCQsZnWHat49WRy7MZLl98cw0ewMelu004TsubUarNzV4xwmX";
    // 2) Seguros comprados
    static const string URL_SEGURO_COMPRADO = "https://discord.com/api/webhooks/1518431980591513692/d747HK3j03kIm0JmmWOPcxB3dguPOiOAU7tiTI2W-IyiZzhjdWtfGXn94Wwf9-7jXtz2";
    // 3) Seguros usados (carro destruido -> moedas devolvidas)
    static const string URL_SEGURO_USADO = "https://discord.com/api/webhooks/1518432177736388758/djPvUMRvpy7NdiBQdePfyk3VG33WefGFAY9OEWaZSZyBKA4rRY0KhEK7X8cRjGVzvjpH";

    static const int COR_COMPRA  = 3447003;  // azul
    static const int COR_SEGURO  = 3066993;  // verde
    static const int COR_USADO   = 15158332; // vermelho


    static void SendCompra(string playerName, string steamID, string itemName, int price)
    {
        string desc = "**Item:** " + JsonEscape(itemName) + "\\n";
        desc += "**Preco:** " + price.ToString() + " ZCoins";
        SendEmbed(URL_COMPRAS, "Compra na Loja", COR_COMPRA, playerName, steamID, desc);
    }

    static void SendSeguroComprado(string playerName, string steamID, string vehicleName, int cost)
    {
        string desc = "**Veiculo:** " + JsonEscape(vehicleName) + "\\n";
        desc += "**Custo do seguro:** " + cost.ToString() + " ZCoins";
        SendEmbed(URL_SEGURO_COMPRADO, "Seguro Comprado", COR_SEGURO, playerName, steamID, desc);
    }

    static void SendSeguroUsado(string playerName, string steamID, string vehicleName, int returnAmount)
    {
        string desc = "**Veiculo destruido:** " + JsonEscape(vehicleName) + "\\n";
        desc += "**Moedas devolvidas:** " + returnAmount.ToString() + " ZCoins";
        SendEmbed(URL_SEGURO_USADO, "Seguro Usado", COR_USADO, playerName, steamID, desc);
    }


    static void SendEmbed(string url, string title, int color, string playerName, string steamID, string description)
    {
        if (!GetGame() || !GetGame().IsServer())
            return;

        if (url == "")
            return;

        string footer = "Player: " + JsonEscape(playerName) + " | SteamID: " + steamID + " | " + GetTimeStamp();

        string json = "{ \"embeds\": [ {";
        json += "\"type\": \"rich\",";
        json += "\"title\": \"" + JsonEscape(title) + "\",";
        json += "\"color\": " + color.ToString() + ",";
        json += "\"description\": \"" + description + "\",";
        json += "\"footer\": { \"text\": \"" + footer + "\" }";
        json += "} ] }";

        RestContext ctx = GetRestApi().GetRestContext(url);
        if (!ctx)
            return;

        ctx.SetHeader("application/json");
        ctx.POST(null, "", json);
    }

    // data/hora real do servidor: DD/MM/AAAA HH:MM:SS
    static string GetTimeStamp()
    {
        int y, mo, d, h, mi, s;
        GetYearMonthDay(y, mo, d);
        GetHourMinuteSecond(h, mi, s);

        string ds = Pad2(d) + "/" + Pad2(mo) + "/" + y.ToString();
        string ts = Pad2(h) + ":" + Pad2(mi) + ":" + Pad2(s);
        return ds + " " + ts;
    }

    static string Pad2(int v)
    {
        if (v < 10)
            return "0" + v.ToString();
        return v.ToString();
    }

    static string JsonEscape(string s)
    {
        if (s == "")
            return s;

        s.Replace("\\", "/");   // barra invertida -> barra normal
        s.Replace("\"", "'");   // aspas duplas -> aspas simples
        s.Replace("\n", " ");   // quebra de linha -> espaco
        s.Replace("\r", " ");
        return s;
    }
}
