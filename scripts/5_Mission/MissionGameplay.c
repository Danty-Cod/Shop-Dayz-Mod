modded class MissionGameplay
{
    private bool m_VipCheckDone = false;
    private float m_VipCheckTimer = 0;
    private bool m_TrackerPurgeDone = false;
    
    void MissionGameplay()
    {
        GetRPCManager().AddRPC("DantyzShop", "RPCPurchaseResult", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCUpdateCoins", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestBalance", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReloadComplete", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReceiveShopData", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReceiveConfig", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReceivePlayerVehicles", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCInsurancePurchased", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCOpenVipRoupasMenu", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReceiveVipStatus", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReceiveVipFilaStatus", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCShowCarTracker", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCUpdateCarTracker", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReceiveRoupasSelection", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReceiveRoupasConfig", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCReceiveCharList", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCTestDriveSeat", this, SingleplayerExecutionType.Client);
        GetRPCManager().AddRPC("DantyzShop", "RPCTestDriveExit", this, SingleplayerExecutionType.Client);
    }

    // TEST DRIVE: o servidor sentou o jogador no motorista; o cliente faz o mesmo (igual ao "entrar no carro")
    void RPCTestDriveSeat(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<Object> data;
        if (!ctx.Read(data))
            return;

        CarScript car = CarScript.Cast(data.param1);
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!car || !player || !car.DzIsTestDrive())
            return;

        // fecha a loja para o jogador ja' sair dirigindo
        DantyzShopMenu shop = DantyzShopMenu.Cast(GetGame().GetUIManager().GetMenu());
        if (shop && shop.GetID() == MENU_DANTYZ_SHOP)
            shop.CloseMenu();

        if (player.GetCommand_Vehicle())
            return;

        HumanCommandVehicle cmd = player.StartCommand_Vehicle(car, 0, car.GetSeatAnimationType(0), false);
        if (cmd)
            cmd.SetVehicleType(car.GetAnimInstance());
    }

    // TEST DRIVE: fim do teste - sai do carro antes de voltar ao lugar original
    void RPCTestDriveExit(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
            return;

        HumanCommandVehicle cmd = player.GetCommand_Vehicle();
        if (cmd)
        {
            cmd.KeepInVehicleSpaceAfterLeave(false);
            cmd.GetOutVehicle();
        }
    }
    
    // Recebe do servidor as posicoes dos carros segurados e cria os marcadores
    // 3D (so' pra este player) por 30 minutos. Tambem mostra um resumo no chat.
    void RPCShowCarTracker(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;
        
        Param2<ref array<vector>, ref array<string>> data = new Param2<ref array<vector>, ref array<string>>(new array<vector>(), new array<string>());
        if (!ctx.Read(data))
            return;
        
        array<vector> positions = data.param1;
        array<string> names     = data.param2;
        
        // Cria os marcadores PRIVADOS do BXD (so' pra este player) - aparecem
        // na lista "Private Marker" e no mapa. Somem em 30 min.
        DantyzCarMarkerClient.Get().ShowMarks(positions, names);
        
        // Resumo no chat (util mesmo se o icone estiver longe da tela).
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        vector pp = vector.Zero;
        if (player)
            pp = player.GetPosition();
        
        if (DantyzShopMsg.On())
        {
            if (positions && positions.Count() > 0)
            {
                GetGame().Chat("[Rastreador] " + positions.Count() + " carro(s) marcado(s) no mapa por 30 min:", "");
                for (int i = 0; i < positions.Count(); i++)
                {
                    string nm = "Carro";
                    if (names && i < names.Count())
                        nm = names.Get(i);
                    float d = vector.Distance(pp, positions.Get(i));
                    GetGame().Chat("  - " + nm + ": " + Math.Round(d).ToString() + "m", "");
                }
            }
            else
            {
                GetGame().Chat("[Rastreador] Nenhum carro segurado encontrado pra rastrear.", "");
            }
        }
    }

    // Recebe (a cada 10s) as posicoes atualizadas dos carros e MOVE os
    // marcadores existentes pra acompanhar o carro andando. Sem mensagem no
    // chat (seria spam) - so' atualiza os marcadores no mapa/3D.
    void RPCUpdateCarTracker(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param2<ref array<vector>, ref array<string>> data = new Param2<ref array<vector>, ref array<string>>(new array<vector>(), new array<string>());
        if (!ctx.Read(data))
            return;

        DantyzCarMarkerClient.Get().UpdateMarks(data.param1, data.param2);
    }
    
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        
        // IMPORTANTE: Só roda no CLIENTE
        if (!GetGame().IsClient())
            return;
        
        // Uma vez por sessao: limpa marcadores do rastreador que sobraram de
        // antes (evita ficarem pra sempre apos um restart/relog). Roda depois
        // que o player nasceu (ai o BXD ja carregou os marcadores privados).
        if (!m_TrackerPurgeDone && GetGame().GetPlayer())
        {
            m_TrackerPurgeDone = true;
            DantyzCarMarkerClient.Get().PurgeAll();
        }

        // Desenha os icones 3D dos carros rastreados (sistema proprio, sem BXD).
        DantyzCarMarkerClient.Get().RenderAll();
        
        // Verifica VIP após spawn
        if (!m_VipCheckDone)
        {
            m_VipCheckTimer += timeslice;
            
            // Aguarda 3 segundos após spawn para verificar VIP
            if (m_VipCheckTimer >= 3.0)
            {
                PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
                if (player)
                {
                    PlayerIdentity identity = player.GetIdentity();
                    if (identity)
                    {
                        string steamID = identity.GetPlainId();
                        
                        // Envia RPC para servidor verificar VIP
                        GetRPCManager().SendRPC("DantyzShop", "RPCCheckVipStatus", new Param1<string>(steamID), true, null);
                        
                        m_VipCheckDone = true;
                    }
                }
            }
        }
    }
    
    // Reseta o check quando player morre
    override void OnEvent(EventType eventTypeId, Param params)
    {
        super.OnEvent(eventTypeId, params);
        
        if (eventTypeId == ChatMessageEventTypeID)
        {
            ChatMessageEventParams chatParams = ChatMessageEventParams.Cast(params);
            if (chatParams && chatParams.param1 == CCDirect)
            {
                // Player morreu, reseta check
                m_VipCheckDone = false;
                m_VipCheckTimer = 0;
            }
        }
    }
    
    void RPCReceiveConfig(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param1<ref DantyzShopConfig> data;
            if (!ctx.Read(data))
            {
                return;
            }
            
            DantyzShopConfig config = data.param1;
            
            UIScriptedMenu menu = GetGame().GetUIManager().GetMenu();
            if (menu && menu.IsInherited(DantyzShopMenu))
            {
                DantyzShopMenu shopMenu = DantyzShopMenu.Cast(menu);
                if (shopMenu)
                {
                    shopMenu.OnReceiveConfig(config);
                }
            }
        }
    }
    
    void RPCReceiveShopData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param2<string, ref array<ref DantyzShopItem>> data;
            if (!ctx.Read(data))
            {
                return;
            }
            
            string categoryFileName = data.param1;
            array<ref DantyzShopItem> items = data.param2;
            
            UIScriptedMenu menu = GetGame().GetUIManager().GetMenu();
            if (menu && menu.IsInherited(DantyzShopMenu))
            {
                DantyzShopMenu shopMenu = DantyzShopMenu.Cast(menu);
                if (shopMenu)
                {
                    shopMenu.OnReceiveCategoryData(items);
                }
            }
        }
    }
    
    void RPCReceivePlayerVehicles(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param1<ref array<ref DantyzShopVehicleData>> data;
            if (!ctx.Read(data))
            {
                return;
            }
            
            array<ref DantyzShopVehicleData> vehicles = data.param1;
            
            UIScriptedMenu menu = GetGame().GetUIManager().GetMenu();
            if (menu && menu.IsInherited(DantyzShopMenu))
            {
                DantyzShopMenu shopMenu = DantyzShopMenu.Cast(menu);
                if (shopMenu)
                {
                    shopMenu.OnReceivePlayerVehicles(vehicles);
                }
            }
        }
    }
    
    override void OnKeyPress(int key)
    {
        super.OnKeyPress(key);
        
        // Tecla Y - Toggle loja
        if (key == KeyCode.KC_Y)
        {
            // Ignora se chat está aberto
            if (m_UIManager.IsMenuOpen(MENU_CHAT_INPUT))
                return;
            
            
            // Verifica se o menu está realmente aberto
            UIScriptedMenu currentMenu = GetGame().GetUIManager().GetMenu();
            bool isShopOpen = (currentMenu != NULL && currentMenu.GetID() == MENU_DANTYZ_SHOP);
            
            if (isShopOpen)
            {
                // Fechar loja
                GetGame().GetUIManager().HideScriptedMenu(currentMenu);
            }
            else
            {
                // Abrir loja (só se nenhum menu estiver aberto)
                if (currentMenu == NULL)
                {
                    GetGame().GetUIManager().EnterScriptedMenu(MENU_DANTYZ_SHOP, NULL);
                }
            }
        }
        
        if (key == KeyCode.KC_ESCAPE)
        {
            DantyzShopMenu escShop = DantyzShopMenu.Cast(GetGame().GetUIManager().GetMenu());
            if (escShop && escShop.GetID() == MENU_DANTYZ_SHOP)
            {
                GetGame().GetCallQueue(CALL_CATEGORY_GUI).Call(escShop.CloseMenu);
            }
        }
    }
    
    void RPCPurchaseResult(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param2<bool, string> data;
            if (!ctx.Read(data))
                return;
            
            bool success = data.param1;
            string message = data.param2;
            
            // Toggle global: se as mensagens do mod estiverem desligadas
            // (EnableMessages=0), nao mostra nada.
            if (!DantyzShopMsg.On())
                return;
            
            PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
            if (player)
            {
                player.MessageImportant(message);
            }
        }
    }
    
    void RPCUpdateCoins(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param1<int> data;
            if (!ctx.Read(data))
                return;
            
            int newCoins = data.param1;
            
            UIScriptedMenu currentMenu = GetGame().GetUIManager().GetMenu();
            if (currentMenu && currentMenu.IsInherited(DantyzShopMenu))
            {
                DantyzShopMenu shopMenu = DantyzShopMenu.Cast(currentMenu);
                if (shopMenu)
                {
                    shopMenu.UpdateCoins(newCoins);
                }
            }
        }
    }
    
    void RPCRequestBalance(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param1<int> data;
            if (!ctx.Read(data))
                return;
            
            int coins = data.param1;
            
            UIScriptedMenu currentMenu = GetGame().GetUIManager().GetMenu();
            if (currentMenu && currentMenu.IsInherited(DantyzShopMenu))
            {
                DantyzShopMenu shopMenu = DantyzShopMenu.Cast(currentMenu);
                if (shopMenu)
                {
                    shopMenu.UpdateCoins(coins);
                }
            }
        }
    }
    
    void RPCReloadComplete(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param1<bool> data;
            if (!ctx.Read(data))
                return;
            
            PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
            if (player && DantyzShopMsg.On())
            {
                player.MessageImportant("Servidor recarregado!");
            }
        }
    }
    
    void RPCInsurancePurchased(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            
            UIScriptedMenu menu = GetGame().GetUIManager().GetMenu();
            if (menu && menu.IsInherited(DantyzShopMenu))
            {
                DantyzShopMenu shopMenu = DantyzShopMenu.Cast(menu);
                if (shopMenu)
                {
                    shopMenu.OnInsurancePurchased();
                }
            }
        }
    }
    
    void RPCOpenVipRoupasMenu(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            // O menu de roupas agora fica DENTRO da loja, na aba "Escolher
            // Roupas". Em vez de abrir um menu separado, so' avisamos o player.
            PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
            if (player && DantyzShopMsg.On())
                player.MessageStatus("Abra a loja na aba 'Escolher Roupas' para montar seu visual de spawn.");
        }
    }

    // Recebe a selecao de roupas ja' salva e repassa pro painel embutido na loja.
    void RPCReceiveRoupasSelection(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<ref DantyzVipRoupasSelection> data = new Param1<ref DantyzVipRoupasSelection>(null);
        if (!ctx.Read(data))
            return;

        if (!data.param1)
            return;

        UIScriptedMenu top = GetGame().GetUIManager().GetMenu();
        DantyzShopMenu shopMenu = DantyzShopMenu.Cast(top);
        if (shopMenu)
            shopMenu.OnReceiveRoupasSelection(data.param1);
    }

    // Recebe a LISTA de roupas disponiveis (config) e repassa pro painel.
    void RPCReceiveRoupasConfig(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<ref DantyzVipRoupasConfig> data = new Param1<ref DantyzVipRoupasConfig>(null);
        if (!ctx.Read(data))
            return;

        if (!data.param1)
            return;

        UIScriptedMenu top = GetGame().GetUIManager().GetMenu();
        DantyzShopMenu shopMenu = DantyzShopMenu.Cast(top);
        if (shopMenu)
            shopMenu.OnReceiveRoupasConfig(data.param1);
    }

    // Recebe a lista de personagens jogaveis e repassa pro painel.
    void RPCReceiveCharList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param1<string> dataC;
        if (!ctx.Read(dataC))
            return;

        array<string> chars = new array<string>();
        if (dataC.param1 != "")
            dataC.param1.Split(",", chars);

        UIScriptedMenu topC = GetGame().GetUIManager().GetMenu();
        DantyzShopMenu shopMenuC = DantyzShopMenu.Cast(topC);
        if (shopMenuC)
            shopMenuC.OnReceiveCharList(chars);
    }
    
    void RPCReceiveVipStatus(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param3<bool, int, string> data;
            if (!ctx.Read(data))
            {
                return;
            }
            
            bool hasVip = data.param1;
            int daysRemaining = data.param2;
            string expirationDate = data.param3;
            
            
            // Atualiza o menu se estiver aberto
            UIScriptedMenu currentMenu = GetGame().GetUIManager().GetMenu();
            if (currentMenu && currentMenu.IsInherited(DantyzShopMenu))
            {
                DantyzShopMenu shopMenu = DantyzShopMenu.Cast(currentMenu);
                if (shopMenu)
                {
                    shopMenu.OnReceiveVipStatus(hasVip, daysRemaining, expirationDate);
                }
                else
                {
                }
            }
            else
            {
            }
        }
    }

    // Recebe status do VIP Fura-Fila e repassa pro menu
    void RPCReceiveVipFilaStatus(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Client)
        {
            Param3<bool, int, string> data;
            if (!ctx.Read(data))
            {
                return;
            }

            bool hasFila = data.param1;
            int daysRemaining = data.param2;
            string expirationDate = data.param3;


            UIScriptedMenu currentMenu = GetGame().GetUIManager().GetMenu();
            if (currentMenu && currentMenu.IsInherited(DantyzShopMenu))
            {
                DantyzShopMenu shopMenu = DantyzShopMenu.Cast(currentMenu);
                if (shopMenu)
                    shopMenu.OnReceiveVipFilaStatus(hasFila, daysRemaining, expirationDate);
            }
        }
    }
}
