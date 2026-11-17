
class DantyzShopStockEntry
{
    string Key;
    int Remaining;
}

// Estrutura salva no JSON: lista de entradas.
class DantyzShopStockData
{
    ref array<ref DantyzShopStockEntry> Entries;

    void DantyzShopStockData()
    {
        Entries = new array<ref DantyzShopStockEntry>();
    }
}

class DantyzShopStock
{
    protected static const string STOCK_PATH = "$profile:SimpleModParty/Store/Stock/stock.json";
    protected static ref DantyzShopStockData m_Data;

    // Monta a chave unica do item (mesmo criterio da validacao de compra).
    static string MakeKey(DantyzShopItem item)
    {
        return item.Classname + "|" + item.Price.ToString();
    }

    // Carrega o arquivo de estoque na memoria (uma vez).
    protected static void EnsureLoaded()
    {
        if (m_Data)
            return;

        DantyzShopFileSystem.EnsureDirectoriesExist();

        m_Data = new DantyzShopStockData();

        if (FileExist(STOCK_PATH))
        {
            JsonFileLoader<DantyzShopStockData>.JsonLoadFile(STOCK_PATH, m_Data);
        }

        if (!m_Data.Entries)
            m_Data.Entries = new array<ref DantyzShopStockEntry>();
    }

    protected static void SaveData()
    {
        if (!m_Data)
            return;

        JsonFileLoader<DantyzShopStockData>.JsonSaveFile(STOCK_PATH, m_Data);
    }

    protected static DantyzShopStockEntry FindEntry(string key)
    {
        EnsureLoaded();

        string wanted = key;
        wanted.ToLower();

        for (int i = 0; i < m_Data.Entries.Count(); i++)
        {
            DantyzShopStockEntry entry = m_Data.Entries.Get(i);
            if (!entry)
                continue;

            string current = entry.Key;
            current.ToLower();

            if (current == wanted)
                return entry;
        }

        return null;
    }

    static int GetRemaining(DantyzShopItem item)
    {
        if (!item)
            return -1;

        // Sem controle de estoque -> ilimitado.
        if (item.Stock < 0)
            return -1;

        string key = MakeKey(item);
        DantyzShopStockEntry entry = FindEntry(key);

        if (entry)
            return entry.Remaining;

        // Primeira vez: inicializa com o estoque configurado.
        entry = new DantyzShopStockEntry();
        entry.Key = key;
        entry.Remaining = item.Stock;
        m_Data.Entries.Insert(entry);
        SaveData();

        return item.Stock;
    }

    // true se o item tem estoque limitado e ja esgotou (0 unidades).
    static bool IsSoldOut(DantyzShopItem item)
    {
        int remaining = GetRemaining(item);
        if (remaining < 0)
            return false; // ilimitado nunca esgota
        return remaining <= 0;
    }

    static bool Consume(DantyzShopItem item)
    {
        if (!item)
            return true;

        // Ilimitado: nada a controlar.
        if (item.Stock < 0)
            return true;

        string key = MakeKey(item);
        DantyzShopStockEntry entry = FindEntry(key);

        int remaining;
        if (entry)
        {
            remaining = entry.Remaining;
        }
        else
        {
            // Ainda nao rastreado: comeca do valor configurado.
            entry = new DantyzShopStockEntry();
            entry.Key = key;
            entry.Remaining = item.Stock;
            m_Data.Entries.Insert(entry);
            remaining = item.Stock;
        }

        if (remaining <= 0)
        {
            SaveData();
            return false; // esgotado
        }

        entry.Remaining = remaining - 1;
        SaveData();

        return true;
    }
}
