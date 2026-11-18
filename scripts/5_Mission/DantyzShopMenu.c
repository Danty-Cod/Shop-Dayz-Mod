class DantyzShopMenu extends UIScriptedMenu
{
    protected WrapSpacerWidget m_ItemsGrid;
    protected ScrollWidget m_ItemsScroll;
    protected ScrollWidget m_SidebarScroll;
    protected TextWidget m_CoinsValue;
    protected TextWidget m_SteamIDText;
    protected TextWidget m_VipStatusText;
    protected ButtonWidget m_CloseButton;
    protected ButtonWidget m_ReloadButton;
    protected ref array<Widget> m_ItemCards;
    protected ref array<ButtonWidget> m_CategoryButtons;
    protected ref DantyzShopConfig m_Config;
    protected ref DantyzShopVehicleConfig m_VehicleConfig;
    protected ref DantyzPlayerAccount m_PlayerAccount;
    protected string m_CurrentCategory;
    protected ref array<ref DantyzShopItem> m_CurrentItems;
    protected ref array<ref DantyzShopVehicleData> m_CurrentVehicles;
    protected bool m_IsAdmin;
    protected string m_PlayerSteamID;
    
    // Status VIP (recebido do servidor via RPC)
    protected bool m_HasActiveVip;
    protected int m_VipDaysRemaining;
    protected string m_VipExpirationDate;

    // Status VIP Fura-Fila (mesmo padrao)
    protected bool m_HasActiveVipFila;
    protected int m_VipFilaDaysRemaining;
    protected string m_VipFilaExpirationDate;
    
    // NOVO: Tooltip
    protected Widget m_TooltipPanel;
    protected TextWidget m_TooltipTitle;
    protected TextWidget m_TooltipDescription;
    protected TextWidget m_TooltipPrice;
    protected Widget m_TooltipBackground;
    protected Widget m_LastHoveredCard;
    protected Widget m_ConfirmRoot;
    protected TextWidget m_ConfirmTitle;
    protected TextWidget m_ConfirmItemName;
    protected TextWidget m_ConfirmPrice;
    protected ButtonWidget m_ConfirmYesButton;
    protected ButtonWidget m_ConfirmNoButton;

    // Dados da compra pendente (o que sera comprado se o player clicar em SIM)
    protected int m_PendingKind;             // 0=nenhum, 1=item normal, 2=vip free, 3=seguro
    protected ref DantyzShopItem m_PendingItem;
    protected ref DantyzShopVehicleData m_PendingVehicle;

    // Painel embutido de selecao de roupas VIP (boneco + seletores).
    // Criado dentro da area de conteudo quando abre a categoria de roupas.
    protected ref DantyzVipRoupasEmbed m_RoupasEmbed;
    protected Widget m_ItemsPanel;
    protected Widget       m_VehicleMapPanel;
    protected MapWidget    m_VehicleMap;
    protected TextWidget   m_VehicleMapTitle;
    protected ButtonWidget m_VehicleMapClose;
    protected string       m_MapPendingPID;
    
    void DantyzShopMenu()
    {
        m_ItemCards = new array<Widget>();
        m_CategoryButtons = new array<ButtonWidget>();
        m_CurrentItems = new array<ref DantyzShopItem>();
        m_CurrentVehicles = new array<ref DantyzShopVehicleData>();
        m_IsAdmin = false;
        m_LastHoveredCard = null;
        
        // Inicializa status VIP
        m_HasActiveVip = false;
        m_VipDaysRemaining = 0;
        m_VipExpirationDate = "";

        // Inicializa status VIP Fura-Fila
        m_HasActiveVipFila = false;
        m_VipFilaDaysRemaining = 0;
        m_VipFilaExpirationDate = "";
    }
    
    override Widget Init()
    {
        
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("DantyzShop/GUI/Layouts/shop_menu.layout");
        
        if (!layoutRoot)
        {
            return null;
        }
        
        m_ItemsGrid = WrapSpacerWidget.Cast(layoutRoot.FindAnyWidget("ItemsGrid"));
        m_ItemsScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("ItemsScroll"));
        m_ItemsPanel = layoutRoot.FindAnyWidget("ItemsPanel");
        m_SidebarScroll = ScrollWidget.Cast(layoutRoot.FindAnyWidget("SidebarScroll"));
        m_CoinsValue = TextWidget.Cast(layoutRoot.FindAnyWidget("CoinsValue"));
        m_SteamIDText = TextWidget.Cast(layoutRoot.FindAnyWidget("SteamIDText"));
        m_VipStatusText = TextWidget.Cast(layoutRoot.FindAnyWidget("VipStatusText"));
        m_CloseButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("CloseButton"));
        m_ReloadButton = ButtonWidget.Cast(layoutRoot.FindAnyWidget("ReloadButton"));

        // Painel do mapa de rastreio (nasce escondido).
        m_VehicleMapPanel = layoutRoot.FindAnyWidget("VehicleMapPanel");
        m_VehicleMap      = MapWidget.Cast(layoutRoot.FindAnyWidget("VehicleMap"));
        m_VehicleMapTitle = TextWidget.Cast(layoutRoot.FindAnyWidget("VehicleMapTitle"));
        m_VehicleMapClose = ButtonWidget.Cast(layoutRoot.FindAnyWidget("VehicleMapClose"));
        HideVehicleMap();
        
        GetRPCManager().SendRPC("DantyzShop", "RPCRequestConfig", null, true);
        
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player && m_SteamIDText)
        {
            PlayerIdentity identity = player.GetIdentity();
            if (identity)
            {
                m_PlayerSteamID = identity.GetPlainId();
                m_SteamIDText.SetText("Steam ID: " + m_PlayerSteamID);
                
                // Atualiza status VIP
                UpdateVipStatus();
                
                if (m_ReloadButton)
                {
                    m_ReloadButton.Show(false);
                }
                
                LoadPlayerBalance();
            }
        }
        
        
        // NOVO: Inicializa tooltip
        InitTooltip();

        // NOVO: Inicializa popup de confirmacao de compra
        InitConfirmPopup();
        
        return layoutRoot;
    }
    
    void OnReceiveConfig(ref DantyzShopConfig config)
    {
        
        m_Config = config;
        
        // Carrega VehicleConfig
        m_VehicleConfig = DantyzShopVehicleConfig.Load();
        
        for (int i = 0; i < m_Config.Categories.Count(); i++)
        {
            DantyzShopCategory cat = m_Config.Categories.Get(i);
        }
        
        // Verifica se o player é admin
        m_IsAdmin = m_Config.IsAdmin(m_PlayerSteamID);
        
        // Mostra botão RELOAD apenas para admins
        if (m_ReloadButton)
        {
            if (m_IsAdmin)
            {
                m_ReloadButton.Show(true);
                m_ReloadButton.Enable(true);
            }
            else
            {
                m_ReloadButton.Show(false);
            }
        }
        
        SetupCategoryButtons();
        
        if (m_Config.Categories.Count() > 0)
        {
            LoadCategory(0);
        }
        
    }
    
    void OnReceiveCategoryData(array<ref DantyzShopItem> items)
    {
        
        m_CurrentItems = items;

        // Garante que nenhum tooltip antigo fique preso ao recriar os cards.
        HideTooltip();
        m_LastHoveredCard = null;

        ClearItems();
        
        for (int i = 0; i < items.Count(); i++)
        {
            DantyzShopItem item = items.Get(i);
            
            CreateItemCard(item);
        }
        
        if (m_ItemsScroll)
        {
            m_ItemsScroll.VScrollToPos01(0);
        }
    }
    
    void OnReceivePlayerVehicles(array<ref DantyzShopVehicleData> vehicles)
    {
        
        m_CurrentVehicles = vehicles;
        
        // IMPORTANTE: a categoria de Seguros usa cards de VEICULO (m_CurrentVehicles),
        // nao itens normais. Limpamos m_CurrentItems pra remover os dados da
        // categoria ANTERIOR. Sem isso, o tooltip da categoria passada (ex: kit de
        // construcao) ficava "vazando" nos cards de seguro (bug do tooltip errado).
        m_CurrentItems.Clear();
        
        ClearItems();
        
        for (int i = 0; i < vehicles.Count(); i++)
        {
            DantyzShopVehicleData vehicle = vehicles.Get(i);
            CreateVehicleInsuranceCard(vehicle);
        }
    }
    
    void LoadPlayerBalance()
    {
        if (GetGame().IsClient() && m_PlayerSteamID != "")
        {
            
            if (m_CoinsValue)
            {
                m_CoinsValue.SetText("...");
            }
            
            GetRPCManager().SendRPC("DantyzShop", "RPCGetBalance", new Param1<string>(m_PlayerSteamID), true);
        }
        else if (GetGame().IsServer())
        {
            DantyzPlayerAccount account = DantyzPlayerAccount.Load(m_PlayerSteamID);
            if (m_CoinsValue && account)
            {
                m_CoinsValue.SetText(account.coins.ToString());
            }
        }
    }
    
    void SetupCategoryButtons()
    {
        array<string> buttonNames = new array<string>();
        buttonNames.Insert("Tab1");
        buttonNames.Insert("Tab2");
        buttonNames.Insert("Tab3");
        buttonNames.Insert("Tab4");
        buttonNames.Insert("Tab5");
        buttonNames.Insert("Tab6");
        buttonNames.Insert("Tab7");
        buttonNames.Insert("Tab8");
        buttonNames.Insert("Tab9");
        buttonNames.Insert("Tab10");
        buttonNames.Insert("Tab11");
        buttonNames.Insert("Tab12");
        buttonNames.Insert("Tab13");
        buttonNames.Insert("Tab14");
        buttonNames.Insert("Tab15");
        buttonNames.Insert("Tab16");
        buttonNames.Insert("Tab17");
        buttonNames.Insert("Tab18");
        buttonNames.Insert("Tab19");
        buttonNames.Insert("Tab20");
        buttonNames.Insert("Tab21");
        buttonNames.Insert("Tab22");
        buttonNames.Insert("Tab23");
        buttonNames.Insert("Tab24");
        buttonNames.Insert("Tab25");
        
        // Primeiro, esconde TODOS os botões
        for (int j = 0; j < buttonNames.Count(); j++)
        {
            ButtonWidget btnHide = ButtonWidget.Cast(layoutRoot.FindAnyWidget(buttonNames.Get(j)));
            if (btnHide)
            {
                btnHide.Show(false);
            }
        }
        
        // Agora mostra e configura apenas os botões das categorias ENABLED
        int buttonIndex = 0;
        for (int i = 0; i < m_Config.Categories.Count(); i++)
        {
            DantyzShopCategory category = m_Config.Categories.Get(i);
            
            if (!category.Enabled)
            {
                continue;
            }
            
            if (buttonIndex < buttonNames.Count())
            {
                string widgetName = buttonNames.Get(buttonIndex);
                ButtonWidget btnShow = ButtonWidget.Cast(layoutRoot.FindAnyWidget(widgetName));
                
                if (btnShow)
                {
                    btnShow.Show(true);
                    
                    string iconWidgetName = "Icon" + (buttonIndex + 1);
                    ImageWidget iconWidget = ImageWidget.Cast(btnShow.FindAnyWidget(iconWidgetName));
                    
                    if (iconWidget && category.CategoryIcon != "")
                    {
                        iconWidget.LoadImageFile(0, category.CategoryIcon);
                    }
                    
                    m_CategoryButtons.Insert(btnShow);
                }
            }
            buttonIndex++;
        }
        
        if (m_SidebarScroll)
        {
            m_SidebarScroll.VScrollToPos01(0);
        }
    }
    
    void LoadCategory(int buttonIndex)
    {
        // Trocar de categoria fecha o mapa de rastreio, senao ele ficaria
        // flutuando por cima dos itens da categoria nova.
        HideVehicleMap();

        int enabledIndex = 0;
        int actualIndex = -1;
        
        for (int i = 0; i < m_Config.Categories.Count(); i++)
        {
            DantyzShopCategory tempCat = m_Config.Categories.Get(i);
            
            if (!tempCat.Enabled)
                continue;
            
            if (enabledIndex == buttonIndex)
            {
                actualIndex = i;
                break;
            }
            enabledIndex++;
        }
        
        if (actualIndex < 0 || actualIndex >= m_Config.Categories.Count())
        {
            return;
        }
        
        DantyzShopCategory cat = m_Config.Categories.Get(actualIndex);
        
        m_CurrentCategory = cat.FileName;
        
        // Esconde qualquer tooltip que possa estar visivel ao trocar de
        // categoria (evita tooltip "preso" se o mouse estava em cima de um item).
        HideTooltip();
        m_LastHoveredCard = null;
        
        ClearItems();

        // Sai do modo "roupas" se estava nele (destroi o boneco/painel).
        CloseRoupasEmbed();
        
        if (m_ItemsScroll)
        {
            m_ItemsScroll.VScrollToPos01(0);
        }
        
        // Nomes de arquivo comparados SEM caixa: o admin escreve o FileName no
        // Config.json na mao, e "Seguros.json" deixaria a aba sem funcionar.
        if (DantyzShopUtils.SameClass(cat.FileName, "seguros.json"))
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCRequestPlayerVehicles", new Param1<string>(m_PlayerSteamID), true);
        }
        else if (DantyzShopUtils.SameClass(cat.FileName, "personaclothers.json"))
        {
            // Categoria de ESCOLHER ROUPAS: em vez de cards, mostra o painel
            // embutido com o boneco + seletores dentro da area de conteudo.
            OpenRoupasEmbed();
        }
        else
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCRequestShopData", new Param1<string>(cat.FileName), true);
        }
    }
    
    // ========================================================================
    // PAINEL EMBUTIDO DE ROUPAS (boneco + seletores dentro da loja)
    // ========================================================================
    void OpenRoupasEmbed()
    {
        // Esconde o scroll de itens normais e mostra o painel de roupas.
        if (m_ItemsScroll)
            m_ItemsScroll.Show(false);

        if (!m_RoupasEmbed)
            m_RoupasEmbed = new DantyzVipRoupasEmbed();

        Widget parent = m_ItemsPanel;
        if (!parent)
            parent = layoutRoot;

        m_RoupasEmbed.Create(parent);
    }

    void CloseRoupasEmbed()
    {
        if (m_RoupasEmbed)
        {
            m_RoupasEmbed.Destroy();
            m_RoupasEmbed = null;
        }

        // Volta a mostrar o scroll de itens normais.
        if (m_ItemsScroll)
            m_ItemsScroll.Show(true);
    }

    void OnReceiveRoupasSelection(DantyzVipRoupasSelection sel)
    {
        if (m_RoupasEmbed && m_RoupasEmbed.IsActive())
            m_RoupasEmbed.ApplySavedSelection(sel);
    }

    void OnReceiveRoupasConfig(DantyzVipRoupasConfig cfg)
    {
        if (m_RoupasEmbed && m_RoupasEmbed.IsActive())
            m_RoupasEmbed.SetConfig(cfg);
    }

    void OnReceiveCharList(array<string> chars)
    {
        if (m_RoupasEmbed && m_RoupasEmbed.IsActive())
            m_RoupasEmbed.SetCharList(chars);
    }

    void ClearItems()
    {
        if (!m_ItemsGrid)
            return;
        
        for (int i = 0; i < m_ItemCards.Count(); i++)
        {
            Widget card = m_ItemCards.Get(i);
            if (card)
                card.Unlink();
        }
        
        m_ItemCards.Clear();
    }
    
    void CreateItemCard(ref DantyzShopItem item)
    {
        Widget card = GetGame().GetWorkspace().CreateWidgets("DantyzShop/GUI/Layouts/item_card.layout", m_ItemsGrid);
        
        if (!card)
        {
            return;
        }
        
        TextWidget nameWidget = TextWidget.Cast(card.FindAnyWidget("ItemName"));
        if (nameWidget)
            nameWidget.SetText(item.Name);
        
        TextWidget costWidget = TextWidget.Cast(card.FindAnyWidget("ItemCost"));
        
        // ========================================
        // VIP FREE ITEMS - Lógica especial
        // ========================================
        if (item.IsVipFreeItem)
        {
            // ESCONDE A MOEDA para itens VIP grátis
            ImageWidget coinWidget = ImageWidget.Cast(card.FindAnyWidget("Coin"));
            if (coinWidget)
            {
                coinWidget.Show(false);
            }
            
            // Pega o dia atual do sistema
            int currentYear, currentMonth, currentDay;
            GetYearMonthDayUTC(currentYear, currentMonth, currentDay);
            
            bool hasVip = m_HasActiveVip;
            bool isToday = (currentDay == item.DayOfMonth);
            bool isPast = (currentDay > item.DayOfMonth);
            bool isFuture = (currentDay < item.DayOfMonth);
            
            // Configura o texto do custo
            if (costWidget)
            {
                if (isPast)
                {
                    costWidget.SetText("EXPIRADO");
                    costWidget.SetColor(ARGB(255, 120, 120, 120));
                }
                else if (isFuture)
                {
                    costWidget.SetText("AGUARDE DIA " + item.DayOfMonth);
                    costWidget.SetColor(ARGB(255, 150, 150, 150));
                }
                else if (!hasVip)
                {
                    costWidget.SetText("VIP NECESSÁRIO");
                    costWidget.SetColor(ARGB(255, 255, 100, 0));
                }
                else
                {
                    string freeText;
                    if (item.DayOfMonth == 30)
                    {
                        freeText = "PRÊMIO ESPECIAL!";
                    }
                    else
                    {
                        freeText = "GRÁTIS HOJE!";
                    }
                    costWidget.SetText(freeText);
                    costWidget.SetColor(ARGB(255, 0, 255, 0));
                }
            }
            
            // Configura o botão
            ButtonWidget buyBtn = ButtonWidget.Cast(card.FindAnyWidget("BuyButton"));
            TextWidget buyText = TextWidget.Cast(card.FindAnyWidget("BuyText"));
            
            if (buyBtn && buyText)
            {
                if (isPast)
                {
                    buyBtn.Enable(false);
                    buyBtn.SetColor(ARGB(200, 80, 80, 80));
                    buyText.SetText("EXPIRADO");
                }
                else if (isFuture)
                {
                    buyBtn.Enable(false);
                    buyBtn.SetColor(ARGB(200, 100, 100, 100));
                    buyText.SetText("AGUARDE DIA " + item.DayOfMonth);
                }
                else if (!hasVip)
                {
                    buyBtn.Enable(false);
                    buyBtn.SetColor(ARGB(200, 150, 50, 0));
                    buyText.SetText("COMPRE VIP");
                }
                else
                {
                    buyBtn.Enable(true);
                    buyBtn.SetColor(ARGB(255, 0, 200, 0));
                    buyText.SetText("PEGAR GRÁTIS");
                }
            }
        }
        else
        {
            // Item normal - mostra preço
            if (costWidget)
                costWidget.SetText(item.Price.ToString());
        }
        
        // TEST DRIVE: carros ganham o botao TESTAR ao lado do COMPRAR (o servidor confere tudo)
        bool testDriveOn = (!m_Config || m_Config.TestDriveEnabled != 0);
        if (testDriveOn && !item.IsVipFreeItem && (GetGame().IsKindOf(item.Classname, "CarScript") || GetGame().IsKindOf(item.Classname, "Car")))
        {
            ButtonWidget cardBuyBtn = ButtonWidget.Cast(card.FindAnyWidget("BuyButton"));
            ButtonWidget cardTestBtn = ButtonWidget.Cast(card.FindAnyWidget("TestButton"));
            if (cardBuyBtn && cardTestBtn)
            {
                cardBuyBtn.SetSize(0.47, 0.15);
                cardTestBtn.Show(true);
            }
        }
        
        // ========================================
        // PREVIEW / ÍCONE
        // ========================================
        
        bool hasCustomIcon = (item.IconPath != "");
        
        if (hasCustomIcon)
        {
            ItemPreviewWidget vipPreview = ItemPreviewWidget.Cast(card.FindAnyWidget("ProductPreview"));
            if (vipPreview)
            {
                vipPreview.Show(false);
            }
            
            Widget previewWidget = card.FindAnyWidget("ProductPreview");
            if (previewWidget)
            {
                Widget previewParent = previewWidget.GetParent();
                if (previewParent)
                {
                    ImageWidget iconWidget = ImageWidget.Cast(GetGame().GetWorkspace().CreateWidgets("DantyzShop/GUI/Layouts/vip_icon.layout", previewParent));
                    
                    if (iconWidget)
                    {
                        iconWidget.LoadImageFile(0, item.IconPath);
                        iconWidget.Show(true);
                        
                        float x, y;
                        previewWidget.GetPos(x, y);
                        iconWidget.SetPos(x, y);
                        
                        float w, h;
                        previewWidget.GetSize(w, h);
                        iconWidget.SetSize(w, h);
                    }
                    else
                    {
                    }
                }
                else
                {
                }
            }
            else
            {
            }
        }
        else
        {
            // Para itens normais: Mostra o preview 3D
            ItemPreviewWidget preview = ItemPreviewWidget.Cast(card.FindAnyWidget("ProductPreview"));
            if (preview)
            {
                EntityAI previewItem = EntityAI.Cast(GetGame().CreateObject(item.Classname, "0 0 0", true, false, true));
                if (previewItem)
                {
                    preview.SetItem(previewItem);
                    preview.SetModelPosition(Vector(0, 0, 0.5));
                    preview.SetModelOrientation(vector.Zero);
                    preview.SetView(previewItem.GetViewIndex());
                    preview.Show(true);
                }
            }
        }
        
        m_ItemCards.Insert(card);
    }
    
    void CreateVehicleInsuranceCard(ref DantyzShopVehicleData vehicle)
    {
        Widget card = GetGame().GetWorkspace().CreateWidgets("DantyzShop/GUI/Layouts/insurance_card.layout", m_ItemsGrid);
        
        if (!card)
        {
            return;
        }
        
        TextWidget nameWidget = TextWidget.Cast(card.FindAnyWidget("ItemName"));
        if (nameWidget)
        {
            nameWidget.SetText(vehicle.VehicleName);
        }
        
        TextWidget statusWidget = TextWidget.Cast(card.FindAnyWidget("StatusText"));
        if (statusWidget)
        {
            if (vehicle.HasInsurance)
            {
                statusWidget.SetText("[SEGURADO]");
                statusWidget.SetColor(ARGB(255, 0, 255, 0));
            }
            else
            {
                statusWidget.SetText("[SEM SEGURO]");
                statusWidget.SetColor(ARGB(255, 255, 100, 0));
            }
        }
        
        int insuranceCost = (vehicle.VehiclePrice * m_Config.InsuranceCostPercent) / 100;
        
        TextWidget costWidget = TextWidget.Cast(card.FindAnyWidget("ItemCost"));
        if (costWidget)
        {
            if (vehicle.HasInsurance)
            {
                costWidget.SetText("PROTEGIDO");
            }
            else
            {
                // Nao ha' mais preco de seguro a exibir: o seguro vem do CarLock.
                costWidget.SetText("SEM SEGURO");
            }
        }
        
        ButtonWidget buyBtn = ButtonWidget.Cast(card.FindAnyWidget("BuyButton"));
        TextWidget buyText = TextWidget.Cast(card.FindAnyWidget("BuyText"));
        
        // Este botao era "COMPRAR SEGURO". O seguro passou a vir junto com o
        // CarLock (aplicar o cadeado ja' ativa a apolice), entao o botao virou
        // "RASTREAR": abre o mapa com a ultima posicao conhecida do carro.
        if (buyBtn && buyText)
        {
            if (!vehicle.HasInsurance)
            {
                // Sem CarLock aplicado nao ha' o que rastrear.
                buyBtn.Enable(false);
                buyBtn.SetColor(ARGB(200, 100, 100, 100));
                buyText.SetText("PRECISA DE SEGURO");
            }
            else if (vehicle.TrackActive)
            {
                // Rastreio pago e valendo: abre o mapa direto, sem cobrar.
                int trkMin = vehicle.TrackSecondsLeft / 60;
                if (trkMin < 1)
                    trkMin = 1;

                buyBtn.Enable(true);
                buyBtn.SetColor(ARGB(255, 0, 130, 200));
                buyText.SetText("VER MAPA (" + trkMin + " min)");
            }
            else
            {
                // Precisa comprar/renovar o rastreio.
                buyBtn.Enable(true);
                buyBtn.SetColor(ARGB(255, 180, 120, 0));
                buyText.SetText("RASTREAR (" + vehicle.TrackCost + ")");
            }
        }
        
        // Botao PUXAR CARRO: so' habilitado se o veiculo TEM seguro.
        // Custa a mesma % do seguro (25%) e teleporta o carro pra frente do player.
        ButtonWidget recallBtn = ButtonWidget.Cast(card.FindAnyWidget("RecallButton"));
        TextWidget recallText = TextWidget.Cast(card.FindAnyWidget("RecallText"));
        if (recallBtn && recallText)
        {
            if (vehicle.HasInsurance)
            {
                if (vehicle.RecallState == 2)
                {
                    // Tempo cumprido: pronto pra spawnar (verde)
                    recallBtn.Enable(true);
                    recallBtn.SetColor(ARGB(255, 0, 180, 0));
                    recallText.SetText("SPAWNAR CARRO");
                }
                else if (vehicle.RecallState == 1)
                {
                    // Aguardando o carro chegar (desabilitado, cinza)
                    recallBtn.Enable(false);
                    recallBtn.SetColor(ARGB(200, 120, 120, 120));
                    int cmm = vehicle.RecallSecondsLeft / 60;
                    int css = vehicle.RecallSecondsLeft % 60;
                    if (cmm > 0)
                        recallText.SetText("CHEGANDO (" + cmm + "m)");
                    else
                        recallText.SetText("CHEGANDO (" + css + "s)");
                }
                else
                {
                    // Estado 0: pode puxar (azul)
                    recallBtn.Enable(true);
                    recallBtn.SetColor(ARGB(255, 0, 110, 210));
                    recallText.SetText("PUXAR CARRO");
                }
            }
            else
            {
                recallBtn.Enable(false);
                recallBtn.SetColor(ARGB(200, 100, 100, 100));
                recallText.SetText("PRECISA DE SEGURO");
            }
        }
        
        ItemPreviewWidget preview = ItemPreviewWidget.Cast(card.FindAnyWidget("ProductPreview"));
        if (preview)
        {
            EntityAI previewItem = EntityAI.Cast(GetGame().CreateObject(vehicle.VehicleClass, "0 0 0", true, false, true));
            if (previewItem)
            {
                preview.SetItem(previewItem);
                preview.SetModelPosition(Vector(0, 0, 0.5));
                preview.SetModelOrientation(vector.Zero);
                preview.SetView(previewItem.GetViewIndex());
            }
        }
        
        m_ItemCards.Insert(card);
    }
    
    override bool OnClick(Widget w, int x, int y, int button)
    {
        // Repassa clique pro painel de roupas (setas/aplicar), se ativo.
        if (m_RoupasEmbed && m_RoupasEmbed.IsActive())
        {
            if (m_RoupasEmbed.HandleClick(w))
                return true;
        }

        // Fecha o mapa de rastreio (X do painel do mapa).
        if (m_VehicleMapClose && w == m_VehicleMapClose)
        {
            HideVehicleMap();
            return true;
        }

        if (w == m_CloseButton)
        {
            CloseMenu();
            return true;
        }
        
        if (w == m_ReloadButton)
        {
            ReloadShop();
            return true;
        }

        // Botoes do popup de confirmacao de compra
        if (w == m_ConfirmYesButton)
        {
            OnConfirmYes();
            return true;
        }
        if (w == m_ConfirmNoButton)
        {
            HideConfirmPopup();
            return true;
        }
        
        for (int i = 0; i < m_CategoryButtons.Count(); i++)
        {
            if (w == m_CategoryButtons.Get(i))
            {
                LoadCategory(i);
                return true;
            }
        }
        
        for (int j = 0; j < m_ItemCards.Count(); j++)
        {
            Widget card = m_ItemCards.Get(j);
            ButtonWidget buyBtn = ButtonWidget.Cast(card.FindAnyWidget("BuyButton"));
            ButtonWidget recallBtn = ButtonWidget.Cast(card.FindAnyWidget("RecallButton"));
            ButtonWidget testBtn = ButtonWidget.Cast(card.FindAnyWidget("TestButton"));
            
            // Clique em TESTAR (test drive gratis)
            if (testBtn && w == testBtn)
            {
                if (j < m_CurrentItems.Count())
                    ShowConfirmTestDrive(m_CurrentItems.Get(j));
                return true;
            }
            
            // Clique em PUXAR CARRO (so' existe nos cards de seguro)
            if (recallBtn && w == recallBtn)
            {
                if (DantyzShopUtils.SameClass(m_CurrentCategory, "seguros.json") && j < m_CurrentVehicles.Count())
                {
                    DantyzShopVehicleData recallVehicle = m_CurrentVehicles.Get(j);
                    ShowConfirmRecall(recallVehicle);
                }
                return true;
            }
            
            if (w == buyBtn)
            {
                if (DantyzShopUtils.SameClass(m_CurrentCategory, "seguros.json"))
                {
                    if (j < m_CurrentVehicles.Count())
                    {
                        DantyzShopVehicleData vehicle = m_CurrentVehicles.Get(j);
                        if (vehicle.TrackActive)
                            OpenVehicleMap(vehicle);
                        else
                            ShowConfirmTracking(vehicle);
                    }
                }
                else
                {
                    if (j < m_CurrentItems.Count())
                    {
                        DantyzShopItem item = m_CurrentItems.Get(j);
                        
                        if (item.IsVipFreeItem)
                        {
                            ShowConfirmVipFree(item);
                        }
                        else
                        {
                            ShowConfirmItem(item);
                        }
                    }
                }
                
                return true;
            }
        }
        
        return super.OnClick(w, x, y, button);
    }
    
    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && m_RoupasEmbed && m_RoupasEmbed.IsActive())
        {
            if (m_RoupasEmbed.HandleMouseDown(w))
                return true;
        }
        return super.OnMouseButtonDown(w, x, y, button);
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (button == MouseState.LEFT && m_RoupasEmbed && m_RoupasEmbed.IsActive())
            m_RoupasEmbed.HandleMouseUp();
        return super.OnMouseButtonUp(w, x, y, button);
    }
    
    override void Update(float timeslice)
    {
        // ESC primeiro: se algo abaixo der erro, a loja ainda fecha
        if (KeyState(KeyCode.KC_ESCAPE))
        {
            ClearKey(KeyCode.KC_ESCAPE);
            CloseMenu();
            return;
        }

        super.Update(timeslice);

        // Rotacao do boneco no painel de roupas (arrastar mouse).
        if (m_RoupasEmbed && m_RoupasEmbed.IsActive())
            m_RoupasEmbed.HandleUpdate();
        
        Input input = GetGame().GetInput();
        
        if (input.LocalPress("UADantyzShop", false))
        {
            CloseMenu();
            return;
        }
        
        if (GetUApi().GetInputByID(UAUIBack).LocalPress())
        {
            CloseMenu();
            return;
        }
    }
    
    void UpdateVipStatus()
    {
        if (!m_VipStatusText || m_PlayerSteamID == "")
            return;
        
        // Solicita status VIP do servidor
        GetRPCManager().SendRPC("DantyzShop", "RPCRequestVipStatus", new Param1<string>(m_PlayerSteamID), true);
        // Solicita tambem status do VIP Fura-Fila
        GetRPCManager().SendRPC("DantyzShop", "RPCRequestVipFilaStatus", new Param1<string>(m_PlayerSteamID), true);
    }
    
    // Atualiza a label do rodape combinando VIP + VIP Fila
    void RefreshVipStatusLabel()
    {
        if (!m_VipStatusText) return;

        string text = "";

        if (m_HasActiveVip)
            text = "VIP: " + m_VipDaysRemaining + " dias";
        else
            text = "VIP: Inativo";

        if (m_HasActiveVipFila)
            text += "  |  VIP Fila: " + m_VipFilaDaysRemaining + " dias";
        else
            text += "  |  VIP Fila: Inativo";

        m_VipStatusText.SetText(text);

        // Cor verde se tiver QUALQUER um dos dois ativos, senao laranja
        if (m_HasActiveVip || m_HasActiveVipFila)
            m_VipStatusText.SetColor(ARGB(255, 0, 255, 0));
        else
            m_VipStatusText.SetColor(ARGB(255, 255, 100, 0));
    }
    
    void OnReceiveVipStatus(bool hasVip, int daysRemaining, string expirationDate)
    {
        if (!m_VipStatusText)
            return;
        
        m_HasActiveVip = hasVip;
        m_VipDaysRemaining = daysRemaining;
        m_VipExpirationDate = expirationDate;
        
        
        RefreshVipStatusLabel();
        
        if (DantyzShopUtils.SameClass(m_CurrentCategory, "vipitensfree.json"))
        {
            
            int categoryIdx = -1;
            int enabledIdx = 0;
            
            for (int idx = 0; idx < m_Config.Categories.Count(); idx++)
            {
                DantyzShopCategory category = m_Config.Categories.Get(idx);
                
                if (!category.Enabled)
                    continue;
                
                if (DantyzShopUtils.SameClass(category.FileName, "vipitensfree.json"))
                {
                    categoryIdx = enabledIdx;
                    break;
                }
                
                enabledIdx++;
            }
            
            if (categoryIdx >= 0)
            {
                LoadCategory(categoryIdx);
            }
        }
    }
    
    // Recebe o status do VIP Fura-Fila do servidor e atualiza a label
    void OnReceiveVipFilaStatus(bool hasFila, int daysRemaining, string expirationDate)
    {
        m_HasActiveVipFila = hasFila;
        m_VipFilaDaysRemaining = daysRemaining;
        m_VipFilaExpirationDate = expirationDate;


        RefreshVipStatusLabel();
    }
    
    void CloseMenu()
    {
        GetGame().GetUIManager().HideScriptedMenu(this);
    }
    
    void ReloadShop()
    {
        
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player && DantyzShopMsg.On())
        {
            player.MessageImportant("Recarregando loja...");
        }
        
        if (GetGame().IsClient())
        {
            
            ClearItems();
            m_CategoryButtons.Clear();
            
            GetRPCManager().SendRPC("DantyzShop", "RPCReloadShop", new Param1<string>(m_PlayerSteamID), true);
            GetRPCManager().SendRPC("DantyzShop", "RPCRequestConfig", null, true);
            
        }
    }
    
    void PurchaseItem(string itemClass, int price)
    {
        
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
        {
            return;
        }
        
        PlayerIdentity identity = player.GetIdentity();
        if (!identity)
        {
            return;
        }
        
        string steamID = identity.GetPlainId();
        
        if (GetGame().IsClient())
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseItem", new Param3<string, string, int>(steamID, itemClass, price), true);
        }
    }
    
    void ClaimVipFreeItem(ref DantyzShopItem item)
    {
        
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player)
        {
            return;
        }
        
        if (GetGame().IsClient())
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCClaimVipFreeItem", new Param3<string, string, int>(m_PlayerSteamID, item.Classname, item.DayOfMonth), true);
        }
    }
    
    void PurchaseInsurance(int networkID)
    {
        
        if (GetGame().IsClient())
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseInsurance", new Param2<string, int>(m_PlayerSteamID, networkID), true);
        }
    }
    
    void OnInsurancePurchased()
    {
        
        if (DantyzShopUtils.SameClass(m_CurrentCategory, "seguros.json"))
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCRequestPlayerVehicles", new Param1<string>(m_PlayerSteamID), true);
        }
    }
    
    void UpdateCoins(int newAmount)
    {
        
        if (m_CoinsValue)
        {
            m_CoinsValue.SetText(newAmount.ToString());
        }
        
        if (m_PlayerAccount)
        {
            m_PlayerAccount.coins = newAmount;
        }
    }
    
    override void OnShow()
    {
        super.OnShow();
        
        GetGame().GetInput().ChangeGameFocus(1);
        GetGame().GetUIManager().ShowUICursor(true);
        GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
        
        LoadPlayerBalance();
    }
    
    override void OnHide()
    {
        super.OnHide();
        
        // Esconde o tooltip caso o menu seja fechado com o mouse
        // ainda em cima de um item (ex: ESC apertado com hover ativo).
        // Sem isso, o tooltip ficaria preso na tela mesmo com a loja fechada.
        HideTooltip();
        m_LastHoveredCard = null;

        // Fecha o popup de confirmacao (e limpa o preview) se ficou aberto.
        HideConfirmPopup();

        // Fecha o mapa de rastreio e tira o redesenho pendente da fila.
        HideVehicleMap();

        // Destroi o painel de roupas (e o boneco) se estava aberto.
        CloseRoupasEmbed();
        
        GetGame().GetInput().ChangeGameFocus(-1);
        GetGame().GetUIManager().ShowUICursor(false);
        GetGame().GetMission().PlayerControlEnable(false);
        GetGame().GetInput().ResetGameFocus();
    }
    
    // ========================================
    // POPUP DE CONFIRMACAO DE COMPRA
    // ========================================
    void InitConfirmPopup()
    {
        // Criamos o popup como FILHO do layoutRoot pra que o OnClick do menu
        // receba os cliques dos botoes SIM/NAO. Comeca escondido.
        m_ConfirmRoot = GetGame().GetWorkspace().CreateWidgets("DantyzShop/GUI/Layouts/confirm_popup.layout", layoutRoot);

        if (!m_ConfirmRoot)
        {
            return;
        }

        m_ConfirmTitle     = TextWidget.Cast(m_ConfirmRoot.FindAnyWidget("ConfirmTitle"));
        m_ConfirmItemName  = TextWidget.Cast(m_ConfirmRoot.FindAnyWidget("ConfirmItemName"));
        m_ConfirmPrice     = TextWidget.Cast(m_ConfirmRoot.FindAnyWidget("ConfirmPrice"));
        m_ConfirmYesButton = ButtonWidget.Cast(m_ConfirmRoot.FindAnyWidget("ConfirmYesButton"));
        m_ConfirmNoButton  = ButtonWidget.Cast(m_ConfirmRoot.FindAnyWidget("ConfirmNoButton"));

        m_ConfirmRoot.Show(false);
        m_PendingKind = 0;

    }

    // Monta e mostra o popup com nome e preco (sem preview).
    void FillConfirmPopup(string title, string itemName, string priceText)
    {
        if (!m_ConfirmRoot)
            return;

        if (m_ConfirmTitle)
            m_ConfirmTitle.SetText(title);
        if (m_ConfirmItemName)
            m_ConfirmItemName.SetText(itemName);
        if (m_ConfirmPrice)
            m_ConfirmPrice.SetText(priceText);

        m_ConfirmRoot.Show(true);
    }

    void ShowConfirmItem(ref DantyzShopItem item)
    {
        if (!item)
            return;
        m_PendingKind = 1;
        m_PendingItem = item;
        m_PendingVehicle = null;
        FillConfirmPopup("Comprar este item?", item.Name, "Preco: " + item.Price.ToString() + " ZCoins");
    }

    // TEST DRIVE (kind 6): gratis
    void ShowConfirmTestDrive(ref DantyzShopItem item)
    {
        if (!item)
            return;
        m_PendingKind = 6;
        m_PendingItem = item;
        m_PendingVehicle = null;
        int mins = 15;
        if (m_Config && m_Config.TestDriveSeconds > 0)
            mins = m_Config.TestDriveSeconds / 60;
        FillConfirmPopup("Fazer test drive?", item.Name, "Gratis - " + mins.ToString() + " min. Sair do carro encerra o teste.");
    }

    void RequestTestDrive(ref DantyzShopItem item)
    {
        GetRPCManager().SendRPC("DantyzShop", "RPCTestDrive", new Param2<string, int>(item.Classname, item.Price), true);
    }

    void ShowConfirmVipFree(ref DantyzShopItem item)
    {
        if (!item)
            return;
        m_PendingKind = 2;
        m_PendingItem = item;
        m_PendingVehicle = null;
        FillConfirmPopup("Pegar este item gratis?", item.Name, "GRATIS (VIP)");
    }

    void OpenVehicleMap(ref DantyzShopVehicleData vehicle)
    {
        if (!vehicle)
            return;

        // Desenha na hora com o que temos (feedback imediato).
        ShowVehicleMap(vehicle);

        // E pede a posicao atualizada. Guardamos o PersistentID porque a resposta
        // troca o array de veiculos por um novo - o indice do card nao serve.
        m_MapPendingPID = vehicle.PersistentID;

        if (GetGame().IsClient())
            GetRPCManager().SendRPC("DantyzShop", "RPCRequestPlayerVehicles", new Param1<string>(m_PlayerSteamID), true);

        GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(RedrawPendingMap);
        GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(RedrawPendingMap, 400, false);
    }

    // Redesenha o mapa com a posicao que chegou na resposta do servidor.
    void RedrawPendingMap()
    {
        if (m_MapPendingPID == "" || !m_VehicleMapPanel || !m_VehicleMapPanel.IsVisible())
            return;

        if (!m_CurrentVehicles)
            return;

        for (int i = 0; i < m_CurrentVehicles.Count(); i++)
        {
            DantyzShopVehicleData v = m_CurrentVehicles.Get(i);
            if (v && v.PersistentID == m_MapPendingPID)
            {
                ShowVehicleMap(v);
                return;
            }
        }
    }

    // Marca a posicao do carro no mapa e mostra o painel.
    void ShowVehicleMap(ref DantyzShopVehicleData vehicle)
    {
        if (!vehicle || !m_VehicleMap || !m_VehicleMapPanel)
            return;

        if (!vehicle.HasLastPos)
        {
            if (DantyzShopMsg.On())
            {
                PlayerBase pb = PlayerBase.Cast(GetGame().GetPlayer());
                if (pb)
                    pb.MessageImportant("Ainda nao ha posicao conhecida deste carro. Chegue perto dele uma vez.");
            }
            return;
        }

        vector pos = Vector(vehicle.LastPosX, vehicle.LastPosY, vehicle.LastPosZ);

        m_VehicleMap.ClearUserMarks();
        m_VehicleMap.AddUserMark(pos, vehicle.VehicleName, ARGB(255, 0, 255, 100), "\\dz\\gear\\navigation\\data\\map_viewpoint_ca.paa");
        m_VehicleMap.SetMapPos(pos);
        m_VehicleMap.SetScale(0.3);

        if (m_VehicleMapTitle)
        {
            int leftMin = vehicle.TrackSecondsLeft / 60;
            if (leftMin < 1)
                leftMin = 1;

            m_VehicleMapTitle.SetText("RASTREANDO: " + vehicle.VehicleName + "  (expira em " + leftMin + " min)");
        }

        if (m_ItemsScroll)
            m_ItemsScroll.Show(false);

        m_VehicleMapPanel.Show(true);
    }

    void HideVehicleMap()
    {
        if (GetGame())
            GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(RedrawPendingMap);

        m_MapPendingPID = "";

        if (m_VehicleMap)
            m_VehicleMap.ClearUserMarks();

        if (m_VehicleMapPanel)
            m_VehicleMapPanel.Show(false);

        // Devolve o scroll dos cards (nao no modo roupas, que usa o painel dele).
        if (m_ItemsScroll && !m_RoupasEmbed)
            m_ItemsScroll.Show(true);
    }

    void ShowConfirmInsurance(ref DantyzShopVehicleData vehicle)
    {
        if (!vehicle)
            return;
        m_PendingKind = 3;
        m_PendingItem = null;
        m_PendingVehicle = vehicle;

        int insuranceCost = 0;
        if (m_Config)
            insuranceCost = (vehicle.VehiclePrice * m_Config.InsuranceCostPercent) / 100;

        FillConfirmPopup("Comprar seguro?", vehicle.VehicleName, "Preco: " + insuranceCost.ToString() + " ZCoins");
    }

    // Confirmacao de PUXAR CARRO (custa a mesma % do seguro).
    void ShowConfirmRecall(ref DantyzShopVehicleData vehicle)
    {
        if (!vehicle)
            return;
        m_PendingKind = 4;
        m_PendingItem = null;
        m_PendingVehicle = vehicle;

        int recallCost = 0;
        if (m_Config)
            recallCost = (vehicle.VehiclePrice * m_Config.InsuranceCostPercent) / 100;

        // ESTADO 2: tempo ja cumprido -> confirma o SPAWN (aqui e' que cobra).
        if (vehicle.RecallState == 2)
        {
            if (!vehicle.FreeRecallUsed)
                FillConfirmPopup("Spawnar o carro agora?", vehicle.VehicleName, "1a puxada GRATIS! Aparece na sua frente.");
            else
                FillConfirmPopup("Spawnar o carro agora?", vehicle.VehicleName, "Preco: " + recallCost.ToString() + " ZCoins. Aparece na sua frente.");
            return;
        }

        // ESTADO 0: iniciar a puxada.
        int delaySec = 0;
        if (m_Config)
            delaySec = m_Config.VehicleRecallDelaySeconds;

        if (delaySec > 0)
        {
            // Com delay: so' inicia a contagem (nao cobra agora, cobra no spawn).
            int delayMin = delaySec / 60;
            FillConfirmPopup("Chamar o carro?", vehicle.VehicleName, "Chega em ~" + delayMin.ToString() + " min. Depois clique em SPAWNAR CARRO.");
        }
        else
        {
            // Sem delay: puxa na hora (comportamento antigo, cobra agora).
            if (!vehicle.FreeRecallUsed)
                FillConfirmPopup("Puxar o carro pra sua frente?", vehicle.VehicleName, "1a puxada GRATIS!");
            else
                FillConfirmPopup("Puxar o carro pra sua frente?", vehicle.VehicleName, "Preco: " + recallCost.ToString() + " ZCoins");
        }
    }

    // Player clicou em SIM -> executa a compra que estava pendente.
    void OnConfirmYes()
    {
        int kind = m_PendingKind;
        DantyzShopItem item = m_PendingItem;
        DantyzShopVehicleData vehicle = m_PendingVehicle;

        // Fecha o popup primeiro (ja limpa o estado pendente)
        HideConfirmPopup();

        if (kind == 1 && item)
        {
            PurchaseItem(item.Classname, item.Price);
        }
        else if (kind == 2 && item)
        {
            ClaimVipFreeItem(item);
        }
        else if (kind == 3 && vehicle)
        {
            PurchaseInsurance(vehicle.NetworkID);
        }
        else if (kind == 4 && vehicle)
        {
            RecallVehicle(vehicle);
        }
        else if (kind == 6 && item)
        {
            RequestTestDrive(item);
        }
        else if (kind == 5 && vehicle)
        {
            PurchaseTracking(vehicle);
        }
    }

    // Pede ao servidor pra liberar o rastreio (cobra o % configurado).
    void PurchaseTracking(ref DantyzShopVehicleData vehicle)
    {
        if (!vehicle)
            return;

        if (GetGame().IsClient())
            GetRPCManager().SendRPC("DantyzShop", "RPCPurchaseTracking", new Param2<string, int>(m_PlayerSteamID, vehicle.NetworkID), true);
    }

    // Confirmacao de COMPRAR RASTREIO.
    void ShowConfirmTracking(ref DantyzShopVehicleData vehicle)
    {
        if (!vehicle)
            return;

        m_PendingKind = 5;
        m_PendingItem = null;
        m_PendingVehicle = vehicle;

        int mins = 30;
        if (m_Config && m_Config.TrackDurationSeconds > 0)
            mins = m_Config.TrackDurationSeconds / 60;

        FillConfirmPopup("Comprar rastreio?", vehicle.VehicleName, "Preco: " + vehicle.TrackCost.ToString() + " ZCoins - dura " + mins.ToString() + " min");
    }

    // Envia ao servidor o pedido de puxar o carro (PID + NetworkID).
    void RecallVehicle(ref DantyzShopVehicleData vehicle)
    {
        if (!vehicle)
            return;


        if (GetGame().IsClient())
        {
            GetRPCManager().SendRPC("DantyzShop", "RPCRecallVehicle", new Param3<string, string, int>(m_PlayerSteamID, vehicle.PersistentID, vehicle.NetworkID), true);
        }
    }

    void HideConfirmPopup()
    {
        if (m_ConfirmRoot)
            m_ConfirmRoot.Show(false);

        m_PendingKind = 0;
        m_PendingItem = null;
        m_PendingVehicle = null;
    }

    // ========================================
    // TOOLTIP SYSTEM
    // ========================================
    
    void InitTooltip()
    {
        m_TooltipPanel = GetGame().GetWorkspace().CreateWidgets("DantyzShop/GUI/Layouts/tooltip.layout");
        
        if (m_TooltipPanel)
        {
            m_TooltipBackground = m_TooltipPanel.FindAnyWidget("TooltipBackground");
            m_TooltipTitle = TextWidget.Cast(m_TooltipPanel.FindAnyWidget("TooltipTitle"));
            m_TooltipDescription = TextWidget.Cast(m_TooltipPanel.FindAnyWidget("TooltipDescription"));
            m_TooltipPrice = TextWidget.Cast(m_TooltipPanel.FindAnyWidget("TooltipPrice"));
            
            m_TooltipPanel.Show(false);
            
        }
        else
        {
        }
    }
    
    void ShowTooltip(string title, string description, int price)
    {
        
        if (!m_TooltipPanel)
        {
            return;
        }
        
        if (description == "")
        {
            return;
        }
        
        if (!m_TooltipTitle)
        {
            return;
        }
        
        if (!m_TooltipDescription)
        {
            return;
        }
        
        if (!m_TooltipPrice)
        {
            return;
        }
        
        if (!m_TooltipBackground)
        {
            return;
        }
        
        m_TooltipTitle.SetText(title);
        
        m_TooltipDescription.SetText(description);
        
        string priceText = "Preço: " + price.ToString() + " ZCoins";
        m_TooltipPrice.SetText(priceText);
        
        int mouseX, mouseY;
        GetMousePos(mouseX, mouseY);
        
        
        float tooltipX = mouseX + 15;
        float tooltipY = mouseY + 15;
        
        float tooltipWidth = 400;
        float tooltipHeight = 300;
        
        float screenW = 1920;
        float screenH = 1080;
        
        
        if (tooltipX + tooltipWidth > screenW)
        {
            tooltipX = mouseX - tooltipWidth - 15;
        }
        
        if (tooltipY + tooltipHeight > screenH)
        {
            tooltipY = mouseY - tooltipHeight - 15;
        }
        
        if (tooltipX < 0)
        {
            tooltipX = 10;
        }
        if (tooltipY < 0)
        {
            tooltipY = 10;
        }
        
        m_TooltipBackground.SetPos(tooltipX, tooltipY);
        
        m_TooltipDescription.Update();
        m_TooltipPanel.Update();
        
        m_TooltipPanel.Show(true);
        m_TooltipBackground.Show(true);
        
    }
    
    void HideTooltip()
    {
        if (m_TooltipPanel)
        {
            m_TooltipPanel.Show(false);
        }
    }
    
    string GetItemDisplayName(string classname)
    {
        string displayName = "";
        
        GetGame().ConfigGetText("CfgVehicles " + classname + " displayName", displayName);
        
        if (displayName == "")
        {
            GetGame().ConfigGetText("CfgWeapons " + classname + " displayName", displayName);
        }
        
        if (displayName == "")
        {
            GetGame().ConfigGetText("CfgMagazines " + classname + " displayName", displayName);
        }
        
        if (displayName == "")
        {
            displayName = classname;
        }
        
        return displayName;
    }
    
    string GetVipRoupasTooltip(int kitID)
    {
        string tooltip = "";
        
        string path = "$profile:SimpleModParty/Store/Vip/VipRoupasKits.json";
        
        if (!FileExist(path))
        {
            path = "DantyzShop/Store/Vip/VipRoupasKits.json";
        }
        
        array<ref DantyzVipRoupasKit> kits = new array<ref DantyzVipRoupasKit>();
        JsonFileLoader<array<ref DantyzVipRoupasKit>>.JsonLoadFile(path, kits);
        
        for (int i = 0; i < kits.Count(); i++)
        {
            DantyzVipRoupasKit kit = kits.Get(i);
            if (kit.KitID == kitID)
            {
                tooltip = "ROUPAS:\n";
                
                for (int c = 0; c < kit.ClothingItems.Count(); c++)
                {
                    DantyzVipRoupasClothingItem clothingItem = kit.ClothingItems.Get(c);
                    tooltip += "• " + clothingItem.DisplayName + "\n";
                }
                
                tooltip += "\nBONUS:\n";
                
                for (int b = 0; b < kit.BonusItems.Count(); b++)
                {
                    DantyzVipRoupasBonusItem bonusItem = kit.BonusItems.Get(b);
                    tooltip += "• " + bonusItem.Quantity.ToString() + "x " + bonusItem.DisplayName + "\n";
                }
                
                break;
            }
        }
        
        return tooltip;
    }
    
    override bool OnMouseEnter(Widget w, int x, int y)
    {
        super.OnMouseEnter(w, x, y);
        
        for (int i = 0; i < m_ItemCards.Count(); i++)
        {
            Widget card = m_ItemCards.Get(i);
            if (!card)
                continue;
            
            Widget parent = w;
            while (parent)
            {
                if (parent == card)
                {
                    
                    if (m_ItemCards.Count() == m_CurrentItems.Count() && i < m_CurrentItems.Count())
                    {
                        DantyzShopItem item = m_CurrentItems.Get(i);
                        
                        if (item && item.TooltipActive == "true")
                        {
                            string description = "";
                            
                            if (item.Classname.IndexOf("VIP_ROUPAS_") == 0 && item.KitID > 0)
                            {
                                description = GetVipRoupasTooltip(item.KitID);
                            }
                            else if (item.Ground.Count() > 0 || item.Attachments.Count() > 0)
                            {
                                description = "Conteudo:\n";
                                string displayName;
                                
                                for (int g = 0; g < item.Ground.Count(); g++)
                                {
                                    DantyzShopAttachment groundItem = item.Ground.Get(g);
                                    displayName = GetItemDisplayName(groundItem.Classname);
                                    description += groundItem.Quantity.ToString() + "x " + displayName + "\n";
                                }
                                
                                for (int a = 0; a < item.Attachments.Count(); a++)
                                {
                                    DantyzShopAttachment attachItem = item.Attachments.Get(a);
                                    displayName = GetItemDisplayName(attachItem.Classname);
                                    description += attachItem.Quantity.ToString() + "x " + displayName + "\n";
                                }
                            }
                            else if (item.Description != "")
                            {
                                TStringArray parts = new TStringArray;
                                item.Description.Split("|", parts);
                                
                                for (int p = 0; p < parts.Count(); p++)
                                {
                                    string part = parts.Get(p);
                                    part.Trim();
                                    description += part + "\n";
                                }
                            }
                            else
                            {
                                description = item.Name;
                            }
                            
                            m_LastHoveredCard = card;
                            ShowTooltip(item.Name, description, item.Price);
                        }
                        
                    }
                    return false;
                }
                parent = parent.GetParent();
            }
        }
        
        return false;
    }
    
    override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
    {
        super.OnMouseLeave(w, enterW, x, y);
        
        for (int i = 0; i < m_ItemCards.Count(); i++)
        {
            Widget card = m_ItemCards.Get(i);
            if (!card)
                continue;
            
            Widget parent = w;
            while (parent)
            {
                if (parent == card && card == m_LastHoveredCard)
                {
                    HideTooltip();
                    m_LastHoveredCard = null;
                    return false;
                }
                parent = parent.GetParent();
            }
        }
        
        return false;
    }
}