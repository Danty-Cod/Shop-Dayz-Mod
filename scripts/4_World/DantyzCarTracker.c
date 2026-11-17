
class DantyzTrackerSession
{
    string steamID;
    int    expiryTime;   
    ref array<ref DantyzShopVehicleData> insured;
    bool dirty;         

class DantyzCarTracker
{
    static ref DantyzCarTracker g_Instance;

    ref array<ref DantyzTrackerSession> m_ActiveSessions = new array<ref DantyzTrackerSession>();
     protected bool m_TickRunning = false;
    static const int TRACK_DURATION_MS = 1800000;
    static const int TRACK_INTERVAL_MS = 10000;

    static DantyzCarTracker Get()
    {
        if (!g_Instance)
            g_Instance = new DantyzCarTracker();
        return g_Instance;
    }

    
    protected void LoadSessionSnapshot(DantyzTrackerSession sess)
    {
        DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(sess.steamID);
        sess.insured = pv.GetInsuredVehicles();
        sess.dirty = false;
    }

    
    protected void FlushSessionSnapshot(DantyzTrackerSession sess)
    {
        if (!sess || !sess.dirty || !sess.insured)
            return;

        DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(sess.steamID);

        for (int i = 0; i < sess.insured.Count(); i++)
        {
            DantyzShopVehicleData snap = sess.insured.Get(i);
            if (!snap || !snap.HasLastPos)
                continue;

            DantyzShopVehicleData real = pv.GetVehicleByPersistentID(snap.PersistentID);
            if (!real)
                continue;

            real.LastPosX = snap.LastPosX;
            real.LastPosY = snap.LastPosY;
            real.LastPosZ = snap.LastPosZ;
            real.HasLastPos = true;
        }

        pv.Save();
        sess.dirty = false;
    }

    void GatherInsuredPositions(DantyzTrackerSession sess, out array<vector> positions, out array<string> names)
    {
        if (!sess || !sess.insured)
            return;

        for (int i = 0; i < sess.insured.Count(); i++)
        {
            DantyzShopVehicleData v = sess.insured.Get(i);
            if (!v)
                continue;

            vector pos = vector.Zero;
            bool found = false;

            // 1) Carro CARREGADO agora? Posicao AO VIVO, direto da memoria.
            CarScript car = CarScript.DzFindInsuredByPID(v.PersistentID);
            if (car)
            {
                pos = car.GetPosition();
                found = true;

                // Atualiza a ultima posicao SO' no snapshot. O disco e' gravado
                // uma unica vez, no fim da sessao (FlushSessionSnapshot).
                v.LastPosX = pos[0];
                v.LastPosY = pos[1];
                v.LastPosZ = pos[2];
                v.HasLastPos = true;
                sess.dirty = true;
            }

            // 2) Descarregado: usa a ultima posicao conhecida (onde foi visto).
            if (!found && v.HasLastPos)
            {
                pos = Vector(v.LastPosX, v.LastPosY, v.LastPosZ);
                found = true;
            }

            if (found)
            {
                positions.Insert(pos);
                names.Insert(v.VehicleName);
            }
        }
    }

    // Acha uma sessao ativa pelo steamID (null se nao existe).
    protected DantyzTrackerSession FindSession(string steamID)
    {
        for (int i = 0; i < m_ActiveSessions.Count(); i++)
        {
            if (m_ActiveSessions.Get(i).steamID == steamID)
                return m_ActiveSessions.Get(i);
        }
        return null;
    }

    // Acha o player ONLINE dono do steamID (null se offline).
    protected PlayerBase FindOnlinePlayer(string steamID)
    {
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase p = PlayerBase.Cast(players.Get(i));
            if (p && p.GetIdentity() && p.GetIdentity().GetPlainId() == steamID)
                return p;
        }
        return null;
    }

