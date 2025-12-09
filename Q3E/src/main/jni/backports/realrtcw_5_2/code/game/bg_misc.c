/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		bg_misc.c
 *
 * desc:		both games misc functions, all completely stateless
 *
*/


#include "../qcommon/q_shared.h"
#include "bg_public.h"

#ifdef CGAMEDLL
extern vmCvar_t cg_gameType;
#endif
#ifdef GAMEDLL
extern vmCvar_t g_gametype;
#endif

// New ET vehicle path system
int numSplinePaths;
splinePath_t splinePaths[MAX_SPLINE_PATHS];

int numPathCorners;
pathCorner_t pathCorners[MAX_PATH_CORNERS];

// NOTE: This once-static data is included in both Client and Game modules.
//       Both now load values into here from weap files.
//       All values are empty because of that
ammoTable_t ammoTable[WP_NUM_WEAPONS] = {
    // No weapon
	{   
		WP_NONE,             // 1.weaponindex
	    WEAPON_CLASS_NONE,   // 2. weaponClass
		WP_NONE,             // 3. weap alt
		WEAPON_TEAM_NONE,    //  4. weapon team
	    0,                   // 5. maxammo
		0,                   // 6. maxammoUpgraded
		0,                   // 7. uses
		0,                   // 7.5 uses upgraded
		0,                   // 8. maxclip
		0,                   // 9. maxclipUpgraded
		0,                   // 10. reloadTime
		0,                   // 11. reloadTimeFull
		50,                  // 12. fireDelayTime
		0,                   // 13. nextShotTime
		0,                   // 14. nextShotTimeUpgraded
		0,                   // 15. nextShotTime2
		0,                   // 16. nextShotTime2Upgraded
		0,                   // 17. maxHeat
		0,                   // 18. coolRate
		0,                   // 19. playerDamage
		0,                   // 20. playerDamageUpgraded
		0,                   // 21. aiDamage
		0,                   // 22. playerSplashRadius
		0,                   // 23. aiSplashRadius
		0,                   // 24. spread
		0,                   // 25. spreadUpgraded
		0,                   // 26. aimSpreadScaleadd
		0.0f,                // 27. spreadScale
		0,                   // 28. weapRecoilDuration
		{0, 0},              // 29. weapRecoilPitch
		{0,0},               // 30. weapRecoilYaw
		1.00,                // 31. soundRange
		1.00,                // 32. moveSpeed
		0,                   // 33. twoHand
		0,                   // 34. upAngle
		{0.0, 0.0},          // 35. falloffdistance
		0,                   // 36. mod     
		0,                   // 37. shotgunReloadStart
		0,                   // 38. shotgunReloadLoop;
		0,                   // 39. shotgunReloadEnd;
		0,                   // 40. shotgunPumpStart;
		0,                   // 41. shotgunPumpLoop;
		0,                   // 42. shotgunPumpEnd;
		0,                   // 43. brassDelayEmpty;
		0,                   // 44. brassDelay;
	}, 
    
	// Melee weapons
	{   
		WP_KNIFE,             
	    WEAPON_CLASS_MELEE,
		WP_NONE,
		WEAPON_TEAM_COMMON,              
	    0,                  
		0,                         
		0,                  
		0,
		0,
		0,
		0,
		0,
		0,
		0,                   
		0,
		0,                 
		0,                  
		0,
		0,                  
		0,                   
		0,                    
		0,                   
		0,                    
		0,                  
		0,                   
		0,                   
		0,                    
		0.0f,                 
		0,                   
		{0, 0},               
		{0,0},                
		0,                   
		0.00,                 
		0,                  
		0,
		{0.0, 0.0},                            
		MOD_KNIFE,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                    
	}, 
    
    // One handed pistols
	{   
		WP_LUGER,              
		WEAPON_CLASS_PISTOL,
		WP_NONE,
		WEAPON_TEAM_AXIS,          
		0,          
		0,                     
		0,
		0,                     
		0,                 
		0,
		0,
		0,
		0,
		0,
		0,                 
		0,
		0,                 
		0,                 
		0,
		0,                    
		0,                   
		0,                   
		0,                    
		0,                    
		0,                    
		0,                  
		0,                   
		0.0f,                
		0,                   
		{.0f, .0f},           
		{0,0},              
		0,                  
		0.0,                 
		0,                    
		0,
		{0.0, 0.0},                    
		MOD_LUGER,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	},  

	{  
		WP_SILENCER,           
		WEAPON_CLASS_PISTOL,
		WP_NONE,
		WEAPON_TEAM_AXIS,       
		0,        
		0,                     
		0,                     
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                 
		0,                  
		0,                  
		0,
		0,                  
		0,                    
		0,                   
		0,                    
		0,                     
		0,                    
		0,                    
		0,                
		0,                  
		0.0f,                
		0,                  
		{.0f, .0f},          
		{0,0},              
		0,                  
		0.0,                
		0,                   
		0,
		{0.0, 0.0},                   
		MOD_SILENCER,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                
	},  

	{   
		WP_COLT,             
		WEAPON_CLASS_PISTOL,
		WP_NONE,
		WEAPON_TEAM_ALLIES,  
	    0,         
		0,                   
		0,                   
		0,              
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                 
		0, 
		0,              
		0,                
		0,                  
		0,                  
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.00,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_COLT,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                    
	}, 

	{   
		WP_TT33,
		WEAPON_CLASS_PISTOL,
		WP_NONE,
		WEAPON_TEAM_SOVIET,             
		0,     
		0,       
		0,        
		0,
		0,
		0,  
		0,
		0,
		0,
		0,
		0,      
		0,
		0,       
		0,       
		0,       
		0,      
		0,      
		0,        
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,           
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_TT33,      
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	}, 

	{   
	    WP_REVOLVER,
		WEAPON_CLASS_PISTOL,
		WP_NONE,
		WEAPON_TEAM_ALLIES,         
		0,   
		0,       
		0,        
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,       
		0,       
		0,      
		0,      
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_REVOLVER,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                               
	},

	{   
		WP_HDM,
		WEAPON_CLASS_PISTOL,
		WP_NONE,
		WEAPON_TEAM_ALLIES,           
		0,     
		0,       
		0,        
		0,        
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,      
		0,
		0,      
		0,      
		0,      
		0,       
		0,         
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},    
		{0,0},              
		0,                 
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_HDM, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                    
	}, 

		{   
		WP_AKIMBO,
		WEAPON_CLASS_AKIMBO,
		WP_NONE,
		WEAPON_TEAM_ALLIES,           
		0,         
		0,       
		0,        
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0, 
		0,      
		0,       
		0,       
		0,
		0,      
		0,      
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,                
		0.0,               
		0,               
		0,  
		{0.0, 0.0},             
		MOD_AKIMBO,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                
    }, 

	{   
		WP_DUAL_TT33,
		WEAPON_CLASS_AKIMBO,
		WP_NONE,
		WEAPON_TEAM_SOVIET,           
		0,         
		0,       
		0,        
		0,        
		0, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,      
		0,
		0,       
		0,       
		0,      
		0,      
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,                
		0.0,               
		0,               
		0,  
		{0.0, 0.0},             
		MOD_DUAL_TT33,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                
    }, 
	
    // SMGs
	{   
		WP_MP40,
		WEAPON_CLASS_SMG,
		WP_NONE,
		WEAPON_TEAM_AXIS,             
	    0,        
		0,       
		0,       
		0,        
		0,       
		0,
		0, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,      
		0,       
		0,      
		0,      
		0,        
		0,          
		0,          
		0,         
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,              
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_MP40,  
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                     
	},

	{   
		WP_THOMPSON,  
		WEAPON_CLASS_SMG,
		WP_NONE,
		WEAPON_TEAM_ALLIES,       
		0,         
		0,       
		0,       
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,
		0,        
		0,        
		0,      
		0,
		0,      
		0,        
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_THOMPSON,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                
	}, 

	{   
		WP_STEN,
		WEAPON_CLASS_SMG,
		WP_NONE,
		WEAPON_TEAM_ALLIES,             
		0,        
		0,       
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,        
		0,       
		0,
		0,       
		0,       
		0,    
		0,    
		0,        
		0,
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,                 
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_STEN,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                    
	}, 

	{   
		WP_PPSH,
		WEAPON_CLASS_SMG,
		WP_NONE,
		WEAPON_TEAM_SOVIET,             
		0,     
		0,       
		0,
		0,       
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,        
		0,        
		0, 
		0,     
		0,      
		0,        
		0,          
		0,          
		0,          
		0,       
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.00,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_PPSH,      
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	}, 
	
	{   
		WP_MP34,
		WEAPON_CLASS_SMG,
		WP_NONE,
		WEAPON_TEAM_AXIS,             
		0,        
		0,       
		0,
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,        
		0,       
		0,      
		0,       
		0,
		0,      
		0,      
		0,        
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,           
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_MP34,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	},
	// Rifles
	{   
		WP_MAUSER,
		WEAPON_CLASS_RIFLE | WEAPON_CLASS_SCOPABLE,
		WP_SNIPERRIFLE,
		WEAPON_TEAM_AXIS,           
		0,     
		0,       
		0,        
		0,
		0,
		0,        
		0,
		0,
		0,
		0,
		0,
		0,       
		0,      
		0,      
		0,      
		0,      
		0,
		0,       
		0,         
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{0.0f, 0.0f},   
		{.0f, .0f},         
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_MAUSER,  
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	},  

	{   
		WP_GARAND,
		WEAPON_CLASS_RIFLE | WEAPON_CLASS_SCOPABLE,
		WP_SNOOPERSCOPE,
		WEAPON_TEAM_ALLIES,           
		0,     
		0,       
		0,
		0,        
		0,        
		0,       
		0,      
		0, 
		0,
		0,
		0,
		0,
		0,
		0,     
		0,
		0,      
		0,      
		0,       
		0,         
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{0.0f, 0.0f},    
		{.0f,.0f},          
		0,                
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_GARAND,     
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                
	},  

	{   
		WP_MOSIN,
		WEAPON_CLASS_RIFLE,
		WP_NONE,
		WEAPON_TEAM_SOVIET,            
		0,     
		0,       
		0,        
		0,        
		0,       
		0,     
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,      
		0,      
		0,      
		0,       
		0,         
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{0.0f, 0.0f},    
		{.0f, .0f},         
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_MOSIN,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                             
	}, 

	{   
		WP_DELISLE,
		WEAPON_CLASS_RIFLE | WEAPON_CLASS_SCOPABLE,
		WP_DELISLESCOPE,
		WEAPON_TEAM_ALLIES,          
		0,         
		0,       
		0,        
		0,
		0,        
		0,       
		0,  
		0,    
		0,
		0, 
		0,
		0,
		0,
		0,
		0,     
		0,      
		0,      
		0,       
		0,         
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,             
		{0.0f, 0.0f},    
		{.0f, .0f},         
		0,                
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_DELISLE,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	},  

	// Semi auto rifles
	{   
		WP_M1GARAND,
		WEAPON_CLASS_RIFLE,
		WP_M7,
		WEAPON_TEAM_ALLIES,         
		0,    
		0,       
		0,
		0,        
		0,        
		0,  
		0,     
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,       
		0,      
		0,      
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f,.0f},       
		{.0f, .0f},         
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_M1GARAND,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                              
	}, 

	{   
		WP_G43,
		WEAPON_CLASS_RIFLE,
		WP_NONE,
		WEAPON_TEAM_AXIS,              
		0,     
		0,       
		0,       
		0,        
		0,       
		0,       
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,      
		0,      
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f,.0f},       
		{.0f, .0f},         
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_G43,     
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                  
	},

	{   
		WP_M1941,
		WEAPON_CLASS_RIFLE | WEAPON_CLASS_SCOPABLE,
		WP_M1941SCOPE,
		WEAPON_TEAM_ALLIES,              
		0,     
		0,       
		0,       
		0,        
		0,       
		0,       
		0, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,      
		0,
		0,      
		0,      
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f,.0f},       
		{.0f, .0f},         
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_M1941,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                               
	},
	// Assault Rifles
	{   
		WP_MP44,
		WEAPON_CLASS_ASSAULT_RIFLE,
		WP_NONE,
		WEAPON_TEAM_AXIS,             
		0,     
		0,
		0,       
		0,       
		0,        
		0,  
		0,     
		0,
		0,       
		0, 
		0,
		0,
		0,
		0,
		0,      
		0,      
		0,      
		0,        
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_MP44,     
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                  
	},

	{   
		WP_FG42,
		WEAPON_CLASS_ASSAULT_RIFLE | WEAPON_CLASS_SCOPABLE,
		WP_FG42SCOPE,
		WEAPON_TEAM_AXIS,            
		0,       
		0,       
		0,
		0,       
		0,  
		0,      
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,       
		0,       
		0,      
		0,      
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_FG42,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	},

	{   
		WP_BAR,
		WEAPON_CLASS_ASSAULT_RIFLE,
		WP_NONE,
		WEAPON_TEAM_ALLIES,              
		0,    
		0,       
		0,       
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,       
		0,       
		0,      
		0,      
		0,       
		0,
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.00,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_BAR,    
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                    
	},
   // Shotguns
	{   
		WP_M97,
		WEAPON_CLASS_SHOTGUN,
		WP_NONE,
		WEAPON_TEAM_ALLIES,              
		0,        
		0,       
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,        
		0,       
		0,      
		0,      
		0,      
		0,      
		0,       
		0,
		0,          
		0,          
		0,          
		0,       
		0,                 
		0.0f,            
		0,             
		{.0f, .0f},     
		{.0f, .0f},         
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_M97,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                     
	},

	{   
		WP_AUTO5,
		WEAPON_CLASS_SHOTGUN,
		WP_NONE,
		WEAPON_TEAM_ALLIES,              
		0,        
		0,       
		0,        
		0,        
		0,       
		0,
		0,      
		0,      
		0,
		0,
		0,
		0,
		0,
		0,
		0,      
		0,      
		0,       
		0,
		0,          
		0,          
		0,          
		0,       
		0,                 
		0.0f,            
		0,             
		{.0f, .0f},     
		{.0f, .0f},         
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_AUTO5, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                       
	},
   // Heavy Weapons
	{   
		WP_BROWNING,
		WEAPON_CLASS_MG,
		WP_NONE,
		WEAPON_TEAM_ALLIES,         
		0,    
		0,       
		0,      
		0,        
		0,       
		0,
		0,
		0,
		0,
		0,
		0,  
		0,      
		0,
		0,        
		0,   
		0,    
		0,
		0,       
		0,          
		0,          
		0,          
		0,       
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{.0f, .0f},         
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_BROWNING, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                  
	}, 

	{   
		WP_MG42M,
		WEAPON_CLASS_MG,
		WP_NONE,
		WEAPON_TEAM_AXIS,           
		0,      
		0,       
		0,
		0,      
		0,
		0,
		0,
		0,
		0,
		0,  
		0,      
		0,
		0,       
		0,        
		0,        
		0,   
		0,    
		0,       
		0,          
		0,          
		0,          
		0,       
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{.0f, .0f},         
		0,               
		0.0,              
		0,               
		0,
		{0.0, 0.0},             
		MOD_MG42M,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	},

	{   
		WP_PANZERFAUST,
		WEAPON_CLASS_LAUNCHER,
		WP_NONE,
		WEAPON_TEAM_COMMON,      
		0,                   
		0,       
		0,        
		0,        
		0,        
		0,
		0, 
		0,
		0,
		0,
		0,
		0,
		0,     
		0,
		0,      
		0,      
		0,      
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                 
		0.0f,            
		0,               
		{.0, 0},         
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_PANZERFAUST, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                               
	},

	{   
		WP_FLAMETHROWER,
		WEAPON_CLASS_BEAM,
		WP_NONE,
		WEAPON_TEAM_COMMON,     
		0,                 
		0,       
		0,      
		0,        
		0,
		0, 
		0,
		0,
		0,
		0,
		0,
		0,      
		0,
		0,        
		0,        
		0,      
		0,      
		0,        
		0,          
		0,          
		0,          
		0,          
		0,                 
		0.0f,            
		0,               
		{0, 0},          
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_FLAMETHROWER,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                            
	},
	// Secret Weapons
	{   
		WP_VENOM,
		WEAPON_CLASS_MG,
		WP_NONE,
		WEAPON_TEAM_AXIS,            
		0,      
		0,       
		0,      
		0, 
		0,
		0,
		0,
		0,
		0,       
		0, 
		0,  
		0,    
		0,
		0,        
		0,        
		0,   
		0,    
		0,       
		0,          
		0,          
		0,          
		0,       
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{.0f, .0f},         
		0,               
		0.0,               
		1,               
		0,
		{0.0, 0.0},               
		MOD_VENOM,     
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	},  

	{   
		WP_TESLA,
		WEAPON_CLASS_BEAM,
		WP_NONE,
		WEAPON_TEAM_AXIS,            
		0,                  
		0,
		0,       
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,        
		0,       
		0,       
		0,       
		0,      
		0,      
		0,       
		0,          
		0,          
		0,          
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},          
		{0,0},              
		0,               
		0.0,               
		0,               
		0, 
		{0.0, 0.0},             
		MOD_TESLA,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	}, 
   // Explosives
	{  
		WP_GRENADE_LAUNCHER,
		WEAPON_CLASS_GRENADE,
		WP_NONE,
		WEAPON_TEAM_AXIS,
		0,                   
		0,
		0,       
		0,        
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,      
		0,
		0,      
		0,      
		0,      
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},          
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_GRENADE_LAUNCHER,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                        
	},

	{  
		WP_GRENADE_PINEAPPLE,
		WEAPON_CLASS_GRENADE,
		WP_NONE,
		WEAPON_TEAM_AXIS | WEAPON_TEAM_SOVIET, 
		0,                   
		0,       
		0,        
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,      
		0,      
		0,      
		0,      
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                  
		0.0f,           
		0,               
		{0, 0},          
		{0,0},              
		0,               
		0.0,               
		0,               
		0, 
		{0.0, 0.0},            
		MOD_GRENADE_PINEAPPLE, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                         
	}, 

	{   
		WP_DYNAMITE,
		WEAPON_CLASS_GRENADE,
		WP_NONE,
		WEAPON_TEAM_COMMON,         
		0,                   
		0,       
		0,        
		0,        
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,      
		0,
		0,      
		0,      
		0,      
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_DYNAMITE,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                
	}, 

	{   
		WP_AIRSTRIKE,
		WEAPON_CLASS_GRENADE,
		WP_NONE,
		WEAPON_TEAM_COMMON,        
		0,                 
		0,      
		0,      
		0,           
		0,        
		0,         
		0,
		0,
		0,
		0,
		0,
		0,
		0,         
		0,
		0,
		0,      
		0,      
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},          
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		0,           
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	}, 

	{   
		WP_ARTY,
		WEAPON_CLASS_NONE,
		WP_NONE,
		WEAPON_TEAM_COMMON,        
		0,                 
		0,      
		0,      
		0,           
		0,        
		0,         
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,         
		0,
		0,      
		0,      
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},          
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_ARTY,           
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	}, 

	{   
		WP_POISONGAS,
		WEAPON_CLASS_GRENADE,
		WP_NONE, 
		WEAPON_TEAM_COMMON,       
		0,                   
		0,       
		0,        
		0,           
		0,        
		0, 
		0,  
		0,      
		0,
		0,
		0,
		0,
		0,
		0,
		0,         
		0,      
		0,      
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},         
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_POISONGAS, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	}, 

		{   
		WP_POISONGAS_MEDIC,
		WEAPON_CLASS_GRENADE,
		WP_NONE, 
		WEAPON_TEAM_COMMON,       
		0,                   
		0,       
		0,  
		0,      
		0,           
		0,        
		0,         
		0,
		0,         
		0,
		0,
		0,
		0,
		0,
		0,      
		0, 
		0,     
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},         
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_POISONGAS, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	}, 

		{   
		WP_DYNAMITE_ENG,
		WEAPON_CLASS_GRENADE,
		WP_NONE, 
		WEAPON_TEAM_COMMON,       
		0,                   
		0,       
		0,        
		0,           
		0,        
		0,         
		0,
		0,  
		0,       
		0,      
		0,
		0,
		0,
		0,
		0,
		0,      
		0,      
		0,        
		0,        
		0, 
		0,       
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},         
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_DYNAMITE, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	}, 

		{   
		WP_SMOKETRAIL,
		WEAPON_CLASS_GRENADE,
		WP_NONE, 
		WEAPON_TEAM_COMMON,       
		0,                   
		0,       
		0,        
		0,           
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,         
		0,
		0,         
		0,      
		0,      
		0,      
		0,        
		0,        
		0,        
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},         
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},             
		MOD_POISONGAS, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	}, 

	{   
		WP_HOLYCROSS,
		WEAPON_CLASS_BEAM,
		WP_NONE,
		WEAPON_TEAM_NONE,        
		0,                  
		0,       
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,        
		0,       
		0,       
		0,  
		0,     
		0,      
		0,      
		0,      
		0,        
		0,          
		0,          
		0,          
		0,                  
		0.0f,            
		0,               
		{0, 0},          
		{0, 0},             
		0,                 
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_HOLYCROSS,      
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                            
	}, 

	// Misc alt modes
	{   
		WP_SNIPERRIFLE,
		WEAPON_CLASS_RIFLE | WEAPON_CLASS_SCOPED,
		WP_MAUSER,
		WEAPON_TEAM_AXIS,      
		0,     
		0,       
		0,        
		0,
		0,  
		0,      
		0,
		0,
		0,
		0,
		0,
		0, 
		0,        
		0,      
		0,      
		0,      
		0,      
		0,       
		0,         
		0,          
		0,          
		0,        
		0,                  
		0.0f,           
		0,               
		{0,0},           
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_SNIPERRIFLE,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                             
	}, 

	{   
		WP_SNOOPERSCOPE,
		WEAPON_CLASS_RIFLE | WEAPON_CLASS_SCOPED,
		WP_GARAND,
		WEAPON_TEAM_ALLIES,     
		0,     
		0,       
		0,        
		0,
		0, 
		0,
		0,       
		0,         
		0,
		0,
		0,
		0,
		0,
		0,      
		0,      
		0,      
		0,      
		0,       
		0,         
		0,          
		0,          
		0,        
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0,0},              
		0,                
		0.0,               
		0,               
		0,
		{0.0, 0.0},              
		MOD_SNOOPERSCOPE,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                           
	}, 

	{   
		WP_DELISLESCOPE,
		WEAPON_CLASS_RIFLE | WEAPON_CLASS_SCOPED, 
		WP_DELISLE,  
		WEAPON_TEAM_ALLIES,  
		0,         
		0,       
		0,
		0,  
		0,      
		0,        
		0, 
		0,
		0,
		0,
		0,
		0,        
		0,
		0,      
		0,      
		0,      
		0,      
		0,       
		0,        
		0,          
		0,          
		0,        
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0,0},              
		0,                
		0.00,               
		0,               
		0, 
		{0.0, 0.0},              
		MOD_DELISLESCOPE,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                            
	}, 

	{   
		WP_M1941SCOPE,
		WEAPON_CLASS_RIFLE | WEAPON_CLASS_SCOPED, 
		WP_M1941,  
		WEAPON_TEAM_ALLIES,  
		0,         
		0,       
		0,
		0,        
		0,        
		0,
		0,
		0,
		0,
		0,
		0,
		0,         
		0,
		0,      
		0,      
		0,      
		0,      
		0,       
		0,        
		0,          
		0,          
		0,        
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0,0},              
		0,                
		0.00,               
		0,               
		0, 
		{0.0, 0.0},              
		MOD_M1941SCOPE,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                            
	}, 
	
	
	{   
		WP_FG42SCOPE,
		WEAPON_CLASS_ASSAULT_RIFLE | WEAPON_CLASS_SCOPED,
		WP_FG42,
		WEAPON_TEAM_AXIS,        
		0,       
		0,       
		0,       
		0,        
		0,       
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,       
		0,      
		0,
		0,
		0,      
		0,       
		0,          
		0,          
		0,          
		0,        
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0,0},              
		0,               
		0.0,               
		0,               
		0, 
		{0.0, 0.0},             
		MOD_FG42SCOPE, 
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                 
	}, 

	{   
		WP_M7,
		WEAPON_CLASS_RIFLENADE,
		WP_M1GARAND,
		WEAPON_TEAM_ALLIES,               
		0,                   
		0,       
		0, 
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,        
		0,      
		0,       
		0,
		0,       
		0,      
		0,      
		0,       
		0,         
		0,          
		0,          
		0,          
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0, 0},             
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_M7,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                  
	},

    // Misc stuff, not actual weapons
		{   
		WP_DUMMY_MG42,
		WEAPON_CLASS_UNUSED,
		WP_NONE,
		WEAPON_TEAM_COMMON,             
		0,     
		0,
		0,       
		0,       
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,        
		0,       
		0,       
		0,       
		0,      
		0,      
		0,        
		0,          
		0,          
		0,          
		0,        
		0,                 
		0.0f,            
		0,              
		{.0f, .0f},      
		{0,0},              
		0,               
		0.0,               
		0,               
		0,
		{0.0, 0.0},               
		MOD_MACHINEGUN,   
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                    
	},
	
	{   
		WP_MONSTER_ATTACK1,
		WEAPON_CLASS_NONE,
		WP_NONE,
		WEAPON_TEAM_NONE,  
		999,
		999,                 
		0,
		0,       
		999,
		999,      
		0,
		0,           
		50,        
		1000,
		1000,      
		1000,
		1000,      
		0,
		0,      
		0,      
		0,        
		0,
		0,          
		0,          
		0,          
		0,          
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0,0},              
		1000,               
		0.0f,                 
		0,               
		0,
		{0.0, 0.0},               
		0,           
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	},

	{   
		WP_MONSTER_ATTACK2,
		WEAPON_CLASS_NONE,
		WP_NONE, 
		WEAPON_TEAM_NONE, 
		999,
		999,                 
		0,
		0,       
		999,
		999,      
		0,
		0,           
		50,        
		250,
		250,       
		250,
		250,       
		0,      
		0,
		0, 
		0,     
		0,        
		0,          
		0,          
		0,          
		0,          
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0,0},              
		1000,               
		0,                  
		0,               
		0, 
		{0.0, 0.0},            
		0,      
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                        
	}, 

	{   
		WP_MONSTER_ATTACK3,
		WEAPON_CLASS_NONE,
		WP_NONE,
		WEAPON_TEAM_NONE,  
		999,
		999,                 
		0,  
		0,     
		999,
		999,
		0,      
		0,           
		50,        
		250, 
		250,      
		250,
		250,      
		0,
		0,      
		0,
		0,      
		0,        
		0,          
		0,          
		0,          
		0,          
		0,                  
		0.0f,            
		0,               
		{0,0},           
		{0,0},              
		1000,               
		0,                  
		0,               
		0,  
		{0.0, 0.0},             
		0,          
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,                                   
	},  
};

