modded class CarScript
{
    static ref array<CarScript> m_DantyzLoadedCars = new array<CarScript>();
    static ref array<string> m_DantyzPendingDelete = new array<string>();
    static bool m_DantyzPendingLoaded = false;



    //! Indice dos carros SEGURADOS carregados no mundo: PersistentID -> carro.
    static ref map<string, CarScript> m_DzInsuredIndex = new map<string, CarScript>();

   
    protected string m_DzBuyerSteamID = "";
    protected int    m_DzBuyerPrice   = 0;

    //! Chamado pela loja logo depois de criar/registrar o carro comprado.
    void DzBindBuyer(string steamID, int price)
    {
        m_DzBuyerSteamID = steamID;
        m_DzBuyerPrice   = price;
    }

    // TEST DRIVE: carro so' para testar (criado pela loja, nao e' de ninguem).
    // Nao deixa tirar/colocar pecas nem itens e nao aceita CarLock/seguro.
    // A variavel e' sincronizada para o cliente tambem bloquear na interface.
    protected bool m_DzTestDrive = false;

    void DzSetTestDrive(bool state)
    {
        m_DzTestDrive = state;
        SetSynchDirty();
    }

    bool DzIsTestDrive()
    {
        return m_DzTestDrive;
    }

    override bool CanReleaseAttachment(EntityAI attachment)
    {
        if (m_DzTestDrive)
            return false;
        return super.CanReleaseAttachment(attachment);
    }

    override bool CanReceiveAttachment(EntityAI attachment, int slotId)
    {
        // durante a criacao do carro (antes de marcar como teste) as pecas entram normalmente
        if (m_DzTestDrive)
            return false;
        return super.CanReceiveAttachment(attachment, slotId);
    }

    override bool CanReceiveItemIntoCargo(EntityAI item)
    {
        if (m_DzTestDrive)
            return false;
        return super.CanReceiveItemIntoCargo(item);
    }

    override bool CanReleaseCargo(EntityAI cargo)
    {
        if (m_DzTestDrive)
            return false;
        return super.CanReleaseCargo(cargo);
    }

    void CarScript()
    {
        // O estado do seguro e' do CarLock e ja' e' sincronizado por ele.
        RegisterNetSyncVariableBool("m_DzTestDrive");
    }

    bool DzIsInsured()
    {
        return InsIsActive();
    }

    string DzGetInsOwner()
    {
        return InsGetOwner();
    }

    int DzGetInsPrice()
    {
        return InsGetPrice();
    }

    void DzSetInsured(bool state, string ownerSteamID = "", int price = 0)
    {
        InsSet(state, ownerSteamID, price);

        if (state)
            DzIndexAdd();
        else
            DzIndexRemove();

        SetSynchDirty();
    }

    protected void DzIndexAdd()
    {
        if (!GetGame() || !GetGame().IsServer())
            return;

        string pid = DantyzShop_GetPID();
        if (pid == "" || pid == "0_0_0_0")
            return;

        m_DzInsuredIndex.Set(pid, this);
    }

    protected void DzIndexRemove()
    {
        if (!GetGame() || !GetGame().IsServer())
            return;

        string pid = DantyzShop_GetPID();
        if (pid != "" && pid != "0_0_0_0")
            m_DzInsuredIndex.Remove(pid);
    }

    void DzRefreshIndex()
    {
        if (InsIsActive())
            DzIndexAdd();
    }

    static CarScript DzFindInsuredByPID(string pid)
    {
        if (pid == "" || pid == "0_0_0_0")
            return null;

        CarScript found;
        if (m_DzInsuredIndex.Find(pid, found))
            return found;

        return null;
    }
    override void SetCarLockOwner(int low_uid)
    {
        // carro de test drive nao pode ser trancado (nem ganhar seguro)
        if (m_DzTestDrive && low_uid != -1)
            return;

        super.SetCarLockOwner(low_uid);

        if (!GetGame() || !GetGame().IsServer())
            return;

        // low_uid == -1 significa CarLock REMOVIDO: cai o seguro tambem.
        if (low_uid == -1)
        {
            if (InsIsActive())
            {
                DzSetInsured(false);
            }
            return;
        }

        if (InsIsActive())
            return; // ja' estava segurado, nada a fazer

        DzActivateInsuranceFromCarLock(low_uid);
    }

    protected void DzActivateInsuranceFromCarLock(int low_uid)
    {
        if (m_DzBuyerSteamID != "")
        {
            DzActivateInsuranceForBuyer();
            return;
        }

        string pid = DantyzShop_GetPID();

        int netLow, netHigh;
        GetNetworkID(netLow, netHigh);
        int netID = netLow | netHigh;

        // O low id do CarLock e' derivado do SteamID (PlayerBase.SetLowSteamUID),
        // entao basta achar o player ONLINE com o mesmo low id.
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase p = PlayerBase.Cast(players.Get(i));
            if (!p || !p.GetIdentity())
                continue;
            if (p.GetCarLockLowId() != low_uid)
                continue;

            string steamID = p.GetIdentity().GetPlainId();

            // Le o registro de compra UMA vez, so' pra saber o preco do carro.
            DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(steamID);
            DantyzShopVehicleData v = pv.GetVehiclePIDorNet(pid, netID);

            if (!v)
            {
                return;
            }

            v.HasInsurance = true;

            // Aproveita pra deixar a chave estavel em dia, se ja' estiver valida.
            if (pid != "" && pid != "0_0_0_0")
                v.PersistentID = pid;

            pv.Save();

            DzSetInsured(true, steamID, v.VehiclePrice);
            return;
        }

        // Nenhum player online com esse low id: tambem era silencioso.
    }

    //! Ativa o seguro usando o vinculo gravado na compra (caminho rapido).
    protected void DzActivateInsuranceForBuyer()
    {
        string pid = DantyzShop_GetPID();

        int netLow, netHigh;
        GetNetworkID(netLow, netHigh);
        int netID = netLow | netHigh;

        DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(m_DzBuyerSteamID);
        DantyzShopVehicleData v = pv.GetVehiclePIDorNet(pid, netID);

        if (v)
        {
            v.HasInsurance = true;

            if (pid != "" && pid != "0_0_0_0")
                v.PersistentID = pid;

            pv.Save();

            DzSetInsured(true, m_DzBuyerSteamID, v.VehiclePrice);
            return;
        }

        DzSetInsured(true, m_DzBuyerSteamID, m_DzBuyerPrice);
    }
    
    override void EEInit()
    {
        super.EEInit();
        if (GetGame() && GetGame().IsServer())
        {
            if (m_DantyzLoadedCars.Find(this) == -1)
                m_DantyzLoadedCars.Insert(this);

            if (InsIsActive())
                DzIndexAdd();

            if (!m_DantyzPendingLoaded)
            {
                m_DantyzPendingLoaded = true;
                DantyzPendingDeleteStore.LoadInto(m_DantyzPendingDelete);
            }

            // Este carro estava marcado pra deletar (duplicado de uma puxada)?
            if (m_DantyzPendingDelete && m_DantyzPendingDelete.Count() > 0)
            {
                string myPID = DantyzShop_GetPID();
                if (myPID != "" && myPID != "0_0_0_0")
                {
                    int pidx = m_DantyzPendingDelete.Find(myPID);
                    if (pidx != -1)
                    {
                        m_DantyzPendingDelete.Remove(pidx);
                        // Persiste a remocao no disco (ja foi tratado).
                        DantyzPendingDeleteStore.Remove(myPID);
                        GetGame().ObjectDelete(this);
                    }
                }
            }
        }
    }
    
    override void EEDelete(EntityAI parent)
    {
        if (GetGame() && GetGame().IsServer())
        {
            int idx = m_DantyzLoadedCars.Find(this);
            if (idx != -1)
                m_DantyzLoadedCars.Remove(idx);

            // Sai do indice de segurados pra nao deixar referencia morta.
            DzIndexRemove();
        }
        super.EEDelete(parent);
    }

    
    override void EEKilled(Object killer)
    {
        super.EEKilled(killer);
        
        if (GetGame().IsServer())
        {
            int netLow, netHigh;
            GetNetworkID(netLow, netHigh);
            int vehicleNetID = netLow | netHigh;
            
            string pid = DantyzShop_GetPID();
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CheckVehicleInsuranceOwned, 100, false, pid, vehicleNetID, InsGetOwner());
        }
    }
    
    override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
    {
        super.EEHealthLevelChanged(oldLevel, newLevel, zone);
        
        if (GetGame().IsServer() && zone == "Engine" && newLevel == GameConstants.STATE_RUINED && oldLevel != newLevel)
        {
            int netLow, netHigh;
            GetNetworkID(netLow, netHigh);
            int vehicleNetID = netLow | netHigh;
            
            string pid = DantyzShop_GetPID();
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CheckVehicleInsuranceOwned, 100, false, pid, vehicleNetID, InsGetOwner());
        }
    }
    void CheckVehicleInsuranceOwned(string pid, int vehicleNetID, string ownerSteamID)
    {
        if (ownerSteamID == "")
        {
            CheckVehicleInsurance(pid, vehicleNetID);
            return;
        }

        DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(ownerSteamID);
        DantyzShopVehicleData vehicle = pv.GetVehiclePIDorNet(pid, vehicleNetID);

        if (!vehicle || !vehicle.HasInsurance || vehicle.VehicleDestroyed)
            return;

        pv.MarkVehicleDestroyedPID(pid, vehicleNetID);

        DantyzShopConfig config = DantyzShopConfig.Load();
        int waitTime = config.InsuranceWaitTimeSeconds * 1000;

        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ProcessInsurancePayment, waitTime, false, ownerSteamID, pid, vehicleNetID);

        // Avisa o dono se ele estiver online (sem varrer arquivo nenhum).
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase p = PlayerBase.Cast(players.Get(i));
            if (p && p.GetIdentity() && p.GetIdentity().GetPlainId() == ownerSteamID)
            {
                if (DantyzShopMsg.On())
                    p.MessageImportant("Seu veiculo foi destruido! Seguro sera pago em " + config.InsuranceWaitTimeSeconds + " segundos.");
                break;
            }
        }

    }
    
    // PersistentID do carro em string (ID ESTAVEL, sobrevive a restart).
    string DantyzShop_GetPID()
    {
        int b1, b2, b3, b4;
        GetPersistentID(b1, b2, b3, b4);
        return "" + b1 + "_" + b2 + "_" + b3 + "_" + b4;
    }

    bool DantyzShop_HasPlayerInside()
    {
        int seats = CrewSize();
        for (int i = 0; i < seats; i++)
        {
            Human crew = CrewMember(i);
            if (crew && PlayerBase.Cast(crew))
                return true;
        }
        return false;
    }
    
    void CheckVehicleInsurance(string pid, int vehicleNetID)
    {
        string playerVehiclesDir = "$profile:SimpleModParty/Store/PlayerVehicles/";
        
        if (!FileExist(playerVehiclesDir))
            return;
        
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        // 1) Tenta achar o dono ONLINE (avisa na hora + agenda o pagamento).
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (!player)
                continue;
            
            PlayerIdentity identity = player.GetIdentity();
            if (!identity)
                continue;
            
            string steamID = identity.GetPlainId();
            string vehicleFile = playerVehiclesDir + steamID + ".json";
            
            if (!FileExist(vehicleFile))
                continue;
            
            DantyzShopPlayerVehicles playerVehicles = DantyzShopPlayerVehicles.Load(steamID);
            // Acha pelo PID (estavel) e cai no NetworkID se for registro antigo.
            DantyzShopVehicleData vehicle = playerVehicles.GetVehiclePIDorNet(pid, vehicleNetID);
            
            if (vehicle)
            {
                if (!vehicle.HasInsurance || vehicle.VehicleDestroyed)
                    return;
                
                playerVehicles.MarkVehicleDestroyedPID(pid, vehicleNetID);
                
                DantyzShopConfig config = DantyzShopConfig.Load();
                int waitTime = config.InsuranceWaitTimeSeconds * 1000;
                
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ProcessInsurancePayment, waitTime, false, steamID, pid, vehicleNetID);
                
                if (DantyzShopMsg.On())
                    player.MessageImportant("Seu veiculo foi destruido! Seguro sera pago em " + config.InsuranceWaitTimeSeconds + " segundos.");
                
                return;
            }
        }
        string offlineOwner = DantyzInsurance.MarkDestroyedByOwnerScan(pid, vehicleNetID);
    }
    
    void ProcessInsurancePayment(string steamID, string pid, int vehicleNetID)
    {
        // Resolve o nome do player se estiver online (senao o helper usa SteamID).
        string whUserName = "";
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int wi = 0; wi < players.Count(); wi++)
        {
            PlayerBase wp = PlayerBase.Cast(players.Get(wi));
            if (wp && wp.GetIdentity() && wp.GetIdentity().GetPlainId() == steamID)
            {
                whUserName = wp.GetIdentity().GetName();
                break;
            }
        }
        int paid = DantyzInsurance.PayClaimByKey(steamID, pid, vehicleNetID, whUserName);

        if (paid <= 0)
            return;

        // Avisa o player se ele estiver online.
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (player)
            {
                PlayerIdentity identity = player.GetIdentity();
                if (identity && identity.GetPlainId() == steamID)
                {
                    if (DantyzShopMsg.On())
                        player.MessageImportant("Seguro pago! +" + paid + " ZCoins depositados!");
                    // Atualiza o saldo na tela do player na hora.
                    DantyzPlayerAccount acc = DantyzPlayerAccount.Load(steamID);
                    GetRPCManager().SendRPC("DantyzShop", "RPCUpdateCoins", new Param1<int>(acc.coins), true, identity);
                    return;
                }
            }
        }
    }
}
