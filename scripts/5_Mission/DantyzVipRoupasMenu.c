class DantyzVipRoupasEmbed
{
    protected Widget m_Root;
    protected ref DantyzVipRoupasConfig m_Config;

    protected ref map<string, int> m_SelIndex;
    protected ref map<string, TextWidget> m_Labels;
    protected ref map<string, ButtonWidget> m_PrevBtns;
    protected ref map<string, ButtonWidget> m_NextBtns;
    protected ref array<string> m_Categories;

    protected ButtonWidget m_ApplyButton;

    // Seletor de personagem (modelos do jogo)
    protected ref array<string> m_CharList;
    protected int m_CharIndex;            // -1 = padrao do jogo
    protected ButtonWidget m_CharPrev;
    protected ButtonWidget m_CharNext;
    protected TextWidget m_CharLabel;

    protected PlayerPreviewWidget m_PlayerPreview;
    protected PlayerBase m_DummyPlayer;
    protected bool  m_IsRotating;
    protected int   m_LastMouseX;
    protected bool  m_HasLastMouse;
    protected float m_PreviewYaw;

    void DantyzVipRoupasEmbed()
    {
        m_SelIndex = new map<string, int>();
        m_Labels = new map<string, TextWidget>();
        m_PrevBtns = new map<string, ButtonWidget>();
        m_NextBtns = new map<string, ButtonWidget>();
        m_CharList = new array<string>();
        m_CharIndex = -1;

        m_Categories = new array<string>();
        m_Categories.Insert("Tops");
        m_Categories.Insert("Pants");
        m_Categories.Insert("Boots");
        m_Categories.Insert("Gloves");
        m_Categories.Insert("Helmets");
        m_Categories.Insert("Vests");
        m_Categories.Insert("Backpacks");
        m_Categories.Insert("Belts");

        for (int i = 0; i < m_Categories.Count(); i++)
            m_SelIndex.Set(m_Categories.Get(i), -1);
    }

    void ~DantyzVipRoupasEmbed()
    {
        Destroy();
    }

    // Cria os widgets dentro do parent (a area de conteudo do menu da loja).
    void Create(Widget parent)
    {
        if (m_Root)
            return;

        m_Root = GetGame().GetWorkspace().CreateWidgets("DantyzShop/GUI/Layouts/vip_roupas_embed.layout", parent);
        if (!m_Root)
        {
            return;
        }

        m_Config = DantyzVipRoupasConfig.Load();

        for (int i = 0; i < m_Categories.Count(); i++)
        {
            string cat = m_Categories.Get(i);
            m_Labels.Set(cat, TextWidget.Cast(m_Root.FindAnyWidget(cat + "Label")));
            m_PrevBtns.Set(cat, ButtonWidget.Cast(m_Root.FindAnyWidget(cat + "Prev")));
            m_NextBtns.Set(cat, ButtonWidget.Cast(m_Root.FindAnyWidget(cat + "Next")));
        }

        m_ApplyButton = ButtonWidget.Cast(m_Root.FindAnyWidget("ApplyButton"));
        m_PlayerPreview = PlayerPreviewWidget.Cast(m_Root.FindAnyWidget("PlayerPreview"));

        m_CharPrev = ButtonWidget.Cast(m_Root.FindAnyWidget("CharPrev"));
        m_CharNext = ButtonWidget.Cast(m_Root.FindAnyWidget("CharNext"));
        m_CharLabel = TextWidget.Cast(m_Root.FindAnyWidget("CharLabel"));

        for (int j = 0; j < m_Categories.Count(); j++)
            RefreshLabel(m_Categories.Get(j));

        RefreshCharLabel();

        // O VipRoupasConfig.json so' existe no profile do SERVIDOR, entao no
        // cliente pedimos a lista via RPC. Tambem pedimos a selecao salva.
        GetRPCManager().SendRPC("DantyzShop", "RPCRequestRoupasConfig", null, true);
        GetRPCManager().SendRPC("DantyzShop", "RPCRequestRoupasSelection", null, true);

        RebuildPreview();
    }

    // Recebe a lista de roupas (config) do servidor e atualiza os labels.
    void SetConfig(DantyzVipRoupasConfig cfg)
    {
        if (!cfg || !m_Root)
            return;
        m_Config = cfg;
        for (int i = 0; i < m_Categories.Count(); i++)
            RefreshLabel(m_Categories.Get(i));
        RebuildPreview();
    }

    // Recebe a lista de personagens jogaveis do servidor.
    void SetCharList(array<string> chars)
    {
        if (!chars || !m_Root)
            return;
        m_CharList.Clear();
        for (int i = 0; i < chars.Count(); i++)
            m_CharList.Insert(chars.Get(i));
        RefreshCharLabel();
        // Recria o boneco com o personagem certo (caso ja' tenha selecao).
        RebuildPreview();
    }

    string GetSelectedCharType()
    {
        if (m_CharIndex < 0 || !m_CharList || m_CharIndex >= m_CharList.Count())
            return "";
        return m_CharList.Get(m_CharIndex);
    }

    void RefreshCharLabel()
    {
        if (!m_CharLabel)
            return;
        string t = GetSelectedCharType();
        if (t == "")
            m_CharLabel.SetText("Personagem: padrao");
        else
            m_CharLabel.SetText("Personagem: " + t);
    }

    void CycleChar(int dir)
    {
        if (!m_CharList || m_CharList.Count() == 0)
            return;

        m_CharIndex += dir;
        if (m_CharIndex < -1)
            m_CharIndex = m_CharList.Count() - 1;
        if (m_CharIndex >= m_CharList.Count())
            m_CharIndex = -1;

        RefreshCharLabel();
        RebuildPreview();   // troca o modelo do boneco
    }

    void Destroy()
    {
        DeleteDummy();
        if (m_Root)
        {
            m_Root.Unlink();
            m_Root = null;
        }
    }

    bool IsActive()
    {
        return m_Root != null;
    }

    // ---- helpers de dados ----
    array<ref DantyzRoupaItem> GetList(string cat)
    {
        if (!m_Config)
            return null;
        if (cat == "Tops")           return m_Config.Tops;
        else if (cat == "Pants")     return m_Config.Pants;
        else if (cat == "Boots")     return m_Config.Boots;
        else if (cat == "Gloves")    return m_Config.Gloves;
        else if (cat == "Helmets")   return m_Config.Helmets;
        else if (cat == "Vests")     return m_Config.Vests;
        else if (cat == "Backpacks") return m_Config.Backpacks;
        else if (cat == "Belts")     return m_Config.Belts;
        return null;
    }

    string GetCategoryPrefix(string cat)
    {
        if (cat == "Tops")           return "Camiseta";
        else if (cat == "Pants")     return "Calca";
        else if (cat == "Boots")     return "Botas";
        else if (cat == "Gloves")    return "Luvas";
        else if (cat == "Helmets")   return "Cabeca";
        else if (cat == "Vests")     return "Colete";
        else if (cat == "Backpacks") return "Mochila";
        else if (cat == "Belts")     return "Cinto";
        return cat;
    }

    void RefreshLabel(string cat)
    {
        TextWidget lbl = m_Labels.Get(cat);
        if (!lbl)
            return;

        string prefix = GetCategoryPrefix(cat);
        int idx = m_SelIndex.Get(cat);
        array<ref DantyzRoupaItem> list = GetList(cat);

        if (idx < 0 || !list || idx >= list.Count())
            lbl.SetText(prefix + ": (nenhum)");
        else
            lbl.SetText(prefix + ": " + list.Get(idx).Name);
    }

    void Cycle(string cat, int dir)
    {
        array<ref DantyzRoupaItem> list = GetList(cat);
        if (!list || list.Count() == 0)
            return;

        int idx = m_SelIndex.Get(cat);
        idx += dir;
        if (idx < -1)
            idx = list.Count() - 1;
        if (idx >= list.Count())
            idx = -1;

        m_SelIndex.Set(cat, idx);
        RefreshLabel(cat);
        RebuildPreview();
    }

    string GetSelectedClass(string cat)
    {
        int idx = m_SelIndex.Get(cat);
        array<ref DantyzRoupaItem> list = GetList(cat);
        if (idx < 0 || !list || idx >= list.Count())
            return "";
        return list.Get(idx).Classname;
    }

    // ---- preview (boneco) ----
    void RebuildPreview()
    {
        if (!m_PlayerPreview)
            return;

        CreateDummy();
        if (!m_DummyPlayer)
            return;

        EquipDummy("Tops");
        EquipDummy("Pants");
        EquipDummy("Boots");
        EquipDummy("Gloves");
        EquipDummy("Helmets");
        EquipDummy("Vests");
        EquipDummy("Backpacks");
        EquipDummy("Belts");

        m_PlayerPreview.SetPlayer(m_DummyPlayer);
        // Enquadra o modelo inteiro (cabeca aos pes), centralizado.
        m_PlayerPreview.SetModelPosition(Vector(0, 0, 1.35));
        m_PlayerPreview.SetModelOrientation(Vector(0, m_PreviewYaw, 0));
        m_PlayerPreview.SetSize(1, 1);
    }

    void CreateDummy()
    {
        DeleteDummy();

        PlayerBase realPlayer = PlayerBase.Cast(GetGame().GetPlayer());
        if (!realPlayer)
            return;

        vector dir = GetGame().GetCurrentCameraDirection();
        dir.Normalize();
        vector pos = GetGame().GetCurrentCameraPosition() - dir * 0.5;
        pos[1] = realPlayer.GetPosition()[1];

        // Usa o personagem escolhido (se houver) ou o tipo do player real.
        string charType = GetSelectedCharType();
        if (charType == "")
            charType = realPlayer.GetType();

        m_DummyPlayer = PlayerBase.Cast(GetGame().CreateObjectEx(charType, pos, ECE_LOCAL | ECE_NOLIFETIME));
    }

    void DeleteDummy()
    {
        if (!m_DummyPlayer)
            return;
        GetGame().ObjectDelete(m_DummyPlayer);
        m_DummyPlayer = null;
    }

    void EquipDummy(string cat)
    {
        string cls = GetSelectedClass(cat);
        if (cls == "" || !m_DummyPlayer)
            return;
        m_DummyPlayer.GetInventory().CreateInInventory(cls);
    }

    // ---- aplicar ----
    void ApplyClothes()
    {
        DantyzVipRoupasSelection sel = new DantyzVipRoupasSelection();
        sel.CharacterType = GetSelectedCharType();
        sel.Tops      = GetSelectedClass("Tops");
        sel.Pants     = GetSelectedClass("Pants");
        sel.Boots     = GetSelectedClass("Boots");
        sel.Gloves    = GetSelectedClass("Gloves");
        sel.Helmets   = GetSelectedClass("Helmets");
        sel.Vests     = GetSelectedClass("Vests");
        sel.Backpacks = GetSelectedClass("Backpacks");
        sel.Belts     = GetSelectedClass("Belts");

        GetRPCManager().SendRPC("DantyzShop", "RPCSaveRoupasSelection", new Param1<ref DantyzVipRoupasSelection>(sel), true);

        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player && DantyzShopMsg.On())
            player.MessageStatus("Roupas de spawn salvas! Voce nascera com elas.");
    }

    void ApplySavedSelection(DantyzVipRoupasSelection sel)
    {
        if (!sel || !m_Root)
            return;

        // Pre-seleciona o personagem salvo (se a lista ja' chegou).
        if (sel.CharacterType != "" && m_CharList && m_CharList.Count() > 0)
        {
            int ci = m_CharList.Find(sel.CharacterType);
            if (ci > -1)
                m_CharIndex = ci;
            RefreshCharLabel();
        }

        SetIndexFromClass("Tops", sel.Tops);
        SetIndexFromClass("Pants", sel.Pants);
        SetIndexFromClass("Boots", sel.Boots);
        SetIndexFromClass("Gloves", sel.Gloves);
        SetIndexFromClass("Helmets", sel.Helmets);
        SetIndexFromClass("Vests", sel.Vests);
        SetIndexFromClass("Backpacks", sel.Backpacks);
        SetIndexFromClass("Belts", sel.Belts);

        RebuildPreview();
    }

    void SetIndexFromClass(string cat, string classname)
    {
        array<ref DantyzRoupaItem> list = GetList(cat);
        if (classname == "" || !list)
        {
            m_SelIndex.Set(cat, -1);
            RefreshLabel(cat);
            return;
        }

        for (int i = 0; i < list.Count(); i++)
        {
            if (DantyzShopUtils.SameClass(list.Get(i).Classname, classname))
            {
                m_SelIndex.Set(cat, i);
                RefreshLabel(cat);
                return;
            }
        }
    }

    // Repassado pelo DantyzShopMenu.OnClick. Retorna true se tratou o clique.
    bool HandleClick(Widget w)
    {
        if (!m_Root)
            return false;

        if (w == m_ApplyButton)
        {
            ApplyClothes();
            return true;
        }

        if (w == m_CharPrev)
        {
            CycleChar(-1);
            return true;
        }
        if (w == m_CharNext)
        {
            CycleChar(1);
            return true;
        }

        for (int i = 0; i < m_Categories.Count(); i++)
        {
            string cat = m_Categories.Get(i);
            if (w == m_PrevBtns.Get(cat))
            {
                Cycle(cat, -1);
                return true;
            }
            if (w == m_NextBtns.Get(cat))
            {
                Cycle(cat, 1);
                return true;
            }
        }

        return false;
    }

    // Rotacao do boneco arrastando o mouse (repassado pelo menu da loja).
    bool HandleMouseDown(Widget w)
    {
        if (m_Root && IsPreview(w))
        {
            m_IsRotating = true;
            m_HasLastMouse = false;
            return true;
        }
        return false;
    }

    void HandleMouseUp()
    {
        m_IsRotating = false;
        m_HasLastMouse = false;
    }

    void HandleUpdate()
    {
        if (!m_IsRotating || !m_PlayerPreview)
            return;

        int mx, my;
        GetMousePos(mx, my);

        if (!m_HasLastMouse)
        {
            m_LastMouseX = mx;
            m_HasLastMouse = true;
            return;
        }

        int dx = mx - m_LastMouseX;
        m_LastMouseX = mx;
        if (dx == 0)
            return;

        m_PreviewYaw = m_PreviewYaw + (dx * 0.4);
        m_PlayerPreview.SetModelOrientation(Vector(0, m_PreviewYaw, 0));
    }

    bool IsPreview(Widget w)
    {
        Widget cur = w;
        while (cur)
        {
            if (cur == m_PlayerPreview)
                return true;
            cur = cur.GetParent();
        }
        return false;
    }
}