// Skill-based ammo parameters
ammoskill_t ammoSkill[GSKILL_NUM_SKILLS][WP_NUM_WEAPONS];

int reloadableWeapons[] = {
	WP_MP40, WP_THOMPSON, WP_STEN, WP_GARAND, WP_PANZERFAUST, WP_FLAMETHROWER,
	WP_SILENCER, WP_TT33, WP_FG42, WP_REVOLVER, WP_MG42M, WP_COLT,
	WP_LUGER, WP_MORTAR, WP_AKIMBO, WP_PPSH, WP_M7, WP_MP34,
	WP_MAUSER, WP_SNIPERRIFLE, WP_SNOOPERSCOPE, WP_MOSIN, WP_M1GARAND, WP_G43,
	WP_MP44, WP_BAR, WP_M97, WP_FG42SCOPE, WP_BROWNING, WP_VENOM,
	WP_DELISLE, WP_DELISLESCOPE, WP_TESLA, WP_M1941, WP_AUTO5,
	WP_M1941SCOPE, WP_DUAL_TT33, WP_HDM, -1};

// new (10/18/00)
char *animStrings[] = {
	"BOTH_DEATH1",
	"BOTH_DEAD1",
	"BOTH_DEAD1_WATER",
	"BOTH_DEATH2",
	"BOTH_DEAD2",
	"BOTH_DEAD2_WATER",
	"BOTH_DEATH3",
	"BOTH_DEAD3",
	"BOTH_DEAD3_WATER",

	"BOTH_CLIMB",
	"BOTH_CLIMB_DOWN",
	"BOTH_CLIMB_DISMOUNT",

	"BOTH_SALUTE",

	"BOTH_PAIN1",
	"BOTH_PAIN2",
	"BOTH_PAIN3",
	"BOTH_PAIN4",
	"BOTH_PAIN5",
	"BOTH_PAIN6",
	"BOTH_PAIN7",
	"BOTH_PAIN8",

	"BOTH_GRAB_GRENADE",

	"BOTH_ATTACK1",
	"BOTH_ATTACK2",
	"BOTH_ATTACK3",
	"BOTH_ATTACK4",
	"BOTH_ATTACK5",

	"BOTH_EXTRA1",
	"BOTH_EXTRA2",
	"BOTH_EXTRA3",
	"BOTH_EXTRA4",
	"BOTH_EXTRA5",
	"BOTH_EXTRA6",
	"BOTH_EXTRA7",
	"BOTH_EXTRA8",
	"BOTH_EXTRA9",
	"BOTH_EXTRA10",
	"BOTH_EXTRA11",
	"BOTH_EXTRA12",
	"BOTH_EXTRA13",
	"BOTH_EXTRA14",
	"BOTH_EXTRA15",
	"BOTH_EXTRA16",
	"BOTH_EXTRA17",
	"BOTH_EXTRA18",
	"BOTH_EXTRA19",
	"BOTH_EXTRA20",

	"TORSO_GESTURE",
	"TORSO_GESTURE2",
	"TORSO_GESTURE3",
	"TORSO_GESTURE4",

	"TORSO_DROP",

	"TORSO_RAISE",   // (low)
	"TORSO_ATTACK",
	"TORSO_STAND",
	"TORSO_STAND_ALT1",
	"TORSO_STAND_ALT2",
	"TORSO_READY",
	"TORSO_RELAX",

	"TORSO_RAISE2",  // (high)
	"TORSO_ATTACK2",
	"TORSO_STAND2",
	"TORSO_STAND2_ALT1",
	"TORSO_STAND2_ALT2",
	"TORSO_READY2",
	"TORSO_RELAX2",

	"TORSO_RAISE3",  // (pistol)
	"TORSO_ATTACK3",
	"TORSO_STAND3",
	"TORSO_STAND3_ALT1",
	"TORSO_STAND3_ALT2",
	"TORSO_READY3",
	"TORSO_RELAX3",

	"TORSO_RAISE4",  // (shoulder)
	"TORSO_ATTACK4",
	"TORSO_STAND4",
	"TORSO_STAND4_ALT1",
	"TORSO_STAND4_ALT2",
	"TORSO_READY4",
	"TORSO_RELAX4",

	"TORSO_RAISE5",  // (throw)
	"TORSO_ATTACK5",
	"TORSO_ATTACK5B",
	"TORSO_STAND5",
	"TORSO_STAND5_ALT1",
	"TORSO_STAND5_ALT2",
	"TORSO_READY5",
	"TORSO_RELAX5",

	"TORSO_RELOAD1", // (low)
	"TORSO_RELOAD2", // (high)
	"TORSO_RELOAD3", // (pistol)
	"TORSO_RELOAD4", // (shoulder)

	"TORSO_MG42",        // firing tripod mounted weapon animation

	"TORSO_MOVE",        // torso anim to play while moving and not firing (swinging arms type thing)
	"TORSO_MOVE_ALT",        // torso anim to play while moving and not firing (swinging arms type thing)

	"TORSO_EXTRA",
	"TORSO_EXTRA2",
	"TORSO_EXTRA3",
	"TORSO_EXTRA4",
	"TORSO_EXTRA5",
	"TORSO_EXTRA6",
	"TORSO_EXTRA7",
	"TORSO_EXTRA8",
	"TORSO_EXTRA9",
	"TORSO_EXTRA10",

	"LEGS_WALKCR",
	"LEGS_WALKCR_BACK",
	"LEGS_WALK",
	"LEGS_RUN",
	"LEGS_BACK",
	"LEGS_SWIM",
	"LEGS_SWIM_IDLE",

	"LEGS_JUMP",
	"LEGS_JUMPB",
	"LEGS_LAND",

	"LEGS_IDLE",
	"LEGS_IDLE_ALT", //	"LEGS_IDLE2"
	"LEGS_IDLECR",

	"LEGS_TURN",

	"LEGS_BOOT",     // kicking animation

	"LEGS_EXTRA1",
	"LEGS_EXTRA2",
	"LEGS_EXTRA3",
	"LEGS_EXTRA4",
	"LEGS_EXTRA5",
	"LEGS_EXTRA6",
	"LEGS_EXTRA7",
	"LEGS_EXTRA8",
	"LEGS_EXTRA9",
	"LEGS_EXTRA10",
};


// old
char *animStringsOld[] = {
	"BOTH_DEATH1",
	"BOTH_DEAD1",
	"BOTH_DEATH2",
	"BOTH_DEAD2",
	"BOTH_DEATH3",
	"BOTH_DEAD3",

	"BOTH_CLIMB",
	"BOTH_CLIMB_DOWN",
	"BOTH_CLIMB_DISMOUNT",

	"BOTH_SALUTE",

	"BOTH_PAIN1",
	"BOTH_PAIN2",
	"BOTH_PAIN3",
	"BOTH_PAIN4",
	"BOTH_PAIN5",
	"BOTH_PAIN6",
	"BOTH_PAIN7",
	"BOTH_PAIN8",

	"BOTH_EXTRA1",
	"BOTH_EXTRA2",
	"BOTH_EXTRA3",
	"BOTH_EXTRA4",
	"BOTH_EXTRA5",

	"TORSO_GESTURE",
	"TORSO_GESTURE2",
	"TORSO_GESTURE3",
	"TORSO_GESTURE4",

	"TORSO_DROP",

	"TORSO_RAISE",   // (low)
	"TORSO_ATTACK",
	"TORSO_STAND",
	"TORSO_READY",
	"TORSO_RELAX",

	"TORSO_RAISE2",  // (high)
	"TORSO_ATTACK2",
	"TORSO_STAND2",
	"TORSO_READY2",
	"TORSO_RELAX2",

	"TORSO_RAISE3",  // (pistol)
	"TORSO_ATTACK3",
	"TORSO_STAND3",
	"TORSO_READY3",
	"TORSO_RELAX3",

	"TORSO_RAISE4",  // (shoulder)
	"TORSO_ATTACK4",
	"TORSO_STAND4",
	"TORSO_READY4",
	"TORSO_RELAX4",

	"TORSO_RAISE5",  // (throw)
	"TORSO_ATTACK5",
	"TORSO_ATTACK5B",
	"TORSO_STAND5",
	"TORSO_READY5",
	"TORSO_RELAX5",

	"TORSO_RELOAD1", // (low)
	"TORSO_RELOAD2", // (high)
	"TORSO_RELOAD3", // (pistol)
	"TORSO_RELOAD4", // (shoulder)

	"TORSO_MG42",        // firing tripod mounted weapon animation

	"TORSO_MOVE",        // torso anim to play while moving and not firing (swinging arms type thing)

	"TORSO_EXTRA2",
	"TORSO_EXTRA3",
	"TORSO_EXTRA4",
	"TORSO_EXTRA5",

	"LEGS_WALKCR",
	"LEGS_WALKCR_BACK",
	"LEGS_WALK",
	"LEGS_RUN",
	"LEGS_BACK",
	"LEGS_SWIM",

	"LEGS_JUMP",
	"LEGS_LAND",

	"LEGS_IDLE",
	"LEGS_IDLE2",
	"LEGS_IDLECR",

	"LEGS_TURN",

	"LEGS_BOOT",     // kicking animation

	"LEGS_EXTRA1",
	"LEGS_EXTRA2",
	"LEGS_EXTRA3",
	"LEGS_EXTRA4",
	"LEGS_EXTRA5",
};

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) SUSPENDED SPIN PERSISTANT
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
SUSPENDED - will allow items to hang in the air, otherwise they are dropped to the next surface.
SPIN - will allow items to spin in place.
PERSISTANT - some items (ex. clipboards) can be picked up, but don't disappear

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
"stand" if the item has a stand (ex: mp40_stand.md3) this specifies which stand tag to attach the weapon to ("stand":"4" would mean "tag_stand4" for example)  only weapons support stands currently
*/

gitem_t bg_itemlist[] =
{
	{
		NULL,  // classname
		NULL,  // pickup sound
		{ NULL, //model1
		  NULL, //model2
		  0  }, //model3
		NULL,   // icon
		NULL,   // pickup name
		0,      // quantity
		0,      //giType
		WP_NONE, //giWeapon
		0,       //giTag
		0,          // ammoindex
		0,          // ammoindexSurv
		0,          // clipindex
		"",          // precache
		"",          // sounds
		{0,0,0,0,0,0}   // gameskill
	},  // leave index 0 alone



/*QUAKED item_clipboard (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN PERSISTANT
"model" - model to display in the world.  defaults to 'models/powerups/clipboard/clipboard.md3' (the clipboard laying flat is 'clipboard2.md3')
"popup" - menu to popup.  no default since you won't want the same clipboard laying around. (clipboard will display a 'put popup here' message)
"notebookpage" - when clipboard is picked up, this page (menu) will be added to your notebook (FIXME: TODO: more info goes here)

We currently use:
clip_interrogation
clip_codeddispatch
clip_alertstatus

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/clipboard/clipboard.md3"
*/
/*
"scriptName"
*/
	{
		"item_clipboard",
		"sound/pickup/armor/body_pickup.wav",
		{   
		"models/powerups/clipboard/clipboard.md3",
		0,
		0 
		},
		"icons/iconh_small",
		"",
		1,
		IT_CLIPBOARD,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0,0,0}
	},

/*QUAKED item_treasure (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN
Items the player picks up that are just used to tally a score at end-level
"model" defaults to 'models/powerups/treasure/goldbar.md3'
"noise" sound to play on pickup.  defaults to 'sound/pickup/treasure/gold.wav'
"message" what to call the item when it's picked up.  defaults to "Treasure Item" (SA: temp)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/treasure/goldbar.md3"
*/
/*
"scriptName"
*/
	{
		"item_treasure",
		"sound/pickup/treasure/gold.wav",
		{ 
		"models/powerups/treasure/goldbar.md3",
		0,
		0 
		},
		"icons/iconh_small",
		"Treasure Item",
		5,
		IT_TREASURE,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0,0,0}
	},


	//
	// ARMOR/HEALTH/STAMINA
	//


