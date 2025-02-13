MODULE Module1

    
    VAR socketdev socket1;
    VAR string received_string;
    VAR socketdev server_socket;
    VAR socketdev client_socket;
    VAR string server_ip := "192.168.125.2";
    VAR intnum port := 1025;
    VAR num foundx :=10;
    VAR num foundy :=10;
    VAR string sposx;
    VAR string sposy;
    VAR dnum posx;
    VAR dnum posy;
    VAR string slenx;
    VAR string sleny;
    VAR num lenx;
    VAR num leny;
    var bool result;
    VAR num index := 0;
    VAR num distance_pallet := 81.5;
    VAR num x := 1;
    VAR num y := 1; 
    VAR num z_distance_mesa_giratoria := 108;
    VAR num z_distance_pallet := 145;
    CONST num z_distance_ref := 300;    
    VAR signaldo do6;
    VAR bool init := TRUE;
    VAR num checkxy := 0;
    
	CONST robtarget pHome:=[[448.988602945,0,522.002691924],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];
    CONST robtarget pDrop:=[[706.13,250,z_distance_ref],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];
    CONST robtarget PTriangulo_10:=[[-157.93,646.86,196.36],[1,-3.16512E-05,-1.39085E-05,-8.98549E-06],[1,-1,1,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];
    CONST robtarget PTriangulo_20:=[[161.2,650.4,353.82],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];
    VAR robtarget xydrop:=[[0,0,0],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];
    
    PROC main()
       VAR robtarget targetxy:=[[627.674983597,-164.420163875,z_distance_ref],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];        
       
       SocketCreate server_socket;
       SocketConnect server_socket, server_ip, port;

       WHILE init DO
       SocketSend server_socket \Str:="hello server";
       SocketReceive server_socket \Str := received_string;
       IF received_string = "wsupabb!" THEN
           SocketSend server_socket \Str := "coordinates!";
           init := FALSE; 
      ENDIF
       ENDWHILE
       WHILE TRUE DO
          ! WaitTime 0.3;
           SocketReceive server_socket \Str := received_string;
           foundx := StrMatch(received_string,1,"X");
           foundy := StrMatch(received_string,1,"Y");
           

           IF foundx = 1 THEN
  
                lenx := StrLen(received_string);
                sposx := StrPart(received_string,2,lenx-1);
                foundx := 0;
                result := StrToVal(sposx,posx);
                checkxy := checkxy + 1;
                       
           ELSEIF foundy = 1 THEN

                leny := StrLen(received_string);
                sposy := StrPart(received_string,2,leny-1);
                foundy := 0;
                result := StrToVal(sposy,posy);
                checkxy := checkxy + 1;

           ELSEIF received_string = "DONE" THEN
               
               targetxy:=[[DnumToNum(posx),DnumToNum(posy),z_distance_ref],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];        
               Move_XY targetxy,x,y;
               SocketSend server_socket \Str := "coordinates!";  
               
                    !Go to Pallet
               xydrop := Offs (pDrop,-(x-1)*distance_pallet,-(y-1)*distance_pallet, 0);
               MoveJ xydrop, v500, z100, tPaletizado\WObj:=wobj0;
               xydrop := Offs (xydrop,0,0,-z_distance_pallet);
               MoveJ xydrop, v500, fine, tPaletizado\WObj:=wobj0;
               SetDO\SDelay:=0, Activar_pinza, 0; !abre pinza
               WaitTime (0.5);
               xydrop := Offs (xydrop,0,0,z_distance_pallet);
               MoveJ xydrop, v500, z100, tPaletizado\WObj:=wobj0;
               MoveJ pHome, v500, fine, tPaletizado\WObj:=wobj0;
               index := index + 1;  
               WaitRob \InPos;
               x := x + 1;
               IF x = 4 THEN
                   x:= 1;
                   y:= y+1;
               ENDIF
              IF index = 9 THEN
                  index := 0;
                  x := 1;
                  y := 1;
              ENDIF
              
           ENDIF
           
           
       ENDWHILE
    
       ERROR
       IF ERRNO=ERR_SOCK_TIMEOUT THEN
           RETRY;
        ENDIF
    ENDPROC
    
    
    PROC Move_XY(VAR robtarget mytarget, VAR num row, VAR num column)
        
        !Go to Object
        MoveJ mytarget, v800, fine, tPaletizado\WObj:=wobj0;
        mytarget := Offs (mytarget,0, 0, -z_distance_mesa_giratoria);
        MoveJ mytarget, v800, fine, tPaletizado\WObj:=wobj0;
        SetDO\SDelay:=0, Activar_pinza, 1; !cierra pinza
        WaitTime (0.5);
        mytarget := Offs (mytarget,0, 0, z_distance_mesa_giratoria);
        MoveJ mytarget, v800, z100, tPaletizado\WObj:=wobj0;
        
    ENDPROC

    
ENDMODULE
