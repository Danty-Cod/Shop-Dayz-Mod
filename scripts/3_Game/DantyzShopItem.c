class DantyzShopItem
{
    string Name;
    string Classname;
    int Price;
    int Quantity;
    int QuantityDrop;
    string Description;  // Descrição que aparece no tooltip
    string TooltipActive;  // "true" = ativa tooltip, "" = desativa
    int KitID;  // ID do kit VIP (usado para VIP Roupas)
    int Days;   // Quantidade de dias (usado para VIP Fura-Fila e qualquer outro VIP por tempo)
    string IconPath;  // Caminho do ícone customizado (usado para VIP)
    bool IsVipFreeItem;  // true = item grátis para VIP (1 por dia)
    int DayOfMonth;  // Dia do mês que este item está disponível (1-30)
    int Stock;  // Estoque limitado. -1 = ilimitado (padrao). Ex: 1 = so' 1 unidade; some da loja ao esgotar
    ref array<ref DantyzShopAttachment> Attachments;
    ref array<ref DantyzShopAttachment> Ground;
    ref array<ref DantyzShopAttachment> CargoItems;  // Itens no cargo ao COMPRAR o veiculo
    ref array<ref DantyzShopAttachment> RecallCargoItems;  // Itens no cargo ao PUXAR o veiculo (seguro/recall)
    
    void DantyzShopItem()
    {
        Quantity = -1;
        QuantityDrop = 1;
        Description = "";
        TooltipActive = "";
        KitID = 0;
        Days = 0;
        IconPath = "";
        IsVipFreeItem = false;
        DayOfMonth = 0;
        Stock = -1;  
        Attachments = new array<ref DantyzShopAttachment>();
        Ground = new array<ref DantyzShopAttachment>();
        CargoItems = new array<ref DantyzShopAttachment>();
        RecallCargoItems = new array<ref DantyzShopAttachment>();
    }


    DantyzShopItem Clone()
    {
        DantyzShopItem copy = new DantyzShopItem();

        copy.Name = Name;
        copy.Classname = Classname;
        copy.Price = Price;
        copy.Quantity = Quantity;
        copy.QuantityDrop = QuantityDrop;
        copy.Description = Description;
        copy.TooltipActive = TooltipActive;
        copy.KitID = KitID;
        copy.Days = Days;
        copy.IconPath = IconPath;
        copy.IsVipFreeItem = IsVipFreeItem;
        copy.DayOfMonth = DayOfMonth;
        copy.Stock = Stock;

        CopyAttachmentList(Attachments, copy.Attachments);
        CopyAttachmentList(Ground, copy.Ground);
        CopyAttachmentList(CargoItems, copy.CargoItems);
        CopyAttachmentList(RecallCargoItems, copy.RecallCargoItems);

        return copy;
    }

    private void CopyAttachmentList(array<ref DantyzShopAttachment> from, array<ref DantyzShopAttachment> to)
    {
        if (!from || !to)
            return;

        for (int i = 0; i < from.Count(); i++)
        {
            DantyzShopAttachment att = from.Get(i);
            if (att)
                to.Insert(att.Clone());
        }
    }
}

class DantyzShopAttachment
{
    string Classname;
    int Quantity;
    
    void DantyzShopAttachment()
    {
        Quantity = 1;
    }

    //! So' tipos primitivos aqui, copia direta.
    DantyzShopAttachment Clone()
    {
        DantyzShopAttachment copy = new DantyzShopAttachment();
        copy.Classname = Classname;
        copy.Quantity = Quantity;
        return copy;
    }
}

//! Uma categoria ja lida do disco. Mesmo padrao do DantyzShopStockEntry.
class DantyzShopCategoryCache
{
    string FileName;
    ref array<ref DantyzShopItem> Items;

    void DantyzShopCategoryCache()
    {
        FileName = "";
        Items = null;
    }
}

class DantyzShopItemList
{
   
    private static ref array<ref DantyzShopCategoryCache> m_CategoryCache;

    
    static void InvalidateCache()
    {
        if (m_CategoryCache)
            m_CategoryCache.Clear();

    }

  
    static ref array<ref DantyzShopItem> LoadCategory(string fileName)
    {
        if (!m_CategoryCache)
            m_CategoryCache = new array<ref DantyzShopCategoryCache>();

        for (int i = 0; i < m_CategoryCache.Count(); i++)
        {
            DantyzShopCategoryCache entry = m_CategoryCache.Get(i);

        
            if (entry && entry.Items && DantyzShopUtils.SameClass(entry.FileName, fileName))
                return entry.Items;
        }

        DantyzShopCategoryCache fresh = new DantyzShopCategoryCache();
        fresh.FileName = fileName;
        fresh.Items = ReadCategoryFromDisk(fileName);

        m_CategoryCache.Insert(fresh);

        return fresh.Items;
    }


    static ref array<ref DantyzShopItem> LoadCategoryCopy(string fileName)
    {
        array<ref DantyzShopItem> source = LoadCategory(fileName);
        array<ref DantyzShopItem> copy = new array<ref DantyzShopItem>();

        if (!source)
            return copy;

        for (int i = 0; i < source.Count(); i++)
        {
            DantyzShopItem item = source.Get(i);
            if (item)
                copy.Insert(item.Clone());
        }

        return copy;
    }

    //! Leitura crua do JSON. So' roda quando a categoria ainda nao esta em cache.
    private static ref array<ref DantyzShopItem> ReadCategoryFromDisk(string fileName)
    {
        string path = "$profile:SimpleModParty/Store/Categories/" + fileName;

        array<ref DantyzShopItem> items = new array<ref DantyzShopItem>();

        if (!FileExist(path))
        {
            return items;
        }

        FileHandle file = OpenFile(path, FileMode.READ);
        if (file == 0)
        {
            return items;
        }

        string jsonContent = "";
        string line = "";
        while (FGets(file, line) >= 0)
        {
            jsonContent += line;
        }
        CloseFile(file);

        JsonSerializer js = new JsonSerializer();
        string errorMessage = "";

        if (!js.ReadFromString(items, jsonContent, errorMessage))
        {
            return items;
        }

        for (int i = 0; i < items.Count(); i++)
        {
            DantyzShopItem item = items.Get(i);

   
            if (item.QuantityDrop <= 0)
            {
                item.QuantityDrop = 1;
            }

            if (item.Stock == 0)
            {
                item.Stock = -1;
            }
        }


        return items;
    }
}