/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_s.md3"
*/
	{
		"item_health_small",
		"sound/pickup/health/health_pickup.wav",
		{   
		"models/powerups/health/health_s.md3",
		0,
		0 
		},
		"icons/iconh_small",
		"Small Health",
		5,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{15,10,5,5,1,10}
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_m.md3"
*/
	{
		"item_health",
		"sound/pickup/health/health_pickup.wav",
		{   
		"models/powerups/health/health_m.md3",
		0,
		0 
		},
		"icons/iconh_med",
		"Med Health",
		25,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{30,25,15,10,3,20}
	},

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_l.md3"
*/
	{
		"item_health_large",
		"sound/pickup/health/health_pickup.wav",
		{   
		"models/powerups/health/health_l.md3",
		0, 
		0
		},
		"icons/iconh_large",
		"Large Health",
		50,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{50,40,30,20,5,25}
	},

/*QUAKED item_health_turkey (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
multi-stage health item.
gives amount on first use based on skill:
skill 1: 50
skill 2: 50
skill 3: 50
skill 4: 40
skill 5: 30

then gives 15 on "finishing up"

player will only eat what he needs.  health at 90, turkey fills up and leaves remains (leaving 15).  health at 5 you eat the whole thing.
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_t1.md3"
*/
	{
		"item_health_turkey",
		"sound/pickup/health/hot_pickup.wav",
		{   
		"models/powerups/health/health_t3.md3",  // just plate (should now be destructable)
		"models/powerups/health/health_t2.md3",  // half eaten
		"models/powerups/health/health_t1.md3"  // whole turkey
		},
		"icons/iconh_turkey",
		"Hot Meal",
		5,                 // amount given in last stage
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{15,15,15,10,2,10}   // amount given in first stage based on gameskill level
	},

/*QUAKED item_health_breadandmeat (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
multi-stage health item.
gives amount on first use based on skill:
skill 1: 30
skill 2: 30
skill 3: 30
skill 4: 20
skill 5: 10

then gives 10 on "finishing up"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_b1.md3"
*/
	{
		"item_health_breadandmeat",
		"sound/pickup/health/cold_pickup.wav",
		{   
		"models/powerups/health/health_b3.md3",  // just plate (should now be destructable)
		"models/powerups/health/health_b2.md3",  // half eaten
		"models/powerups/health/health_b1.md3"  // whole turkey
		},
		"icons/iconh_breadandmeat",
		"Cold Meal",
		5,                 // amount given in last stage
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{15,15,15,10,2,10}   // amount given in first stage based on gameskill level
	},

/*QUAKED item_health_wall_box (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED - - RESPAWN
single use health with dual state model.
please set the suspended flag to keep it from falling on the ground
defaults to 50 pts health
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_wallbox.md3"
*/
	{
		"item_health_wall_box",
		"sound/pickup/health/health_pickup.wav",
		{   
		"models/powerups/health/health_wallbox2.md3",
		"models/powerups/health/health_wallbox1.md3",
		0 
		},
		"icons/iconh_wall",
		"Health",
		25,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{25,25,25,15,3,15}
	},

/*QUAKED item_health_wall (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED - - RESPAWN
defaults to 50 pts health
you will probably want to check the 'suspended' box to keep it from falling to the ground
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_w.md3"
*/
	{
		"item_health_wall",
		"sound/pickup/health/health_pickup.wav",
		{   
		"models/powerups/health/health_w.md3",
		0, 
		0
		},
		"icons/iconh_wall",
		"Health",
		25,
		IT_HEALTH,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{30,20,15,15,3,15}
	},

	//
	// STAMINA
	//


/*QUAKED item_stamina_stein (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
defaults to 30 sec stamina boost
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/instant/stamina_stein.md3"
*/

	{
		"item_stamina_stein",
		"sound/pickup/health/stamina_pickup.wav",
		{
		"models/powerups/instant/stamina_stein.md3",
		0, 
		0
		},
		"icons/icons_stein",
		"Stamina",
		25,
		IT_POWERUP,
		WP_NONE,
		PW_NOFATIGUE,
		0,
		0,
		0,
		"",
		"",
		{30,25,20,15,1,20}
	},


/*QUAKED item_stamina_brandy (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
defaults to 30 sec stamina boost

multi-stage health item.
gives amount on first use based on skill:
skill 1: 50
skill 2: 50
skill 3: 50
skill 4: 40
skill 5: 30

then gives 15 on "finishing up"

player will only eat what he needs.  health at 90, turkey fills up and leaves remains (leaving 15).  health at 5 you eat the whole thing.
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/instant/stamina_brandy1.md3"
*/

	{
		"item_stamina_brandy",
		"sound/sound/pickup/health/stamina_pickup.wav",
		{   
		"models/powerups/instant/stamina_brandy2.md3",
		"models/powerups/instant/stamina_brandy1.md3",
		0
		},
		"icons/icons_brandy",
		"Stamina",
		25,
		IT_POWERUP,
		WP_NONE,
		PW_NOFATIGUE,
		0,
		0,
		0,
		"",
		"",
		{30,25,20,15,1,20}
	},


	//
	// ARMOR
	//


/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/armor/armor_body1.md3"
*/
	{
		"item_armor_body",
		"sound/pickup/armor/body_pickup.wav",
		{   
		"models/powerups/armor/armor_body1.md3",
		0, 
		0
		},
		"icons/iconr_body",
		"Flak Jacket",
		75,
		IT_ARMOR,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{75,75,75,75,75,75}
	},

/*QUAKED item_armor_body_hang (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED - - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/armor/armor_body2.md3"
*/
	{
		"item_armor_body_hang",
		"sound/pickup/armor/body_pickup.wav",
		{   
		"models/powerups/armor/armor_body2.md3",
		0, 
		0
		},
		"icons/iconr_bodyh",
		"Flak Jacket",
		75,
		IT_ARMOR,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{75,75,75,75,75,75}
	},

/*QUAKED item_armor_head (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/armor/armor_head1.md3"
*/
	{
		"item_armor_head",
		"sound/pickup/armor/head_pickup.wav",
		{   
		"models/powerups/armor/armor_head1.md3",
		0,
		0
		},
		"icons/iconr_head",
		"Armored Helmet",
		25,
		IT_ARMOR,
		WP_NONE,
		0,
		0,
		0,
		0,
		"",
		"",
		{25,25,25,25,25,25}
	},



	//
	// WEAPONS
	//


/*QUAKED weapon_knife (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/knife/knife.md3"
*/
	{
		"weapon_knife",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_knife_1",   
		"Knife",             
		50,
		IT_WEAPON,
	    WP_KNIFE,
		WP_KNIFE,
		WP_KNIFE,
		WP_KNIFE,
		WP_KNIFE,
		"",                     
		"",                     
		{0,0,0,0,0,0}
	},


/*QUAKED weapon_luger (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/luger/luger.md3"
*/
	{
		"weapon_luger",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_luger_1",   
		"Luger",             
		50,
		IT_WEAPON,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                     
		"",                      
		{0,0,0,0,0,0}
	},


/*QUAKED weapon_mauserRifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mauser/mauser.md3"
*/
	{
		"weapon_mauserRifle",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_mauser_1", 
		"Mauser Rifle",          
		50,
		IT_WEAPON,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_thompson (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/thompson/thompson.md3"
*/
	{
		"weapon_thompson",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_thompson_1",  
		"Thompson",              
		30,
		IT_WEAPON,
		WP_THOMPSON,
		WP_THOMPSON,
		WP_COLT,
		WP_THOMPSON,
		WP_THOMPSON,
		"",                 
		"",                  
		{0,0,0,0,0,0}
	},

	/*QUAKED weapon_delisle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mauser/mauser.md3"
*/
	{
		"weapon_delisle",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_delisle_1", 
		"delisle",          
		50,
		IT_WEAPON,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},


	/*QUAKED weapon_delislescope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mauser/mauser.md3"
*/
	{
		"weapon_delislescope",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_delislescope_1", 
		"delislescope",          
		50,
		IT_WEAPON,
		WP_DELISLESCOPE,
		WP_DELISLESCOPE,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},

	/*QUAKED weapon_m1941scope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mauser/mauser.md3"
*/
	{
		"weapon_m1941scope",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_m1941scope_1", 
		"m1941scope",          
		50,
		IT_WEAPON,
		WP_M1941SCOPE,
		WP_M1941SCOPE,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_sten (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/sten/sten.md3"
*/
	{
		"weapon_sten",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},
		"icons/iconw_sten_1",    
		"Sten",                  
		30,
		IT_WEAPON,
		WP_STEN,
		WP_STEN,
		WP_LUGER,
		WP_STEN,
		WP_STEN,
		"",                  
		"",                 
		{0,0,0,0,0,0}
	},

/*weapon_akimbo
dual colts
*/
	{
		"weapon_akimbo",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_colt_1",    
		"Dual Colts",            
		50,
		IT_WEAPON,
		WP_AKIMBO,
		WP_AKIMBO,
		WP_COLT,
		WP_AKIMBO,
		WP_AKIMBO,
		"",                     
		"",                     
		{0,0,0,0,0,0}
	},


/*weapon_akimbo
dual TT33
*/
	{
		"weapon_dualtt33",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_tt33",    
		"Dual TT33",            
		50,
		IT_WEAPON,
		WP_DUAL_TT33,
		WP_DUAL_TT33,
		WP_TT33,
		WP_DUAL_TT33,
		WP_DUAL_TT33,
		"",                     
		"",                     
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_colt (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/colt/colt.md3"
*/
	{
		"weapon_colt",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_colt_1",    
		"Colt",                  
		50,
		IT_WEAPON,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},


// (SA) snooper is the parent, so 'garand' is no longer available as a stand-alone weap w/ an optional scope
/*
weapon_garandRifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/garand/garand.md3"
*/
	{
		"NOT_weapon_garandRifle",    //----(SA)	modified so it can no longer be given individually
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_garand_1",  
		"garand",                      
		50,
		IT_WEAPON,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_mp40 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models\weapons2\mp40\mp40.md3"
*/
	{
		"weapon_mp40",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_mp40_1",      
		"MP40",              
		30,
		IT_WEAPON,
		WP_MP40,
		WP_MP40,
		WP_LUGER,
		WP_MP40,
		WP_MP40,
		"",                  
		"",                
		{0,0,0,0,0,0}
	},



/*QUAKED weapon_fg42 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/fg42/fg42.md3"
*/
	{
		"weapon_fg42",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_fg42_1",   
		"FG42 Paratroop Rifle",      
		10,
		IT_WEAPON,
		WP_FG42,
		WP_FG42,
		WP_MAUSER,
		WP_FG42,
		WP_FG42,
		"",                  
		"",                  
		{0,0,0,0,0,0}
	},



//----(SA)	modified sp5 to be silencer mod for luger
/*QUAKED weapon_silencer (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/sp5/sp5.md3"
*/
	{
		"weapon_silencer",
		"sound/misc/w_pkup.wav",
		{  
		"",
		"",
		""
		},

		"icons/iconw_silencer_1",    
		"sp5 pistol",
		10,
		IT_WEAPON,
		WP_SILENCER,
		WP_SILENCER,
		WP_LUGER,
		WP_SILENCER,
		WP_LUGER,
		"",                 
		"",                  
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_panzerfaust (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/panzerfaust/pf.md3"
*/
	{
		"weapon_panzerfaust",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_panzerfaust_1", 
		"Panzerfaust",               
		1,
		IT_WEAPON,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},


//----(SA)	removed the quaked for this.  we don't actually have a grenade launcher as such.  It's given implicitly
//			by virtue of getting grenade ammo.  So we don't need to have them in maps
/*
weapon_grenadelauncher
*/
	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_grenade_1", 
		"Grenade",               
		6,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		"",                      
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",             
		{0,0,0,0,0,0}
	},

/*
weapon_grenadePineapple
*/
	{
		"weapon_grenadepineapple",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_pineapple_1",  
		"Pineapple",             
		6,
		IT_WEAPON,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		"",                      
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",            
		{0,0,0,0,0,0}
	},

//weapon_dynamite

	{
		"weapon_dynamite",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_dynamite_1",    
		"Dynamite Weapon",       
		7,
		IT_WEAPON,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		"",                      
		"",                     
		{0,0,0,0,0,0}
	},




/*QUAKED weapon_venom (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/venom/pu_venom.md3"
*/
	{
		"weapon_venom",
		"sound/misc/w_pkup.wav",
		{  
		"",
		"",
		""
		},

		"icons/iconw_venom_1",   
		"Venom",             
		700,
		IT_WEAPON,
		WP_VENOM,
		WP_VENOM,
		WP_MG42M,
		WP_VENOM,
		WP_VENOM,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},



/*QUAKED weapon_flamethrower (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/flamethrower/pu_flamethrower.md3"
*/
	{
		"weapon_flamethrower",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_flamethrower_1",    
		"Flamethrower",             
		200,
		IT_WEAPON,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		"",                          
		"",                          
		{0,0,0,0,0,0}
	},


/*QUAKED weapon_tesla (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/tesla/pu_tesla.md3"
*/
	{
		"weapon_tesla",
		"sound/misc/w_pkup.wav",

		{   
		"",
		"",
		""
		},

		"icons/iconw_tesla_1",   
		"Tesla Gun",             
		200,
		IT_WEAPON,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		"",                          
		"",                          
		{0,0,0,0,0,0}
	},



/*QUAKED weapon_sniperScope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mauser/pu_mauser_scope.md3"
*/
	{
		"weapon_sniperScope",
		"sound/misc/w_pkup.wav",
		{  
		"",
	    "",
		""
		},

		"icons/iconw_mauser_1",  
		"Sniper Scope",              
		200,
		IT_WEAPON,
		WP_SNIPERRIFLE,
		WP_SNIPERRIFLE,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                         
		"",                         
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_snooperrifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/garand/garand.md3"
*/
	{
		"weapon_snooperrifle",
		"sound/misc/w_pkup.wav",
		{  
		"",
	    "",
		""
		},

		"icons/iconw_garand_1",  
		"Snooper Rifle",
		20,
		IT_WEAPON,
		WP_SNOOPERSCOPE,
		WP_SNOOPERSCOPE,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                          
		"",                          
		{0,0,0,0,0,0}
	},

/* weapon_fg42scope
*/
	{
		"weapon_fg42scope",  
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_fg42_1",    
		"FG42 Scope",                	
		0,
		IT_WEAPON,
		WP_FG42SCOPE,
		WP_FG42SCOPE,      
		WP_MAUSER,
		WP_FG42,      
		WP_FG42,        
		"",                          
		"",                          
		{0,0,0,0,0,0}
	},


// Ridah, need this for the scripting
/*
weapon_monster_attack1 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_monster_attack1",
		"",
		{   
		"",
		"",
		0
		},
		"",  
		"MonsterAttack1",            
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,        
		WP_MONSTER_ATTACK1,         
		"",                         
		"",                          
		{0,0,0,0,0,0}
	},
/*
weapon_monster_attack2 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_monster_attack2",
		"",
		{   
		"",
		"",
		0 
		},
		"", 
		"MonsterAttack2",            
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK2,
		WP_MONSTER_ATTACK2,
		WP_MONSTER_ATTACK2,
		WP_MONSTER_ATTACK2,        
		WP_MONSTER_ATTACK2,         
		"",                          
		"",                          
		{0,0,0,0,0,0}
	},
/*
weapon_monster_attack3 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_monster_attack3",
		"",
		{
		"",
		"",
		0
		},
		"",  
		"MonsterAttack3",            
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK3,
		WP_MONSTER_ATTACK3,
		WP_MONSTER_ATTACK3,
		WP_MONSTER_ATTACK3,        
		WP_MONSTER_ATTACK3,
		"",                          
		"",                          
		{0,0,0,0,0,0}
	},

/*
weapon_mortar (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_mortar",
		"sound/misc/w_pkup.wav",
		{   
		"models/weapons2/grenade/grenade.md3",
		"models/weapons2/grenade/v_grenade.md3",
		"models/weapons2/grenade/pu_grenade.md3"
		},
		"icons/iconw_grenade_1",
		"nopickup(WP_MORTAR)",      
		6,
		IT_WEAPON,
		WP_MORTAR,
		WP_MORTAR,
		WP_MORTAR,
		WP_MORTAR,
		WP_MORTAR,
		"",                     
		"sound/weapons/mortar/mortarf1.wav",             
		{0,0,0,0,0,0}
	},


// RealRTCW weapons

/*QUAKED weapon_mp34 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mp34/mp34_3rd.md3"
*/
	{
		"weapon_mp34",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_mp34",    
		"MP34",              
		30,
		IT_WEAPON,
		WP_MP34,
		WP_MP34,
		WP_LUGER,
		WP_MP34,
		WP_MP34,
		"",                  
		"",                 
		{0,0,0,0,0,0}
	},

	/*QUAKED weapon_tt33 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/tt33/tt33.md3"
*/
	{
		"weapon_tt33",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_tt33",    
		"tt33",             
		30,
		IT_WEAPON,
		WP_TT33,
		WP_TT33,
		WP_TT33,
		WP_TT33,
		WP_TT33,
		"",                 
		"",                 
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_ppsh (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/ppsh/ppsh.md3"
*/
	{
		"weapon_ppsh",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_ppsh_1",   
		"ppsh",              
		30,
		IT_WEAPON,
		WP_PPSH,
		WP_PPSH,
		WP_TT33,
		WP_PPSH,
		WP_PPSH,
		"",                  
		"",                 
		{0,0,0,0,0}
	},

/*QUAKED weapon_mosin (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mosin/mosin.md3"
*/
	{
		"weapon_mosin",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_mosin",    
		"mosin",               
		30,
		IT_WEAPON,
		WP_MOSIN,
		WP_MOSIN,
		WP_MOSIN,
		WP_MOSIN,
		WP_MOSIN,
		"",                 
		"",                 
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_g43 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/multiplayer/g43/g43_3rd.md3"
*/
	{
		"weapon_g43",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_g43",    
		"g43",             
		30,
		IT_WEAPON,
		WP_G43,
		WP_G43,
		WP_MAUSER,
		WP_G43,
		WP_G43,
		"",                  
		"",                  
		{0,0,0,0,0,0}
	},


/*QUAKED weapon_m1941 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/multiplayer/m1941/m1941_3rd.md3"
*/
	{
		"weapon_m1941",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_m1941",    
		"m1941",             
		30,
		IT_WEAPON,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		"",                  
		"",                  
		{0,0,0,0,0,0}
	},


/*QUAKED weapon_m1_garand (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/multiplayer/m1_garand/m1_garand_3rd.md3"
*/
	{
		"weapon_m1garand",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_m1garand",    
		"m1garand",              
		30,
		IT_WEAPON,
		WP_M1GARAND,
		WP_M1GARAND,
		WP_BAR,
		WP_M1GARAND,
		WP_M1GARAND,
		"",                 
		"",                  
		{0,0,0,0,0,0}
	},

	/*QUAKED weapon_m7 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mauser/mauser.md3"
*/
	{
		"weapon_m7",
		"sound/misc/w_pkup.wav",
		{
		"",
		"",
		""
		},

		"icons/iconw_m1_garand_1", 
		"m7", 
		200,
		IT_WEAPON,
		WP_M7,
		WP_M7,
		WP_M7,
		WP_M7,
		WP_M7,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_bar (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/bar/bar3rd.md3"
*/
	{
		"weapon_bar",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_bar",    
		"BAR",              
		30,
		IT_WEAPON,
		WP_BAR,
		WP_BAR,
		WP_BAR,
		WP_BAR,
		WP_BAR,
		"",                  
		"",                  
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_mp44 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mp44/mp44.md3"
*/
	{
		"weapon_mp44",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_mp44",    
		"MP44",             
		30,
		IT_WEAPON,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		"",                 
		"",                
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_mg42m (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/multiplayer/mg42/mg42_3rd.md3"
*/
	{
		"weapon_mg42m",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_mg42m",   
		"mg42m",             
		700,
		IT_WEAPON,
		WP_MG42M,
		WP_MG42M,
		WP_MG42M,
		WP_MG42M,
		WP_MG42M,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_browning (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/multiplayer/mg42/mg42_3rd.md3"
*/
	{
		"weapon_browning",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_browning",   
		"browning",             
		700,
		IT_WEAPON,
		WP_BROWNING,
		WP_BROWNING,
		WP_MG42M,
		WP_BROWNING,
		WP_BROWNING,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},

/*QUAKED weapon_m97 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/m97/m97_3rd.md3"
*/
	{
		"weapon_m97",
		"sound/misc/w_pkup.wav",
		{ 
		"",
		"",
		""
		},

			"icons/iconw_m97",  
			"m97",            
			700,
			IT_WEAPON,
			WP_M97,
			WP_M97,
			WP_M97,
			WP_M97,
			WP_M97,
			"",                      
			"",                     
			{ 0,0,0,0,0,0 }
	},


	/*QUAKED weapon_auto5 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/m97/m97_3rd.md3"
*/
	{
		"weapon_auto5",
		"sound/misc/w_pkup.wav",
		{ 
		"",
		"",
		""
		},

			"icons/iconw_auto5",  
			"auto5",            
			700,
			IT_WEAPON,
			WP_AUTO5,
			WP_AUTO5,
			WP_M97,
			WP_AUTO5,
			WP_AUTO5,
			"",                      
			"",                     
			{ 0,0,0,0,0,0 }
	},


/*QUAKED weapon_hdm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/p38/luger.md3"
*/
	{
		"weapon_hdm",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		"",
		},

		"icons/iconw_hdm",    
		"hdm",              
		30,
		IT_WEAPON,
		WP_HDM,
		WP_HDM,
		WP_HDM,
		WP_HDM,
		WP_HDM,
		"",                  
		"",                 
		{0,0,0,0,0,0}
	},

	/*QUAKED weapon_holycross (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/tesla/pu_tesla.md3"
*/
	{
		"weapon_holycross",
		"sound/weapons/holycross/holycross_pickup.wav",
		{   
		"",
		"",
		""
		},
		"icons/iconw_holycross_1",   
		"Holy Cross",             
		30,
		IT_WEAPON,
		WP_HOLYCROSS,
		WP_HOLYCROSS,
		WP_HOLYCROSS,
		WP_HOLYCROSS,
		WP_HOLYCROSS,
		"",                         
		"",                          
		{0,0,0,0,0,0}
	},

	/*QUAKED weapon_revolver (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/p38/luger.md3"
*/
	{
		"weapon_revolver",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_revolver",    
		"revolver",              
		30,
		IT_WEAPON,
		WP_REVOLVER,
		WP_REVOLVER,
		WP_REVOLVER,
		WP_REVOLVER,
		WP_REVOLVER,
		"",                 
		"",                  
		{0,0,0,0,0,0}
	},

	{
		"weapon_grenadesmoke",
		"sound/misc/w_pkup.wav",
		{  
		"",
		"",
		""
		},

		"icons/iconw_smokegrenade_1",    
		"smokeGrenade",              
		50,
		IT_WEAPON,
		WP_AIRSTRIKE,
		WP_AIRSTRIKE,
		WP_AIRSTRIKE,
		WP_AIRSTRIKE,
		WP_AIRSTRIKE,
		"",                      
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",            
		{0,0,0,0,0,0}
	},


	{
		"weapon_poison_med",
		"sound/misc/w_pkup.wav",
		{  
		"",
		"",
		""
		},

		"icons/iconw_smokegrenade_1",    
		"poison_med",              
		50,
		IT_WEAPON,
		WP_POISONGAS_MEDIC,
		WP_POISONGAS_MEDIC,
		WP_POISONGAS_MEDIC,
		WP_POISONGAS_MEDIC,
		WP_POISONGAS_MEDIC,
		"",                      
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",            
		{0,0,0,0,0,0}
	},


	//weapon_dynamite

	{
		"weapon_dynamite_eng",
		"sound/misc/w_pkup.wav",
		{   
		"",
		"",
		""
		},

		"icons/iconw_dynamite_1",    
		"Dynamite Weapon",       
		7,
		IT_WEAPON,
		WP_DYNAMITE_ENG,
		WP_DYNAMITE_ENG,
		WP_DYNAMITE_ENG,
		WP_DYNAMITE_ENG,
		WP_DYNAMITE_ENG,
		"",                      
		"",                     
		{0,0,0,0,0,0}
	},

/*
weapon_arty (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_arty",
		"sound/misc/w_pkup.wav",
		{   
		"models/multiplayer/syringe/syringe.md3",
		"models/multiplayer/syringe/v_syringe.md3",
		0
		},

		"icons/iconw_syringe_1",
		"Artillery",             
		50, // this should never be picked up
		IT_WEAPON,
		WP_ARTY,
		WP_ARTY,
		WP_ARTY,
		WP_ARTY,
		WP_ARTY,
		"",                     
		"sound/multiplayer/allies/a-firing.wav sound/multiplayer/axis/g-firing.wav sound/multiplayer/allies/a-art_abort.wav sound/multiplayer/axis/g-art_abort.wav", 
		{0,0,0,0,0,0}
	},

/* JPW NERVE
weapon_smoketrail -- only used as a special effects emitter for smoke trails (artillery spotter etc)
*/
	{
		"weapon_smoketrail",
		"sound/misc/w_pkup.wav",
		{   
		"models/multiplayer/smokegrenade/smokegrenade.md3",
		"models/multiplayer/smokegrenade/v_smokegrenade.md3",
		0
		},

		"icons/iconw_smokegrenade_1",   
		"smokeTrail",                
		50,
		IT_WEAPON,
		WP_SMOKETRAIL,
		WP_SMOKETRAIL,
		WP_SMOKETRAIL,
		WP_SMOKETRAIL,
		WP_SMOKETRAIL,
		"",                     
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",             
		{0,0,0,0,0,0}
	},

		{
		"weapon_poisongas",
		"sound/misc/w_pkup.wav",
		{
		"",
		"",
		""
		},
		"icons/iconw_poisongrenade_1",    
		"Poison Gas",
		0,
		IT_WEAPON,
		WP_POISONGAS,		
		WP_POISONGAS,
		WP_POISONGAS,
		WP_POISONGAS,
		WP_POISONGAS,
		"",                      
		"",                      
		{0,0,0,0,0,0}
	},


	//
	// AMMO ITEMS
	//

// RealRTCW ammo

	{
		"ammo_poison_gas",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/amgren_bag.md3",
		0, 
		0 
		},
		"",
		"Poison Gas",
		1,
		IT_AMMO,
		WP_NONE,
		WP_POISONGAS,
		WP_POISONGAS,
		WP_POISONGAS,
		WP_POISONGAS,
		"",                  
		"", 
		{5,4,3,2,2,3}                
	},


/*QUAKED ammo_m7 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/m7ammo_bag.md3"
*/
	{
		"ammo_m7",
		"sound/misc/w_pkup.wav",
		{ 
		"models/powerups/ammo/m7ammo_bag.md3",
		0,
		0
		},

		"icons/iconw_m1_garand_1",      
		"m7_ammo",               
		200,
		IT_AMMO,
		WP_NONE,
		WP_M7,
		WP_M7,
		WP_M7,
		WP_M7,
		"",                        
		"",                         
		{5,4,3,2,2,3}
	},

/*QUAKED ammo_holycross (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN

Boosts recharge on Tesla
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amcell.md3"
*/
	{
		"ammo_holyspirit",
		"sound/weapons/holycross/holycrossammo_pickup.wav",
		{ 
		"models/powerups/ammo/amholycross.md3",
		0,
		0  
		},
		"icons/icona_spirit",  
		"spirit",              
		5,
		IT_AMMO,
		WP_NONE,
		WP_HOLYCROSS,
		WP_HOLYCROSS,
		WP_HOLYCROSS,
		WP_HOLYCROSS,
		"",                  
		"",                  
		{10,10,10,10,10,10}
	},

/*QUAKED ammo_ttammo (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: TT33, PPSH

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/ttammo.md3"
*/
	{
		"ammo_ttammo",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/ttammo.md3",
		0,
		0 
		},

		"icons/iconw_luger_1", 
		"ttammo",           		
		60,
		IT_AMMO,
		WP_NONE,
		WP_PPSH,
		WP_TT33,
		WP_TT33,
		WP_PPSH,
		"",                  
		"",                  
		{71,71,50,50,50,50}	
	},

/*QUAKED ammo_ttammo_l (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: TT33, PPSH

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/ttammo.md3"
*/
	{
		"ammo_ttammo_l",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/ttammo_l.md3",
		0, 
		0
		},

		"icons/iconw_luger_1",
		"ttammol",         		
		60,
		IT_AMMO,
		WP_NONE,
		WP_PPSH,
		WP_TT33,
		WP_TT33,
		WP_PPSH,
		"",                  
		"",                  
		{142,142,100,100,100,100}	
	},

/*QUAKED ammo_mosina (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: mosin nagant

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/mosina.md3"
*/
	{
		"ammo_mosina",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/mosina.md3",
		0, 
		0 
		},

		"icons/icona_machinegun",   
		"mosina",			       
		50,
		IT_AMMO,
		WP_NONE,
		WP_MOSIN,
		WP_MOSIN,
		WP_MOSIN,
		WP_MOSIN,
		"",                          
		"",                          
		{20,20,15,15,15,20}		
	},

/*QUAKED ammo_barammo (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Bar, M1 Garand

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/barammo.md3"
*/
{
		"ammo_barammo",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/barammo.md3",
		0, 
		0
		},

		"icons/iconw_luger_1", 
		"barammo",           
		60,
		IT_AMMO,
		WP_NONE,
		WP_BAR,
		WP_M1GARAND,
		WP_M1GARAND,
		WP_BAR,
		"",                 
		"",                  
		{40,40,30,30,30,40}	
	},

/*QUAKED ammo_barammo_l (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Bar, M1 Garand

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/barammo_l.md3"
*/
{
		"ammo_barammo_l",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/barammo_l.md3",
		0,
		0
		},

		"icons/iconw_luger_1", 
		"barammol",           
		60,
		IT_AMMO,
		WP_NONE,
		WP_BAR,
		WP_M1GARAND,
		WP_M1GARAND,
		WP_BAR,
		"",                 
		"",                  
		{60,60,45,45,45,60}	
	},

/*QUAKED ammo_m1941ammo (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: M1941

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/barammo.md3"
*/
{
		"ammo_m1941",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/barammo.md3",
		0, 
		0
		},

		"icons/iconw_luger_1", 
		"ammo_m1941",           
		60,
		IT_AMMO,
		WP_NONE,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		WP_M1941,
		"",                 
		"",                  
		{40,40,30,30,30,40}	
	},

/*QUAKED ammo_hdmammo (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: HDM

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/barammo.md3"
*/
{
		"ammo_hdm",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/22cal.md3",
		0, 
		0
		},

		"icons/iconw_luger_1", 
		"ammo_hdm",           
		60,
		IT_AMMO,
		WP_NONE,
		WP_HDM,
		WP_HDM,
		WP_HDM,
		WP_HDM,
		"",                 
		"",                  
		{50,40,40,30,30,40}	
	},

/*QUAKED ammo_delisle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: HDM

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am45cal_m.md3"
*/
{
		"ammo_delisle",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am45cal_m.md3",
		0, 
		0
		},

		"icons/iconw_luger_1", 
		"ammo_delisle",           
		60,
		IT_AMMO,
		WP_NONE,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		WP_DELISLE,
		"",                 
		"",                  
		{30,30,30,20,20,30}	
	},

/*QUAKED ammo_44ammo (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: MP44

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/44ammo.md3"
*/
{
		"ammo_44ammo",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/44ammo.md3",
		0,
		0
		},

		"icons/iconw_luger_1",
		"44ammo",           		
		60,
		IT_AMMO,
		WP_NONE,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		"",                 
		"",                  
		{60,60,45,45,45,50}	
	},

/*QUAKED ammo_44ammo_l (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: MP44

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/44ammo_l.md3"
*/
{
		"ammo_44ammo_l",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/44ammo_l.md3",
		0,
		0
		},

		"icons/iconw_luger_1", 
		"44ammol",         		
		60,
		IT_AMMO,
		WP_NONE,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		WP_MP44,
		"",                  
		"",                 
		{90,90,75,75,75,75}	
	},

		/*QUAKED ammo_m97ammo (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
		used by: M97

		-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
		model="models/powerups/ammo/m97ammo.md3"
		*/
	{
		"ammo_m97ammo",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/m97ammo.md3",
		0, 
		0
		},

		"icons/iconw_luger_1", 
		"m97ammo",          		
		10,
		IT_AMMO,
		WP_NONE,
		WP_M97,
		WP_M97,
		WP_M97,
		WP_M97,
		"",                  
		"",                 
		{ 10,10,10,10,10,10 }
	},

			/*QUAKED ammo_revolver (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
		used by: revolver

		-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
		model="models/powerups/ammo/revolverammo.md3"
		*/
	{
		"ammo_revolver",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/revolverammo.md3",
		0, 
		0 
		},

		"icons/iconw_luger_1",
		"revolverammo",          	
		12,
		IT_AMMO,
		WP_NONE,
		WP_REVOLVER,
		WP_REVOLVER,
		WP_REVOLVER,
		WP_REVOLVER,
		"",                 
		"",                  
		{ 12,12,12,12,12,12 }
	},





/*QUAKED ammo_9mm_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Luger pistol, MP40 machinegun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am9mm_s.md3"
*/
	{
		"ammo_9mm_small",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am9mm_s.md3",
		0, 
		0 
		},

		"icons/iconw_luger_1", 
		"9mm Rounds",        
		30,
		IT_AMMO,
		WP_NONE,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                  
		"",                  
		{32,24,16,16,16,24}
	},
/*QUAKED ammo_9mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Luger pistol, MP40 machinegun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am9mm_m.md3"
*/
	{
		"ammo_9mm",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am9mm_m.md3",
		0, 
		0
		},

		"icons/iconw_luger_1", 
		"9mm",          
		60,
		IT_AMMO,
		WP_NONE,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                  
		"",                  
		{64,48,32,16,16,32}
	},

/*QUAKED ammo_9mm_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Luger pistol, MP40 machinegun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am9mm_l.md3"
*/
	{
		"ammo_9mm_large",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am9mm_l.md3",
		0, 
		0 
		},

		"icons/iconw_luger_1", 
		"9mm Box",           
		100,
		IT_AMMO,
		WP_NONE,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                
		"",                
		{96,64,48,48,48,48}
	},


/*QUAKED ammo_45cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Thompson, Colt

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am45cal_s.md3"
*/
	{
		"ammo_45cal_small",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am45cal_s.md3",
		0,
		0
		},

		"icons/iconw_luger_1", 
		".45cal Rounds", 
		20,
		IT_AMMO,
		WP_NONE,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                
		"",                  
		{40,30,20,20,20,30}
	},
/*QUAKED ammo_45cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Thompson, Colt

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am45cal_m.md3"
*/
	{
		"ammo_45cal",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am45cal_m.md3",
		0,
		0
		},

		"icons/iconw_luger_1", 
		".45cal",
		60,
		IT_AMMO,
		WP_NONE,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                
		"",                
		{60,45,30,30,30,45}
	},
