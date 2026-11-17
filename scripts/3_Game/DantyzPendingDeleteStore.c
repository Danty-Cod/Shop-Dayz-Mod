
class DantyzPendingDeleteData
{
    ref array<string> PIDs;

    void DantyzPendingDeleteData()
    {
        PIDs = new array<string>();
    }
}

class DantyzPendingDeleteStore
{
    static const string PATH = "$profile:SimpleModParty/Store/PendingDelete.json";

    // Le o arquivo pra dentro de 'data'. Retorna sempre com data.PIDs valido.
    static void ReadInto(DantyzPendingDeleteData data)
    {
        DantyzShopFileSystem.EnsureDirectoriesExist();
        if (FileExist(PATH))
        {
            JsonFileLoader<DantyzPendingDeleteData>.JsonLoadFile(PATH, data);
        }
        if (!data.PIDs)
            data.PIDs = new array<string>();
    }

    static void LoadInto(array<string> target)
    {
        if (!target)
            return;
        DantyzPendingDeleteData data = new DantyzPendingDeleteData();
        ReadInto(data);
        for (int i = 0; i < data.PIDs.Count(); i++)
        {
            string p = data.PIDs.Get(i);
            if (p != "" && p != "0_0_0_0" && target.Find(p) == -1)
                target.Insert(p);
        }
    }

    // Marca um PID pra deletar (persiste no disco). Ignora PID invalido.
    static void Add(string pid)
    {
        if (pid == "" || pid == "0_0_0_0")
            return;
        DantyzPendingDeleteData data = new DantyzPendingDeleteData();
        ReadInto(data);
        if (data.PIDs.Find(pid) == -1)
        {
            data.PIDs.Insert(pid);
            JsonFileLoader<DantyzPendingDeleteData>.JsonSaveFile(PATH, data);
        }
    }

    // Remove um PID da lista persistida (quando o carro ja foi deletado).
    static void Remove(string pid)
    {
        if (pid == "" || pid == "0_0_0_0")
            return;
        DantyzPendingDeleteData data = new DantyzPendingDeleteData();
        ReadInto(data);
        int idx = data.PIDs.Find(pid);
        if (idx != -1)
        {
            data.PIDs.Remove(idx);
            JsonFileLoader<DantyzPendingDeleteData>.JsonSaveFile(PATH, data);
        }
    }
}
