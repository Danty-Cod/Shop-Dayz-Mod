
class DantyzVipFreeOnce
{
    private static const string FOLDER = "$profile:SimpleModParty/Store/Vip/PlayersVipFreeOnce";

    // ---------------------------------------------------------
    // Garante que a pasta existe. Chamado no startup (FileSystem).
    // ---------------------------------------------------------
    static void EnsureDirectory()
    {
        if (!FileExist("$profile:SimpleModParty"))
            MakeDirectory("$profile:SimpleModParty");
        if (!FileExist("$profile:SimpleModParty/Store"))
            MakeDirectory("$profile:SimpleModParty/Store");
        if (!FileExist("$profile:SimpleModParty/Store/Vip"))
            MakeDirectory("$profile:SimpleModParty/Store/Vip");
        if (!FileExist(FOLDER))
            MakeDirectory(FOLDER);
    }

    // ---------------------------------------------------------
    // Retorna true se o player JA usou o VIP Free.
    // ---------------------------------------------------------
    static bool AlreadyClaimed(string steamID)
    {
        EnsureDirectory();
        string path = FOLDER + "/" + steamID + ".claimed";
        return FileExist(path);
    }

    // ---------------------------------------------------------
    // Marca o player como tendo usado o VIP Free.
    // Chamado apos o VIP ser ativado com sucesso.
    // ---------------------------------------------------------
    static void MarkAsClaimed(string steamID)
    {
        EnsureDirectory();
        string path = FOLDER + "/" + steamID + ".claimed";

        // Cria o arquivo com conteudo minimo para que FileExist funcione.
        FileHandle fh = OpenFile(path, FileMode.WRITE);
        if (fh != 0)
        {
            FPrint(fh, "1");
            CloseFile(fh);
        }

    }
}
