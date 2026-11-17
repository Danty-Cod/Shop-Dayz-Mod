class DantyzPlaytimeSession
{
    int PlayerId;      // id da conexao atual (muda ao reconectar)
    int Seconds;       // tempo seguido acumulado no ciclo atual
    int LastSeenTick;  // ultima checagem em que o player estava online
}

class DantyzPlaytimeRewards
{
    protected static const int TICK_SECONDS = 60;
    // Some por mais que isso (ticks) -> tira da memoria.
    protected static const int FORGET_AFTER_TICKS = 10;

    protected ref map<string, ref DantyzPlaytimeSession> m_Sessions;
    protected int m_TickCount;

    void DantyzPlaytimeRewards()
    {
        m_Sessions = new map<string, ref DantyzPlaytimeSession>();
    }

    void ~DantyzPlaytimeRewards()
    {
        if (GetGame())
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Tick);
    }

    void Start()
    {
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, TICK_SECONDS * 1000, true);
    }

    protected void Tick()
    {
        m_TickCount++;

        // Config fica em cache (Load nao le o disco de novo); reload do admin vale aqui.
        DantyzShopConfig config = DantyzShopConfig.Load();
        if (!config || config.PlaytimeRewardEnabled == 0 || config.PlaytimeRewardMinutes <= 0 || config.PlaytimeRewardCoins <= 0)
        {
            m_Sessions.Clear();
            return;
        }

        int neededSeconds = config.PlaytimeRewardMinutes * 60;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (!player)
                continue;

            PlayerIdentity identity = player.GetIdentity();
            if (!identity)
                continue;

            string steamID = identity.GetPlainId();
            int playerId = identity.GetPlayerId();

            DantyzPlaytimeSession session;
            if (!m_Sessions.Find(steamID, session) || session.PlayerId != playerId)
            {
                // Entrou agora (ou reconectou): comeca do zero.
                session = new DantyzPlaytimeSession();
                session.PlayerId = playerId;
                session.Seconds = 0;
                m_Sessions.Set(steamID, session);
            }
            else
            {
                session.Seconds += TICK_SECONDS;
            }

            session.LastSeenTick = m_TickCount;

            if (session.Seconds >= neededSeconds)
            {
                session.Seconds = 0;
                GiveReward(player, identity, steamID, config);
            }
        }

        // Limpa da memoria quem saiu do servidor.
        array<string> gone = new array<string>();
        foreach (string sid, DantyzPlaytimeSession s : m_Sessions)
        {
            if (m_TickCount - s.LastSeenTick > FORGET_AFTER_TICKS)
                gone.Insert(sid);
        }

        foreach (string goneID : gone)
        {
            m_Sessions.Remove(goneID);
        }
    }

    protected void GiveReward(PlayerBase player, PlayerIdentity identity, string steamID, DantyzShopConfig config)
    {
        DantyzPlayerAccount account = DantyzPlayerAccount.Load(steamID);
        account.AddCoins(config.PlaytimeRewardCoins);

        // Atualiza o saldo na tela da loja.
        GetRPCManager().SendRPC("DantyzShop", "RPCUpdateCoins", new Param1<int>(account.coins), true, identity);

        if (DantyzShopMsg.On())
            player.MessageImportant("Voce ganhou " + config.PlaytimeRewardCoins + " " + config.CoinName + " por jogar " + config.PlaytimeRewardMinutes + " minutos seguidos!");

    }
}
