modded class MissionBase
{
    override UIScriptedMenu CreateScriptedMenu(int id)
    {
        UIScriptedMenu menu = NULL;
        menu = super.CreateScriptedMenu(id);
        
        if (!menu)
        {
            if (id == MENU_DANTYZ_SHOP)
            {
                menu = new DantyzShopMenu;
            }
        }
        
        return menu;
    }
}
