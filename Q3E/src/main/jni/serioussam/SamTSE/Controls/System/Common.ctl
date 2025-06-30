Button
 Name: TTRS Menu Save
 Key1: F2
 Key2: None
 Pressed:  sam_bMenuSave=1;
 Released: 

Button
 Name: TTRS Menu Load
 Key1: F3
 Key2: None
 Pressed:  sam_bMenuLoad=1;
 Released: 

Button
 Name: TTRS Menu Controls
 Key1: F4
 Key2: None
 Pressed:  sam_bMenuControls=1;
 Released: 

Button
 Name: TTRS Quick Save
 Key1: F6
 Key2: None
 Pressed:  gam_bQuickSave=1;
 Released: 

Button
 Name: TTRS Quick Load
 Key1: F9
 Key2: None
 Pressed:  gam_bQuickLoad=1;
 Released: 

Button
 Name: TTRS Screenshot
 Key1: F11
 Key2: None
 Pressed:  SaveScreenShot();
 Released: 

Button
 Name: TTRS StartDemoRecording
 Key1: F7
 Key2: None
 Pressed:  StartDemoRecording();
 Released: 

Button
 Name: TTRS StopDemoRecording
 Key1: F8
 Key2: None
 Pressed:  StopDemoRecording();
 Released: 


Button
 Name: TTRS Toggle frame rate
 Key1: None
 Key2: None
 Pressed:  hud_iStats=(hud_iStats+1)%3;
 Released: 

Button
 Name: TTRS Show elapsed time and/or crosshair coordinates
 Key1: None
 Key2: None
 Pressed:  extern FLOAT fTmp=hud_bShowTime*2+hud_bShowCoords; fTmp = (fTmp+1)%4; hud_bShowCoords=fTmp%2; hud_bShowTime=(fTmp-hud_bShowCoords)/2;
 Released: 

Button
 Name: TTRS Toggle Triangles
 Key1: None
 Key2: None
 Pressed:  wld_bShowTriangles=!wld_bShowTriangles;
 Released: 

Button
 Name: TTRS Breakpoint
 Key1: None
 Key2: None
 Pressed:  dbg_bBreak = 1;
 Released: 


Button
 Name: TTRS Observe No Players
 Key1: Num 0
 Key2: None
 Pressed:  gam_iObserverConfig=0;
 Released: 

Button
 Name: TTRS Observe 1 Player
 Key1: Num 1
 Key2: None
 Pressed:  gam_iObserverConfig=1;
 Released: 

Button
 Name: TTRS Observe 2 Players
 Key1: Num 2
 Key2: None
 Pressed:  gam_iObserverConfig=2;
 Released: 

Button
 Name: TTRS Observe 3 Players
 Key1: Num 3
 Key2: None
 Pressed:  gam_iObserverConfig=3;
 Released: 

Button
 Name: TTRS Observe 4 Players
 Key1: Num 4
 Key2: None
 Pressed:  gam_iObserverConfig=4;
 Released: 

Button
 Name: TTRS Observe Next
 Key1: Num +
 Key2: None
 Pressed:  gam_iObserverOffset=(gam_iObserverOffset+1)%16;
 Released: 

Button
 Name: TTRS Observe Prev
 Key1: Num -
 Key2: None
 Pressed:  gam_iObserverOffset=(gam_iObserverOffset+16-1)%16;
 Released: 


Button
 Name: TTRS Demo FWD one tick
 Key1: None
 Key2: None
 Pressed:  dem_tmTimer=dem_tmTimer+0.05;
 Released: 

Button
 Name: TTRS Demo Jogg
 Key1: None
 Key2: None
 Pressed:  dem_fSyncRate=0.05;
 Released: dem_fSyncRate=-1.0;

Button
 Name: TTRS Demo PLAY
 Key1: None
 Key2: None
 Pressed:  dem_fSyncRate=0.0;
 Released: 

Button
 Name: TTRS Demo PAUSE
 Key1: None
 Key2: None
 Pressed:  dem_fSyncRate=-1.0;
 Released: 

Button
 Name: TTRS Demo Toggle OSD
 Key1: None
 Key2: None
 Pressed:  dem_bOnScreenDisplay=!dem_bOnScreenDisplay;
 Released: 


Button
 Name: TTRS Demo Fast
 Key1: None
 Key2: None
 Pressed:  dem_fRealTimeFactor=4.0f;
 Released: dem_fRealTimeFactor=1.0f;

Button
 Name: TTRS Demo Super Fast
 Key1: '
 Key2: None
 Pressed:  dem_fRealTimeFactor=16.0f;
 Released: dem_fRealTimeFactor=1.0f;



Button
 Name: TTRS Time slower
 Key1: None
 Key2: None
 Pressed:  gam_fRealTimeFactor  = gam_fRealTimeFactor / 2;
 Released: 

Button
 Name: TTRS Time faster
 Key1: None
 Key2: None
 Pressed:  gam_fRealTimeFactor  = gam_fRealTimeFactor * 2;
 Released: 