/*QUAKED ammo_45cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Thompson, Colt

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am45cal_l.md3"
*/
	{
		"ammo_45cal_large",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am45cal_l.md3",
		0,
		0
		},
		"icons/iconw_luger_1", 
		".45cal Box",        
		100,
		IT_AMMO,
		WP_NONE,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                  
		"",                  
		{90,60,45,45,45,45}
	},




/*QUAKED ammo_792mm_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Mauser rifle, FG42

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am792mm_s.md3"
*/
	{
		"ammo_792mm_small",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am792mm_s.md3",
		0, 
		0
		},

		"icons/icona_machinegun",   
		"7.92mm Rounds",        
		50,
		IT_AMMO,
		WP_NONE,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                          
		"",                         
		{20,15,10,5,5,15}
	},

/*QUAKED ammo_792mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Mauser rifle, FG42

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am792mm_m.md3"
*/
	{
		"ammo_792mm",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am792mm_m.md3",
		0, 
		0
		},
		"icons/icona_machinegun",    
		"7.92mm",                
		10,
		IT_AMMO,
		WP_NONE,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                         
		"",                          
		{40,20,15,10,10,20}
	},

/*QUAKED ammo_792mm_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Mauser rifle, FG42

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am792mm_l.md3"
*/
	{
		"ammo_792mm_large",
		"sound/misc/am_pkup.wav",
		{
		"models/powerups/ammo/am792mm_l.md3",
		0, 
	    0
		},

		"icons/icona_machinegun",   
		"7.92mm Box",                
		50,
		IT_AMMO,
		WP_NONE,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                         
		"",                          
		{60,40,30,20,20,40}
	},

/*QUAKED ammo_30cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Garand rifle

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am30cal_s.md3"
*/
	{
		"ammo_30cal_small",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am30cal_s.md3",
		0, 
		0 
		},

		"icons/icona_machinegun",   
		".30cal Rounds",        
		50,
		IT_AMMO,
		WP_NONE,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                         
		"",                          
		{5,2,2,2,2,5}
	},

/*QUAKED ammo_30cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Garand rifle

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am30cal_m.md3"
*/
	{
		"ammo_30cal",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am30cal_m.md3",
		0, 
		0
		},

		"icons/icona_machinegun",    
		".30cal",               
		50,
		IT_AMMO,
		WP_NONE,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                        
		"",                          
		{5,5,5,5,5,5}
	},

/*QUAKED ammo_30cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Garand rifle

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am30cal_l.md3"
*/
	{
		"ammo_30cal_large",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am30cal_l.md3",
		0, 
		0
		},

		"icons/icona_machinegun",    
		".30cal Box",                
		50,
		IT_AMMO,
		WP_NONE,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                          
		"",                          
		{10,10,10,10,10,10}
	},

/*QUAKED ammo_127mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Venom gun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am127mm.md3"
*/
	{
		"ammo_127mm",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/am127mm.md3",
		0, 
		0 
		},

		"icons/icona_machinegun",    
		"12.7mm",                    
		100,
		IT_AMMO,
		WP_NONE,
		WP_MG42M,
		WP_MG42M,
		WP_MG42M,
		WP_MG42M,
		"",                         
		"",                        
		{100,100,100,100,100,100}
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amgren_bag.md3"
*/
	{
		"ammo_grenades",
		"sound/misc/am_pkup.wav",
		{
		"models/powerups/ammo/amgren_bag.md3",
		0, 
		0
		},

		"icons/icona_grenade",   
		"Grenades",             
		5,
		IT_AMMO,
		WP_NONE,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		"",                     
		"",                    
		{4,3,2,2,2,3}
	},

/*QUAKED ammo_grenades_american (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amgrenus_bag.md3"
*/
	{
		"ammo_grenades_american",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/amgrenus_bag.md3",
		0, 
		0
		},

		"icons/icona_pineapple", 
		"Pineapples",           
		5,
		IT_AMMO,
		WP_NONE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		"",                      
		"",                    
		{4,3,2,2,2,2}
	},

/*QUAKED ammo_dynamite (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN

 -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/dynamite.md3"
*/
	{
		"ammo_dynamite",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/dynamite.md3",
		0, 
		0
		},

		"icons/icona_dynamite",  
		"Dynamite",            
		1,
		IT_AMMO,
		WP_NONE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		"",                    
		"",                      
		{1,1,1,1,1,1}
	},


/*QUAKED ammo_cell (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Tesla

Boosts recharge on Tesla
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amcell.md3"
*/
	{
		"ammo_cell",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/amcell.md3",
		0, 
		0
		},

		"icons/icona_cell",  
		"Cell",              
		500,
		IT_AMMO,
		WP_NONE,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		"",                  
		"",                  
		{75,50,30,25,25,50}
	},



/*QUAKED ammo_fuel (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Flamethrower

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amfuel.md3"
*/
	{
		"ammo_fuel",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/amfuel.md3",
		0, 
		0
		},

		"icons/icona_fuel",  
		"Fuel",            
		100,
		IT_AMMO,
		WP_NONE,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		"",                 
		"",                  
		{100,75,50,50,50,50}
	},


/*QUAKED ammo_panzerfaust (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: German Panzerfaust

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/ampf.md3"
*/
	{
		"ammo_panzerfaust",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/ampf.md3",
		0, 
		0
		},

		"icons/icona_panzerfaust",   
		"Panzerfaust Rockets",               
		5,
		IT_AMMO,
		WP_NONE,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		"",                     
		"",                     
		{4,3,2,2,2,3}
	},


