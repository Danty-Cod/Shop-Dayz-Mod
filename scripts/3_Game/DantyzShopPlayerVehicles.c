class DantyzShopVehicleData
{
    int NetworkID;
    string PersistentID;   // ID ESTAVEL (sobrevive a restart). Chave principal. "" em registros antigos.
    string VehicleClass;
    string VehicleName;
    int VehiclePrice;
    int PurchaseTime;
    bool HasInsurance;
    bool InsurancePaid;
    bool VehicleDestroyed;
    int DestructionTime;
    int DestructionEpoch;   
    bool InsuranceClaimed;
    bool FreeRecallUsed;   
    int RecallReadyEpoch;
    int LastRecallEpoch;
    int RecallState;
    int RecallSecondsLeft; 
    float LastPosX;
    float LastPosY;
    float LastPosZ;
    bool HasLastPos;
    int TrackUntilEpoch;
    int    CarLockOwnerLow;
    int    CarLockPass;
    string CarLockOwnerName;
    bool TrackActive;        
    int  TrackSecondsLeft;   
    int  TrackCost;          
    
    void DantyzShopVehicleData()
    {
        NetworkID = 0;
        PersistentID = "";
        VehicleClass = "";
        VehicleName = "";
        VehiclePrice = 0;
        PurchaseTime = 0;
        HasInsurance = false;
        InsurancePaid = false;
        VehicleDestroyed = false;
        DestructionTime = 0;
        DestructionEpoch = 0;
        InsuranceClaimed = false;
        FreeRecallUsed = false;
        RecallReadyEpoch = 0;
        LastRecallEpoch = 0;
        RecallState = 0;
        RecallSecondsLeft = 0;
        LastPosX = 0;
        LastPosY = 0;
        LastPosZ = 0;
        HasLastPos = false;
        TrackUntilEpoch = 0;
        CarLockOwnerLow = -1;
        CarLockPass = -1;
        CarLockOwnerName = "";
        TrackActive = false;
        TrackSecondsLeft = 0;
        TrackCost = 0;
    }
}

class DantyzShopPlayerVehicles
{
    string SteamID;
    ref array<ref DantyzShopVehicleData> Vehicles;
    
    void DantyzShopPlayerVehicles()
    {
        Vehicles = new array<ref DantyzShopVehicleData>();
    }
    
    static DantyzShopPlayerVehicles Load(string steamID)
    {
        string path = "$profile:SimpleModParty/Store/PlayerVehicles/" + steamID + ".json";
        
        DantyzShopPlayerVehicles playerVehicles = new DantyzShopPlayerVehicles();
        playerVehicles.SteamID = steamID;
        
        if (FileExist(path))
        {
            JsonFileLoader<DantyzShopPlayerVehicles>.JsonLoadFile(path, playerVehicles);
        }
        
        return playerVehicles;
    }
    
    void Save()
    {
        string dirPath = "$profile:SimpleModParty/Store/PlayerVehicles/";
        if (!FileExist(dirPath))
        {
            MakeDirectory(dirPath);
        }
        
        string path = dirPath + SteamID + ".json";
        JsonFileLoader<DantyzShopPlayerVehicles>.JsonSaveFile(path, this);
    }
    
    void RegisterVehicle(int networkID, string persistentID, string vehicleClass, string vehicleName, int vehiclePrice)
    {
        DantyzShopVehicleData vehicle = new DantyzShopVehicleData();
        vehicle.NetworkID = networkID;
        vehicle.PersistentID = persistentID;
        vehicle.VehicleClass = vehicleClass;
        vehicle.VehicleName = vehicleName;
        vehicle.VehiclePrice = vehiclePrice;
        vehicle.PurchaseTime = GetGame().GetTime();
        vehicle.HasInsurance = false;
        vehicle.InsurancePaid = false;
        vehicle.VehicleDestroyed = false;
        vehicle.DestructionTime = 0;
        vehicle.InsuranceClaimed = false;
        
        Vehicles.Insert(vehicle);
        Save();
        
    }
    
    DantyzShopVehicleData GetVehicleByNetworkID(int networkID)
    {
        for (int i = 0; i < Vehicles.Count(); i++)
        {
            DantyzShopVehicleData vehicle = Vehicles.Get(i);
            if (vehicle.NetworkID == networkID)
            {
                return vehicle;
            }
        }
        return null;
    }
    
    // Busca pelo PersistentID (chave ESTAVEL). Ignora PID vazio.
    DantyzShopVehicleData GetVehicleByPersistentID(string pid)
    {
        if (pid == "" || pid == "0_0_0_0")
            return null;
        for (int i = 0; i < Vehicles.Count(); i++)
        {
            DantyzShopVehicleData vehicle = Vehicles.Get(i);
            if (vehicle.PersistentID == pid)
                return vehicle;
        }
        return null;
    }
    
