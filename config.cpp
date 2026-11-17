#define _ARMA_

class CfgPatches
{
	class DantyzShop
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Worlds_Chernarusplus_World", "CarLock_Script"};
	};
};

class CfgMods
{
	class DantyzShop
	{
		dir = "DantyzShop";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "DantyzShop";
		credits = "Dantyz";
		author = "Dantyz";
		authorID = "0";
		version = "1.0";
		extra = 0;
		type = "mod";
		inputs = "DantyzShop/Data/Inputs.xml";
		
		dependencies[] = {"Game", "World", "Mission"};
		
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"DantyzShop/scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"DantyzShop/scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"DantyzShop/scripts/5_Mission"};
			};
		};
	};
};


class CfgVehicles
{
	class GPSReceiver;
	class rastreadordecarro: GPSReceiver
	{
		scope = 2;
		displayName = "Rastreador de Carro";
		descriptionShort = "Dispositivo de rastreamento veicular. Precisa de bateria. Ao ligar, marca no mapa (so para voce) a localizacao dos seus carros com SEGURO por 30 minutos. O dispositivo se autodestroi 1 minuto apos a ativacao (uso unico).";
		weight = 100;
	};
};
class CfgWorlds
{
	class CAWorld;
	class ChernarusPlus: CAWorld
	{
		mapTextureOpened = "DantyzShop\data\zmalta\karta_co.paa";
		mapTextureLegend = "DantyzShop\data\zmalta\karta_side_co.paa";
	};
};