//----(SA)	hopefully it doesn't need to be a quaked thing.
//			apologies if it does and I'll put it back.
/*
ammo_monster_attack1 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
used by: Monster Attack 1 (specific to each monster)
*/
	{
		"ammo_monster_attack1",
		"",
		{ 
		"",
		0, 
		0
		},

		"",                      
		"MonsterAttack1",       
		60,
		IT_AMMO,
		WP_NONE,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		"",
		"",
		{0,0,0,0,0,0}
	},

// Those  entries below are needed for Survival ammo logic
{
		"luger_ammo",
		"sound/misc/am_pkup.wav",
		{ 
		"models/powerups/ammo/default.md3",
		0,
		0
		},

		"icons/iconw_default",
		"luger_ammo",           		
		60,
		IT_AMMO,
		WP_NONE,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                 
		"",                  
		{50,50,50,50,50,50}	
	},

{
	"silencer_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"silencer_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_SILENCER,
	WP_SILENCER,
	WP_SILENCER,
	WP_SILENCER,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"colt_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"colt_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_COLT,
	WP_COLT,
	WP_COLT,
	WP_COLT,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"tt33_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"tt33_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_TT33,
	WP_TT33,
	WP_TT33,
	WP_TT33,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"revolver_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"revolver_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_REVOLVER,
	WP_REVOLVER,
	WP_REVOLVER,
	WP_REVOLVER,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"hdm_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"hdm_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_HDM,
	WP_HDM,
	WP_HDM,
	WP_HDM,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"akimbo_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"akimbo_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_AKIMBO,
	WP_AKIMBO,
	WP_AKIMBO,
	WP_AKIMBO,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"dual_tt33_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"dual_tt33_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_DUAL_TT33,
	WP_DUAL_TT33,
	WP_DUAL_TT33,
	WP_DUAL_TT33,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"mp40_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"mp40_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_MP40,
	WP_MP40,
	WP_MP40,
	WP_MP40,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"thompson_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"thompson_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_THOMPSON,
	WP_THOMPSON,
	WP_THOMPSON,
	WP_THOMPSON,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"sten_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"sten_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_STEN,
	WP_STEN,
	WP_STEN,
	WP_STEN,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"ppsh_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"ppsh_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_PPSH,
	WP_PPSH,
	WP_PPSH,
	WP_PPSH,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"mp34_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"mp34_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_MP34,
	WP_MP34,
	WP_MP34,
	WP_MP34,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"mauser_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"mauser_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_MAUSER,
	WP_MAUSER,
	WP_MAUSER,
	WP_MAUSER,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"garand_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"garand_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_GARAND,
	WP_GARAND,
	WP_GARAND,
	WP_GARAND,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"mosin_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"mosin_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_MOSIN,
	WP_MOSIN,
	WP_MOSIN,
	WP_MOSIN,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"delisle_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"delisle_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_DELISLE,
	WP_DELISLE,
	WP_DELISLE,
	WP_DELISLE,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"m1garand_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"m1garand_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_M1GARAND,
	WP_M1GARAND,
	WP_M1GARAND,
	WP_M1GARAND,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"g43_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"g43_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_G43,
	WP_G43,
	WP_G43,
	WP_G43,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"m1941_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"m1941_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_M1941,
	WP_M1941,
	WP_M1941,
	WP_M1941,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"mp44_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"mp44_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_MP44,
	WP_MP44,
	WP_MP44,
	WP_MP44,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"fg42_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"fg42_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_FG42,
	WP_FG42,
	WP_FG42,
	WP_FG42,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"bar_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"bar_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_BAR,
	WP_BAR,
	WP_BAR,
	WP_BAR,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"m97_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"m97_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_M97,
	WP_M97,
	WP_M97,
	WP_M97,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"auto5_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"auto5_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_AUTO5,
	WP_AUTO5,
	WP_AUTO5,
	WP_AUTO5,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"browning_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"browning_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_BROWNING,
	WP_BROWNING,
	WP_BROWNING,
	WP_BROWNING,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"mg42m_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/default.md3", 0, 0 },
	"icons/iconw_default",
	"mg42m_ammo",
	60,
	IT_AMMO,
	WP_NONE,
	WP_MG42M,
	WP_MG42M,
	WP_MG42M,
	WP_MG42M,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"panzerfaust_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/rocket.md3", 0, 0 },
	"icons/iconw_panzerfaust",
	"panzerfaust_ammo",
	5,
	IT_AMMO,
	WP_NONE,
	WP_PANZERFAUST,
	WP_PANZERFAUST,
	WP_PANZERFAUST,
	WP_PANZERFAUST,
	"",
	"",
	{1,1,1,1,1,1}
},
{
	"flamethrower_fuel",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/fuel.md3", 0, 0 },
	"icons/iconw_flamethrower",
	"flamethrower_fuel",
	100,
	IT_AMMO,
	WP_NONE,
	WP_FLAMETHROWER,
	WP_FLAMETHROWER,
	WP_FLAMETHROWER,
	WP_FLAMETHROWER,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"venom_ammo",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/venom.md3", 0, 0 },
	"icons/iconw_venom",
	"venom_ammo",
	200,
	IT_AMMO,
	WP_NONE,
	WP_VENOM,
	WP_VENOM,
	WP_VENOM,
	WP_VENOM,
	"",
	"",
	{50,50,50,50,50,50}
},
{
	"tesla_energy",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/tesla.md3", 0, 0 },
	"icons/iconw_tesla",
	"tesla_energy",
	100,
	IT_AMMO,
	WP_NONE,
	WP_TESLA,
	WP_TESLA,
	WP_TESLA,
	WP_TESLA,
	"",
	"",
	{50,50,50,50,50,50}
},

/*QUAKED holdable_wine (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN

pickup sound : "sound/pickup/holdable/get_wine.wav"
use sound : "sound/pickup/holdable/use_wine.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/wine.md3"
*/
	{
		"holdable_wine",
		"sound/pickup/holdable/get_wine.wav",
		{
		"models/powerups/holdable/wine.md3",
		0, 
		0
		},

		"icons/wine",                    
		"1921 Chateau Lafite",           
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_WINE,
		0,
		0,
		0,
		"",                             
		"sound/pickup/holdable/use_wine.wav",       
		{3,0,0,0,0,0}
	},


/*QUAKED holdable_adrenaline(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_adrenaline.wav"
use sound : "sound/pickup/holdable/use_adrenaline.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/adrenaline.md3"
*/
	{
		"holdable_adrenaline",
		"sound/pickup/holdable/get_adrenaline.wav",
		{
		"models/powerups/holdable/adrenaline.md3",
		0, 
		0
		},

		"icons/adrenaline",            
		"Adrenaline used",             
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_ADRENALINE,
		0,
		0,
		0,
		"",                              
		"sound/pickup/holdable/use_adrenaline.wav", 
		{1,1,1,1,1,1}
	},

/*QUAKED holdable_eg_syringe(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
pickup sound : "sound/pickup/holdable/get_adrenaline.wav"
use sound : "sound/pickup/holdable/use_adrenaline.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/eg_syringe.md3"
*/
	{
		"holdable_eg_syringe",
		"sound/pickup/holdable/get_adrenaline.wav",
		{
		"models/powerups/holdable/eg_syringe.md3",
		0, 
		0
		},

		"icons/eg_syringe",            
		"EG Syringe used",             
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_EG_SYRINGE,
		0,
		0,
		0,
		"",                              
		"sound/pickup/holdable/use_adrenaline.wav", 
		{1,1,1,1,1,1}
	},


/*QUAKED holdable_bg_syringe(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
pickup sound : "sound/pickup/holdable/get_adrenaline.wav"
use sound : "sound/pickup/holdable/use_adrenaline.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bg_syringe.md3"
*/
	{
		"holdable_bg_syringe",
		"sound/pickup/holdable/get_adrenaline.wav",
		{
		"models/powerups/holdable/bg_syringe.md3",
		0, 
		0
		},

		"icons/bg_syringe",            
		"BG Syringe used",             
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_BG_SYRINGE,
		0,
		0,
		0,
		"",                              
		"sound/pickup/holdable/use_adrenaline.wav", 
		{1,1,1,1,1,1}
	},


/*QUAKED holdable_lp_syringe(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
pickup sound : "sound/pickup/holdable/get_adrenaline.wav"
use sound : "sound/pickup/holdable/use_adrenaline.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/lp_syringe.md3"
*/
	{
		"holdable_lp_syringe",
		"sound/pickup/holdable/get_adrenaline.wav",
		{
		"models/powerups/holdable/lp_syringe.md3",
		0, 
		0
		},

		"icons/lp_syringe",            
		"LP Syringe used",             
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_LP_SYRINGE,
		0,
		0,
		0,
		"",                              
		"sound/pickup/holdable/use_adrenaline.wav", 
		{1,1,1,1,1,1}
	},


/*QUAKED holdable_bandages(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"holdable_bandages",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/bandages",             
		"Bandages used",             
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_BANDAGES,
		0,
		0,
		0,
		"",                             
		"sound/pickup/holdable/use_bandages.wav",
		{1,1,1,1,1,1}
	},



/*QUAKED holdable_book1(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/venom_book.md3"
*/
	{
		"holdable_book1",
		"sound/pickup/holdable/get_book1.wav",
		{
		"models/powerups/holdable/venom_book.md3",
		0, 
		0
		},

		"icons/icon_vbook",              
		"Venom Tech Manual",     
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_BOOK1,
		0,
		0,
		0,
		"",                             
		"sound/pickup/holdable/use_book.wav",    
		{0,0,0,0,0,0}
	},


/*QUAKED holdable_book2(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/paranormal_book.md3"
*/
	{
		"holdable_book2",
		"sound/pickup/holdable/get_book2.wav",
		{
		"models/powerups/holdable/paranormal_book.md3",
		0, 
		0
		},

		"icons/icon_pbook",            
		"Project Book",                  
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_BOOK2,
		0,
		0,
		0,
		"",                             
		"sound/pickup/holdable/use_book.wav",  
		{0,0,0,0,0,0}
	},


/*QUAKED holdable_book3(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/zemphr_book.md3"
*/
	{
		"holdable_book3",
		"sound/pickup/holdable/get_book3.wav",
		{
		"models/powerups/holdable/zemphr_book.md3",
		0, 
		0
		},

		"icons/icon_zbook",              
		"Dr. Zemph's Journal",      
		1,
		IT_HOLDABLE,
		WP_NONE,
		HI_BOOK3,
		0,
		0,
		0,
		"",                            
		"sound/pickup/holdable/use_book.wav",    
		{0,0,0,0,0,0}
	},

	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_quad",
		"sound/misc/powerup_quad.wav",
		{
		"models/powerups/survival/thule_b.md3",
		0, 
		0
		},
		"",              
		"Veil Empower",       
		30,
		IT_POWERUP,
		WP_NONE,
		PW_QUAD,
		0,
		0,
		0,
		"",                            
		"sound/items/damage2.wav sound/items/damage3.wav",   
		{0,0,0,0,0,0}
	},

/*QUAKED item_vampire (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_vampire",
		"sound/misc/powerup_vampirism.wav",
		{
		"models/powerups/survival/thule_r.md3",
		0, 
		0
		},
		"",              
		"Veil Essence Reaver",       
		30,
		IT_POWERUP,
		WP_NONE,
		PW_VAMPIRE,
		0,
		0,
		0,
		"",                            
		"",   
		{0,0,0,0,0,0}
	},

/*QUAKED item_ammopw (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_ammopw",
		"sound/misc/powerup_resupply.wav",
		{
		"models/powerups/survival/thule_gr.md3",
		0, 
		0
		},
		"",              
		"Veil Ressuply",       
		1,
		IT_POWERUP,
		WP_NONE,
		PW_AMMO,
		0,
		0,
		0,
		"",                            
		"",   
		{0,0,0,0,0,0}
	},

	/*QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_haste",
		"sound/misc/powerup_pickup.wav",
		{
		"models/powerups/instant/haste.md3",
		0, 
		0
		},
		"",                    
		"Haste",     
		30,
		IT_POWERUP,
		WP_NONE,
		PW_HASTE,
		0,
		0,
		0,
		"",                          
		"",   
		{0,0,0,0,0,0}
	},

	/*QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_haste_surv",
		"sound/misc/powerup_pickup.wav",
		{
		"models/powerups/instant/haste.md3",
		0, 
		0
		},
		"",                    
		"Haste",     
		30,
		IT_POWERUP,
		WP_NONE,
		PW_HASTE_SURV,
		0,
		0,
		0,
		"",                          
		"",   
		{0,0,0,0,0,0}
	},



		/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_enviro",
		"sound/misc/powerup_pickup.wav",
		{
		"models/powerups/instant/enviro.md3",
		0, 
		0
		},
		"",                             
		"Battle Suit",     
		30,
		IT_POWERUP,
		WP_NONE,
		PW_BATTLESUIT,
		0,
		0,
		0,
		"",                          
		"sound/items/airout.wav sound/items/protect3.wav",   
		{0,0,0,0,0,0}
	},


		/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_enviro_surv",
		"sound/misc/powerup_shield.wav",
		{
		"models/powerups/survival/thule_g.md3",
		0, 
		0
		},
		"",                             
		"Veil Shield",     
		30,
		IT_POWERUP,
		WP_NONE,
		PW_BATTLESUIT_SURV,
		0,
		0,
		0,
		"",                          
		"sound/items/airout.wav sound/items/protect3.wav",   
		{0,0,0,0,0,0}
	},



/*QUAKED item_invis (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
		{
		"item_invis",
		"sound/misc/powerup_invis.wav",
		{
		"models/powerups/instant/invis.md3",
		0, 
		0
		},
		"",                               
		"Invisibility",     
		30,
		IT_POWERUP,
		WP_NONE,
		PW_INVIS,
		0,
		0,
		0,
		"",                          
		"",   
		{0,0,0,0,0,0}
	},


/*QUAKED perk_resilience(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_resilience",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_regen",             
		"Resilience",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_RESILIENCE,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0,0,0}
	},

/*QUAKED perk_scavenger(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_scavenger",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_scavenger",             
		"Scavenger",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_SCAVENGER,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0,0,0}
	},


/*QUAKED perk_runner(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_runner",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_runner",             
		"Runner",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_RUNNER,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0,0,0}
	},


/*QUAKED perk_weaponhandling(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_weaponhandling",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_weaponhandling",             
		"Weapon Handling",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_WEAPONHANDLING,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0,0,0}
	},


/*QUAKED perk_rifling(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_rifling",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_rifling",             
		"Advanced Rifling",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_RIFLING,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0,0,0}
	},


/*QUAKED perk_secondchance(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_bandages.wav"
use sound : "sound/pickup/holdable/use_bandages.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/bandages.md3"
*/
	{
		"perk_secondchance",
		"sound/pickup/holdable/get_bandages.wav",
		{
		"models/powerups/holdable/bandages.md3",
		0, 
		0
		},

		"icons/perk_secondchance",             
		"Second Chance",             
		1,
		IT_PERK,
		WP_NONE,
		PERK_SECONDCHANCE,
		0,
		0,
		0,
		"",                             
		"",
		{0,0,0,0,0,0}
	},

/*QUAKED key_binocs (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Binoculars.

pickup sound : "sound/pickup/keys/binocs.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/binoculars.md3"
*/
	{
		"key_binocs",
		"sound/pickup/keys/binocs.wav",
		{
		"models/powerups/keys/binoculars.md3",
		0, 
		0
		},

		"icons/binocs",          
		"Binoculars",           
		0,
		IT_KEY,
		WP_NONE,
		INV_BINOCS,
		0,
		0,
		0,
		"",                      
		"models/keys/key.wav",
		{0,0,0,0,0,0}
	},


/*
weapon_magicammo (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_magicammo",
		"sound/misc/w_pkup.wav",
		{
			"models/multiplayer/ammopack/ammopack.md3",
			"models/multiplayer/ammopack/v_ammopack.md3",
			"models/multiplayer/ammopack/ammopack_pickup.md3"
		},

		"icons/iconw_ammopack_1",    // icon
		"Ammo Pack",             // pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_NONE,
		WP_NONE,
		WP_NONE,
		WP_NONE,
		WP_NONE,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0,0,0}
	},

	// end of list marker
	{NULL}
};
// END JOSEPH

int	bg_numItems = ARRAY_LEN( bg_itemlist ) - 1;


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t *BG_FindItemForPowerup( powerup_t pw ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( ( bg_itemlist[i].giType == IT_POWERUP ||
			   bg_itemlist[i].giType == IT_TEAM ) &&
			 bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t *BG_FindItemForHoldable( holdable_t pw ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

//	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}


/*
==============
BG_FindItemForPerk
==============
*/
gitem_t *BG_FindItemForPerk( perk_t perk ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_PERK && bg_itemlist[i].giTag == perk ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}

/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t *BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static gitem_t  *lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;

	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}

	if ( weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindItemForWeapon: weapon out of range %i", weapon );
	}

	if ( !lookupTable[weapon] ) {
		Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}

//----(SA) added

#define DEATHMATCH_SHARED_AMMO 0


/*
==============
BG_FindClipForWeapon
==============
*/
weapon_t BG_FindClipForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static weapon_t lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;

	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it->giClipIndex;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}

	if ( weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindClipForWeapon: weapon out of range %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}



/*
==============
BG_FindAmmoForWeapon
==============
*/
weapon_t BG_FindAmmoForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static weapon_t lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;
	qboolean survival = qfalse;

    #ifdef GAMEDLL
	    if (g_gametype.integer == GT_SURVIVAL)
    #endif
    #ifdef CGAMEDLL
		if (cg_gameType.integer == GT_SURVIVAL)
    #endif
			survival = qtrue;

    if (survival) {
	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it->giAmmoIndexSurv;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}
	} else {
	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it->giAmmoIndex;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}
	}

	if ( weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindAmmoForWeapon: weapon out of range %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}

/*
==============
BG_AkimboFireSequence
	returns 'true' if it's the left hand's turn to fire, 'false' if it's the right hand's turn
==============
*/
//qboolean BG_AkimboFireSequence( playerState_t *ps ) {
qboolean BG_AkimboFireSequence( int weapon, int akimboClip, int coltClip ) {
	// NOTE: this doesn't work when clips are turned off (dmflags 64)

	if ( weapon != WP_AKIMBO && weapon != WP_DUAL_TT33 ) {
		return qfalse;
	}

	if ( !akimboClip ) {
		return qfalse;
	}

	// no ammo in colt, must be akimbo turn
	if ( !coltClip ) {
		return qtrue;
	}

	// at this point, both have ammo

	// now check 'cycle'   // (removed old method 11/5/2001)
	if ( ( akimboClip + coltClip ) & 1 ) {
		return qfalse;
	}

	return qtrue;
}

//----(SA) end

//----(SA) Added keys
/*
==============
BG_FindItemForKey
==============
*/
gitem_t *BG_FindItemForKey( wkey_t k, int *indexreturn ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_KEY && bg_itemlist[i].giTag == k ) {
			{
				if ( indexreturn ) {
					*indexreturn = i;
				}
				return &bg_itemlist[i];
			}
		}
	}

	Com_Error( ERR_DROP, "Key %d not found", k );
	return NULL;
}
//----(SA) end


//----(SA) added
/*
==============
BG_FindItemForAmmo
==============
*/
gitem_t *BG_FindItemForAmmo( int ammo ) {
	int i = 0;

	for (; i < bg_numItems; i++ )
	{
		if ( bg_itemlist[i].giType == IT_AMMO && bg_itemlist[i].giAmmoIndex == ammo ) {
			return &bg_itemlist[i];
		}
	}
	Com_Error( ERR_DROP, "Item not found for ammo: %d", ammo );
	return NULL;
}
//----(SA) end


