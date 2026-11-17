class DantyzTime
{
    static int DaysFromCivil(int y, int m, int d)
    {
        if (m <= 2)
            y = y - 1;

        int era;
        if (y >= 0)
            era = y / 400;
        else
            era = (y - 399) / 400;

        int yoe = y - era * 400;

        int mp;
        if (m > 2)
            mp = m - 3;
        else
            mp = m + 9;

        int doy = (153 * mp + 2) / 5 + d - 1;
        int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + doe - 719468;
    }
    static int NowEpoch()
    {
        int y, mo, d, h, mi, s;
        GetYearMonthDay(y, mo, d);
        GetHourMinuteSecond(h, mi, s);
        int days = DaysFromCivil(y, mo, d);
        return ((days * 24 + h) * 60 + mi) * 60 + s;
    }
}

class DantyzInsurance
{
    static int PayClaimByKey(string steamID, string pid, int netID, string playerName)
    {
        if (!GetGame() || !GetGame().IsServer())
            return 0;

        DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(steamID);
        DantyzShopVehicleData vehicle = pv.GetVehiclePIDorNet(pid, netID);

        if (!vehicle)
            return 0;

        DantyzShopConfig config = DantyzShopConfig.Load();
        
        if (!vehicle.HasInsurance || !vehicle.VehicleDestroyed || vehicle.InsuranceClaimed)
            return 0;
            
        if (config.RefundInsuranceOnDestroyed != 1)
            return 0;

        int returnAmount = (vehicle.VehiclePrice * config.InsuranceReturnPercent) / 100;

        DantyzPlayerAccount account = DantyzPlayerAccount.Load(steamID);
        account.AddCoins(returnAmount);

        vehicle.InsuranceClaimed = true;
        pv.Save();


        string name = playerName;
        if (name == "")
            name = steamID;
        DantyzShopWebhook.SendSeguroUsado(name, steamID, vehicle.VehicleName, returnAmount);

        return returnAmount;
    }

    static void ProcessAllClaimableForPlayer(string steamID, string playerName)
    {
        if (!GetGame() || !GetGame().IsServer())
            return;

        DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(steamID);
        if (!pv || !pv.Vehicles)
            return;

        DantyzShopConfig config = DantyzShopConfig.Load();
        int now = DantyzTime.NowEpoch();
        int wait = config.InsuranceWaitTimeSeconds;

        for (int i = 0; i < pv.Vehicles.Count(); i++)
        {
            DantyzShopVehicleData v = pv.Vehicles.Get(i);
            
            if (v.VehicleDestroyed && v.HasInsurance && !v.InsuranceClaimed)
            {

                bool due = (v.DestructionEpoch == 0) || ((now - v.DestructionEpoch) >= wait);
                if (due)
                    PayClaimByKey(steamID, v.PersistentID, v.NetworkID, playerName);
            }
        }
    }

    static string MarkDestroyedByOwnerScan(string pid, int netID)
    {
        if (!GetGame() || !GetGame().IsServer())
            return "";
        if (pid == "" || pid == "0_0_0_0")
            return "";

        string dir = "$profile:SimpleModParty/Store/PlayerVehicles/";
        if (!FileExist(dir))
            return "";

        string fileName;
        FileAttr attr;
        FindFileHandle h = FindFile(dir + "*.json", fileName, attr, FindFileFlags.ALL);
        bool found = (h != 0);
        string ownerSteamID = "";

        while (found)
        {
            if (fileName != "" && fileName != "." && fileName != "..")
            {
                string steamID = fileName;
                int dot = steamID.IndexOf(".");
                if (dot > 0)
                    steamID = steamID.Substring(0, dot);

                DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(steamID);
                DantyzShopVehicleData v = pv.GetVehicleByPersistentID(pid);
                if (v && v.HasInsurance && !v.VehicleDestroyed)
                {
                    pv.MarkVehicleDestroyedPID(pid, netID);
                    ownerSteamID = steamID;
                    break;
                }
            }
            found = FindNextFile(h, fileName, attr);
        }

        if (h != 0)
            CloseFindFile(h);

        return ownerSteamID;
    }
}
