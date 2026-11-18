class DantyzTestDriveSession
{
    string     SteamID;
    PlayerBase TestPlayer;
    CarScript  TestCar;
    vector     ReturnPos;
    vector     ReturnOri;
    int        EndTime;     // ms (GetGame().GetTime())
    int        GraceEnd;    // ms: prazo para entrar no carro
    bool       Entered;
    bool       Warned;
    bool       TimeUp;          // acabou pelo tempo (fodase volta onde estava)
    int        FullCooldownMs;  // espera depois de usar o tempo todo
    int        EarlyCooldownMs; // espera depois de sair antes do tempo
}

class DantyzTestDriveManager
{
    protected static ref map<string, ref DantyzTestDriveSession> m_Sessions = new map<string, ref DantyzTestDriveSession>();
    protected static ref map<string, int> m_NextAllowed = new map<string, int>();
    protected static bool m_Ticking = false;

    static const int ENTER_GRACE_MS = 30000;   // tempo para entrar no carro se o assento automatico falhar
    static const int TICK_MS = 1000;

    static bool IsTesting(string steamID)
    {
        return m_Sessions.Contains(steamID);
    }

    // ---------------------------------------------------------------------------------
    // INICIO
    // ---------------------------------------------------------------------------------
    static bool Start(PlayerBase player, string steamID, string vehicleClass, DantyzShopVehicleConfig vehicleConfig, DantyzShopConfig cfg, out string message)
    {
        if (!cfg || cfg.TestDriveEnabled == 0)
        {
            message = "Test drive desativado.";
            return false;
        }
        if (m_Sessions.Contains(steamID))
        {
            message = "Voce ja esta em um test drive.";
            return false;
        }

        int now = GetGame().GetTime();
        int allowed;
        if (m_NextAllowed.Find(steamID, allowed) && now < allowed)
        {
            message = "Aguarde " + ((allowed - now) / 1000 + 1) + " segundos para outro test drive.";
            return false;
        }

        if (!player || !player.IsAlive() || player.IsUnconscious() || player.IsRestrained() || player.IsInVehicle())
        {
            message = "Voce nao pode fazer test drive agora.";
            return false;
        }

        DantyzVehicleParts vehConfig = null;
        if (vehicleConfig)
            vehConfig = vehicleConfig.GetVehicleConfig(vehicleClass);
        if (!vehConfig)
        {
            message = "Configuracao do veiculo nao encontrada!";
            return false;
        }

        DantyzTestDriveSpot spot = FindFreeSpot(cfg);
        if (!spot)
        {
            message = "Local de test drive ocupado ou nao configurado. Tente em instantes.";
            return false;
        }

        // carro de teste: nao entra na persistencia do mundo (some num restart)
        int flags = ECE_SETUP | ECE_UPDATEPATHGRAPH | ECE_CREATEPHYSICS | ECE_NOPERSISTENCY_WORLD;
        CarScript car = CarScript.Cast(GetGame().CreateObjectEx(vehicleClass, spot.Position, flags));
        if (!car)
        {
            message = "Erro ao criar o carro de teste!";
            return false;
        }

        car.SetOrientation(Vector(spot.Heading, 0, 0));

        for (int k = 0; k < vehConfig.Parts.Count(); k++)
            car.GetInventory().CreateAttachment(vehConfig.Parts.Get(k));

        car.Fill(CarFluid.FUEL, car.GetFluidCapacity(CarFluid.FUEL));
        car.Fill(CarFluid.OIL, car.GetFluidCapacity(CarFluid.OIL));
        car.Fill(CarFluid.BRAKE, car.GetFluidCapacity(CarFluid.BRAKE));
        car.Fill(CarFluid.COOLANT, car.GetFluidCapacity(CarFluid.COOLANT));

        // depois das pecas: a partir daqui nada entra/sai do carro
        car.DzSetTestDrive(true);

        DantyzTestDriveSession s = new DantyzTestDriveSession();
        s.SteamID   = steamID;
        s.TestPlayer    = player;
        s.TestCar       = car;
        s.ReturnPos = player.GetPosition();
        s.ReturnOri = player.GetOrientation();
        int seconds = cfg.TestDriveSeconds;
        if (seconds <= 0)
            seconds = 900;
        s.EndTime   = now + seconds * 1000;
        s.GraceEnd  = now + ENTER_GRACE_MS;
        m_Sessions.Set(steamID, s);

        // a espera para o proximo teste conta a partir do FIM deste (ver End)
        int cooldown = cfg.TestDriveCooldownSeconds;
        if (cooldown < 0)
            cooldown = 0;
        int early = cfg.TestDriveEarlyCooldownSeconds;
        if (early < 0)
            early = 0;
        s.FullCooldownMs = cooldown * 1000;
        s.EarlyCooldownMs = early * 1000;

        // leva o jogador para a porta do motorista e senta (pequeno atraso para o carro existir no cliente)
        vector entryPos, entryDir;
        car.CrewEntryWS(0, entryPos, entryDir);
        player.SetPosition(entryPos);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DantyzTestDriveManager.SeatPlayer, 800, false, steamID);