/*
===============
BG_FindItem

===============
*/
gitem_t *BG_FindItem( const char *pickupName ) {
	gitem_t *it;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, pickupName ) ) {
			return it;
		}
	}

	return NULL;
}


gitem_t *BG_FindItemForClassName( const char *className ) {
	gitem_t *it;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->classname, className ) ) {
			return it;
		}
	}

	return NULL;
}

/*
==============
BG_FindItem2
	also check classname
==============
*/
gitem_t *BG_FindItem2( const char *name ) {
	gitem_t *it;
	char *name2;

	name2 = (char*)name;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, name ) ) {
			return it;
		}

		if ( !Q_strcasecmp( it->classname, name2 ) ) {
			return it;
		}
	}

	Com_Printf( "BG_FindItem2(): unable to locate item '%s'\n", name );

	return NULL;
}

//----(SA)	added
/*
==============
BG_PlayerSeesItem
	Try to quickly determine if an item should be highlighted as per the current cg_drawCrosshairPickups.integer value.
	pvs check should have already been done by the time we get in here, so we shouldn't have to check
==============
*/

//----(SA)	not used
/*
qboolean BG_PlayerSeesItem(playerState_t *ps, entityState_t *item, int atTime)
{
   vec3_t	vorigin, eorigin, viewa, dir;
   float	dot, dist, foo;

   BG_EvaluateTrajectory( &item->pos, atTime, eorigin );

   VectorCopy(ps->origin, vorigin);
   vorigin[2] += ps->viewheight;			// get the view loc up to the viewheight
   eorigin[2] += 16;						// and subtract the item's offset (that is used to place it on the ground)
   VectorSubtract(vorigin, eorigin, dir);

   dist = VectorNormalize(dir);			// dir is now the direction from the item to the player

   if(dist > 255)
	   return qfalse;						// only run the remaining stuff on items that are close enough

   // (SA) FIXME: do this without AngleVectors.
   //		It'd be nice if the angle vectors for the player
   //		have already been figured at this point and I can
   //		just pick them up.  (if anybody is storing this somewhere,
   //		for the current frame please let me know so I don't
   //		have to do redundant calcs)
   AngleVectors(ps->viewangles, viewa, 0, 0);
   dot = DotProduct(viewa, dir );

   // give more range based on distance (the hit area is wider when closer)

   foo = -0.94f - (dist/255.0f) * 0.057f;	// (ranging from -0.94 to -0.997) (it happened to be a pretty good range)

//	Com_Printf("test: if(%f > %f) return qfalse (dot > foo)\n", dot, foo);
   if(dot > foo)
	   return qfalse;

   return qtrue;
}
*/
//----(SA)	end


/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/

extern int trap_Cvar_VariableIntegerValue( const char *var_name );

qboolean    BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		 || ps->origin[0] - origin[0] < -50
		 || ps->origin[1] - origin[1] > 36
		 || ps->origin[1] - origin[1] < -36
		 || ps->origin[2] - origin[2] > 36
		 || ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}

/*
=================================
BG_AddMagicAmmo:
	if numOfClips is 0, no ammo is added, it just return whether any ammo CAN be added;
	otherwise return whether any ammo was ACTUALLY added.

WARNING: when numOfClips is 0, DO NOT CHANGE ANYTHING under ps.
=================================
*/
// Gordon: setting numOfClips = 0 allows you to check if the client needs ammo, but doesnt give any
qboolean BG_AddMagicAmmo(playerState_t *ps, int numOfClips) {
	int i, weapon;
	qboolean ammoAdded = qfalse;

	for (i = 0; reloadableWeapons[i] >= 0; i++) {
		weapon = reloadableWeapons[i];
		if (!COM_BitCheck(ps->weapons, weapon))
			continue;

		int ammoIndex = BG_FindAmmoForWeapon(weapon);
		int maxammo   = BG_GetMaxAmmo(ps, weapon, 1.5f);
		int maxclip   = BG_GetMaxClip(ps, weapon);

		if (weapon == WP_FLAMETHROWER || weapon == WP_TESLA) {
			if (ps->ammoclip[ammoIndex] < maxammo) {
				if (!numOfClips)
					return qtrue;

				ps->ammoclip[ammoIndex] = maxammo;
				ammoAdded = qtrue;
			}
		} else {
			if (ps->ammo[ammoIndex] < maxammo) {
				if (!numOfClips)
					return qtrue;

				int clipsToAdd = (weapon == WP_AKIMBO || weapon == WP_DUAL_TT33)
					? numOfClips * 2
					: numOfClips;

				ps->ammo[ammoIndex] += clipsToAdd * maxclip;

				if (ps->ammo[ammoIndex] > maxammo) {
					ps->ammo[ammoIndex] = maxammo;
				}

				ammoAdded = qtrue;
			}
		}
	}

	return ammoAdded;
}



#define AMMOFORWEAP BG_FindAmmoForWeapon( item->giTag )
/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/

qboolean isClipOnly( int weap ) {
	switch ( weap ) {
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_TESLA:
	case WP_FLAMETHROWER:
    case WP_HOLYCROSS:
		return qtrue;
	}
	return qfalse;
}


qboolean    BG_CanItemBeGrabbed( const entityState_t *ent, const playerState_t *ps ) {
	gitem_t *item;
	int ammoweap;
	qboolean multiplayer = qfalse;

	if (ent->modelindex < 1 || ent->modelindex >= bg_numItems)
	{
		Com_Error(ERR_DROP, "BG_CanItemBeGrabbed: index out of range");
	}

	item = &bg_itemlist[ent->modelindex];

	switch ( item->giType ) {

	case IT_WEAPON:
		if (multiplayer)
		{
			if ((ps->stats[STAT_PLAYER_CLASS] == PC_MEDIC) || (ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER))
			{
				if (!COM_BitCheck(ps->weapons, item->giTag))
				{
					return qfalse;
				}
			}
		}
		else
		{
			if (COM_BitCheck(ps->weapons, item->giTag))
			{
				if (isClipOnly(item->giTag))
				{
					int maxclip = BG_GetMaxClip(ps, item->giTag);

					if (ps->ammoclip[item->giAmmoIndex] >= maxclip)
					{
						return qfalse;
					}
				}
				else
				{
					int maxammo = BG_GetMaxAmmo(ps, item->giTag, 1.5f);

					if (ps->ammo[item->giAmmoIndex] >= maxammo)
					{
						return qfalse;
					}
				}
			}
		}
		return qtrue;

	case IT_AMMO:
		ammoweap = BG_FindAmmoForWeapon(item->giTag);

		if (isClipOnly(ammoweap))
		{
			int maxclip = BG_GetMaxClip(ps, ammoweap);

			if (ps->ammoclip[ammoweap] >= maxclip)
			{
				return qfalse;
			}
		}

		int maxammo = BG_GetMaxAmmo(ps, ammoweap, 1.5f);
		if (ps->ammo[ammoweap] >= maxammo)
		{
			return qfalse;
		}

		return qtrue;
	case IT_ARMOR:
		// we also clamp armor to the maxhealth for handicapping
//			if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
		if ( ps->stats[STAT_ARMOR] >= 100 ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		if ( ent->density == ( 1 << 9 ) ) { // density tracks how many uses left
			return qfalse;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
		if ( ent->density == ( 1 << 9 ) ) { // density tracks how many uses left
			return qfalse;
		}

		if ( ps->powerups[PW_NOFATIGUE] == 60000 ) { // full
			return qfalse;
		}

		return qtrue;

	case IT_TEAM:     // team items, such as flags

		// DHM - Nerve :: otherEntity2 is now used instead of modelindex2
		// ent->modelindex2 is non-zero on items if they are dropped
		// we need to know this because we can pick up our dropped flag (and return it)
		// but we can't pick up our flag at base
		if ( ps->persistant[PERS_TEAM] == TEAM_RED ) {
			if ( item->giTag == PW_BLUEFLAG ||
				 ( item->giTag == PW_REDFLAG && ent->otherEntityNum2 /*ent->modelindex2*/ ) ||
				 ( item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG] ) ) {
				return qtrue;
			}
		} else if ( ps->persistant[PERS_TEAM] == TEAM_BLUE ) {
			if ( item->giTag == PW_REDFLAG ||
				 ( item->giTag == PW_BLUEFLAG && ent->otherEntityNum2 /*ent->modelindex2*/ ) ||
				 ( item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG] ) ) {
				return qtrue;
			}
		}
		return qfalse;


	case IT_HOLDABLE:
		return qtrue;

	case IT_TREASURE:       // treasure always picked up
		return qtrue;

	case IT_CLIPBOARD:      // clipboards always picked up
		return qtrue;

		//---- (SA) Wolf keys
	case IT_KEY:
		return qtrue;       // keys are always picked up

	case IT_BAD:
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
	default:
#ifndef Q3_VM
#ifndef NDEBUG
          Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
#endif
         break;
	}

	return qfalse;
}

//======================================================================

void BG_CalculateSpline_r( splinePath_t* spline, vec3_t out1, vec3_t out2, float tension ) {
	vec3_t points[18];
	int i;
	int count = spline->numControls + 2;
	vec3_t dist;

	VectorCopy( spline->point.origin, points[0] );
	for ( i = 0; i < spline->numControls; i++ ) {
		VectorCopy( spline->controls[i].origin, points[i + 1] );
	}
	if ( !spline->next ) {
		return;
//		Com_Error( ERR_DROP, "Spline (%s) with no target referenced", spline->point.name );
	}
	VectorCopy( spline->next->point.origin, points[i + 1] );


	while ( count > 2 ) {
		for ( i = 0; i < count - 1; i++ ) {
			VectorSubtract( points[i + 1], points[i], dist );
			VectorMA( points[i], tension, dist, points[i] );
		}
		count--;
	}

	VectorCopy( points[0], out1 );
	VectorCopy( points[1], out2 );
}

qboolean BG_TraverseSpline( float* deltaTime, splinePath_t** pSpline ) {
	float dist;

	while ( ( *deltaTime ) > 1 ) {
		( *deltaTime ) -= 1;
		dist = ( *pSpline )->length * ( *deltaTime );

		if ( !( *pSpline )->next || !( *pSpline )->next->length ) {
			return qfalse;
//			Com_Error( ERR_DROP, "Spline path end passed (%s)", (*pSpline)->point.name );
		}

		( *pSpline ) = ( *pSpline )->next;
		*deltaTime = dist / ( *pSpline )->length;
	}

	while ( ( *deltaTime ) < 0 ) {
		dist = -( ( *pSpline )->length * ( *deltaTime ) );

		if ( !( *pSpline )->prev || !( *pSpline )->prev->length ) {
			return qfalse;
//			Com_Error( ERR_DROP, "Spline path end passed (%s)", (*pSpline)->point.name );
		}

		( *pSpline ) = ( *pSpline )->prev;
		( *deltaTime ) = 1 - ( dist / ( *pSpline )->length );
	}

	return qtrue;
}

/*
================
BG_RaySphereIntersection

================
*/

qboolean BG_RaySphereIntersection( float radius, vec3_t origin, splineSegment_t* path, float *t0, float *t1 ) {
	vec3_t v;
	float b, c, d;

	VectorSubtract( path->start, origin, v );

	b = 2 * DotProduct( v, path->v_norm );
	c = DotProduct( v, v ) - ( radius * radius );

	d = ( b * b ) - ( 4 * c );
	if ( d < 0 ) {
		return qfalse;
	}
	d = sqrt( d );

	*t0 = ( -b + d ) * 0.5f;
	*t1 = ( -b - d ) * 0.5f;

	return qtrue;
}

void BG_LinearPathOrigin2( float radius, splinePath_t** pSpline, float *deltaTime, vec3_t result, qboolean backwards ) {
	qboolean first = qtrue;
	float t = 0.f;
	int i = floor( ( *deltaTime ) * ( MAX_SPLINE_SEGMENTS ) );
	float frac;
//	int x = 0;
//	splinePath_t* start = *pSpline;

	if ( i >= MAX_SPLINE_SEGMENTS ) {
		i = MAX_SPLINE_SEGMENTS - 1;
		frac = 1.f;
	} else {
		frac = ( ( ( *deltaTime ) * ( MAX_SPLINE_SEGMENTS ) ) - i );
	}

	while ( qtrue ) {
		float t0, t1;

		while ( qtrue ) {
			if ( BG_RaySphereIntersection( radius, result, &( *pSpline )->segments[i], &t0, &t1 ) ) {
				qboolean found = qfalse;

				t0 /= ( *pSpline )->segments[i].length;
				t1 /= ( *pSpline )->segments[i].length;

				if ( first ) {
					if ( radius < 0 ) {
						if ( t0 < frac && ( t0 >= 0.f && t0 <= 1.f ) ) {
							t = t0;
							found = qtrue;
						} else if ( t1 < frac ) {
							t = t1;
							found = qtrue;
						}
					} else {
						if ( t0 > frac && ( t0 >= 0.f && t0 <= 1.f ) ) {
							t = t0;
							found = qtrue;
						} else if ( t1 > frac ) {
							t = t1;
							found = qtrue;
						}
					}
				} else {
					if ( radius < 0 ) {
						if ( t0 < t1 && ( t0 >= 0.f && t0 <= 1.f ) ) {
							t = t0;
							found = qtrue;
						} else {
							t = t1;
							found = qtrue;
						}
					} else {
						if ( t0 > t1 && ( t0 >= 0.f && t0 <= 1.f ) ) {
							t = t0;
							found = qtrue;
						} else {
							t = t1;
							found = qtrue;
						}
					}
				}

				if ( found ) {
					if ( t >= 0.f && t <= 1.f ) {
						*deltaTime = ( i / (float)( MAX_SPLINE_SEGMENTS ) ) + ( t / (float)( MAX_SPLINE_SEGMENTS ) );
						VectorMA( ( *pSpline )->segments[i].start, t * ( *pSpline )->segments[i].length, ( *pSpline )->segments[i].v_norm, result );
						return;
					}
				}
				found = qfalse;
			}

			first = qfalse;
			if ( radius < 0 ) {
				i--;
				if ( i < 0 ) {
					i = MAX_SPLINE_SEGMENTS - 1;
					break;
				}
			} else {
				i++;
				if ( i >= MAX_SPLINE_SEGMENTS ) {
					i = 0;
					break;
				}
			}
		}

		if ( radius < 0 ) {
			if ( !( *pSpline )->prev ) {
				return;
//				Com_Error( ERR_DROP, "End of spline reached (%s)\n", start->point.name );
			}
			*pSpline = ( *pSpline )->prev;
		} else {
			if ( !( *pSpline )->next ) {
				return;
//				Com_Error( ERR_DROP, "End of spline reached (%s)\n", start->point.name );
			}
			*pSpline = ( *pSpline )->next;
		}
	}
}

void BG_ComputeSegments( splinePath_t* pSpline ) {
	int i;
	float granularity = 1 / ( (float)( MAX_SPLINE_SEGMENTS ) );
	vec3_t vec[4];

	for ( i = 0; i < MAX_SPLINE_SEGMENTS; i++ ) {
		BG_CalculateSpline_r( pSpline, vec[0], vec[1], i * granularity );
		VectorSubtract( vec[1], vec[0], pSpline->segments[i].start );
		VectorMA( vec[0], i * granularity, pSpline->segments[i].start, pSpline->segments[i].start );

		BG_CalculateSpline_r( pSpline, vec[2], vec[3], ( i + 1 ) * granularity );
		VectorSubtract( vec[3], vec[2], vec[0] );
		VectorMA( vec[2], ( i + 1 ) * granularity, vec[0], vec[0] );

		VectorSubtract( vec[0], pSpline->segments[i].start, pSpline->segments[i].v_norm );
		pSpline->segments[i].length = VectorLength( pSpline->segments[i].v_norm );
		VectorNormalize( pSpline->segments[i].v_norm );
	}
}

/*
================
BG_EvaluateTrajectoryET

================
*/
void BG_EvaluateTrajectoryET( const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splinePath ) {
	float deltaTime;
	float phase;
	vec3_t v;

	splinePath_t* pSpline;
	vec3_t vec[2];
	qboolean backwards = qfalse;
	float deltaTime2;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
	case TR_GRAVITY_PAUSED: //----(SA)
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
//----(SA)	removed
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
		// Ridah
	case TR_GRAVITY_LOW:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * ( DEFAULT_GRAVITY * 0.3 ) * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
		// done.
//----(SA)
	case TR_GRAVITY_FLOAT:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * ( DEFAULT_GRAVITY * 0.2 ) * deltaTime;
		break;
//----(SA)	end
		// RF, acceleration
	case TR_ACCELERATE:     // trDelta is the ultimate speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		// phase is the acceleration constant
		phase = VectorLength( tr->trDelta ) / ( tr->trDuration * 0.001 );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time
		VectorMA( tr->trBase, phase * 0.5 * deltaTime * deltaTime, result, result );
		break;
	case TR_DECCELERATE:    // trDelta is the starting speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		// phase is the breaking constant
		phase = VectorLength( tr->trDelta ) / ( tr->trDuration * 0.001 );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time (without breaking)
		VectorMA( tr->trBase, deltaTime, tr->trDelta, v );
		// subtract breaking force
		VectorMA( v, -phase * 0.5 * deltaTime * deltaTime, result, result );
		break;
	case TR_SPLINE:
		if ( !( pSpline = BG_GetSplineData( splinePath, &backwards ) ) ) {
			return;
		}

		deltaTime = tr->trDuration ? ( atTime - tr->trTime ) / ( (float)tr->trDuration ) : 0;

		if ( deltaTime < 0.f ) {
			deltaTime = 0.f;
		} else if ( deltaTime > 1.f ) {
			deltaTime = 1.f;
		}

		if ( backwards ) {
			deltaTime = 1 - deltaTime;
		}

/*		if(pSpline->isStart) {
			deltaTime = 1 - sin((1 - deltaTime) * M_PI * 0.5f);
		} else if(pSpline->isEnd) {
			deltaTime = sin(deltaTime * M_PI * 0.5f);
		}*/

		deltaTime2 = deltaTime;

		BG_CalculateSpline_r( pSpline, vec[0], vec[1], deltaTime );

		if ( isAngle ) {
			qboolean dampin = qfalse;
			qboolean dampout = qfalse;
			float base1;

			if ( tr->trBase[0] ) {
//				int pos = 0;
				vec3_t result2;
				splinePath_t* pSp2 = pSpline;

				deltaTime2 += tr->trBase[0] / pSpline->length;

				if ( BG_TraverseSpline( &deltaTime2, &pSp2 ) ) {

					VectorSubtract( vec[1], vec[0], result );
					VectorMA( vec[0], deltaTime, result, result );

					BG_CalculateSpline_r( pSp2, vec[0], vec[1], deltaTime2 );

					VectorSubtract( vec[1], vec[0], result2 );
					VectorMA( vec[0], deltaTime2, result2, result2 );

					if ( tr->trBase[0] < 0 ) {
						VectorSubtract( result, result2, result );
					} else {
						VectorSubtract( result2, result, result );
					}
				} else {
					VectorSubtract( vec[1], vec[0], result );
				}
			} else {
				VectorSubtract( vec[1], vec[0], result );
			}

			vectoangles( result, result );

			base1 = tr->trBase[1];
			if ( base1 >= 10000 || base1 < -10000 ) {
				dampin = qtrue;
				if ( base1 < 0 ) {
					base1 += 10000;
				} else {
					base1 -= 10000;
				}
			}

			if ( base1 >= 1000 || base1 < -1000 ) {
				dampout = qtrue;
				if ( base1 < 0 ) {
					base1 += 1000;
				} else {
					base1 -= 1000;
				}
			}

			if ( dampin && dampout ) {
				result[ROLL] = base1 + ( ( sin( ( ( deltaTime * 2 ) - 1 ) * M_PI * 0.5f ) + 1 ) * 0.5f * tr->trBase[2] );
			} else if ( dampin ) {
				result[ROLL] = base1 + ( sin( deltaTime * M_PI * 0.5f ) * tr->trBase[2] );
			} else if ( dampout ) {
				result[ROLL] = base1 + ( ( 1 - sin( ( 1 - deltaTime ) * M_PI * 0.5f ) ) * tr->trBase[2] );
			} else {
				result[ROLL] = base1 + ( deltaTime * tr->trBase[2] );
			}
		} else {
			VectorSubtract( vec[1], vec[0], result );
			VectorMA( vec[0], deltaTime, result, result );
		}

		break;
	case TR_LINEAR_PATH:
		if ( !( pSpline = BG_GetSplineData( splinePath, &backwards ) ) ) {
			return;
		}

		deltaTime = tr->trDuration ? ( atTime - tr->trTime ) / ( (float)tr->trDuration ) : 0;

		if ( deltaTime < 0.f ) {
			deltaTime = 0.f;
		} else if ( deltaTime > 1.f ) {
			deltaTime = 1.f;
		}

		if ( backwards ) {
			deltaTime = 1 - deltaTime;
		}

		if ( isAngle ) {
			int pos = floor( deltaTime * ( MAX_SPLINE_SEGMENTS ) );
			float frac;

			if ( pos >= MAX_SPLINE_SEGMENTS ) {
				pos = MAX_SPLINE_SEGMENTS - 1;
				frac = pSpline->segments[pos].length;
			} else {
				frac = ( ( deltaTime * ( MAX_SPLINE_SEGMENTS ) ) - pos ) * pSpline->segments[pos].length;
			}

			if ( tr->trBase[0] ) {
				VectorMA( pSpline->segments[pos].start, frac, pSpline->segments[pos].v_norm, result );
				VectorCopy( result, v );

				BG_LinearPathOrigin2( tr->trBase[0], &pSpline, &deltaTime, v, backwards );
				if ( tr->trBase[0] < 0 ) {
					VectorSubtract( v, result, result );
				} else {
					VectorSubtract( result, v, result );
				}

				vectoangles( result, result );
			} else {
				vectoangles( pSpline->segments[pos].v_norm, result );
			}

		} else {
			int pos = floor( deltaTime * ( MAX_SPLINE_SEGMENTS ) );
			float frac;

			if ( pos >= MAX_SPLINE_SEGMENTS ) {
				pos = MAX_SPLINE_SEGMENTS - 1;
				frac = pSpline->segments[pos].length;
			} else {
				frac = ( ( deltaTime * ( MAX_SPLINE_SEGMENTS ) ) - pos ) * pSpline->segments[pos].length;
			}

			VectorMA( pSpline->segments[pos].start, frac, pSpline->segments[pos].v_norm, result );
		}

		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}


