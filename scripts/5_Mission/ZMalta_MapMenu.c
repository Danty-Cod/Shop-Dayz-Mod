
modded class MapMenu
{
	// Icone custom que substitui a bolinha vermelha vanilla.
	protected const string ZMALTA_PLAYER_ICON = "DantyzShop\\data\\zmalta\\player.paa";

	// Tamanho (px) do icone no mapa. Escala com o zoom mas nunca some.
	protected const float ZMALTA_ICON_MIN = 34.0;
	protected const float ZMALTA_ICON_MAX = 90.0;

	override Widget Init()
	{
		// Deixa o menu vanilla montar tudo normalmente.
		Widget root = super.Init();

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player || !m_MapWidgetInstance)
			return root;

		// Fonte da posicao real do player (GetPositionReal == GetPosition).
		if (!m_MapNavigationBehaviour)
			m_MapNavigationBehaviour = player.GetMapNavigationBehaviour();

		// Liga o marcador de posicao independentemente de ter GPS.
		m_HasGPS = true;
		SetGPSMarkerVisibility(true);

		// Troca a textura da bolinha vermelha pelo icone custom do player.
		ImageWidget marker = ImageWidget.Cast(m_GPSMarker);
		if (marker)
		{
			marker.LoadImageFile(0, ZMALTA_PLAYER_ICON);
			marker.SetImage(0);
		}

		// Sem seta de direcao (compass) - so' o icone.
		SetGPSDirectionVisibility(false);

		// Centraliza o mapa no player ao abrir.
		if (m_MapNavigationBehaviour)
			m_MapWidgetInstance.SetMapPos(m_MapNavigationBehaviour.GetPositionReal());

		return root;
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		if (!player || !m_MapWidgetInstance || !m_GPSMarker)
			return;

		if (!m_MapNavigationBehaviour)
		{
			m_MapNavigationBehaviour = player.GetMapNavigationBehaviour();
			if (!m_MapNavigationBehaviour)
				return;
		}

		// Reforca visivel (o vanilla pode ter escondido em algum caminho).
		m_GPSMarker.Show(true);

		// Tamanho com minimo garantido, acompanhando o zoom do mapa.
		float scale = 1.0 - m_MapWidgetInstance.GetScale();
		float size = Math.Clamp(scale * 100.0, ZMALTA_ICON_MIN, ZMALTA_ICON_MAX);
		m_GPSMarker.SetSize(size, size);

		// Posiciona o icone na posicao REAL do player, centralizado no ponto.
		vector screenPos = m_MapWidgetInstance.MapToScreen(m_MapNavigationBehaviour.GetPositionReal());
		float sizeX, sizeY;
		m_GPSMarker.GetSize(sizeX, sizeY);
		m_GPSMarker.SetPos(screenPos[0] - sizeX * 0.5, screenPos[1] - sizeY * 0.5);
	}
};
