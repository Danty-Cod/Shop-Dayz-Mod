modded class MissionServer
{
    ref DantyzShopVehicleConfig m_VehicleConfig;
    ref DantyzShopConfig m_ShopConfig;
    protected bool m_IsInitialized = false;
    ref DantyzPlaytimeRewards m_PlaytimeRewards;
    
    void MissionServer()
    {
        // sem licença/keys: o mod sempre inicia
        m_IsInitialized = true;
        
        
        DantyzShopFileSystem.EnsureDirectoriesExist();
        
        m_ShopConfig = DantyzShopConfig.Load();
        
        m_VehicleConfig = DantyzShopVehicleConfig.Load();
        
        // Inicializa VIP Manager
        DantyzVipManager.Init();
        
        // Inicializa VIP Fura-Fila Manager (priority.txt automatico)
        DantyzVipFilaManager.Init();
        
        // Inicializa sistema de kits VIP Roupas
        DantyzVipRoupasKitManager.Init();
        
        // Inicializa sistema de prêmios diários VIP
        DantyzVipDailyRewards.Init();
        
        // Moedas por tempo de jogo consecutivo (Config.json: PlaytimeReward*)
        m_PlaytimeRewards = new DantyzPlaytimeRewards();
        m_PlaytimeRewards.Start();
        
        GetRPCManager().AddRPC("DantyzShop", "RPCPurchaseItem", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCGetBalance", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCReloadShop", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestShopData", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestConfig", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestPlayerVehicles", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCPurchaseInsurance", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCPurchaseTracking", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRecallVehicle", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCCheckVipStatus", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCClaimVipFreeItem", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestVipStatus", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestVipFilaStatus", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCSaveRoupasSelection", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestRoupasSelection", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestRoupasConfig", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCRequestCharList", this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC("DantyzShop", "RPCTestDrive", this, SingleplayerExecutionType.Server);
        
        
    }
    
    bool IsInitialized()
    {
        return m_IsInitialized;
    }
    
    void RPCRequestConfig(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            if (!m_IsInitialized) return; // Licença inválida - não processar
            
            DantyzShopConfig config = DantyzShopConfig.Load();
            
            GetRPCManager().SendRPC("DantyzShop", "RPCReceiveConfig", new Param1<ref DantyzShopConfig>(config), true, sender);
        }
    }
    
    void RPCRequestShopData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            if (!m_IsInitialized) return; // Licença inválida - não processar
            
            Param1<string> data;
            if (!ctx.Read(data))
                return;
            
            string categoryFileName = data.param1;
            
            // COPIA, nao a instancia do cache: abaixo gravamos o estoque
            // restante em stockItem.Stock para o cliente exibir, e isso nao
            // pode sujar o valor de configuracao guardado no cache.
            array<ref DantyzShopItem> items = DantyzShopItemList.LoadCategoryCopy(categoryFileName);
            
            // ESTOQUE LIMITADO: monta a lista final so' com itens disponiveis.
            // Itens esgotados (Stock chegou a 0) NAO sao enviados -> o card
            // some da loja pro cliente. Itens ilimitados (Stock = -1) passam
            // normalmente. O campo Stock vai com o valor REAL restante.
            array<ref DantyzShopItem> availableItems = new array<ref DantyzShopItem>();
            for (int si = 0; si < items.Count(); si++)
            {
                DantyzShopItem stockItem = items.Get(si);
                
                if (DantyzShopStock.IsSoldOut(stockItem))
                {
                    continue;
                }
                
                // Sincroniza o estoque restante real para exibicao no cliente.
                stockItem.Stock = DantyzShopStock.GetRemaining(stockItem);
                availableItems.Insert(stockItem);
            }
            
            GetRPCManager().SendRPC("DantyzShop", "RPCReceiveShopData", new Param2<string, ref array<ref DantyzShopItem>>(categoryFileName, availableItems), true, sender);
        }
    }
    
    void RPCPurchaseItem(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            if (!m_IsInitialized) return; // Licença inválida - não processar
            
            Param3<string, string, int> data;
            if (!ctx.Read(data))
                return;
            
            string steamID = data.param1;
            string itemClass = data.param2;
            int price = data.param3;
            
            
            // BUSCA UNICA DO ITEM.
            //
            // O cliente manda apenas classname + preco, nunca a categoria, e o
            // servidor nao confia nesses valores: ele tem que achar o item real
            // nos JSON para saber preco, QuantityDrop, Attachments, Ground,
            // CargoItems e Stock. Antes essa busca era repetida tres vezes na
            // mesma compra (estoque, item completo, webhook). E' o MESMO item
            // com o MESMO preco nas tres, entao agora buscamos uma vez e
            // reaproveitamos abaixo.
            DantyzShopItem purchaseItem = FindItemInCategoriesByPrice(itemClass, price);
            
            // ESTOQUE LIMITADO: antes de cobrar, verifica se ainda ha unidades.
            // Defesa contra menu desatualizado (cliente que ainda mostra o card
            // de um item que ja esgotou). Itens ilimitados (Stock = -1) passam.
            if (purchaseItem && DantyzShopStock.IsSoldOut(purchaseItem))
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Item esgotado!"), true, sender);
                return;
            }
            
            DantyzPlayerAccount account = DantyzPlayerAccount.Load(steamID);
            
            if (!account.CanAfford(price))
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Saldo insuficiente!"), true, sender);
                return;
            }
            
            if (!account.Purchase(price))
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Erro ao processar compra!"), true, sender);
                return;
            }
            
            PlayerBase player = GetPlayerBySteamID(steamID);
            if (player)
            {
                bool success = false;
                string message = "";
                
                // ESCOLHER_ROUPAS: card que abre o menu de selecao de roupas de
                // spawn (so' pra quem tem VIP ativo). Nao entrega item nenhum.
                // Classnames "virtuais" (nao existem no jogo, sao cards especiais).
                // Comparados sem caixa, pra nao quebrar se alguem editar o JSON
                // escrevendo "vip_roupas_7" em vez de "VIP_ROUPAS_7".
                if (DantyzShopUtils.SameClass(itemClass, "ESCOLHER_ROUPAS"))
                {
                    if (DantyzVipManager.HasActiveVip(steamID))
                    {
                        GetRPCManager().SendRPC("DantyzShop", "RPCOpenVipRoupasMenu", null, true, sender);
                        message = "Escolha suas roupas de spawn!";
                        success = true;
                    }
                    else
                    {
                        message = "Voce precisa de VIP Roupas ativo para escolher!";
                        success = false;
                    }
                }
                // Verifica se é compra de VIP
                else if (DantyzShopUtils.SameClass(itemClass, "VIP_ROUPAS_FREE_2D"))
                {
                    // ============================================
                    // VIP Gratuito: 3 dias, uma vez por SteamID.
                    // ============================================

                    if (DantyzVipFreeOnce.AlreadyClaimed(steamID))
                    {
                        message = "Voce ja utilizou o VIP gratuito! Cada player so pode resgatar uma vez.";
                        success = false;
                    }
                    else
                    {
                        // Os dias vem do campo "Days" do card no JSON (viproupas.json).
                        // Se falhar em achar o card ou o campo Days estiver invalido,
                        // usa 3 dias como padrao fixo.
                        DantyzShopItem fullFreeItem = purchaseItem;
                        int freeDays = 3;  // padrao: 3 dias
                        int freeKit = 1;   // padrao: kit 1

                        if (fullFreeItem)
                        {
                            if (fullFreeItem.Days > 0)
                                freeDays = fullFreeItem.Days;
                            if (fullFreeItem.KitID > 0)
                                freeKit = fullFreeItem.KitID;
                        }
                        else
                        {
                        }

                        DantyzVipManager.AddVip(steamID, freeDays, freeKit);
                        DantyzVipFreeOnce.MarkAsClaimed(steamID);
                        message = "VIP Roupas gratuito por " + freeDays + " dias ativado! Voce recebera as roupas ao morrer. Aproveite!";
                        success = true;
                    }
                }
                else if (itemClass.Contains("VIP_ROUPAS_"))
                {
                    int days = 0;
                    int kitID = 1; // Padrão
                    
                    // Extrai dias e kitID do classname
                    if (DantyzShopUtils.SameClass(itemClass, "VIP_ROUPAS_7"))
                    {
                        days = 7;
                        kitID = 1;
                    }
                    else if (DantyzShopUtils.SameClass(itemClass, "VIP_ROUPAS_20"))
                    {
                        days = 20;
                        kitID = 2;
                    }
                    else if (DantyzShopUtils.SameClass(itemClass, "VIP_ROUPAS_30"))
                    {
                        days = 30;
                        kitID = 3;
                    }
                    
                    if (days > 0)
                    {
                        DantyzVipManager.AddVip(steamID, days, kitID);
                        message = "VIP Roupas ativado por " + days + " dias! Você receberá as roupas ao morrer.";
                        success = true;
                    }
                    else
                    {
                        message = "Erro ao ativar VIP!";
                        success = false;
                    }
                }
                else if (itemClass.Contains("VIP_FILA_"))
                {
                    // ============================================
                    // VIP Fura-Fila: adiciona o SteamID no priority.txt
                    // do servidor, dando prioridade na loginQueue do DayZ.
                    // Se ja existe, soma os dias.
                    // Quando expirar, o manager limpa sozinho.
                    //
                    // Os dias vem do campo "Days" no JSON da categoria
                    // (vipfila.json). Voce pode criar quantos cards quiser
                    // sem mexer em codigo: VIP_FILA_7, VIP_FILA_45, etc.
                    // ============================================
                    DantyzShopItem fullFilaItem = purchaseItem;
                    int filaDays = 0;
                    if (fullFilaItem) filaDays = fullFilaItem.Days;

                    if (filaDays > 0)
                    {
                        DantyzVipFilaManager.AddVipFila(steamID, filaDays);
                        message = "VIP Fura-Fila ativado por " + filaDays + " dias! Voce entra na frente da fila.";
                        success = true;
                    }
                    else
                    {
                        message = "Erro: configure o campo 'Days' no JSON do card!";
                        success = false;
                    }
                }
                else if (m_VehicleConfig.IsVehicle(itemClass))
                {
                    // Busca o item completo para pegar CargoItems
                    DantyzShopItem fullVehicleItem = purchaseItem;
                    
                    success = SpawnVehicle(player, itemClass, fullVehicleItem, message);
                    
                    if (success)
                    {
                        vector playerPos = player.GetPosition();
                        array<Object> nearbyObjects = new array<Object>();
                        GetGame().GetObjectsAtPosition(playerPos, 15, nearbyObjects, null);
                        
                        for (int vIdx = 0; vIdx < nearbyObjects.Count(); vIdx++)
                        {
                            Object nearbyObj = nearbyObjects.Get(vIdx);
                            CarScript spawnedCar = CarScript.Cast(nearbyObj);
                            // Comparacao SEM caixa: GetType() devolve o classname
                            // do config (ex: "Offroad_02") e itemClass vem do JSON
                            // da loja (ex: "offroad_02"). Com "==" isso nunca batia,
                            // o RegisterVehiclePurchase nao era chamado e o carro
                            // NAO aparecia na aba de Seguros.
                            if (spawnedCar && DantyzShopUtils.SameClass(spawnedCar.GetType(), itemClass))
                            {
                                RegisterVehiclePurchase(player, spawnedCar, itemClass, price);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    DantyzShopItem fullItem = purchaseItem;
                    
                    if (fullItem)
                    {
                    }
                    else
                    {
                    }
                    
                    if (fullItem && (fullItem.Attachments.Count() > 0 || fullItem.Ground.Count() > 0))
                    {
                        success = GiveItemWithExtras(player, fullItem, message);
                    }
                    else if (fullItem && fullItem.QuantityDrop > 1)
                    {
                        success = GiveMultipleItems(player, fullItem, message);
                    }
                    else
                    {
                        success = GiveItem(player, itemClass, message);
                    }
                }
                
                if (success)
                {

                    // Webhook Discord: compra na loja (informativo)
                    string whItemName = itemClass;
                    DantyzShopItem whItem = purchaseItem;
                    if (whItem) whItemName = whItem.Name;
                    
                    // ESTOQUE LIMITADO: desconta 1 unidade agora que a compra
                    // deu certo. Se zerar, o item nao sera mais enviado ao abrir
                    // a loja (some o card). Ilimitado (-1) nao faz nada.
                    if (whItem)
                        DantyzShopStock.Consume(whItem);
                    string whBuyerName = steamID;
                    if (sender) whBuyerName = sender.GetName();
                    DantyzShopWebhook.SendCompra(whBuyerName, steamID, whItemName, price);

                    GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(true, message), true, sender);
                    GetRPCManager().SendRPC("DantyzShop", "RPCUpdateCoins", new Param1<int>(account.coins), true, sender);
                }
                else
                {
                    account.AddCoins(price);
                    GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, message), true, sender);
                }
            }
            else
            {
                account.AddCoins(price);
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Player nao encontrado!"), true, sender);
            }
        }
    }
    
    DantyzShopItem FindItemInCategories(string itemClass)
    {
        
        for (int i = 0; i < m_ShopConfig.Categories.Count(); i++)
        {
            DantyzShopCategory cat = m_ShopConfig.Categories.Get(i);
            
            if (!cat.Enabled)
                continue;
            
            array<ref DantyzShopItem> items = DantyzShopItemList.LoadCategory(cat.FileName);
            
            for (int j = 0; j < items.Count(); j++)
            {
                DantyzShopItem item = items.Get(j);
                
                if (DantyzShopUtils.SameClass(item.Classname, itemClass))
                {
                    return item;
                }
            }
        }
        
        return null;
    }
    
    DantyzShopItem FindItemInCategoriesByPrice(string itemClass, int price)
    {
        
        for (int i = 0; i < m_ShopConfig.Categories.Count(); i++)
        {
            DantyzShopCategory cat = m_ShopConfig.Categories.Get(i);
            
            if (!cat.Enabled)
                continue;
            
            array<ref DantyzShopItem> items = DantyzShopItemList.LoadCategory(cat.FileName);
            
            for (int j = 0; j < items.Count(); j++)
            {
                DantyzShopItem item = items.Get(j);
                
                if (DantyzShopUtils.SameClass(item.Classname, itemClass) && item.Price == price)
                {
                    return item;
                }
            }
        }
        
        return null;
    }
    
    bool GiveItem(PlayerBase player, string itemClass, out string message)
    {
        EntityAI item = player.GetInventory().CreateInInventory(itemClass);
        
        if (item)
        {
            message = "Item adicionado ao inventario!";
            return true;
        }
        
        vector playerPos = player.GetPosition();
        item = EntityAI.Cast(GetGame().CreateObject(itemClass, playerPos, false, true, true));
        
        if (item)
        {
            message = "Inventario cheio! Item dropado no chao.";
            return true;
        }
        
        message = "Erro ao criar item!";
        return false;
    }
    
    bool GiveMultipleItems(PlayerBase player, ref DantyzShopItem shopItem, out string message)
    {
        
        int totalInInventory = 0;
        int totalOnGround = 0;
        
        for (int i = 0; i < shopItem.QuantityDrop; i++)
        {
            EntityAI item = player.GetInventory().CreateInInventory(shopItem.Classname);
            
            if (item)
            {
                totalInInventory++;
            }
            else
            {
                vector playerPos = player.GetPosition();
                
                float radius = 0.5 + (Math.RandomFloat01() * 0.5);
                float angle = (360.0 / Math.Max(shopItem.QuantityDrop - totalInInventory, 1)) * totalOnGround + (Math.RandomFloat01() * 30.0);
                float radians = angle * Math.DEG2RAD;
                
                vector offset = Vector(Math.Cos(radians) * radius, 0, Math.Sin(radians) * radius);
                vector spawnPos = playerPos + offset;
                spawnPos[1] = playerPos[1];
                
                EntityAI groundItem = EntityAI.Cast(GetGame().CreateObject(shopItem.Classname, spawnPos, false, true, true));
                
                if (groundItem)
                {
                    totalOnGround++;
                }
                else
                {
                }
            }
        }
        
        if (totalInInventory > 0 && totalOnGround > 0)
        {
            message = totalInInventory.ToString() + " item(ns) no inventario + " + totalOnGround.ToString() + " no chao!";
        }
        else if (totalInInventory > 0)
        {
            message = totalInInventory.ToString() + " item(ns) adicionado(s) ao inventario!";
        }
        else if (totalOnGround > 0)
        {
            message = totalOnGround.ToString() + " item(ns) dropado(s) no chao!";
        }
        else
        {
            message = "Erro ao criar itens!";
            return false;
        }
        
        return true;
    }
    
    bool GiveItemWithExtras(PlayerBase player, ref DantyzShopItem shopItem, out string message)
    {
        
        EntityAI mainItem = player.GetInventory().CreateInInventory(shopItem.Classname);
        bool droppedOnGround = false;
        
        if (!mainItem)
        {
            vector playerPos = player.GetPosition();
            mainItem = EntityAI.Cast(GetGame().CreateObject(shopItem.Classname, playerPos, false, true, true));
            droppedOnGround = true;
            
            if (!mainItem)
            {
                message = "Erro ao criar item principal!";
                return false;
            }
        }
        
        
        int totalItemsInside = 0;
        int totalItemsOnGround = 0;
        
        vector basePos;
        vector spawnPos;
        vector offset;
        float radius;
        float angle;
        float radians;
        EntityAI groundEntity;
        
        for (int i = 0; i < shopItem.Attachments.Count(); i++)
        {
            DantyzShopAttachment attach = shopItem.Attachments.Get(i);
            int qty = attach.Quantity;
            
            if (qty <= 0)
                qty = 1;
            
            
            for (int q = 0; q < qty; q++)
            {
                EntityAI attachment = mainItem.GetInventory().CreateAttachment(attach.Classname);
                
                if (attachment)
                {
                    totalItemsInside++;
                    continue;
                }
                
                ItemBase itemBase = ItemBase.Cast(mainItem);
                if (itemBase)
                {
                    EntityAI cargoItem = itemBase.GetInventory().CreateInInventory(attach.Classname);
                    if (cargoItem)
                    {
                        totalItemsInside++;
                        continue;
                    }
                }
                
                basePos = mainItem.GetPosition();
                radius = 0.5 + (Math.RandomFloat01() * 0.5);
                angle = (360.0 / Math.Max(qty, 1)) * totalItemsOnGround + (Math.RandomFloat01() * 30.0);
                radians = angle * Math.DEG2RAD;
                offset = Vector(Math.Cos(radians) * radius, 0, Math.Sin(radians) * radius);
                spawnPos = basePos + offset;
                spawnPos[1] = basePos[1];
                
                groundEntity = EntityAI.Cast(GetGame().CreateObject(attach.Classname, spawnPos, false, true, true));
                
                if (groundEntity)
                {
                    totalItemsOnGround++;
                }
                else
                {
                }
            }
        }
        
        for (int j = 0; j < shopItem.Ground.Count(); j++)
        {
            DantyzShopAttachment groundAttach = shopItem.Ground.Get(j);
            int qtyGround = groundAttach.Quantity;
            
            if (qtyGround <= 0)
                qtyGround = 1;
            
            
            for (int k = 0; k < qtyGround; k++)
            {
                basePos = mainItem.GetPosition();
                radius = 0.5 + (Math.RandomFloat01() * 0.5);
                angle = (360.0 / Math.Max(qtyGround, 1)) * totalItemsOnGround + (Math.RandomFloat01() * 30.0);
                radians = angle * Math.DEG2RAD;
                offset = Vector(Math.Cos(radians) * radius, 0, Math.Sin(radians) * radius);
                spawnPos = basePos + offset;
                spawnPos[1] = basePos[1];
                
                groundEntity = EntityAI.Cast(GetGame().CreateObject(groundAttach.Classname, spawnPos, false, true, true));
                
                if (groundEntity)
                {
                    totalItemsOnGround++;
                }
                else
                {
                }
            }
        }
        
        if (droppedOnGround)
        {
            message = "Item dropado no chao";
        }
        else
        {
            message = "Item adicionado ao inventario";
        }
        
        if (totalItemsInside > 0)
        {
            message += " com " + totalItemsInside + " item(ns) dentro";
        }
        
        if (totalItemsOnGround > 0)
        {
            message += " + " + totalItemsOnGround + " item(ns) no chao";
        }
        
        message += "!";
        
        return true;
    }
    
    void AddAttachmentsRecursive(EntityAI parentItem, ref array<ref DantyzShopItem> attachments)
    {
        // Método removido - não é mais necessário
    }
    
    bool SpawnVehicle(PlayerBase player, string vehicleClass, ref DantyzShopItem fullItem, out string message)
    {
        DantyzVehicleParts vehConfig = m_VehicleConfig.GetVehicleConfig(vehicleClass);
        
        if (!vehConfig)
        {
            message = "Configuracao do veiculo nao encontrada!";
            return false;
        }
        
        vector playerPos = player.GetPosition();
        vector playerDir = player.GetDirection();
        playerDir.Normalize();
        
        vector spawnPos;
        bool foundFreeSpot = false;
        float distance = vehConfig.SpawnDistance;
        
        array<vector> directions = new array<vector>();
        directions.Insert(playerDir);
        directions.Insert(playerDir * -1);
        directions.Insert(Vector(playerDir[2], 0, -playerDir[0]));
        directions.Insert(Vector(-playerDir[2], 0, playerDir[0]));
        directions.Insert((playerDir + Vector(playerDir[2], 0, -playerDir[0])).Normalized());
        directions.Insert((playerDir + Vector(-playerDir[2], 0, playerDir[0])).Normalized());
        directions.Insert((playerDir * -1 + Vector(playerDir[2], 0, -playerDir[0])).Normalized());
        directions.Insert((playerDir * -1 + Vector(-playerDir[2], 0, playerDir[0])).Normalized());
        
        for (int i = 0; i < directions.Count(); i++)
        {
            vector testDir = directions.Get(i);
            spawnPos = playerPos + (testDir * distance);
            spawnPos[1] = GetGame().SurfaceY(spawnPos[0], spawnPos[2]) + 0.5;
            
            array<Object> objects = new array<Object>();
            array<CargoBase> cargos = new array<CargoBase>();
            
            GetGame().GetObjectsAtPosition(spawnPos, 3.0, objects, cargos);
            
            bool hasObstacle = false;
            for (int j = 0; j < objects.Count(); j++)
            {
                Object obj = objects.Get(j);
                
                if (obj == player)
                    continue;
                
                if (obj.IsBuilding() || obj.IsTree() || obj.IsRock() || obj.IsInherited(CarScript))
                {
                    hasObstacle = true;
                    break;
                }
            }
            
            if (!hasObstacle)
            {
                foundFreeSpot = true;
                break;
            }
        }
        
        if (!foundFreeSpot)
        {
            message = "Nao ha espaco livre para spawnar o veiculo! Afaste-se de obstaculos.";
            return false;
        }
        
        CarScript vehicle = CarScript.Cast(GetGame().CreateObject(vehicleClass, spawnPos, false, true, true));
        
        if (!vehicle)
        {
            message = "Erro ao spawnar veiculo!";
            return false;
        }
        
        vehicle.SetOrientation(player.GetOrientation());
        
        // Adiciona peças do veículo
        for (int k = 0; k < vehConfig.Parts.Count(); k++)
        {
            string partClass = vehConfig.Parts.Get(k);
            vehicle.GetInventory().CreateAttachment(partClass);
        }
        
        // Enche fluidos
        vehicle.Fill(CarFluid.FUEL, vehicle.GetFluidCapacity(CarFluid.FUEL));
        vehicle.Fill(CarFluid.OIL, vehicle.GetFluidCapacity(CarFluid.OIL));
        vehicle.Fill(CarFluid.BRAKE, vehicle.GetFluidCapacity(CarFluid.BRAKE));
        vehicle.Fill(CarFluid.COOLANT, vehicle.GetFluidCapacity(CarFluid.COOLANT));
        
        // Adiciona itens no cargo do veículo
        if (fullItem && fullItem.CargoItems && fullItem.CargoItems.Count() > 0)
        {
            
            EntityAI vehicleEntity = EntityAI.Cast(vehicle);
            if (vehicleEntity)
            {
                for (int m = 0; m < fullItem.CargoItems.Count(); m++)
                {
                    DantyzShopAttachment cargoItem = fullItem.CargoItems.Get(m);
                    int qty = cargoItem.Quantity;
                    
                    if (qty <= 0)
                        qty = 1;
                    
                    for (int n = 0; n < qty; n++)
                    {
                        EntityAI item = vehicleEntity.GetInventory().CreateInInventory(cargoItem.Classname);
                        if (item)
                        {
                        }
                        else
                        {
                        }
                    }
                }
            }
        }
        
        message = "Veiculo spawnado com sucesso!";

        return true;
    }
    
    void RegisterVehiclePurchase(PlayerBase player, CarScript vehicle, string vehicleClass, int vehiclePrice)
    {
        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
            return;
        
        string steamID = identity.GetPlainId();
        
        int netLow, netHigh;
        vehicle.GetNetworkID(netLow, netHigh);
        int vehicleNetID = netLow | netHigh;
        
        // Captura o PersistentID (ID ESTAVEL, sobrevive a restart). Em carro
        // recem-criado pode ainda nao estar pronto (fica "0_0_0_0"); nesse caso
        // ele e' (re)capturado de forma garantida na hora de COMPRAR o seguro.
        int pp1, pp2, pp3, pp4;
        vehicle.GetPersistentID(pp1, pp2, pp3, pp4);
        string persistentID = "" + pp1 + "_" + pp2 + "_" + pp3 + "_" + pp4;
        
        DantyzShopItem vehicleItem = FindItemInCategoriesByPrice(vehicleClass, vehiclePrice);
        string vehicleName = vehicleClass;
        if (vehicleItem)
        {
            vehicleName = vehicleItem.Name;
        }
        
        DantyzShopPlayerVehicles playerVehicles = DantyzShopPlayerVehicles.Load(steamID);
        playerVehicles.RegisterVehicle(vehicleNetID, persistentID, vehicleClass, vehicleName, vehiclePrice);
        

        // VINCULO NA COMPRA: grava dono e preco no proprio carro (em memoria).
        // Quando o jogador aplicar o CarLock, o seguro liga na hora - sem varrer
        // players e sem depender do PersistentID, que num carro recem-criado
        // ainda pode nao estar pronto.
        vehicle.DzBindBuyer(steamID, vehiclePrice);

        // Captura o PersistentID de forma GARANTIDA logo apos a COMPRA (com
        // retentativas). Carro recem-criado retorna "0_0_0_0" ate' o sistema de
        // persistencia atribuir o ID, por isso a 1a captura acima costuma vir
        // vazia. Assim o rastreador/seguro ja' tem a chave estavel ANTES mesmo
        // de comprar o seguro. (A captura no seguro continua como rede de
        // seguranca -> compativel com registros antigos.)
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CaptureVehiclePID, 2000, false, steamID, vehicle, vehicleNetID, 5);
        
        if (DantyzShopMsg.On())
            player.MessageImportant("Carro comprado! Va na aba SEGUROS para proteger seu veiculo.");
    }

    // Captura o PersistentID de um carro recem-comprado e grava no registro.
    // ---------------------------------------------------------------------
    // Um carro criado agora retorna "0_0_0_0" ate' a persistencia atribuir o
    // ID. Entao esta funcao tenta de novo (a cada 2s, ate' 'attemptsLeft'
    // vezes) ate' o ID ficar valido. So' grava se o registro ainda nao tiver um
    // PID valido, pra NUNCA sobrescrever um PID ja' capturado (compatibilidade
    // total com o fluxo antigo do seguro).
    void CaptureVehiclePID(string steamID, CarScript vehicle, int vehicleNetID, int attemptsLeft)
    {
        if (!vehicle)
            return;

        int p1, p2, p3, p4;
        vehicle.GetPersistentID(p1, p2, p3, p4);
        string pid = "" + p1 + "_" + p2 + "_" + p3 + "_" + p4;

        // Ainda nao esta' pronto: tenta de novo mais tarde.
        if (pid == "" || pid == "0_0_0_0")
        {
            if (attemptsLeft > 0)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CaptureVehiclePID, 2000, false, steamID, vehicle, vehicleNetID, attemptsLeft - 1);
            return;
        }

        // Agora que o PID e' valido, o carro segurado entra no indice em memoria.
        // (No spawn/puxada o PID ainda era 0_0_0_0 e o indice nao aceitava.)
        vehicle.DzRefreshIndex();

        DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(steamID);
        DantyzShopVehicleData v = pv.GetVehicleByNetworkID(vehicleNetID);
        if (!v)
            return;

        // So' grava se ainda nao houver PID valido (nao sobrescreve o antigo).
        if (v.PersistentID == "" || v.PersistentID == "0_0_0_0")
        {
            v.PersistentID = pid;

            // Guarda a posicao atual como ultima posicao conhecida (pro rastreador).
            vector pos = vehicle.GetPosition();
            v.LastPosX = pos[0];
            v.LastPosY = pos[1];
            v.LastPosZ = pos[2];
            v.HasLastPos = true;

            pv.Save();
        }
    }
    
    // TEST DRIVE (gratis): o cliente manda so' classe e preco do card; quem testa e'
    // sempre quem enviou (sender). O carro precisa existir numa categoria da loja.
    void RPCTestDrive(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server || !m_IsInitialized || !sender)
            return;

        Param2<string, int> data;
        if (!ctx.Read(data))
            return;

        string steamID = sender.GetPlainId();
        string message = "";
        bool success = false;

        DantyzShopItem item = FindItemInCategoriesByPrice(data.param1, data.param2);
        PlayerBase player = GetPlayerBySteamID(steamID);

        if (!item || !m_VehicleConfig.IsVehicle(item.Classname))
            message = "Este item nao pode ser testado.";
        else if (!player)
            message = "Jogador nao encontrado.";
        else
            success = DantyzTestDriveManager.Start(player, steamID, item.Classname, m_VehicleConfig, DantyzShopConfig.Load(), message);

        GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(success, message), true, sender);
    }

    override void InvokeOnDisconnect(PlayerBase player)
    {
        DantyzTestDriveManager.OnDisconnect(player);
        super.InvokeOnDisconnect(player);
    }

    PlayerBase GetPlayerBySteamID(string steamID)
    {
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (player)
            {
                PlayerIdentity identity = player.GetIdentity();
                if (identity && identity.GetPlainId() == steamID)
                {
                    return player;
                }
            }
        }
        
        return null;
    }
    
    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        
        if (identity)
        {
            string steamID = identity.GetPlainId();
            DantyzPlayerAccount account = DantyzPlayerAccount.Load(steamID);
            

            // RESISTENCIA: paga seguros pendentes que venceram a espera enquanto
            // o player estava offline ou o servidor reiniciou (o CallLater nao
            // sobrevive a isso). Usa o tempo REAL (epoch) gravado na destruicao.
            // Espera 5s pra o player carregar antes de creditar/avisar.
            string whName = identity.GetName();
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ProcessPlayerClaimsOnConnect, 5000, false, steamID, whName);
        }
    }

    // Retomada de seguros pendentes na conexao (cobre restart e logout).
    void ProcessPlayerClaimsOnConnect(string steamID, string playerName)
    {
        DantyzInsurance.ProcessAllClaimableForPlayer(steamID, playerName);

        // Resincroniza o saldo na tela do player (caso tenha recebido offline).
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players.Get(i));
            if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == steamID)
            {
                DantyzPlayerAccount acc = DantyzPlayerAccount.Load(steamID);
                GetRPCManager().SendRPC("DantyzShop", "RPCUpdateCoins", new Param1<int>(acc.coins), true, player.GetIdentity());
                break;
            }
        }
    }
    
    override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
    {
        super.StartingEquipSetup(player, clothesChosen);
        
        
        // Este método é chamado quando o player escolhe personagem/spawna
        PlayerIdentity identity = player.GetIdentity();
        if (identity)
        {
            string steamID = identity.GetPlainId();
            
            
            // Verifica se player tem VIP ativo e equipa roupas
            DantyzVipData vipData = DantyzVipManager.GetVipData(steamID);
            if (vipData)
            {
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EquipVipClothes, 3000, false, player, vipData.KitID);
            }
            else
            {
            }
        }
    }
    
    // Hook para SpawnSelect - chamado quando player spawna pela primeira vez
    // Sobrescreve a criacao do personagem: se o player ESCOLHEU um modelo de
    // personagem (seletor no menu de roupas), nasce com esse modelo. Senao,
    // usa o tipo padrao que o jogo decidiu. Vale pra spawn novo e respawn.
    override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
    {
        if (identity)
        {
            string steamID = identity.GetPlainId();
            if (DantyzVipRoupasSelection.HasSelection(steamID))
            {
                DantyzVipRoupasSelection sel = DantyzVipRoupasSelection.Load(steamID);
                if (sel && sel.CharacterType != "")
                {
                    // So' usa se for um personagem valido do jogo (anti-erro).
                    if (g_Game.ListAvailableCharacters().Find(sel.CharacterType) > -1)
                    {
                        characterName = sel.CharacterType;
                    }
                }
            }
        }

        return super.CreateCharacter(identity, pos, ctx, characterName);
    }

    override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
    {
        PlayerBase player = super.OnClientNewEvent(identity, pos, ctx);
        
        if (player && identity)
        {
            string steamID = identity.GetPlainId();
            
            
            // Agenda equipar VIP após 5 segundos (tempo para SpawnSelect posicionar o player)
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(EquipVipOnNewSpawn, 5000, false, player, steamID);
        }
        
        return player;
    }
    
    void EquipVipOnNewSpawn(PlayerBase player, string steamID)
    {
        if (!player || !player.IsAlive())
        {
            return;
        }
        
        
        DantyzVipData vipData = DantyzVipManager.GetVipData(steamID);
        if (vipData)
        {
            EquipVipClothes(player, vipData.KitID);
        }
        else
        {
        }
    }
    
    void EquipVipClothes(PlayerBase player, int kitID)
    {
        if (!player)
        {
            return;
        }

        // GUARD DE VIP (fonte de verdade = sistema novo PlayersVip, o MESMO
        // usado no status da UI). Sem VIP ATIVO, NAO equipa nada: nem a selecao
        // de roupas salva, nem o kit fixo. Corrige o bug de nascer com a roupa
        // VIP mesmo sem ter VIP (dessincronia com o sistema antigo VipRoupas.json).
        PlayerIdentity vipIdent = player.GetIdentity();
        if (!vipIdent || !DantyzVipPlayerChecker.HasActiveVip(vipIdent.GetPlainId()))
        {
            return;
        }

        // NOVO: se o player ESCOLHEU roupas de spawn (menu de selecao), usa a
        // escolha dele. Senao, cai no KIT FIXO de sempre (compativel).
        // Em AMBOS os casos os ITENS BONUS do VIP sao entregues.
        PlayerIdentity ident = player.GetIdentity();
        if (ident)
        {
            string steamID = ident.GetPlainId();
            if (DantyzVipRoupasSelection.HasSelection(steamID))
            {
                DantyzVipRoupasSelection sel = DantyzVipRoupasSelection.Load(steamID);
                if (sel && sel.HasAny())
                {
                    DantyzVipRoupasSelectionHelper.EquipSelection(player, sel);
                    // Entrega os itens bonus do kit do VIP (comida/faca/etc).
                    DantyzVipRoupasKitManager.GiveBonusItems(player, kitID);
                    if (DantyzShopMsg.On())
                        player.MessageImportant("Suas roupas VIP foram equipadas!");
                    return;
                }
            }
        }

        DantyzVipRoupasKitManager.EquipKitOnPlayer(player, kitID);
        
        // Envia mensagem para o player
        if (DantyzShopMsg.On())
            player.MessageImportant("Suas roupas VIP foram equipadas!");
    }
    
    void SendOpenVipMenuRPC(PlayerIdentity identity)
    {
        if (identity)
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCOpenVipRoupasMenu", null, true, identity);
        }
    }

    // ========================================================================
    // SELECAO DE ROUPAS DE SPAWN (player escolhe o que vai nascer vestindo)
    // ------------------------------------------------------------------------
    // Recebe a selecao do cliente, valida VIP ativo + whitelist (anti-cheat) e
    // salva no arquivo do player. Aplicada no proximo spawn (EquipVipClothes).
    // ========================================================================
    void RPCSaveRoupasSelection(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;

        Param1<ref DantyzVipRoupasSelection> data = new Param1<ref DantyzVipRoupasSelection>(null);
        if (!ctx.Read(data))
            return;

        DantyzVipRoupasSelection sel = data.param1;
        if (!sel || !sender)
            return;

        string steamID = sender.GetPlainId();

        // So' VIP ativo pode salvar selecao de roupas.
        if (!DantyzVipManager.HasActiveVip(steamID))
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Voce precisa de VIP Roupas ativo!"), true, sender);
            return;
        }

        // Anti-cheat: filtra qualquer classname que nao esteja no VipRoupasConfig.
        DantyzVipRoupasConfig config = DantyzVipRoupasConfig.Load();
        DantyzVipRoupasSelectionHelper.Sanitize(sel, config);

        // Valida o personagem escolhido contra a lista oficial do jogo.
        if (sel.CharacterType != "" && g_Game.ListAvailableCharacters().Find(sel.CharacterType) == -1)
            sel.CharacterType = "";

        sel.SteamID = steamID;
        sel.Save();


        GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(true, "Roupas de spawn salvas! Voce nascera com elas."), true, sender);
    }

    // Envia a selecao atual do player de volta pro cliente (pra marcar o que ja
    // estava escolhido ao abrir o menu).
    void RPCRequestRoupasSelection(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;

        if (!sender)
            return;

        string steamID = sender.GetPlainId();
        DantyzVipRoupasSelection sel = DantyzVipRoupasSelection.Load(steamID);
        GetRPCManager().SendRPC("DantyzShop", "RPCReceiveRoupasSelection", new Param1<ref DantyzVipRoupasSelection>(sel), true, sender);
    }

    // Envia a LISTA de roupas disponiveis (VipRoupasConfig) pro cliente, pois o
    // arquivo so' existe no profile do SERVIDOR. Sem isso o menu fica vazio.
    void RPCRequestRoupasConfig(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        DantyzVipRoupasConfig cfg = DantyzVipRoupasConfig.Load();
        GetRPCManager().SendRPC("DantyzShop", "RPCReceiveRoupasConfig", new Param1<ref DantyzVipRoupasConfig>(cfg), true, sender);
    }

    // Envia a lista de PERSONAGENS jogaveis (modelos do jogo) pro cliente.
    void RPCRequestCharList(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;
        if (!sender)
            return;

        TStringArray chars = g_Game.ListAvailableCharacters();
        string joined = "";
        if (chars)
        {
            for (int i = 0; i < chars.Count(); i++)
            {
                if (i > 0)
                    joined += ",";
                joined += chars.Get(i);
            }
        }
        GetRPCManager().SendRPC("DantyzShop", "RPCReceiveCharList", new Param1<string>(joined), true, sender);
    }

    
    void RPCGetBalance(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            Param1<string> data;
            if (!ctx.Read(data))
            {
                return;
            }
            
            string steamID = data.param1;
            
            
            DantyzPlayerAccount account = DantyzPlayerAccount.Load(steamID);
            
            
            GetRPCManager().SendRPC("DantyzShop", "RPCRequestBalance", new Param1<int>(account.coins), true, sender);
            
        }
    }
    
    void RPCReloadShop(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            Param1<string> data;
            if (!ctx.Read(data))
            {
                return;
            }
            
            string steamID = data.param1;
            

            // SEGURANCA: o botao de reload so' aparece pro admin no cliente, mas
            // qualquer um podia mandar este RPC na mao e forcar o servidor a
            // reler todos os JSON. Agora o servidor confere na lista de admins
            // usando a identidade de QUEM mandou (nao o steamID do payload).
            string senderID = "";
            if (sender)
                senderID = sender.GetPlainId();

            if (senderID == "" || !DantyzShopConfig.Load().IsAdmin(senderID))
            {
                return;
            }
            
            // Descarta as categorias em cache: e' aqui que o admin pede pro
            // servidor reler os JSON do disco depois de editar um arquivo.
            DantyzShopItemList.InvalidateCache();
            
            // Reload() forca a releitura do Config.json (Load() devolveria o
            // que ja' esta em memoria e o admin nao veria a alteracao).
            DantyzShopConfig config = DantyzShopConfig.Reload();
            // Troca tambem a referencia que a MissionServer guarda, senao ela
            // continuaria usando o config antigo (precos do seguro etc).
            m_ShopConfig = config;
            
            m_VehicleConfig = DantyzShopVehicleConfig.Load();
            
            GetRPCManager().SendRPC("DantyzShop", "RPCReloadComplete", new Param1<bool>(true), true, sender);
        }
    }

    // ── ESTE MÉTODO ESTAVA FORA DA CLASSE — AGORA ESTÁ DENTRO ──
    void RPCRequestPlayerVehicles(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            Param1<string> data;
            if (!ctx.Read(data))
                return;
            
            string steamID = data.param1;
            
            DantyzShopPlayerVehicles playerVehicles = DantyzShopPlayerVehicles.Load(steamID);
            array<ref DantyzShopVehicleData> allVehicles = playerVehicles.GetActiveVehicles();
            array<ref DantyzShopVehicleData> validVehicles = new array<ref DantyzShopVehicleData>();
            
            DantyzShopConfig config = DantyzShopConfig.Load();
            int currentTime = GetGame().GetTime();
            int expireTime = config.InsuranceTimeToExpire * 1000;

            // Antes o Save() era chamado DENTRO do loop, duas vezes por veiculo.
            // Agora acumula e grava UMA vez no fim (ver needSave mais abaixo).
            bool needSave = false;
            
            for (int i = 0; i < allVehicles.Count(); i++)
            {
                DantyzShopVehicleData vehicle = allVehicles.Get(i);
                
                // Tenta encontrar o veículo no mundo.
                // ORDEM IMPORTA: primeiro pelo indice em memoria (chave
                // PersistentID, que e' ESTAVEL e sobrevive a restart) e so' depois
                // pelo NetworkID. O NetworkID e' reatribuido a cada restart, e era
                // por isso que depois de reiniciar o servidor o carro "nao era
                // achado" e o mapa caia na ultima posicao gravada em vez da real.
                // E' o mesmo princípio do Sparda, que indexa por um ID proprio
                // persistido em vez do NetworkID (INSURANCE_LIST).
                CarScript car = CarScript.DzFindInsuredByPID(vehicle.PersistentID);

                if (!car)
                {
                    Object obj = GetGame().GetObjectByNetworkId(vehicle.NetworkID, 0);
                    car = CarScript.Cast(obj);
                }
                
                if (!car)
                {
                    // CARRO COM SEGURO: nao achado no mundo agora (longe /
                    // descarregado / pos-restart com NetworkID novo). Isso NAO
                    // significa destruido! Mantemos o card. A destruicao de
                    // verdade e' detectada por EVENTO no proprio carro (EEKilled /
                    // motor RUINED) e casa pelo PersistentID (estavel). So' sai do
                    // card quando o carro for destruido MESMO.
                    if (vehicle.HasInsurance)
                    {
                        validVehicles.Insert(vehicle);
                        continue;
                    }
                    playerVehicles.MarkVehicleDestroyed(vehicle.NetworkID);
                    continue;
                }
                
                // Carro carregado: captura/atualiza a chave ESTAVEL (PersistentID)
                // no registro. Migra registros antigos (sem PID) e mantem o PID
                // fresco - e' por ele que a destruicao e' detectada certo, mesmo
                // depois de restart. Operacao barata (so quando o carro esta perto).
                int rpb1, rpb2, rpb3, rpb4;
                car.GetPersistentID(rpb1, rpb2, rpb3, rpb4);
                string liveCarPID = "" + rpb1 + "_" + rpb2 + "_" + rpb3 + "_" + rpb4;
                if (liveCarPID != "0_0_0_0" && vehicle.PersistentID != liveCarPID)
                {
                    vehicle.PersistentID = liveCarPID;
                    needSave = true;
                }

                // MIGRACAO: registros antigos tem o seguro somente no JSON. Se o
                // carro esta carregado e o JSON diz que tem seguro, grava o
                // estado NO CARRO agora (dai em diante nao se le mais o arquivo
                // pra saber isso).
                if (vehicle.HasInsurance && !car.DzIsInsured())
                {
                    car.DzSetInsured(true, steamID, vehicle.VehiclePrice);
                }
                
                // Atualiza a ultima posicao conhecida (pro rastreador de carro).
                vector liveCarPos = car.GetPosition();
                vehicle.LastPosX = liveCarPos[0];
                vehicle.LastPosY = liveCarPos[1];
                vehicle.LastPosZ = liveCarPos[2];
                vehicle.HasLastPos = true;
                needSave = true;

                // Guarda o estado do CARLOCK (dono, senha e nome). E' isso que
                // permite devolver o carro JA' TRANCADO quando ele for puxado,
                // sem o jogador precisar aplicar o CarLock outra vez.
                if (car.CarLockOwner != -1)
                {
                    vehicle.CarLockOwnerLow  = car.CarLockOwner;
                    vehicle.CarLockPass      = car.CarLockPassword;
                    vehicle.CarLockOwnerName = car.CarLockOwnerName;
                }
                
                // Verifica se o motor está RUINED (destruído)
                EntityAI engine = car.FindAttachmentBySlotName("Engine");
                if (engine)
                {
                    int engineHealth = engine.GetHealthLevel();
                    
                    if (engineHealth == GameConstants.STATE_RUINED)
                    {
                        playerVehicles.MarkVehicleDestroyed(vehicle.NetworkID);
                        continue;
                    }
                }
                
                // Se não tem seguro, verifica se expirou
                if (!vehicle.HasInsurance)
                {
                    int timeSincePurchase = currentTime - vehicle.PurchaseTime;
                    
                    if (timeSincePurchase >= expireTime)
                    {
                        playerVehicles.MarkVehicleDestroyed(vehicle.NetworkID);
                        continue;
                    }
                }
                
                validVehicles.Insert(vehicle);
            }

            // Calcula o ESTADO do botao "puxar" pra cada carro, pra mandar
            // pronto pro cliente (evita comparar relogio cliente x servidor).
            //   RecallState: 0=pode puxar | 1=aguardando chegar | 2=pronto pra spawnar
            int recallDelayCfg = config.VehicleRecallDelaySeconds;
            int nowEpoch = DantyzTime.NowEpoch();
            for (int rv = 0; rv < validVehicles.Count(); rv++)
            {
                DantyzShopVehicleData vd = validVehicles.Get(rv);
                if (recallDelayCfg <= 0 || vd.RecallReadyEpoch <= 0)
                {
                    vd.RecallState = 0;
                    vd.RecallSecondsLeft = 0;
                }
                else if (nowEpoch >= vd.RecallReadyEpoch)
                {
                    vd.RecallState = 2;
                    vd.RecallSecondsLeft = 0;
                }
                else
                {
                    vd.RecallState = 1;
                    vd.RecallSecondsLeft = vd.RecallReadyEpoch - nowEpoch;
                }

                // RASTREIO PAGO: manda pronto se o mapa esta liberado e quanto
                // falta, pra o cliente nao precisar comparar relogios.
                vd.TrackCost = (vd.VehiclePrice * config.InsuranceCostPercentPreviwMap) / 100;

                if (vd.TrackUntilEpoch > nowEpoch)
                {
                    vd.TrackActive = true;
                    vd.TrackSecondsLeft = vd.TrackUntilEpoch - nowEpoch;
                }
                else
                {
                    vd.TrackActive = false;
                    vd.TrackSecondsLeft = 0;
                }
            }
            
            // UNICA gravacao de disco desta rotina (antes eram 2 por veiculo).
            if (needSave)
                playerVehicles.Save();

            GetRPCManager().SendRPC("DantyzShop", "RPCReceivePlayerVehicles", new Param1<ref array<ref DantyzShopVehicleData>>(validVehicles), true, sender);
        }
    }
    
    void RPCPurchaseInsurance(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            Param2<string, int> data;
            if (!ctx.Read(data))
                return;
            
            string steamID = data.param1;
            int networkID = data.param2;

            // ================================================================
            // DESATIVADO: o seguro agora vem junto com o CarLock.
            // ----------------------------------------------------------------
            // Aplicar o CarLock no carro ja' ativa a apolice (ver o override de
            // SetCarLockOwner em DantyzShop/scripts/4_World/CarScript.c). O botao
            // da loja virou "RASTREAR", entao este RPC nao deveria mais chegar -
            // se chegar, e' cliente antigo ou pacote forjado. Recusamos sem
            // cobrar nada pra ninguem pagar duas vezes pelo mesmo seguro.
            // ================================================================
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "O seguro agora vem junto com o CarLock: aplique o CarLock no carro."), true, sender);
        }
    }

    // ========================================================================
    // COMPRAR RASTREIO (abre o mapa por TrackDurationSeconds)
    // ------------------------------------------------------------------------
    // Custa InsuranceCostPercentPreviwMap % do valor do carro. Enquanto estiver
    // valendo, o jogador abre o mapa quantas vezes quiser sem pagar de novo.
    // Quando expira, o botao volta a cobrar.
    // ========================================================================
    void RPCPurchaseTracking(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;

        Param2<string, int> data;
        if (!ctx.Read(data))
            return;

        if (!sender)
            return;

        // SEGURANCA: usa sempre a identidade de quem mandou, nunca o steamID do
        // payload - senao daria pra comprar rastreio gastando a moeda de outro.
        string steamID = sender.GetPlainId();
        int networkID = data.param2;

        DantyzShopPlayerVehicles pv = DantyzShopPlayerVehicles.Load(steamID);
        DantyzShopVehicleData vehicle = pv.GetVehicleByNetworkID(networkID);

        if (!vehicle)
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Veiculo nao encontrado!"), true, sender);
            return;
        }

        int nowEp = DantyzTime.NowEpoch();

        // Ja' tem rastreio valendo: nao cobra de novo.
        if (vehicle.TrackUntilEpoch > nowEp)
        {
            int leftMin = (vehicle.TrackUntilEpoch - nowEp) / 60;
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(true, "Rastreio ja esta ativo por mais " + leftMin + " minuto(s)."), true, sender);
            return;
        }

        int trackCost = (vehicle.VehiclePrice * m_ShopConfig.InsuranceCostPercentPreviwMap) / 100;

        DantyzPlayerAccount account = DantyzPlayerAccount.Load(steamID);

        if (trackCost > 0)
        {
            if (!account.CanAfford(trackCost))
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Saldo insuficiente! Rastreio custa " + trackCost + " " + m_ShopConfig.CoinName + "."), true, sender);
                return;
            }

            if (!account.Purchase(trackCost))
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Erro ao processar o pagamento!"), true, sender);
                return;
            }
        }

        int duration = m_ShopConfig.TrackDurationSeconds;
        if (duration <= 0)
            duration = 1800; // fallback: 30 min

        vehicle.TrackUntilEpoch = nowEp + duration;

        // Se o carro estiver carregado agora, ja' grava a posicao fresca pra o
        // mapa abrir certo na primeira vez.
        Object trkObj = GetGame().GetObjectByNetworkId(networkID, 0);
        CarScript trkCar = CarScript.Cast(trkObj);
        if (trkCar)
        {
            vector trkPos = trkCar.GetPosition();
            vehicle.LastPosX = trkPos[0];
            vehicle.LastPosY = trkPos[1];
            vehicle.LastPosZ = trkPos[2];
            vehicle.HasLastPos = true;
        }

        pv.Save();


        GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(true, "Rastreio ativado por " + (duration / 60) + " minutos!"), true, sender);
        GetRPCManager().SendRPC("DantyzShop", "RPCUpdateCoins", new Param1<int>(account.coins), true, sender);
        // Recarrega os cards pra o botao ja' aparecer como ativo.
        GetRPCManager().SendRPC("DantyzShop", "RPCInsurancePurchased", null, true, sender);
    }

    // ========================================================================
    // PUXAR CARRO (recall)
    // ------------------------------------------------------------------------
    // So' funciona em veiculo SEGURADO. Custa a mesma % do seguro
    // (InsuranceCostPercent, 25%). Deleta a instancia CARREGADA do carro (se
    // existir, pra nao duplicar) e cria o carro na frente do player. O registro
    // e' atualizado com o novo NetworkID e um novo PersistentID e' capturado
    // (com retentativas), mantendo o seguro ativo (pode puxar de novo depois).
    // ========================================================================
    void RPCRecallVehicle(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server)
            return;

        Param3<string, string, int> data;
        if (!ctx.Read(data))
            return;

        string steamID = data.param1;
        string pid = data.param2;
        int networkID = data.param3;


        DantyzShopPlayerVehicles playerVehicles = DantyzShopPlayerVehicles.Load(steamID);
        DantyzShopVehicleData vehicle = playerVehicles.GetVehiclePIDorNet(pid, networkID);

        if (!vehicle)
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Veiculo nao encontrado!"), true, sender);
            return;
        }

        if (!vehicle.HasInsurance)
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "So' da pra puxar carro segurado!"), true, sender);
            return;
        }

        PlayerBase player = GetPlayerBySteamID(steamID);
        if (!player)
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Player nao encontrado!"), true, sender);
            return;
        }

        // ------------------------------------------------------------
        // Acha a(s) instancia(s) CARREGADA(s) deste carro no mundo.
        // Casa pelo PersistentID (estavel) e, como reforco, pelo mesmo
        // esquema de NetworkID (nl|nh) usado no resto do mod. Varre a lista
        // de carros carregados (confiavel) em vez do GetObjectByNetworkId
        // (que nao funcionava com o pseudo-id salvo).
        // ------------------------------------------------------------
        string vehPID = vehicle.PersistentID;
        array<CarScript> matches = new array<CarScript>();
        for (int c = CarScript.m_DantyzLoadedCars.Count() - 1; c >= 0; c--)
        {
            CarScript loaded = CarScript.m_DantyzLoadedCars.Get(c);
            if (!loaded)
                continue;

            bool isMatch = false;

            if (vehPID != "" && vehPID != "0_0_0_0" && loaded.DantyzShop_GetPID() == vehPID)
                isMatch = true;

            if (!isMatch && vehicle.NetworkID != 0)
            {
                int lnl, lnh;
                loaded.GetNetworkID(lnl, lnh);
                if ((lnl | lnh) == vehicle.NetworkID)
                    isMatch = true;
            }

            if (isMatch)
                matches.Insert(loaded);
        }

        // ------------------------------------------------------------
        // REGRA: nao deixa puxar se tiver PLAYER dentro do carro atual.
        // (checado ANTES de cobrar, pra nao gastar moeda a toa)
        // ------------------------------------------------------------
        for (int mo = 0; mo < matches.Count(); mo++)
        {
            if (matches.Get(mo).DantyzShop_HasPlayerInside())
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Nao da pra puxar: tem alguem dentro do carro!"), true, sender);
                return;
            }
        }

        // ------------------------------------------------------------
        // DELAY DE PUXADA (VehicleRecallDelaySeconds no Config.json).
        //  - Se for 0: puxa na hora (comportamento antigo, pula este bloco).
        //  - FASE 1 (1o clique "PUXAR CARRO"): inicia a contagem, NAO cobra e
        //    NAO spawna. O carro "chega" depois do tempo.
        //  - Enquanto aguarda: avisa quanto falta e nao spawna.
        //  - FASE 2 (tempo cumprido, clique "SPAWNAR CARRO"): segue abaixo,
        //    cobra e cria o carro.
        // ------------------------------------------------------------
        int recallDelay = m_ShopConfig.VehicleRecallDelaySeconds;
        if (recallDelay > 0)
        {
            int nowEp = DantyzTime.NowEpoch();
            if (vehicle.RecallReadyEpoch <= 0)
            {
                // COOLDOWN: Verifica se pode puxar de novo
                int cooldown = m_ShopConfig.VehicleRecallCooldownSeconds;
                if (cooldown > 0 && vehicle.LastRecallEpoch > 0)
                {
                    int timeSinceLastRecall = nowEp - vehicle.LastRecallEpoch;
                    if (timeSinceLastRecall < cooldown)
                    {
                        int remainingSec = cooldown - timeSinceLastRecall;
                        int remainingMin = remainingSec / 60;
                        GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Aguarde " + remainingMin + " minutos para puxar este carro novamente."), true, sender);
                        return;
                    }
                }
                
                vehicle.RecallReadyEpoch = nowEp + recallDelay;
                playerVehicles.Save();
                int waitMin = recallDelay / 60;
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Carro a caminho! Volte em ~" + waitMin + " min e clique em SPAWNAR CARRO."), true, sender);
                // Reenvia a lista pro cliente atualizar o estado do botao.
                GetRPCManager().SendRPC("DantyzShop", "RPCInsurancePurchased", null, true, sender);
                return;
            }
            if (nowEp < vehicle.RecallReadyEpoch)
            {
                int leftS = vehicle.RecallReadyEpoch - nowEp;
                int lMin = leftS / 60;
                int lSec = leftS % 60;
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Ainda faltam " + lMin + "m " + lSec + "s pro carro chegar."), true, sender);
                return;
            }
            // nowEp >= RecallReadyEpoch: tempo cumprido -> segue e spawna abaixo.
        }
        else
        {
            // Sem delay, mas verifica cooldown mesmo assim
            int nowEpNoDelay = DantyzTime.NowEpoch();
            int cooldownNoDelay = m_ShopConfig.VehicleRecallCooldownSeconds;
            if (cooldownNoDelay > 0 && vehicle.LastRecallEpoch > 0)
            {
                int timeSinceLastRecallNoDelay = nowEpNoDelay - vehicle.LastRecallEpoch;
                if (timeSinceLastRecallNoDelay < cooldownNoDelay)
                {
                    int remainingSecNoDelay = cooldownNoDelay - timeSinceLastRecallNoDelay;
                    int remainingMinNoDelay = remainingSecNoDelay / 60;
                    GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Aguarde " + remainingMinNoDelay + " minutos para puxar este carro novamente."), true, sender);
                    return;
                }
            }
        }

        // 1a puxada e' GRATIS; a partir da 2a custa a mesma % do seguro (25%).
        bool usingFreeRecall = !vehicle.FreeRecallUsed;
        int recallCost = 0;
        if (!usingFreeRecall)
            recallCost = (vehicle.VehiclePrice * m_ShopConfig.InsuranceCostPercent) / 100;

        DantyzPlayerAccount account = DantyzPlayerAccount.Load(steamID);
        if (!account.CanAfford(recallCost))
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Saldo insuficiente!"), true, sender);
            return;
        }
        if (!account.Purchase(recallCost))
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Erro ao processar!"), true, sender);
            return;
        }

        // Deleta a(s) instancia(s) antiga(s) achada(s) acima (evita duplicar).
        for (int d = 0; d < matches.Count(); d++)
        {
            GetGame().ObjectDelete(matches.Get(d));
        }

        // Se o carro antigo estiver DESCARREGADO agora, nao da pra deletar na
        // hora. Marca o PID pra que, quando ele carregar de novo, se autodelete
        // (via CarScript.EEInit). Assim nao fica duplicado no mundo.
        // Marca em memoria E no disco (PendingDelete.json) pra que a marcacao
        // sobreviva a um restart do servidor - senao o carro velho ficava pra
        // sempre no mundo (bug do carro duplicado).
        if (vehPID != "" && vehPID != "0_0_0_0")
        {
            if (CarScript.m_DantyzPendingDelete.Find(vehPID) == -1)
                CarScript.m_DantyzPendingDelete.Insert(vehPID);
            DantyzPendingDeleteStore.Add(vehPID);
        }

        // Cria o carro na frente do player (reusa a logica de spawn testada).
        // Ao PUXAR: adiciona SO' os itens de "RecallCargoItems" configurados no
        // JSON do carro (ex: so' o Carlock). Se o carro nao tiver esse campo,
        // passa null e nada e' adicionado - igual antes.
        string spawnMsg;
        DantyzShopItem recallCfg = FindItemInCategoriesByPrice(vehicle.VehicleClass, vehicle.VehiclePrice);
        DantyzShopItem recallSpawnItem = null;
        if (recallCfg && recallCfg.RecallCargoItems && recallCfg.RecallCargoItems.Count() > 0)
        {
            recallSpawnItem = new DantyzShopItem();
            recallSpawnItem.CargoItems = recallCfg.RecallCargoItems;
        }
        bool ok = SpawnVehicle(player, vehicle.VehicleClass, recallSpawnItem, spawnMsg);

        if (!ok)
        {
            // Devolve o valor cobrado se nao conseguiu spawnar.
            account.AddCoins(recallCost);
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, spawnMsg), true, sender);
            GetRPCManager().SendRPC("DantyzShop", "RPCUpdateCoins", new Param1<int>(account.coins), true, sender);
            return;
        }

        // Consumiu a puxada gratis (persistido junto no Save abaixo).
        if (usingFreeRecall)
            vehicle.FreeRecallUsed = true;

        // Recall concluido: zera o timer pra que a proxima puxada comece
        // uma nova contagem E atualiza o timestamp do cooldown.
        vehicle.RecallReadyEpoch = 0;
        vehicle.LastRecallEpoch = DantyzTime.NowEpoch();

        // Acha o carro recem-criado perto do player pra atualizar o registro.
        CarScript newCar = null;
        vector playerPos = player.GetPosition();
        array<Object> nearby = new array<Object>();
        GetGame().GetObjectsAtPosition(playerPos, 15, nearby, null);
        for (int n = 0; n < nearby.Count(); n++)
        {
            CarScript cand = CarScript.Cast(nearby.Get(n));
            // Mesmo motivo do RegisterVehiclePurchase: comparar sem caixa, senao
            // o carro PUXADO nao era reencontrado e o registro ficava sem NetID
            // e sem PersistentID novos.
            if (cand && DantyzShopUtils.SameClass(cand.GetType(), vehicle.VehicleClass))
            {
                newCar = cand;
                break;
            }
        }

        if (newCar)
        {
            int nl, nh;
            newCar.GetNetworkID(nl, nh);
            vehicle.NetworkID = nl | nh;

            // Zera o PID pra que o CaptureVehiclePID grave o do NOVO carro
            // (carro recem-criado retorna 0_0_0_0 ate' a persistencia atribuir).
            vehicle.PersistentID = "";

            vector newPos = newCar.GetPosition();
            vehicle.LastPosX = newPos[0];
            vehicle.LastPosY = newPos[1];
            vehicle.LastPosZ = newPos[2];
            vehicle.HasLastPos = true;

            playerVehicles.Save();

            // O carro puxado continua SEGURADO: grava o estado no carro novo.
            // O indice em memoria e' preenchido depois, no CaptureVehiclePID,
            // porque um carro recem-criado ainda devolve PID 0_0_0_0.
            newCar.DzBindBuyer(steamID, vehicle.VehiclePrice);

            if (vehicle.HasInsurance)
                newCar.DzSetInsured(true, steamID, vehicle.VehiclePrice);

            // Devolve o carro JA' TRANCADO com o CarLock do dono (mesma coisa que
            // o Sparda faz no CreateCar, PluginSpardaInsuranceServer.c:314-321).
            // Sem isso o jogador teria que aplicar um CarLock novo a cada puxada.
            if (vehicle.CarLockOwnerLow != -1)
            {
                newCar.SetCarLockOwner(vehicle.CarLockOwnerLow);

                if (vehicle.CarLockPass > 0)
                    newCar.SetCarLockPassword(vehicle.CarLockPass);

                if (vehicle.CarLockOwnerName != "")
                    newCar.SetCarLockOwnerName(vehicle.CarLockOwnerName);

                newCar.SetCarLock(true);
            }

            // Captura o PID estavel do novo carro (com retentativas).
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(CaptureVehiclePID, 2000, false, steamID, newCar, vehicle.NetworkID, 5);
        }
        else
        {
            playerVehicles.Save();
        }


        GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(true, "Carro puxado! Esta na sua frente."), true, sender);
        GetRPCManager().SendRPC("DantyzShop", "RPCUpdateCoins", new Param1<int>(account.coins), true, sender);
        GetRPCManager().SendRPC("DantyzShop", "RPCInsurancePurchased", null, true, sender);
    }
    
    void RPCCheckVipStatus(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            Param1<string> data;
            if (!ctx.Read(data))
                return;
            
            string steamID = data.param1;
            
            
            // Verifica se player tem VIP ativo
            if (DantyzVipManager.HasActiveVip(steamID))
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCOpenVipRoupasMenu", null, true, sender);
            }
            else
            {
            }
        }
    }
    
    // ========================================
    // VIP FREE ITEMS - Prêmios Diários
    // ========================================
    
    void RPCClaimVipFreeItem(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            if (!m_IsInitialized) return;
            
            Param3<string, string, int> data;
            if (!ctx.Read(data))
                return;
            
            string steamID = data.param1;
            string itemClass = data.param2;
            int dayOfMonth = data.param3;
            
            
            // Verifica se player tem VIP ativo - USA NOVO SISTEMA OTIMIZADO
            if (!DantyzVipPlayerChecker.HasActiveVip(steamID))
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Você precisa ter VIP ativo para pegar prêmios diários!"), true, sender);
                return;
            }
            
            // Verifica se pode pegar hoje - USA NOVO SISTEMA OTIMIZADO
            if (!DantyzVipPlayerChecker.CanClaimVipItem(steamID, dayOfMonth, itemClass))
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Você já pegou este prêmio hoje ou não é o dia correto!"), true, sender);
                return;
            }
            
            // Pega o player
            PlayerBase player = GetPlayerBySteamID(steamID);
            if (!player)
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Erro: Player não encontrado!"), true, sender);
                return;
            }
            
            // Carrega os itens VIP Free para pegar a quantidade correta
            array<ref DantyzShopItem> vipItems = DantyzShopItemList.LoadCategory("vipitensfree.json");
            int itemQuantity = 1; // Padrão
            
            // Busca o item no JSON para pegar a quantidade
            for (int i = 0; i < vipItems.Count(); i++)
            {
                DantyzShopItem vipItem = vipItems.Get(i);
                if (DantyzShopUtils.SameClass(vipItem.Classname, itemClass) && vipItem.DayOfMonth == dayOfMonth)
                {
                    itemQuantity = vipItem.QuantityDrop;
                    if (itemQuantity <= 0) itemQuantity = 1; // Segurança
                    break;
                }
            }
            
            // Cria os itens (quantidade especificada)
            int itemsCreated = 0;
            for (int q = 0; q < itemQuantity; q++)
            {
                EntityAI item = player.GetInventory().CreateInInventory(itemClass);
                if (item)
                {
                    itemsCreated++;
                }
            }
            
            if (itemsCreated > 0)
            {
                
                // Registra o claim
                DantyzVipDailyRewards.RegisterClaim(steamID, itemClass);
                
                // Mensagem personalizada baseada na quantidade
                string message = "";
                if (itemsCreated == 1)
                {
                    message = "Prêmio VIP diário recebido!";
                }
                else
                {
                    message = "Prêmio VIP diário recebido! (" + itemsCreated + " itens)";
                }
                
                // Envia mensagem de sucesso
                if (DantyzShopMsg.On())
                    player.MessageStatus(message);
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(true, message), true, sender);
                
                // Recarrega a categoria para atualizar o botão
                GetRPCManager().SendRPC("DantyzShop", "RPCReloadCategory", new Param1<string>("vipitensfree.json"), true, sender);
                
            }
            else
            {
                GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseResult", new Param2<bool, string>(false, "Erro ao criar itens! Verifique seu inventário."), true, sender);
            }
            
        }
    }
    
    // ========================================
    // VIP STATUS RPC - Para mostrar status no menu
    // ========================================
    
    void RPCRequestVipStatus(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            Param1<string> data;
            if (!ctx.Read(data))
                return;
            
            string steamID = data.param1;
            
            
            // USA O NOVO SISTEMA OTIMIZADO - Lê diretamente da pasta PlayersVip
            DantyzVipPlayerData vipData = DantyzVipPlayerChecker.GetVipData(steamID);
            
            bool hasVip = false;
            int daysRemaining = 0;
            string expirationDate = "";
            
            if (vipData && vipData.isActive && vipData.daysRemaining > 0)
            {
                hasVip = true;
                daysRemaining = vipData.daysRemaining;
                expirationDate = vipData.expirationDate;
            }
            else
            {
            }
            
            // Envia resposta para o cliente
            GetRPCManager().SendRPC("DantyzShop", "RPCReceiveVipStatus", new Param3<bool, int, string>(hasVip, daysRemaining, expirationDate), true, sender);
            
        }
    }

    // ========================================
    // VIP FURA-FILA STATUS RPC - igual ao VIP Roupas
    // mas le o JSON do PlayersVipFila.
    // Usado para mostrar "VIP Fila: X dias" no rodape do menu.
    // ========================================
    void RPCRequestVipFilaStatus(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type == CallType.Server)
        {
            Param1<string> data;
            if (!ctx.Read(data))
                return;

            string steamID = data.param1;


            DantyzVipFilaPlayerData filaData = DantyzVipFilaManager.GetVipFilaData(steamID);

            bool hasFila = false;
            int daysRemaining = 0;
            string expirationDate = "";

            if (filaData && filaData.isActive && filaData.daysRemaining > 0 && DantyzVipFilaManager.HasActiveVipFila(steamID))
            {
                hasFila = true;
                daysRemaining = filaData.daysRemaining;
                expirationDate = filaData.expirationDate;
            }
            else
            {
            }

            GetRPCManager().SendRPC("DantyzShop", "RPCReceiveVipFilaStatus", new Param3<bool, int, string>(hasFila, daysRemaining, expirationDate), true, sender);
        }
    }

}  // fecha a classe MissionServer