/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float deltaTime;
	float phase;
	vec3_t v;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
	case TR_GRAVITY_PAUSED: //----(SA)
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
//----(SA)	removed
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
		// Ridah
	case TR_GRAVITY_LOW:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * ( DEFAULT_GRAVITY * 0.3 ) * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
		// done.
//----(SA)
	case TR_GRAVITY_FLOAT:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * ( DEFAULT_GRAVITY * 0.2 ) * deltaTime;
		break;
//----(SA)	end
		// RF, acceleration
	case TR_ACCELERATE:     // trDelta is the ultimate speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		// phase is the acceleration constant
		phase = VectorLength( tr->trDelta ) / ( tr->trDuration * 0.001 );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time
		VectorMA( tr->trBase, phase * 0.5 * deltaTime * deltaTime, result, result );
		break;
	case TR_DECCELERATE:    // trDelta is the starting speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		// phase is the breaking constant
		phase = VectorLength( tr->trDelta ) / ( tr->trDuration * 0.001 );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time (without breaking)
		VectorMA( tr->trBase, deltaTime, tr->trDelta, v );
		// subtract breaking force
		VectorMA( v, -phase * 0.5 * deltaTime * deltaTime, result, result );
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result, qboolean isAngle, int splineData ) {
	float deltaTime;
	float phase;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );    // derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
//----(SA)	removed
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;       // FIXME: local gravity...
		break;
		// Ridah
	case TR_GRAVITY_LOW:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= ( DEFAULT_GRAVITY * 0.3 ) * deltaTime;       // FIXME: local gravity...
		break;
		// done.
//----(SA)
	case TR_GRAVITY_FLOAT:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= ( DEFAULT_GRAVITY * 0.2 ) * deltaTime;
		break;
//----(SA)	end
		// RF, acceleration
	case TR_ACCELERATE: // trDelta is eventual speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		phase = deltaTime / (float)tr->trDuration;
		VectorScale( tr->trDelta, deltaTime * deltaTime, result );
		break;
	case TR_DECCELERATE:    // trDelta is breaking force
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorScale( tr->trDelta, deltaTime, result );
		break;
	case TR_SPLINE:
	case TR_LINEAR_PATH:
		VectorClear( result );
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

void BG_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float	backoff;
	float	change;
	int		i;

	backoff = DotProduct (in, normal);

	if ( backoff < 0 ) {
		backoff *= overbounce;
	}
	else {
		backoff /= overbounce;
	}

	for ( i=0 ; i<3 ; i++ ) {
		change = normal[i]*backoff;
		out[i] = in[i] - change;
	}
}


/*
============
BG_GetMarkDir

  used to find a good directional vector for a mark projection, which will be more likely
  to wrap around adjacent surfaces

  dir is the direction of the projectile or trace that has resulted in a surface being hit
============
*/
void BG_GetMarkDir( const vec3_t dir, const vec3_t normal, vec3_t out ) {
	vec3_t ndir, lnormal;
	float minDot = 0.3;

	if ( VectorLength( normal ) < 1.0 ) {
		VectorSet( lnormal, 0, 0, 1 );
	} else {
		VectorCopy( normal, lnormal );
	}

	VectorNegate( dir, ndir );
	VectorNormalize( ndir );
	if ( normal[2] > 0.8 ) {
		minDot = 0.7;
	}
	// make sure it makrs the impact surface
	while ( DotProduct( ndir, lnormal ) < minDot ) {
		VectorMA( ndir, 0.5, lnormal, ndir );
		VectorNormalize( ndir );
	}

	VectorCopy( ndir, out );
}


char *eventnames[] = {
	"EV_NONE",
	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSTEP_WOOD",
	"EV_FOOTSTEP_GRASS",
	"EV_FOOTSTEP_GRAVEL",
	"EV_FOOTSTEP_ROOF",
	"EV_FOOTSTEP_SNOW",
	"EV_FOOTSTEP_CARPET",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",
	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",
	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",
	"EV_FALL_NDIE",
	"EV_FALL_DMG_10",
	"EV_FALL_DMG_15",
	"EV_FALL_DMG_25",
	"EV_FALL_DMG_50",
	"EV_JUMP_PAD",           // boing sound at origin, jump sound on player
	"EV_JUMP",
	"EV_WATER_TOUCH",    // foot touches
	"EV_WATER_LEAVE",    // foot leaves
	"EV_WATER_UNDER",    // head touches
	"EV_WATER_CLEAR",    // head leaves
	"EV_ITEM_PICKUP",            // normal item pickups are predictable
	"EV_ITEM_PICKUP_QUIET",  // (SA) same, but don't play the default pickup sound as it was specified in the ent
	"EV_GLOBAL_ITEM_PICKUP", // powerup / team sounds are broadcast to everyone
	"EV_NOITEM",
	"EV_NOAMMO",
	"EV_WEAPONSWITCHED", // autoreload
	"EV_EMPTYCLIP",
	"EV_FILL_CLIP",
	"EV_FILL_CLIP_FULL",
	"EV_FILL_CLIP_AI",
	"EV_WEAP_OVERHEAT",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_FIRE_WEAPONB",
	"EV_FIRE_WEAPON_LASTSHOT",
	"EV_FIRE_QUICKGREN",
	"EV_FIRE_QUICKGREN2", 
	"EV_NOFIRE_UNDERWATER",
	"EV_FIRE_WEAPON_MG42",
	"EV_SUGGESTWEAP",        //----(SA)	added
	"EV_GRENADE_SUICIDE",    //----(SA)	added
	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",
	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",
	"EV_GRENADE_BOUNCE",     // eventParm will be the soundindex
	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",       // no attenuation
	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",
	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_RAILTRAIL",
	"EV_VENOM",
	"EV_VENOMFULL",
	"EV_HITSOUNDS",             // otherEntity is the shooter
	"EV_LOSE_HAT",
	"EV_REATTACH_HAT",
	"EV_GIB_HEAD",           // only blow off the head
	"EV_PAIN",
	"EV_CROUCH_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_ENTDEATH",           //----(SA)	added
	"EV_OBITUARY",
	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_BATTLESUIT_SURV",
	"EV_POWERUP_REGEN",
	"EV_GIB_PLAYER",         // gib a previously living player
	"EV_GIB_VAMPIRISM",
	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_STOPSTREAMINGSOUND", //----(SA)	added
	"EV_TAUNT",
	"EV_SMOKE",
	"EV_SPARKS",
	"EV_SPARKS_ELECTRIC",
	"EV_BATS",
	"EV_BATS_UPDATEPOSITION",
	"EV_BATS_DEATH",
	"EV_EXPLODE",        // func_explosive
	"EV_EFFECT",     // target_effect
	"EV_MORTAREFX",  // mortar firing
	"EV_SPINUP", // JPW NERVE panzerfaust preamble for MP balance
	"EV_SNOW_ON",
	"EV_SNOW_OFF",
	"EV_MISSILE_MISS_SMALL",
	"EV_MISSILE_MISS_LARGE",
	"EV_WOLFKICK_HIT_FLESH",
	"EV_WOLFKICK_HIT_WALL",
	"EV_WOLFKICK_MISS",
	"EV_SPIT_HIT",
	"EV_SPIT_MISS",
	"EV_SHARD",
	"EV_JUNK",
	"EV_EMITTER",    //----(SA)	added
	"EV_OILPARTICLES",
	"EV_OILSLICK",
	"EV_OILSLICKREMOVE",
	"EV_MG42EFX",
	"EV_FLAMEBARREL_BOUNCE",
	"EV_FLAKGUN1",
	"EV_FLAKGUN2",
	"EV_FLAKGUN3",
	"EV_FLAKGUN4",
	"EV_EXERT1",
	"EV_EXERT2",
	"EV_EXERT3",
	"EV_SNOWFLURRY",
	"EV_CONCUSSIVE",
	"EV_DUST",
	"EV_RUMBLE_EFX",
	"EV_GUNSPARKS",
	"EV_FLAMETHROWER_EFFECT",
	"EV_SNIPER_SOUND",
	"EV_POPUP",
	"EV_POPUPBOOK",
	"EV_OBJECTIVE_MET",
	"EV_CHECKPOINT_PASSED",
	"EV_GAME_SAVED",
	"EV_GIVEPAGE",
	"EV_CLOSEMENU",
	"EV_M97_PUMP", //jaymod
	"EV_THROWKNIFE",
	"EV_COUGH",
	"EV_QUICKGRENS",
	"EV_PLAYER_HIT",
	"EV_STOP_RELOADING_SOUND",

	"EV_MAX_EVENTS"
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void    trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer( "showevents", buf, sizeof( buf ) );
		if ( atof( buf ) != 0 ) {
#ifdef QAGAME
			Com_Printf( " game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm );
#else
			Com_Printf( "Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm );
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & ( MAX_EVENTS - 1 )] = newEvent;
	ps->eventParms[ps->eventSequence & ( MAX_EVENTS - 1 )] = eventParm;
	ps->eventSequence++;
}


/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_flags & PMF_LIMBO ) { // JPW NERVE limbo
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	if ( ps->movementDir > 128 ) {
		s->angles2[YAW] = (float)ps->movementDir - 256;
	} else {
		s->angles2[YAW] = ps->movementDir;
	}

	s->legsAnim     = ps->legsAnim;
	s->torsoAnim    = ps->torsoAnim;
	s->clientNum    = ps->clientNum;    // ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	// Ridah, let clients know if this person is using a mounted weapon
	// so they don't show any client muzzle flashes

	// (SA) moved up since it needs to set the ps->eFlags too.
	//		Seems like this could be the problem Raf was
	//		encountering with the EF_DEAD flag below when guys
	//		dead flags weren't sticking

	if ( ps->persistant[PERS_HWEAPON_USE] ) {
		ps->eFlags |= EF_MG42_ACTIVE;
	} else {
		ps->eFlags &= ~EF_MG42_ACTIVE;
	}

	s->eFlags = ps->eFlags;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

// from MP
	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}
// end
	// Ridah, now using a circular list of events for all entities
	// add any new events that have been added to the playerState_t
	// (possibly overwriting entityState_t events)
	for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
		s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->events[i & ( MAX_EVENTS - 1 )];
		s->eventParms[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->eventParms[i & ( MAX_EVENTS - 1 )];
		s->eventSequence++;
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->aiChar = ps->aiChar; // Ridah
//	s->loopSound = ps->loopSound;
	s->teamNum = ps->teamNum;
	s->aiState = ps->aiState;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_flags & PMF_LIMBO ) { // JPW NERVE limbo
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;       // ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	// Ridah, now using a circular list of events for all entities
	// add any new events that have been added to the playerState_t
	// (possibly overwriting entityState_t events)
	if ( ps->oldEventSequence > ps->eventSequence ) {
		ps->oldEventSequence = ps->eventSequence;
	}
	for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
		s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->events[i & ( MAX_EVENTS - 1 )];
		s->eventParms[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->eventParms[i & ( MAX_EVENTS - 1 )];
		s->eventSequence++;
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

//	s->loopSound = ps->loopSound;
//	s->generic1 = ps->generic1;
	s->aiChar = ps->aiChar; // Ridah
	s->teamNum = ps->teamNum;
	s->aiState = ps->aiState;
}

/*
=============
BG_Find_PathCorner
=============
*/
pathCorner_t *BG_Find_PathCorner( const char *match ) {
	int i;

	for ( i = 0 ; i < numPathCorners; i++ ) {
		if ( !Q_stricmp( pathCorners[i].name, match ) ) {
			return &pathCorners[i];
		}
	}

	return NULL;
}

/*
=============
BG_AddPathCorner
=============
*/
void BG_AddPathCorner( const char* name, vec3_t origin ) {
	if ( numPathCorners >= MAX_PATH_CORNERS ) {
		Com_Error( ERR_DROP, "MAX PATH CORNERS (%i) hit", MAX_PATH_CORNERS );
	}

	VectorCopy( origin, pathCorners[numPathCorners].origin );
	Q_strncpyz( pathCorners[numPathCorners].name, name, 64 );
	numPathCorners++;
}

/*
=============
BG_Find_Spline
=============
*/
splinePath_t *BG_Find_Spline( const char *match ) {
	int i;

	for ( i = 0 ; i < numSplinePaths; i++ ) {
		if ( !Q_stricmp( splinePaths[i].point.name, match ) ) {
			return &splinePaths[i];
		}
	}

	return NULL;
}

splinePath_t* BG_AddSplinePath( const char* name, const char* target, vec3_t origin ) {
	splinePath_t* spline;
	if ( numSplinePaths >= MAX_SPLINE_PATHS ) {
		Com_Error( ERR_DROP, "MAX SPLINES (%i) hit", MAX_SPLINE_PATHS );
	}

	spline = &splinePaths[numSplinePaths];

	memset( spline, 0, sizeof( splinePath_t ) );

	VectorCopy( origin, spline->point.origin );

	Q_strncpyz( spline->point.name, name,                       64 );
	Q_strncpyz( spline->strTarget,  target ? target : "",        64 );

	spline->numControls = 0;

	numSplinePaths++;

	return spline;
}

void BG_AddSplineControl( splinePath_t* spline, const char* name ) {
	if ( spline->numControls >= MAX_SPLINE_CONTROLS ) {
		Com_Error( ERR_DROP, "MAX SPLINE CONTROLS (%i) hit", MAX_SPLINE_CONTROLS );
	}

	Q_strncpyz( spline->controls[spline->numControls].name, name, 64 );

	spline->numControls++;
}

float BG_SplineLength( splinePath_t* pSpline ) {
	float i;
	float granularity = 0.01f;
	float dist = 0;
//	float tension;
	vec3_t vec[2];
	vec3_t lastPoint = { 0 };
	vec3_t result;

	for ( i = 0; i <= 1.f; i += granularity ) {
/*		if(pSpline->isStart) {
			tension = 1 - sin((1 - i) * M_PI * 0.5f);
		} else if(pSpline->isEnd) {
			tension = sin(i * M_PI * 0.5f);
		} else {
			tension = i;
		}*/

		BG_CalculateSpline_r( pSpline, vec[0], vec[1], i );
		VectorSubtract( vec[1], vec[0], result );
		VectorMA( vec[0], i, result, result );

		if ( i != 0 ) {
			VectorSubtract( result, lastPoint, vec[0] );
			dist += VectorLength( vec[0] );
		}

		VectorCopy( result, lastPoint );
	}

	return dist;
}

void BG_BuildSplinePaths() {
	int i, j;
	pathCorner_t* pnt;
	splinePath_t *spline, *st;

	for ( i = 0; i < numSplinePaths; i++ ) {
		spline = &splinePaths[i];

		if ( *spline->strTarget ) {
			for ( j = 0; j < spline->numControls; j++ ) {
				pnt = BG_Find_PathCorner( spline->controls[j].name );

				if ( !pnt ) {
					Com_Printf( "^1Cant find control point (%s) for spline (%s)\n", spline->controls[j].name, spline->point.name );
					// Gordon: Just changing to a warning for now, easier for region compiles...
					continue;

				} else {
					VectorCopy( pnt->origin, spline->controls[j].origin );
				}
			}

			st = BG_Find_Spline( spline->strTarget );
			if ( !st ) {
				Com_Printf( "^1Cant find target point (%s) for spline (%s)\n", spline->strTarget, spline->point.name );
				// Gordon: Just changing to a warning for now, easier for region compiles...
				continue;
			}

			spline->next = st;

			spline->length = BG_SplineLength( spline );
			BG_ComputeSegments( spline );
		}
	}

	for ( i = 0; i < numSplinePaths; i++ ) {
		spline = &splinePaths[i];

		if ( spline->next ) {
			spline->next->prev = spline;
		}
	}
}

splinePath_t* BG_GetSplineData( int number, qboolean* backwards ) {
	if ( number < 0 ) {
		*backwards = qtrue;
		number = -number;
	} else {
		*backwards = qfalse;
	}
	number--;

	if ( number < 0 || number >= numSplinePaths ) {
		return NULL;
	}

	return &splinePaths[number];
}

/*
=================
PC_Int_Parse
=================
*/
qboolean PC_Int_Parse( int handle, int *i ) {
	pc_token_t token;
	int negative = qfalse;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}
	if ( token.string[0] == '-' ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			return qfalse;
		}
		negative = qtrue;
	}
	if ( token.type != TT_NUMBER ) {
		PC_SourceError( handle, "expected integer but found %s\n", token.string );
		return qfalse;
	}
	*i = token.intvalue;
	if ( negative ) {
		*i = -*i;
	}
	return qtrue;
}

/*
=================
PC_String_ParseNoAlloc

Same as one above, but uses a static buff and not the string memory pool
=================
*/
qboolean PC_String_ParseNoAlloc( int handle, char *out, size_t size ) {
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}

	Q_strncpyz( out, token.string, size );
	return qtrue;
}


// Real printable charater count
int BG_drawStrlen( const char *str ) {
	int cnt = 0;

	while ( *str ) {
		if ( Q_IsColorString( str ) ) {
			str += 2;
		} else {
			cnt++;
			str++;
		}
	}
	return( cnt );
}