        if (!m_Ticking)
        {
            m_Ticking = true;
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DantyzTestDriveManager.Tick, TICK_MS, false);
        }

        message = "Test drive iniciado: " + (seconds / 60) + " minuto(s). Sair do carro encerra o teste.";
        return true;
    }

    protected static DantyzTestDriveSpot FindFreeSpot(DantyzShopConfig cfg)
    {
        if (!cfg.TestDriveSpots)
            return null;

        for (int i = 0; i < cfg.TestDriveSpots.Count(); i++)
        {
            DantyzTestDriveSpot spot = cfg.TestDriveSpots.Get(i);
            if (!spot || spot.Position == vector.Zero)
                continue; // local ainda nao configurado

            array<Object> objects = new array<Object>();
            GetGame().GetObjectsAtPosition(spot.Position, 4.0, objects, null);

            bool busy = false;
            for (int j = 0; j < objects.Count(); j++)
            {
                Object obj = objects.Get(j);
                if (obj && (obj.IsInherited(CarScript) || obj.IsInherited(Man)))
                {
                    busy = true;
                    break;
                }
            }

            if (!busy)
                return spot;
        }

        return null;
    }

    // senta no motorista no servidor e pede para o cliente fazer o mesmo
    static void SeatPlayer(string steamID)
    {
        DantyzTestDriveSession s;
        if (!m_Sessions.Find(steamID, s) || !s.TestPlayer || !s.TestCar)
            return;

        if (s.TestCar.CrewMember(0))
        {
            s.TestPlayer.MessageImportant("Entre no carro pelo banco do motorista.");
            return;
        }

        int seat = s.TestCar.GetSeatAnimationType(0);
        HumanCommandVehicle cmd = s.TestPlayer.StartCommand_Vehicle(s.TestCar, 0, seat, false);
        if (cmd)
            cmd.SetVehicleType(s.TestCar.GetAnimInstance());

        PlayerIdentity identity = s.TestPlayer.GetIdentity();
        if (identity)
            GetRPCManager().SendRPC("DantyzShop", "RPCTestDriveSeat", new Param1<Object>(s.TestCar), true, identity);
    }

    // ---------------------------------------------------------------------------------
    // CONTROLE (1x por segundo enquanto houver teste)
    // ---------------------------------------------------------------------------------
    static void Tick()
    {
        int now = GetGame().GetTime();
        array<string> toEnd = new array<string>();
        array<string> reasons = new array<string>();

        for (int i = 0; i < m_Sessions.Count(); i++)
        {
            DantyzTestDriveSession s = m_Sessions.GetElement(i);
            string reason = "";

            if (!s.TestPlayer || !s.TestPlayer.IsAlive() || !s.TestPlayer.GetIdentity())
                reason = "jogador";
            else if (!s.TestCar || s.TestCar.IsDamageDestroyed())
                reason = "Test drive encerrado: o carro foi destruido.";
            else
            {
                bool inside = IsInCar(s.TestPlayer, s.TestCar);
                if (inside)
                    s.Entered = true;

                if (s.Entered && !inside)
                    reason = "Test drive encerrado: voce saiu do carro.";
                else if (!s.Entered && now >= s.GraceEnd)
                    reason = "Test drive cancelado: voce nao entrou no carro.";
                else if (now >= s.EndTime)
                {
                    s.TimeUp = true;
                    reason = "Test drive encerrado: o tempo acabou.";
                }
                else if (!s.Warned && s.EndTime - now <= 60000)
                {
                    s.Warned = true;
                    s.TestPlayer.MessageImportant("Test drive: falta 1 minuto.");
                }
            }

            if (reason != "")
            {
                toEnd.Insert(m_Sessions.GetKey(i));
                reasons.Insert(reason);
            }
        }

        for (int j = 0; j < toEnd.Count(); j++)
            End(toEnd.Get(j), reasons.Get(j));

        if (m_Sessions.Count() > 0)
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DantyzTestDriveManager.Tick, TICK_MS, false);
        else
            m_Ticking = false;
    }

    protected static bool IsInCar(PlayerBase player, CarScript car)
    {
        for (int i = 0; i < car.CrewSize(); i++)
        {
            if (car.CrewMember(i) == player)
                return true;
        }
        return false;
    }

    // ---------------------------------------------------------------------------------
    // FIM: tira do carro, apaga o carro e devolve o jogador
    // ---------------------------------------------------------------------------------
    static void End(string steamID, string reason)
    {
        DantyzTestDriveSession s;
        if (!m_Sessions.Find(steamID, s))
            return;
        m_Sessions.Remove(steamID);
        SetCooldown(steamID, s);

        PlayerBase player = s.TestPlayer;
        bool alive = player && player.IsAlive();

        if (alive && player.GetIdentity() && reason != "jogador")
            player.MessageImportant(reason);

        if (alive && s.TestCar && IsInCar(player, s.TestCar))
        {
            HumanCommandVehicle cmd = player.GetCommand_Vehicle();
            if (cmd)
            {
                cmd.KeepInVehicleSpaceAfterLeave(false);
                cmd.GetOutVehicle();
            }
            if (player.GetIdentity())
                GetRPCManager().SendRPC("DantyzShop", "RPCTestDriveExit", new Param1<bool>(true), true, player.GetIdentity());

            // espera a animacao de saida antes de apagar o carro e teleportar
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DantyzTestDriveManager.Finish, 1500, false, player, s.TestCar, s.ReturnPos, s.ReturnOri);
            return;
        }

        Finish(player, s.TestCar, s.ReturnPos, s.ReturnOri);
    }

    // tempo todo usado = TestDriveCooldownSeconds; saiu antes = TestDriveEarlyCooldownSeconds
    // (+1,5s da saida do carro/volta ao lugar original)
    protected static void SetCooldown(string steamID, DantyzTestDriveSession s)
    {
        int wait = s.EarlyCooldownMs;
        if (s.TimeUp)
            wait = s.FullCooldownMs;
        m_NextAllowed.Set(steamID, GetGame().GetTime() + 1500 + wait);
    }

    static void Finish(PlayerBase player, CarScript car, vector returnPos, vector returnOri)
    {
        if (car)
            GetGame().ObjectDelete(car);

        if (player && player.IsAlive())
        {
            player.SetPosition(returnPos);
            player.SetOrientation(returnOri);
        }
    }

    // saiu do jogo durante o teste: devolve para o lugar original na hora (antes de salvar o personagem)
    static void OnDisconnect(PlayerBase player)
    {
        if (!player)
            return;

        for (int i = 0; i < m_Sessions.Count(); i++)
        {
            DantyzTestDriveSession s = m_Sessions.GetElement(i);
            if (s.TestPlayer == player)
            {
                string key = m_Sessions.GetKey(i);
                m_Sessions.Remove(key);
                SetCooldown(key, s);

                HumanCommandVehicle cmd = player.GetCommand_Vehicle();
                if (cmd)
                {
                    cmd.KeepInVehicleSpaceAfterLeave(false);
                    cmd.GetOutVehicle();
                }
                Finish(player, s.TestCar, s.ReturnPos, s.ReturnOri);
                return;
            }
        }
    }
}