    // Acha pelo PID (estavel); se nao achar, cai no NetworkID (compat com
    // registros antigos que ainda nao tem PID salvo).
    DantyzShopVehicleData GetVehiclePIDorNet(string pid, int networkID)
    {
        DantyzShopVehicleData v = GetVehicleByPersistentID(pid);
        if (v)
            return v;
        return GetVehicleByNetworkID(networkID);
    }
    
    void PurchaseInsurance(int networkID)
    {
        DantyzShopVehicleData vehicle = GetVehicleByNetworkID(networkID);
        if (vehicle)
        {
            vehicle.HasInsurance = true;
            vehicle.InsurancePaid = true;
            Save();
        }
    }
    
    void MarkVehicleDestroyed(int networkID)
    {
        DantyzShopVehicleData vehicle = GetVehicleByNetworkID(networkID);
        if (vehicle)
        {
            vehicle.VehicleDestroyed = true;
            vehicle.DestructionTime = GetGame().GetTime();
            vehicle.DestructionEpoch = DantyzTime.NowEpoch();
            Save();
        }
    }
    
    // Versao ESTAVEL: marca destruido achando pelo PID (fallback NetworkID).
    void MarkVehicleDestroyedPID(string pid, int networkID)
    {
        DantyzShopVehicleData vehicle = GetVehiclePIDorNet(pid, networkID);
        if (vehicle)
        {
            vehicle.VehicleDestroyed = true;
            vehicle.DestructionTime = GetGame().GetTime();
            vehicle.DestructionEpoch = DantyzTime.NowEpoch();
            Save();
        }
    }
    
    void ClaimInsurance(int networkID)
    {
        DantyzShopVehicleData vehicle = GetVehicleByNetworkID(networkID);
        if (vehicle)
        {
            vehicle.InsuranceClaimed = true;
            Save();
        }
    }
    
    // Versao ESTAVEL: reclama o seguro achando pelo PID (fallback NetworkID).
    void ClaimInsurancePID(string pid, int networkID)
    {
        DantyzShopVehicleData vehicle = GetVehiclePIDorNet(pid, networkID);
        if (vehicle)
        {
            vehicle.InsuranceClaimed = true;
            Save();
        }
    }
    
    array<ref DantyzShopVehicleData> GetActiveVehicles()
    {
        array<ref DantyzShopVehicleData> active = new array<ref DantyzShopVehicleData>();
        
        for (int i = 0; i < Vehicles.Count(); i++)
        {
            DantyzShopVehicleData vehicle = Vehicles.Get(i);
            if (!vehicle.VehicleDestroyed)
            {
                active.Insert(vehicle);
            }
        }
        
        return active;
    }
    
    // Carros SEGURADOS e nao destruidos (usado pelo rastreador de carro).
    array<ref DantyzShopVehicleData> GetInsuredVehicles()
    {
        array<ref DantyzShopVehicleData> insured = new array<ref DantyzShopVehicleData>();
        for (int i = 0; i < Vehicles.Count(); i++)
        {
            DantyzShopVehicleData vehicle = Vehicles.Get(i);
            if (vehicle.HasInsurance && !vehicle.VehicleDestroyed)
                insured.Insert(vehicle);
        }
        return insured;
    }
    
    // Atualiza a ultima posicao conhecida do carro (acha por PID, fallback NetID).
    void UpdateLastPositionPID(string pid, int networkID, vector pos)
    {
        DantyzShopVehicleData vehicle = GetVehiclePIDorNet(pid, networkID);
        if (vehicle)
        {
            vehicle.LastPosX = pos[0];
            vehicle.LastPosY = pos[1];
            vehicle.LastPosZ = pos[2];
            vehicle.HasLastPos = true;
            Save();
        }
    }
    
    array<ref DantyzShopVehicleData> GetClaimableInsurances(int waitTimeSeconds)
    {
        array<ref DantyzShopVehicleData> claimable = new array<ref DantyzShopVehicleData>();
        
        int currentTime = GetGame().GetTime();
        int waitTimeMilliseconds = waitTimeSeconds * 1000;
        
        for (int i = 0; i < Vehicles.Count(); i++)
        {
            DantyzShopVehicleData vehicle = Vehicles.Get(i);
            
            if (vehicle.VehicleDestroyed && vehicle.HasInsurance && !vehicle.InsuranceClaimed)
            {
                int timeSinceDestruction = currentTime - vehicle.DestructionTime;
                if (timeSinceDestruction >= waitTimeMilliseconds)
                {
                    claimable.Insert(vehicle);
                }
            }
        }
        
        return claimable;
    }
}