// Copies a color string, with limit of real chars to print
//		in = reference buffer w/color
//		out = target buffer
//		str_max = max size of printable string
//		out_max = max size of target buffer
//
// Returns size of printable string
int BG_colorstrncpyz( char *in, char *out, int str_max, int out_max ) {
	int str_len = 0;    // current printable string size
	int out_len = 0;    // current true string size
	const int in_len = strlen( in );

	out_max--;
	while ( *in && out_len < out_max && str_len < str_max ) {
		if ( *in == '^' ) {
			if ( out_len + 2 >= in_len && out_len + 2 >= out_max ) {
				break;
			}

			*out++ = *in++;
			*out++ = *in++;
			out_len += 2;
			continue;
		}

		*out++ = *in++;
		str_len++;
		out_len++;
	}

	*out = 0;

	return( str_len );
}

int BG_strRelPos( char *in, int index ) {
	int cPrintable = 0;
	const char *ref = in;

	while ( *ref && cPrintable < index ) {
		if ( Q_IsColorString( ref ) ) {
			ref += 2;
		} else {
			ref++;
			cPrintable++;
		}
	}

	return( ref - in );
}

// strip colors and control codes, copying up to dwMaxLength-1 "good" chars and nul-terminating
// returns the length of the cleaned string
int BG_cleanName( const char *pszIn, char *pszOut, unsigned int dwMaxLength, qboolean fCRLF ) {
	const char *pInCopy = pszIn;
	const char *pszOutStart = pszOut;

	while ( *pInCopy && ( pszOut - pszOutStart < dwMaxLength - 1 ) ) {
		if ( *pInCopy == '^' ) {
			pInCopy += ( ( pInCopy[1] == 0 ) ? 1 : 2 );
		} else if ( ( *pInCopy < 32 && ( !fCRLF || *pInCopy != '\n' ) ) || ( *pInCopy > 126 ) )    {
			pInCopy++;
		} else {
			*pszOut++ = *pInCopy++;
		}
	}

	*pszOut = 0;
	return( pszOut - pszOutStart );
}

// Only used locally
typedef struct {
	char *colorname;
	vec4_t *color;
} colorTable_t;

extern void trap_Cvar_Set( const char *var_name, const char *value );



///////////////////////////////////////////////////////////////////////////////
typedef struct locInfo_s {
	vec2_t gridStartCoord;
	vec2_t gridStep;
} locInfo_t;

static locInfo_t locInfo;

void BG_InitLocations( vec2_t world_mins, vec2_t world_maxs ) {
	// keep this in sync with CG_DrawGrid
	locInfo.gridStep[0] = 1200.f;
	locInfo.gridStep[1] = 1200.f;

	// ensure minimal grid density
	while ( ( world_maxs[0] - world_mins[0] ) / locInfo.gridStep[0] < 7 )
		locInfo.gridStep[0] -= 50.f;
	while ( ( world_mins[1] - world_maxs[1] ) / locInfo.gridStep[1] < 7 )
		locInfo.gridStep[1] -= 50.f;

	locInfo.gridStartCoord[0] = world_mins[0] + .5f * ( ( ( ( world_maxs[0] - world_mins[0] ) / locInfo.gridStep[0] ) - ( (int)( ( world_maxs[0] - world_mins[0] ) / locInfo.gridStep[0] ) ) ) * locInfo.gridStep[0] );
	locInfo.gridStartCoord[1] = world_mins[1] - .5f * ( ( ( ( world_mins[1] - world_maxs[1] ) / locInfo.gridStep[1] ) - ( (int)( ( world_mins[1] - world_maxs[1] ) / locInfo.gridStep[1] ) ) ) * locInfo.gridStep[1] );
}

char *BG_GetLocationString( vec_t* pos ) {
	static char coord[6];
	int x, y;

	coord[0] = '\0';

	x = ( pos[0] - locInfo.gridStartCoord[0] ) / locInfo.gridStep[0];
	y = ( locInfo.gridStartCoord[1] - pos[1] ) / locInfo.gridStep[1];

	if ( x < 0 ) {
		x = 0;
	}
	if ( y < 0 ) {
		y = 0;
	}

	Com_sprintf( coord, sizeof( coord ), "%c,%i", 'A' + x, y );

	return coord;
}

qboolean BG_BBoxCollision( vec3_t min1, vec3_t max1, vec3_t min2, vec3_t max2 ) {
	int i;

	for ( i = 0; i < 3; i++ ) {
		if ( min1[i] > max2[i] ) {
			return qfalse;
		}
		if ( min2[i] > max1[i] ) {
			return qfalse;
		}
	}

	return qtrue;
}


/*
=================
PC_SourceError
=================
*/
void PC_SourceError( int handle, char *format, ... ) {
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine( handle, filename, &line );

	Com_Printf( S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string );
}


/*
=================
PC_Vec_Parse
=================
*/
qboolean PC_Vec_Parse( int handle, vec3_t *c ) {
	int i;
	float f;

	for ( i = 0; i < 3; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return qfalse;
		}
		( *c )[i] = f;
	}
	return qtrue;
}


/*
=================
PC_Float_Parse
=================
*/
qboolean PC_Float_Parse( int handle, float *f ) {
	pc_token_t token;
	int negative = qfalse;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}
	if ( token.string[0] == '-' ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			return qfalse;
		}
		negative = qtrue;
	}
	if ( token.type != TT_NUMBER ) {
		PC_SourceError( handle, "expected float but found %s\n", token.string );
		return qfalse;
	}
	if ( negative ) {
		*f = -token.floatvalue;
	} else {
		*f = token.floatvalue;
	}
	return qtrue;
}

/*
=================
PC_Color_Parse
=================
*/
qboolean PC_Color_Parse( int handle, vec4_t *c ) {
	int i;
	float f;

	for ( i = 0; i < 4; i++ ) {
		if ( !PC_Float_Parse( handle, &f ) ) {
			return qfalse;
		}
		( *c )[i] = f;
	}
	return qtrue;
}


// Get weap filename for specified weapon id
// Returns empty string when none found
char *BG_GetWeaponFilename( weapon_t weaponNum )
{
	switch ( weaponNum ) {
		case WP_KNIFE:             return "knife.weap";
		case WP_LUGER:             return "luger.weap";
		case WP_SILENCER:          return "luger_silenced.weap";
		case WP_COLT:              return "colt.weap";
		case WP_AKIMBO:            return "akimbo.weap";
		case WP_TT33:              return "tt33.weap";
		case WP_DUAL_TT33:         return "dualtt33.weap";
		case WP_REVOLVER:          return "revolver.weap";
		case WP_THOMPSON:          return "thompson.weap";
		case WP_STEN:              return "sten.weap";
		case WP_MP34:              return "mp34.weap";
		case WP_PPSH:              return "ppsh.weap";
		case WP_MP40:              return "mp40.weap";
		case WP_MAUSER:            return "mauser.weap";
		case WP_SNIPERRIFLE:       return "sniperrifle.weap";
		case WP_GARAND:            return "garand.weap";
		case WP_SNOOPERSCOPE:      return "snooper.weap";
		case WP_MOSIN:             return "mosin.weap";
		case WP_G43:               return "g43.weap";
		case WP_M1GARAND:          return "m1garand.weap";
		case WP_M7:                return "m7.weap";
		case WP_FG42:              return "fg42.weap";
		case WP_FG42SCOPE:         return "fg42scope.weap";
		case WP_MP44:              return "mp44.weap";
		case WP_BAR:               return "bar.weap";
		case WP_M97:               return "ithaca.weap";
		case WP_AUTO5:             return "auto5.weap";
		case WP_FLAMETHROWER:      return "flamethrower.weap";
		case WP_PANZERFAUST:       return "panzerfaust.weap";
		case WP_MG42M:             return "mg42m.weap";
		case WP_TESLA:             return "tesla.weap";
		case WP_VENOM:             return "venom.weap";
		case WP_GRENADE_LAUNCHER:  return "grenade.weap";
		case WP_GRENADE_PINEAPPLE: return "pineapple.weap";
		case WP_DYNAMITE:          return "dynamite.weap";
		case WP_BROWNING:          return "browning.weap";
		case WP_AIRSTRIKE:         return "airstrike.weap";
		case WP_POISONGAS_MEDIC:   return "poisongas_medic.weap";
		case WP_DYNAMITE_ENG:      return "dynamite_eng.weap";
		case WP_ARTY:              return "arty.weap";
		case WP_SMOKETRAIL:        return "smoketrail.weap";
		case WP_POISONGAS:         return "poisongas.weap";
		case WP_HDM:               return "hdm.weap";
		case WP_HOLYCROSS:         return "cross.weap";
		case WP_DELISLE:           return "delisle.weap";
		case WP_DELISLESCOPE:      return "delislescope.weap";
		case WP_DUMMY_MG42:        return "dummy_mg42.weap";
		case WP_M1941:             return "m1941.weap";
		case WP_M1941SCOPE:        return "m1941scope.weap";
		case WP_NONE:
		case WP_MONSTER_ATTACK1:
		case WP_MONSTER_ATTACK2:
		case WP_MONSTER_ATTACK3:
		case WP_SNIPER:
		case VERYBIGEXPLOSION:
		case WP_MORTAR:            return "";
		default:                   Com_Printf( "Missing filename entry for weapon id %d\n", weaponNum );
    }

    return "";
}


// Read ammo parameters into ammoTable from given file handle
// File handle position expected to be at opening brace of ammo block
// Utilised by Client and Game to update their respective copies of ammoTable
// handle not freed
qboolean BG_ParseAmmoTable( int handle, weapon_t weaponNum )
{
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		PC_SourceError( handle, "expected '{'" );
		return qfalse;
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}
		if ( token.string[0] == '}' ) {
			break;
		}

		// Ammo parameters for each difficulty level
		if (!Q_stricmp(token.string, "maxammoPerSkill"))
		{
			for (int i = 0; i < GSKILL_NUM_SKILLS; ++i)
			{
				if (!PC_Int_Parse(handle, &ammoSkill[i][weaponNum].maxammo))
				{
					PC_SourceError(handle, "expected maxammo value for skill level");
					return qfalse;
				}
			}
		}
		else if (!Q_stricmp(token.string, "maxclipPerSkill"))
		{
			for (int i = 0; i < GSKILL_NUM_SKILLS; ++i)
			{
				if (!PC_Int_Parse(handle, &ammoSkill[i][weaponNum].maxclip))
				{
					PC_SourceError(handle, "expected maxclip value for skill level");
					return qfalse;
				}
			}
		}
		else if (!Q_stricmp(token.string, "maxclipUpgradedPerSkill"))
		{
			for (int i = 0; i < GSKILL_NUM_SKILLS; ++i)
			{
				if (!PC_Int_Parse(handle, &ammoSkill[i][weaponNum].maxclipUpgraded))
				{
					PC_SourceError(handle, "expected maxclipUpgraded value for skill level");
					return qfalse;
				}
			}
		}
		else if (!Q_stricmp(token.string, "maxammoUpgradedPerSkill"))
		{
			for (int i = 0; i < GSKILL_NUM_SKILLS; ++i)
			{
				if (!PC_Int_Parse(handle, &ammoSkill[i][weaponNum].maxammoUpgraded))
				{
					PC_SourceError(handle, "expected maxammoUpgraded value for skill level");
					return qfalse;
				}
			}
		}
		// Values common to all skill levels
		else if (!Q_stricmp(token.string, "uses"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].uses ) ) {
				PC_SourceError( handle, "expected uses value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "usesUpgraded"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].usesUpgraded ) ) {
				PC_SourceError( handle, "expected usesUpgraded value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "reloadTime"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].reloadTime ) ) {
				PC_SourceError( handle, "expected reloadTime value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "reloadTimeFull"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].reloadTimeFull ) ) {
				PC_SourceError( handle, "expected reloadTimeFull value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "fireDelayTime"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].fireDelayTime ) ) {
				PC_SourceError( handle, "expected fireDelayTime value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "nextShotTime"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].nextShotTime ) ) {
				PC_SourceError( handle, "expected nextShotTime value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "nextShotTime2"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].nextShotTime2 ) ) {
				PC_SourceError( handle, "expected nextShotTime2 value" );
				return qfalse;
			}
		}
				else if (!Q_stricmp(token.string, "nextShotTimeUpgraded"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].nextShotTimeUpgraded ) ) {
				PC_SourceError( handle, "expected nextShotTimeUpgraded value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "nextShotTime2Upgraded"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].nextShotTime2Upgraded ) ) {
				PC_SourceError( handle, "expected nextShotTime2Upgraded value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "maxHeat"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].maxHeat ) ) {
				PC_SourceError( handle, "expected maxHeat value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "coolRate"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].coolRate ) ) {
				PC_SourceError( handle, "expected coolRate value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "playerDamage"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].playerDamage ) ) {
				PC_SourceError( handle, "expected playerDamage value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "playerDamageUpgraded"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].playerDamageUpgraded ) ) {
				PC_SourceError( handle, "expected playerDamageUpgraded value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "aiDamage"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].aiDamage ) ) {
				PC_SourceError( handle, "expected aiDamage value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "playerSplashRadius"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].playerSplashRadius ) ) {
				PC_SourceError( handle, "expected playerSplashRadius value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "aiSplashRadius"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].aiSplashRadius ) ) {
				PC_SourceError( handle, "expected aiSplashRadius value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "spread"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].spread ) ) {
				PC_SourceError( handle, "expected spread value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "spreadUpgraded"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].spreadUpgraded ) ) {
				PC_SourceError( handle, "expected spread value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "aimSpreadScaleAdd"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].aimSpreadScaleAdd ) ) {
				PC_SourceError( handle, "expected aimSpreadScaleAdd value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "spreadScale"))
		{
			if ( !PC_Float_Parse( handle, &ammoTable[weaponNum].spreadScale ) ) {
				PC_SourceError( handle, "expected spreadScale value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "weapRecoilDuration"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].weapRecoilDuration ) ) {
				PC_SourceError( handle, "expected weapRecoilDuration value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "weapRecoilPitch"))
		{
			if ( !PC_Float_Parse( handle, &ammoTable[weaponNum].weapRecoilPitch[0] ) ) {
				PC_SourceError( handle, "expected weapRecoilPitch.x value" );
				return qfalse;
			}
			if ( !PC_Float_Parse( handle, &ammoTable[weaponNum].weapRecoilPitch[1] ) ) {
				PC_SourceError( handle, "expected weapRecoilPitch.y value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "weapRecoilYaw"))
		{
			if ( !PC_Float_Parse( handle, &ammoTable[weaponNum].weapRecoilYaw[0] ) ) {
				PC_SourceError( handle, "expected weapRecoilYaw.x value" );
				return qfalse;
			}
			if ( !PC_Float_Parse( handle, &ammoTable[weaponNum].weapRecoilYaw[1] ) ) {
				PC_SourceError( handle, "expected weapRecoilYaw.y value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "soundRange"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].soundRange ) ) {
				PC_SourceError( handle, "expected soundRange value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "moveSpeed"))
		{
			if ( !PC_Float_Parse( handle, &ammoTable[weaponNum].moveSpeed ) ) {
				PC_SourceError( handle, "expected moveSpeed value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "twoHand"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].twoHand ) ) {
				PC_SourceError( handle, "expected twoHand value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "upAngle"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].upAngle ) ) {
				PC_SourceError( handle, "expected upAngle value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "falloffDistance"))
		{
			if ( !PC_Float_Parse( handle, &ammoTable[weaponNum].falloffDistance[0] ) ) {
				PC_SourceError( handle, "expected falloffdistance.min value" );
				return qfalse;
			}
			if ( !PC_Float_Parse( handle, &ammoTable[weaponNum].falloffDistance[1] ) ) {
				PC_SourceError( handle, "expected falloffdistance.max value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "shotgunReloadStart"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].shotgunReloadStart ) ) {
				PC_SourceError( handle, "expected shotgunReloadStart value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "shotgunReloadLoop"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].shotgunReloadLoop ) ) {
				PC_SourceError( handle, "expected shotgunReloadLoop value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "shotgunReloadEnd"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].shotgunReloadEnd ) ) {
				PC_SourceError( handle, "expected shotgunReloadEnd value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "shotgunPumpStart"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].shotgunPumpStart ) ) {
				PC_SourceError( handle, "expected shotgunPumpStart value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "shotgunPumpLoop"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].shotgunPumpLoop ) ) {
				PC_SourceError( handle, "expected shotgunPumpLoop value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "shotgunPumpEnd"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].shotgunPumpEnd  ) ) {
				PC_SourceError( handle, "expected shotgunPumpEnd value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "brassDelayEmpty"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].brassDelayEmpty  ) ) {
				PC_SourceError( handle, "expected brassDelayEmpty value" );
				return qfalse;
			}
		}
		else if (!Q_stricmp(token.string, "brassDelay"))
		{
			if ( !PC_Int_Parse( handle, &ammoTable[weaponNum].brassDelay  ) ) {
				PC_SourceError( handle, "expected brassDelay value" );
				return qfalse;
			}
		}
		else
		{
			PC_SourceError( handle, "unknown token '%s'", token.string );
			return qfalse;
		}
	}

	return qtrue;
}


// Set weapon parameters for specified skill
void BG_SetWeaponForSkill( weapon_t weaponNum, gameskill_t skill ) {
	if ( ammoSkill[skill][weaponNum].maxammo > 0 )
		ammoTable[weaponNum].maxammo = ammoSkill[skill][weaponNum].maxammo;

	if ( ammoSkill[skill][weaponNum].maxclip > 0 )
		ammoTable[weaponNum].maxclip = ammoSkill[skill][weaponNum].maxclip;

	if ( ammoSkill[skill][weaponNum].maxclipUpgraded > 0 )
		ammoTable[weaponNum].maxclipUpgraded = ammoSkill[skill][weaponNum].maxclipUpgraded;

	if ( ammoSkill[skill][weaponNum].maxammoUpgraded > 0 )
		ammoTable[weaponNum].maxammoUpgraded = ammoSkill[skill][weaponNum].maxammoUpgraded;
}

/*
==========================
BG_GetMaxClip

Returns the correct clip size for the given weapon and player state,
taking into account whether the weapon is upgraded.
==========================
*/
int BG_GetMaxClip(const playerState_t *ps, int weapon) {
	if (!ps || weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS) {
		return 0;
	}

	const ammoTable_t *wt = &ammoTable[weapon];

	if (ps->weaponUpgraded[weapon]) {
		return wt->maxclipUpgraded;
	} else {
		return wt->maxclip;
	}
}

/*
==========================
BG_GetMaxAmmo

Returns the correct max ammo capacity for the given weapon and player state,
taking into account whether the weapon is upgraded and any class-specific bonuses.
==========================
*/
int BG_GetMaxAmmo(const playerState_t *ps, int weapon, float ltAmmoBonus) {
	if (!ps || weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS) {
		return 0;
	}

	const ammoTable_t *wt = &ammoTable[weapon];
	int maxAmmo = ps->weaponUpgraded[weapon]
		? wt->maxammoUpgraded
		: wt->maxammo;

	if (ps->stats[STAT_PLAYER_CLASS] == PC_LT) {
		maxAmmo *= ltAmmoBonus;
	}

	return maxAmmo;
}

/*
==========================
BG_GetMaxAmmo

Returns the correct max ammo capacity for the given weapon and player state,
taking into account whether the weapon is upgraded and any class-specific bonuses.
==========================
*/

int BG_GetNextShotTime(const playerState_t *ps, weapon_t weapon, qboolean altFire) {
	const ammoTable_t *wtd = GetWeaponTableData(weapon);
	qboolean upgraded = ps->weaponUpgraded[weapon];

	if (altFire) {
		return upgraded ? wtd->nextShotTime2Upgraded : wtd->nextShotTime2;
	} else {
		return upgraded ? wtd->nextShotTimeUpgraded : wtd->nextShotTime;
	}
}