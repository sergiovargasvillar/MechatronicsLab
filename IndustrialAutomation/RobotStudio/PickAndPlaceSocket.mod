MODULE Module1

    ! This RAPID code controls an ABB industrial robot in RobotStudio.
    ! It establishes a socket connection with a remote server, receives X and Y coordinates dynamically,
    ! and executes pick-and-place operations based on those coordinates.
    ! The robot follows a structured sequence:
    ! 1. Connects to a TCP/IP server.
    ! 2. Requests and receives coordinates for object pickup.
    ! 3. Moves to the target position, picks up an object, and places it at a predefined location.
    ! 4. Handles errors and ensures continuous operation.

    VAR socketdev socket1;     ! Socket device for communication
    VAR string received_string; ! Stores received data from server
    VAR socketdev server_socket; ! Server socket for communication
    VAR socketdev client_socket; ! Client socket for future expansion
    VAR string server_ip := "192.168.125.2"; ! Server IP address
    VAR intnum port := 1025;    ! Port for socket communication
    VAR num foundx := 10;       ! Flag for detecting 'X' coordinate
    VAR num foundy := 10;       ! Flag for detecting 'Y' coordinate
    VAR string sposx;           ! String to store received X coordinate
    VAR string sposy;           ! String to store received Y coordinate
    VAR dnum posx;              ! Converted X coordinate as a number
    VAR dnum posy;              ! Converted Y coordinate as a number
    VAR num lenx;               ! Length of received X coordinate
    VAR num leny;               ! Length of received Y coordinate
    VAR bool result;            ! Conversion success flag
    VAR num index := 0;         ! Index counter for palletizing pattern
    VAR num distance_pallet := 81.5; ! Distance between pallet positions
    VAR num x := 1;             ! Row index for palletizing
    VAR num y := 1;             ! Column index for palletizing
    VAR num z_distance_mesa_giratoria := 108; ! Z height for rotary table
    VAR num z_distance_pallet := 145;        ! Z height for pallet drop
    CONST num z_distance_ref := 300;        ! Reference Z height
    VAR signaldo do6;          ! Digital output signal for controlling the gripper
    VAR bool init := TRUE;      ! Initialization flag
    VAR num checkxy := 0;      ! Check counter for received X and Y values
    
    ! Reference positions
    CONST robtarget pHome := [[448.9886,0,522.0027],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]]; ! Home position
    CONST robtarget pDrop := [[706.13,250,z_distance_ref],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]]; ! Drop-off position
    VAR robtarget xydrop := [[0,0,0],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]]; ! Dynamic drop position
    CONST robtarget pRobotWaiting := [[401.81,408.97,432.14],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];  ! Waiting position
    
    PROC main()
       VAR robtarget targetxy := [[627.67,-164.42,z_distance_ref],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]]; 
       
       ! Create and connect socket to server
       SocketCreate server_socket;
       SocketConnect server_socket, server_ip, port;
       
       ! Initial object pickup process
       Recogida_pieza;
       
       ! Establish connection with the server and request coordinates
       WHILE init DO
           SocketSend server_socket \Str:="hello server";
           SocketReceive server_socket \Str := received_string;
           IF received_string = "wsupabb!" THEN
               SocketSend server_socket \Str := "coordinates!";
               init := FALSE;
           ENDIF
       ENDWHILE
       
       ! Main loop for receiving coordinates and processing movement
       WHILE TRUE DO
           SocketReceive server_socket \Str := received_string;
           foundx := StrMatch(received_string,1,"X");
           foundy := StrMatch(received_string,1,"Y");
            
           ! Extract X coordinate from received string
           IF foundx = 1 THEN
                lenx := StrLen(received_string);
                sposx := StrPart(received_string,2,lenx-1);
                foundx := 0;
                result := StrToVal(sposx,posx);
                checkxy := checkxy + 1;
           
           ! Extract Y coordinate from received string
           ELSEIF foundy = 1 THEN
                leny := StrLen(received_string);
                sposy := StrPart(received_string,2,leny-1);
                foundy := 0;
                result := StrToVal(sposy,posy);
                checkxy := checkxy + 1;
           
           ! When coordinates are complete, move robot
           ELSEIF received_string = "DONE" THEN
               targetxy := [[DnumToNum(posx),DnumToNum(posy),z_distance_ref],[1,0,0,0],[0,0,0,0],[9E+09,9E+09,9E+09,9E+09,9E+09,9E+09]];        
               Move_XY targetxy, x, y;
               SocketSend server_socket \Str := "coordinates!";  
               
               ! Move to pallet position and release object
               xydrop := Offs (pDrop,-(x-1)*distance_pallet,-(y-1)*distance_pallet, 0);
               MoveJ xydrop, v1500, z200, tPaletizado\WObj:=wobj0;
               xydrop := Offs (xydrop,0,0,-z_distance_pallet);
               MoveJ xydrop, v1500, fine, tPaletizado\WObj:=wobj0;
               SetDO \SDelay:=0, Activar_pinza, 0;
               WaitTime (0.5);
               xydrop := Offs (xydrop,0,0,z_distance_pallet);
               MoveJ xydrop, v1000, z200, tPaletizado\WObj:=wobj0;
               MoveJ pRobotWaiting, v800, fine, tPaletizado\WObj:=wobj0;
               index := index + 1;
               
               ! Increment palletizing pattern
               WaitRob \InPos;
               x := x + 1;
               IF x = 4 THEN x:= 1; y:= y+1; ENDIF
               IF index = 9 THEN
                  index := 0;
                  x := 1;
                  y := 1;
                  Cargar_mesa;
                  MoveJ pHome, v1500, fine, tPaletizado\WObj:=wobj0;
               ENDIF
           ENDIF
       ENDWHILE
    ENDPROC

ENDMODULE
