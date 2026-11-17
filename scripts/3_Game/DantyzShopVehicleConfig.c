class DantyzVehicleParts
{
    string VehicleName;
    float SpawnDistance;
    ref array<string> Parts;
    
    void DantyzVehicleParts()
    {
        Parts = new array<string>();
        SpawnDistance = 2.0;
    }
}

class DantyzShopVehicleConfig
{
    ref array<ref DantyzVehicleParts> Vehicles;
    
    void DantyzShopVehicleConfig()
    {
        Vehicles = new array<ref DantyzVehicleParts>();
    }
    
    static ref DantyzShopVehicleConfig Load()
    {
        string path = "$profile:SimpleModParty/Store/VehiclesConfig.json";
        
        DantyzShopVehicleConfig config;
        
        if (!FileExist(path))
        {
            config = CreateDefault();
            config.Save();
        }
        else
        {
            config = new DantyzShopVehicleConfig();
            JsonFileLoader<DantyzShopVehicleConfig>.JsonLoadFile(path, config);
        }
        
        return config;
    }
    
    void Save()
    {
        string path = "$profile:SimpleModParty/Store/VehiclesConfig.json";
        JsonFileLoader<DantyzShopVehicleConfig>.JsonSaveFile(path, this);
    }
    
    static ref DantyzShopVehicleConfig CreateDefault()
    {
        DantyzShopVehicleConfig config = new DantyzShopVehicleConfig();
        
        DantyzVehicleParts olga = new DantyzVehicleParts();
        olga.VehicleName = "Olga_02";
        olga.SpawnDistance = 2.5;
        olga.Parts.Insert("SparkPlug");
        olga.Parts.Insert("CarBattery");
        olga.Parts.Insert("CarRadiator");
        olga.Parts.Insert("HeadlightH7");
        olga.Parts.Insert("HeadlightH7");
        olga.Parts.Insert("Olga_02_Door_1_1");
        olga.Parts.Insert("Olga_02_Door_1_2");
        olga.Parts.Insert("Olga_02_Door_2_1");
        olga.Parts.Insert("Olga_02_Door_2_2");
        olga.Parts.Insert("Olga_02_Hood");
        olga.Parts.Insert("Olga_02_Trunk");
        olga.Parts.Insert("Olga_02_Wheel");
        olga.Parts.Insert("Olga_02_Wheel");
        olga.Parts.Insert("Olga_02_Wheel");
        olga.Parts.Insert("Olga_02_Wheel");
        config.Vehicles.Insert(olga);
        
        DantyzVehicleParts sedan = new DantyzVehicleParts();
        sedan.VehicleName = "Sedan_02";
        sedan.SpawnDistance = 2.5;
        sedan.Parts.Insert("SparkPlug");
        sedan.Parts.Insert("CarBattery");
        sedan.Parts.Insert("CarRadiator");
        sedan.Parts.Insert("HeadlightH7");
        sedan.Parts.Insert("HeadlightH7");
        sedan.Parts.Insert("Sedan_02_Door_1_1");
        sedan.Parts.Insert("Sedan_02_Door_1_2");
        sedan.Parts.Insert("Sedan_02_Door_2_1");
        sedan.Parts.Insert("Sedan_02_Door_2_2");
        sedan.Parts.Insert("Sedan_02_Hood");
        sedan.Parts.Insert("Sedan_02_Trunk");
        sedan.Parts.Insert("Sedan_02_Wheel");
        sedan.Parts.Insert("Sedan_02_Wheel");
        sedan.Parts.Insert("Sedan_02_Wheel");
        sedan.Parts.Insert("Sedan_02_Wheel");
        config.Vehicles.Insert(sedan);
        
        DantyzVehicleParts hatchback = new DantyzVehicleParts();
        hatchback.VehicleName = "Hatchback_02";
        hatchback.SpawnDistance = 2.5;
        hatchback.Parts.Insert("SparkPlug");
        hatchback.Parts.Insert("CarBattery");
        hatchback.Parts.Insert("CarRadiator");
        hatchback.Parts.Insert("HeadlightH7");
        hatchback.Parts.Insert("HeadlightH7");
        hatchback.Parts.Insert("Hatchback_02_Door_1_1");
        hatchback.Parts.Insert("Hatchback_02_Door_1_2");
        hatchback.Parts.Insert("Hatchback_02_Door_2_1");
        hatchback.Parts.Insert("Hatchback_02_Door_2_2");
        hatchback.Parts.Insert("Hatchback_02_Hood");
        hatchback.Parts.Insert("Hatchback_02_Trunk");
        hatchback.Parts.Insert("Hatchback_02_Wheel");
        hatchback.Parts.Insert("Hatchback_02_Wheel");
        hatchback.Parts.Insert("Hatchback_02_Wheel");
        hatchback.Parts.Insert("Hatchback_02_Wheel");
        config.Vehicles.Insert(hatchback);
        
        DantyzVehicleParts sarka = new DantyzVehicleParts();
        sarka.VehicleName = "Sarka_02";
        sarka.SpawnDistance = 2.5;
        sarka.Parts.Insert("SparkPlug");
        sarka.Parts.Insert("CarBattery");
        sarka.Parts.Insert("CarRadiator");
        sarka.Parts.Insert("HeadlightH7");
        sarka.Parts.Insert("HeadlightH7");
        sarka.Parts.Insert("Sarka_02_Door_1_1");
        sarka.Parts.Insert("Sarka_02_Door_1_2");
        sarka.Parts.Insert("Sarka_02_Door_2_1");
        sarka.Parts.Insert("Sarka_02_Door_2_2");
        sarka.Parts.Insert("Sarka_02_Hood");
        sarka.Parts.Insert("Sarka_02_Trunk");
        sarka.Parts.Insert("Sarka_02_Wheel");
        sarka.Parts.Insert("Sarka_02_Wheel");
        sarka.Parts.Insert("Sarka_02_Wheel");
        sarka.Parts.Insert("Sarka_02_Wheel");
        config.Vehicles.Insert(sarka);
        
        return config;
    }
    

    DantyzVehicleParts GetVehicleConfig(string vehicleName)
    {
        for (int i = 0; i < Vehicles.Count(); i++)
        {
            DantyzVehicleParts veh = Vehicles.Get(i);
            if (!veh)
                continue;

            if (DantyzShopUtils.SameClass(veh.VehicleName, vehicleName))
            {
                return veh;
            }
        }
        return null;
    }
    
    bool IsVehicle(string className)
    {
        return GetVehicleConfig(className) != null;
    }
}
