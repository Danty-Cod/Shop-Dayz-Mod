
class DantyzCarMark
{
	vector      pos;
	string      name;
	Widget      root;
	ImageWidget img;
	TextWidget  txt;
	TextWidget  dist;
}

class DantyzCarMarkerClient
{
	static ref DantyzCarMarkerClient g_Instance;

	protected ref array<ref DantyzCarMark> m_Marks = new array<ref DantyzCarMark>();

	protected static const string CAR_MARKER_LAYOUT = "DantyzShop/GUI/Layouts/car_marker3d.layout";
	protected static const string CAR_MARKER_ICON   = "DantyzShop/data/selecticonshop/money/carros.edds";
	protected static const float  CAR_MARKER_HEIGHT = 2.5;   // acima do carro

	static DantyzCarMarkerClient Get()
	{
		if (!g_Instance)
			g_Instance = new DantyzCarMarkerClient();
		return g_Instance;
	}

	protected bool IsClientDisplay()
	{
		return GetGame() && !GetGame().IsDedicatedServer();
	}

	// Mostra os marcadores dos carros (recria do zero). Some em 30 min.
	void ShowMarks(array<vector> positions, array<string> names)
	{
		ClearMarks();

		if (!IsClientDisplay() || !positions || positions.Count() == 0)
			return;

		for (int i = 0; i < positions.Count(); i++)
		{
			string nm = "Carro";
			if (names && i < names.Count())
				nm = names.Get(i);

			DantyzCarMark mk = new DantyzCarMark();
			mk.pos  = positions.Get(i);
			mk.name = nm;
			m_Marks.Insert(mk);
		}

		// Some sozinho em 30 min (backstop caso o servidor pare de atualizar).
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(ClearMarks);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ClearMarks, 1800000, false);
	}

	// Atualiza posicoes (chamado periodicamente). Se o conjunto mudou de
	// tamanho/nomes, recria; senao, so' move.
	void UpdateMarks(array<vector> positions, array<string> names)
	{
		if (!IsClientDisplay() || !positions)
			return;

		if (m_Marks.Count() != positions.Count() || !NamesMatch(names))
		{
			ShowMarks(positions, names);
			return;
		}

		for (int i = 0; i < m_Marks.Count(); i++)
		{
			DantyzCarMark mk = m_Marks.Get(i);
			if (mk)
				mk.pos = positions.Get(i);
		}
	}

	protected bool NamesMatch(array<string> names)
	{
		int have = m_Marks.Count();
		if (!names)
			return have == 0;
		if (have != names.Count())
			return false;
		for (int i = 0; i < have; i++)
		{
			if (m_Marks.Get(i).name != names.Get(i))
				return false;
		}
		return true;
	}

	// Remove todos os marcadores e seus widgets.
	void ClearMarks()
	{
		for (int i = 0; i < m_Marks.Count(); i++)
		{
			DantyzCarMark mk = m_Marks.Get(i);
			if (mk && mk.root)
			{
				mk.root.Unlink();
				mk.root = null;
			}
		}
		m_Marks.Clear();
	}

	// Compatibilidade: antes limpava marcadores orfaos do BXD. Agora os icones
	// sao widgets locais (nao persistem), entao basta limpar tudo.
	void PurgeAll()
	{
		ClearMarks();
	}

	// Cria o widget de um marcador (uma vez).
	protected void EnsureWidget(DantyzCarMark mk)
	{
		if (!mk || mk.root)
			return;

		Widget w = GetGame().GetWorkspace().CreateWidgets(CAR_MARKER_LAYOUT, null);
		if (!w)
			return;

		mk.root = w;
		mk.img  = ImageWidget.Cast(w.FindAnyWidget("Icon"));
		mk.txt  = TextWidget.Cast(w.FindAnyWidget("Label"));
		mk.dist = TextWidget.Cast(w.FindAnyWidget("Dist"));

		if (mk.img)
		{
			mk.img.LoadImageFile(0, CAR_MARKER_ICON);
			mk.img.SetImage(0);
		}
		if (mk.txt)
			mk.txt.SetText(mk.name);
	}

	// Chamado a cada frame pelo MissionGameplay.OnUpdate (cliente).
	void RenderAll()
	{
		if (!IsClientDisplay() || m_Marks.Count() == 0)
			return;

		vector camPos = GetGame().GetCurrentCameraPosition();

		for (int i = 0; i < m_Marks.Count(); i++)
		{
			DantyzCarMark mk = m_Marks.Get(i);
			if (!mk)
				continue;

			EnsureWidget(mk);
			if (!mk.root)
				continue;

			vector wp = mk.pos;
			wp[1] = wp[1] + CAR_MARKER_HEIGHT;

			vector sp = GetGame().GetScreenPos(wp);

			// atras da camera -> esconde
			if (sp[2] <= 0)
			{
				mk.root.Show(false);
				continue;
			}

			mk.root.Show(true);

			float sw, sh;
			mk.root.GetScreenSize(sw, sh);
			mk.root.SetPos(sp[0] - sw * 0.5, sp[1] - sh * 0.5);

			if (mk.dist)
			{
				float d = vector.Distance(camPos, wp);
				mk.dist.SetText("" + Math.Round(d) + "m");
			}
		}
	}
}