    int StartTracking(PlayerBase player)
    {
        if (!player || !player.GetIdentity())
            return 0;

        string steamID = player.GetIdentity().GetPlainId();
        int expiry = GetGame().GetTime() + TRACK_DURATION_MS;

        // Renova a sessao existente ou cria uma nova.
        DantyzTrackerSession sess = FindSession(steamID);
        if (sess)
        {
            sess.expiryTime = expiry;
        }
        else
        {
            sess = new DantyzTrackerSession();
            sess.steamID = steamID;
            sess.expiryTime = expiry;
            m_ActiveSessions.Insert(sess);
        }

        // UNICA leitura de disco do rastreador: o snapshot dos carros segurados.
        LoadSessionSnapshot(sess);

        // Envio inicial: cria os marcadores no cliente (RPCShowCarTracker
        // tambem mostra o resumo no chat).
        array<vector> positions = new array<vector>();
        array<string> names     = new array<string>();
        GatherInsuredPositions(sess, positions, names);

        GetRPCManager().SendRPC("DantyzShop", "RPCShowCarTracker", new Param2<ref array<vector>, ref array<string>>(positions, names), true, player.GetIdentity());

        // Liga o loop de atualizacao (10s) se ainda nao estiver rodando.
        if (!m_TickRunning)
        {
            m_TickRunning = true;
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, TRACK_INTERVAL_MS, true);
        }

        return positions.Count();
    }

    // Loop de atualizacao (roda a cada 10s enquanto houver sessao ativa).
    void Tick()
    {
        int now = GetGame().GetTime();

        // Percorre de tras pra frente pra poder remover sessoes com seguranca.
        for (int i = m_ActiveSessions.Count() - 1; i >= 0; i--)
        {
            DantyzTrackerSession s = m_ActiveSessions.Get(i);

            // Sessao expirou (passou dos 30 min): encerra.
            if (now >= s.expiryTime)
            {
                FlushSessionSnapshot(s);
                m_ActiveSessions.Remove(i);
                continue;
            }

            PlayerBase p = FindOnlinePlayer(s.steamID);
            if (!p || !p.GetIdentity())
            {
                FlushSessionSnapshot(s);
                m_ActiveSessions.Remove(i);
                continue;
            }

            // Recalcula as posicoes AO VIVO e manda o update so' pra este player.
            // Sem disco: le do snapshot + indice em memoria.
            array<vector> positions = new array<vector>();
            array<string> names     = new array<string>();
            GatherInsuredPositions(s, positions, names);

            GetRPCManager().SendRPC("DantyzShop", "RPCUpdateCarTracker", new Param2<ref array<vector>, ref array<string>>(positions, names), true, p.GetIdentity());
        }

        // Sem sessoes: desliga o loop pra nao gastar nada.
        if (m_ActiveSessions.Count() == 0)
            StopTick();
    }

    protected void StopTick()
    {
        m_TickRunning = false;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Tick);
    }

    // Deleta o item do rastreador (chamado por CallLater, com checagem de null).
    void DestroyTrackerItem(ItemBase item)
    {
        if (item)
            GetGame().ObjectDelete(item);
    }
}

class rastreadordecarro extends GPSReceiver
{
    protected bool m_DantyzTrackerUsed = false;

    override void OnWorkStart()
    {
        super.OnWorkStart();

        // So' o servidor processa, e so' uma vez (uso unico).
        if (!GetGame() || !GetGame().IsServer())
            return;
        if (m_DantyzTrackerUsed)
            return;

        // Precisa estar na posse de um player pra mostrar pra ele.
        PlayerBase player = PlayerBase.Cast(GetHierarchyRoot());
        if (!player || !player.GetIdentity())
            return;

        m_DantyzTrackerUsed = true;

        // Abre a sessao de rastreio (30 min, atualizando a cada 10s). Isso ja'
        // manda o primeiro RPC e liga o loop de atualizacao no servidor.
        int count = DantyzCarTracker.Get().StartTracking(player);

        if (DantyzShopMsg.On())
        {
            if (count > 0)
                player.MessageImportant("Rastreador ligado! " + count + " carro(s) marcado(s) no mapa por 30 minutos (acompanha o carro andando).");
            else
                player.MessageImportant("Rastreador ligado, mas voce nao tem carros segurados pra rastrear no momento.");
        }

        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DantyzCarTracker.Get().DestroyTrackerItem, 60000, false, this);
    }
}
