#include <pspsdk.h> 
#include <pspkernel.h> 
#include <pspctrl.h> 
#include <stdio.h> 
#include <string.h> 
#include <systemctrl.h> 

PSP_MODULE_INFO("camera_patch_lite_alt", 0x1007, 1, 0);  // 0x1007

#define THRESHOLD 32 

int PSP_CTRL_MOVEBUTTON;
int PSP_CTRL_BUTTON;
int PSP_CTRL_BUTTON_2;
#define PSP_CTRL_LTRIGGER 	0x000100
#define PSP_CTRL_RTRIGGER 	0x000200
#define PSP_CTRL_TRIANGLE 	0x001000
#define PSP_CTRL_CIRCLE 	0x002000
#define PSP_CTRL_CROSS 		0x004000
#define PSP_CTRL_SQUARE 	0x008000 

#define Rx Rsrv[0] //for readability
#define Ry Rsrv[1]

#define MAKE_JUMP(a, f) _sw(0x08000000 | (((u32)(f) & 0x0FFFFFFC) >> 2), a); 

#define HIJACK_FUNCTION(a, f, ptr) { \
	u32 func = a; \
	static u32 patch_buffer[3]; \
	_sw(_lw(func), (u32)patch_buffer); \
	_sw(_lw(func + 4), (u32)patch_buffer + 8);\
	MAKE_JUMP((u32)patch_buffer + 4, func + 8); \
	_sw(0x08000000 | (((u32)(f) >> 2) & 0x03FFFFFF), func); \
	_sw(0, func + 4); \
	ptr = (void *)patch_buffer; \
} 


typedef struct {
	char *id;		//the games TitleID
	int movebutton;	//button that activates movement
	int button;		//button that activates the camera
	int button2;	//button2 that activates the camera (if there is one)
	int controls;	//buttons that control the camera
	int delay;		//trigger delay (basically a fix) [1sec = 1000000]
} Game_pack;

int time = 0;
int delay = 0;

char gameid[16];

int contr = 0;
#define analog 			1 //analog stick normal
#define analog_inv 		2 //analog fully inverted
#define analog_x_inv	3 //analog with x axis inverted
#define analog_y_inv	4 //analog with y axis inverted
#define analog_x		5 //only x axis working
#define analog_y		6 //only y axis working
#define dpad 			7 //dpad aka arrow keys
#define dpad_lr 		8 //dpad aka arrow keys
#define dpad_inv 		9 //dpad fully inverted
#define buttons 		10 //X O /\ [] 
#define buttons_inv 	11 //X O /\ [] inverted
#define buttons_lr	 	12 //O []
#define triggers 		13 //L&R Triggers
#define triggers_inv	14 //L&R inverted
#define mix_1			15 //L&R + Triangle&Cross

const Game_pack gamelist[] = {
	//*id			//movement button combo		//camera button		//camera button2	//controls		//delay		//game name											//tested? & Notes
	{"ULES-00288",	0, 							0,					0,					buttons_lr,		0  		},	//007 From Russia with love [EU]					also has full camera control with: L-TRIGGER + Analog			
	{"ULES-00289",	0, 							0,					0,					buttons_lr,		0  		},	//007 From Russia with love [EU]					Problem is that L-Trigger is used for targeting enemies too
	{"ULES-00290", 	0,							0,					0,					buttons_lr,		0  		},	//007 From Russia with love [EU]			
	{"ULUS-10080",	0, 							0,					0,					buttons_lr,		0  		},	//007 From Russia with love [US]	
	{"ULAS-42043",	0, 							0,					0,					buttons_lr,		0  		},	//007 From Russia with love [??]
	
	{"ULES-00451",	0, 							PSP_CTRL_LTRIGGER, 	0,					dpad_lr,		0  		},	//50Cent BulletProof G-UNit Ed. [EU]	
	{"ULES-00452",	0, 							PSP_CTRL_LTRIGGER, 	0,					dpad_lr,		0  		},	//50Cent BulletProof G-UNit Ed. [EU]	
	{"ULUS-10128",	0, 							PSP_CTRL_LTRIGGER, 	0,					dpad_lr,		0  		},	//50Cent BulletProof G-UNit Ed. [US]	
	
	{"UCUS-98609",	0,							0, 					0,					dpad_inv,		0  		},	//Ape Escape On the Loose [US]
	
	{"ULES-00972",	0, 							0, 					0, 					triggers,		0  		},	//Aliens vs. Predator Requiem [EU]					OK					
	{"ULUS-10327",	0, 							0, 					0, 					triggers,		0  		},	//Aliens vs. Predator Requiem [US]					OK	

 ///{"NPEH-00029",	0, 							PSP_CTRL_LTRIGGER, 	0,					buttons,		0 		},	//Assassins Creed Bloodlines [EU]					OK
 ///{"ULES-01367",	0, 							PSP_CTRL_LTRIGGER, 	0,					buttons,		0  		},	//Assassins Creed Bloodlines [EU]					OK
 ///{"ULUS-10455",	0, 							PSP_CTRL_LTRIGGER, 	0,					buttons,		0 		},	//Assassins Creed Bloodlines [US]					OK
 ///{"ULJM-05571",	0, 							PSP_CTRL_LTRIGGER, 	0,					buttons,		0 		},	//Assassins Creed Bloodlines [JP]					OK
 
	{"ULES-01357",	0, 							PSP_CTRL_CIRCLE, 	0,					analog,			0  		},	//Avatar The Game [EU]								OK
	{"NPEH-00031",	0, 							PSP_CTRL_CIRCLE, 	0,					analog,			0  		},	//Avatar The Game [EU]								OK
	{"ULUS-10451",	0, 							PSP_CTRL_CIRCLE, 	0,					analog,			0  		},	//Avatar The Game [US]								OK
 
 	{"ULES-01358",	0,							PSP_CTRL_LTRIGGER,	0,					analog_y_inv,	0  		},	//Ben10 AF Vilgax Attacks [EU]			
	{"ULUS-10488",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	0  		},	//Ben10 AF Vilgax Attacks [US]	
 
	{"ULES-01471",	0, 							PSP_CTRL_LTRIGGER,	0,					analog_y_inv,	250000 	},	//Ben10 Ult.Alien Cos.Destr. [EU]					OK
	{"ULUS-10542",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	250000 	},	//Ben10 Ult.Alien Cos.Destr. [US]					OK	
	
 ///{"ULES-00643",	0,							0, 					0,					buttons,		0 		},	//Call of Duty Roads to Victory [EU]				ERROR
 ///{"ULES-00644",	0,							0, 					0,					buttons,		0 		},	//Call of Duty Roads to Victory [EU]				ERROR
 ///{"ULUS-10218",	0,							0, 					0,					buttons,		0 		},	//Call of Duty Roads to Victory [US]				ERROR	Acting weird.- movement controls get fucked up,	
 
	{"ULES-00124",	0,							0, 					0,					buttons,		0  		},	//Coded Arms [EU]						
	{"ULUS-10019",	0,							0, 					0,					buttons,		0  		},	//Coded Arms [US]									OK
	{"ULJM-05024",	0,							0, 					0,					buttons,		0  		},	//Coded Arms [JP]
	{"ULAS-42009",	0,							0, 					0,					buttons,		0  		},	//Coded Arms [CN]
	
	{"ULES-01044",	0, 							0,					0,					triggers,		0  		},	//Crisis Core: Final Fantasy VII [EU]							
	{"ULES-01045",	0, 							0,					0,					triggers,		0  		},	//Crisis Core: Final Fantasy VII [EU]							
	{"ULES-01046",	0, 							0,					0,					triggers,		0  		},	//Crisis Core: Final Fantasy VII [EU]							
	{"ULES-01047",	0, 							0,					0,					triggers,		0  		},	//Crisis Core: Final Fantasy VII [EU]							
	{"ULES-01048",	0, 							0,					0,					triggers,		0 		},	//Crisis Core: Final Fantasy VII [EU]							
	{"ULUS-10336",	0, 							0,					0,					triggers,		0  		},	//Crisis Core: Final Fantasy VII [US]							
	{"ULJM-05254",	0, 							0,					0,					triggers,		0 		},	//Crisis Core: Final Fantasy VII [JP]							
	{"ULJM-05275",	0, 							0,					0,					triggers,		0  		},	//Crisis Core: Final Fantasy VII [JP]
	
	{"ULES-01384",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Dantes Inferno [EU]
	{"ULES-01385",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Dantes Inferno [EU]
	{"ULES-01386",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Dantes Inferno [EU]
	{"ULES-01387",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Dantes Inferno [EU]
	{"ULES-01388",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Dantes Inferno [EU]
	{"ULUS-10469",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Dantes Inferno [US]
	{"ULUS-10496",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Dantes Inferno [US]
	{"NPJH-50220",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Dantes Inferno [JP]
	
	{"UCES-00044",	0,							0, 					0,					triggers_inv,	0  		},	//Daxter [EU]
	{"UCUS-98618",	0,							0, 					0,					triggers_inv,	0  		},	//Daxter [US]
	{"UCKS-45025",	0,							0, 					0,					triggers_inv,	0  		},	//Daxter []
	
 ///{"ULES-00144",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog,			250000 	},	//Death Jr. [EU]									ERROR	does not accept analog input?
 ///{"ULUS-10027",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog,			250000 	},	//Death Jr. [US]									ERROR	
 
 ///{"NPJH-50443",	0,							0, 					0,					dpad,			0  		},	//Final Fantasy Type0								//bugged selection in menus, use built-it Vita remap function instead
 
	{"ULES-00704",	0, 							0,					0,					dpad,			0  		},	//Free Running [EU]	
 
 	{"ULUS-10486",	0,							0, 					0,					buttons,		0  		},	//Ghostbusters: The Video Game [US]
	
	{"NPEG-00023",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Chains of Olympus [EU]	
	{"NPUG-80325",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Chains of Olympus [US]	
	{"UCES-00842",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Chains of Olympus [EU]	
	{"UCUS-98653",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Chains of Olympus [US]	
	{"UCUS-98705",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Chains of Olympus [US]
	{"ULJM-05348",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Chains of Olympus [JP]		
	{"UCAS-40198",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Chains of Olympus [CN]	
	{"UCKS-45084",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Chains of Olympus [KR]
	
	{"NPEG-00044",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Ghost of Sparta [EU]					On request this will bring dodgeing to the right stick just like it should be: 
	{"NPEG-00045",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Ghost of Sparta [EU]					http://thevitalounge.net/wp-content/uploads/2014/04/GodofWarControls.jpg
	{"NPUG-80508",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Ghost of Sparta [US]	
	{"NPHG-00092",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Ghost of Sparta [CN]	
	{"UCES-01473",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Ghost of Sparta [EU]	
	{"UCES-01401",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Ghost of Sparta [EU]	
	{"UCUS-98737",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Ghost of Sparta [US]	
	{"UCJS-10114",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//God of War Ghost of Sparta [JP]	
 
 ///{"ULES-00151",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	220000 	},	//GTA Liberty City Stories [EU]						OK
 ///{"ULES-00182",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	220000 	},	//GTA Liberty City Stories [GER]					OK
 ///{"ULUS-10041",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	220000 	},	//GTA Liberty City Stories [US]						OK
 ///{"ULJM-05255",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	220000 	},	//GTA Liberty City Stories [JP]						OK
		
 ///{"ULES-00502",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	220000	},	//GTA Vice City Stories [EU]						OK
 ///{"ULES-00503",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	220000 	},	//GTA Vice City Stories [EU]						OK
 ///{"ULUS-10160",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	220000 	},	//GTA Vice City Stories [US]						OK
 ///{"ULJM-05297",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog_y_inv,	220000 	},	//GTA Vice City Stories [JP]						OK
 
	{"ULES-00484",	0,							0, 					0,					buttons,		0  		},	//Gun Showdown [EU]	
	{"ULUS-10158",	0,							0, 					0,					buttons,		0  		},	//Gun Showdown [US]	
	
	{"ULUS-10458",	0,							0,					0,					dpad,			0		},	//Harvest Moon: Hero of Leaf Valley [US]
	{"ULUS-01489",	0,							0,					0,					dpad,			0		},	//Harvest Moon: Hero of Leaf Valley [EU]
	{"ULUS-00188",	0,							0,					0,					dpad,			0		},	//Harvest Moon: Hero of Leaf Valley [JP]
	
	{"ULES-00939",	0,							0, 					0,					dpad_inv,		0  		},	//Indiana Jones and Staff of Kings [EU]			
	{"ULUS-10316",	0,							0, 					0,					dpad_inv,		0  		},	//Indiana Jones and Staff of Kings [US]
	
	{"ULES-01070",	0,							0, 					0,					buttons,		0  		},	//Iron Man [EU]	
	{"ULES-01071",	0,							0, 					0,					buttons,		0  		},	//Iron Man [EU]	
	{"ULUS-10347",	0,							0, 					0,					buttons,		0  		},	//Iron Man [US]	
	
	{"ULES-01422",	0,							0, 					0,					buttons,		0 		},	//Iron Man 2 [EU]			
	{"ULUS-10497",	0,							0, 					0,					buttons,		0  		},	//Iron Man 2 [US]
	
	{"UCES-01225",	0,							0, 					0,					triggers,		0  		},	//Jak&Daxter: The Lost Frontier [EU]	
	{"UCUS-98634",	0,							0, 					0,					triggers,		0  		},	//Jak&Daxter: The Lost Frontier [US]	
	{"UCJS-10103",	0,							0, 					0,					triggers,		0  		},	//Jak&Daxter: The Lost Frontier [JP]	
	
	{"ULES-00225",	0,							0, 					0,					buttons,		0  		},	//King Kong [EU]									OK
	{"ULUS-10072",	0,							0, 					0,					buttons,		0  		},	//King Kong [US]									OK
	
 ///{"ULES-01441",	0, 							0,					0,					triggers,		0  		},	//Kingdom Hearts: Birth By Sleep [EU]
 ///{"ULUS-10505",	0, 							0,					0,					triggers,		0  		},	//Kingdom Hearts: Birth By Sleep [US]
 ///{"ULJM-05600",	0, 							0,					0,					triggers,		0  		},	//Kingdom Hearts: Birth By Sleep [JP]
 ///{"ULJM-05775",	0, 							0,					0,					triggers,		0  		},	//Kingdom Hearts: Birth By Sleep [JP]
	
	{"ULES-00756",	0,							0, 					0,					analog_x,		0  		},	//Manhunt 2 [EU]	
	{"ULUS-10280",	0,							0, 					0,					analog_x,		0  		},	//Manhunt 2 [US]
	
	{"ULES-00542",	0, 							PSP_CTRL_LTRIGGER,	0,					buttons_lr,		0  		},	//Marvel Ultimate Alliance [EU]			
	{"ULUS-10167",	0, 							PSP_CTRL_LTRIGGER,	0,					buttons_lr,		0  		},	//Marvel Ultimate Alliance [US]			
	
	{"ULES-01343",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0 		},	//Marvel Ultimate Alliance 2 [EU]					OK			
	{"ULUS-10421",	0, 							PSP_CTRL_LTRIGGER,	PSP_CTRL_RTRIGGER,	analog,			0  		},	//Marvel Ultimate Alliance 2 [US]					OK 	

	{"ULES-00557",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes [EU]	
	{"ULES-00558",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes [EU]	
	{"ULES-00559",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes [EU]	
	{"ULES-00560",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes [EU] (Platinum)	
	{"ULES-00561",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes [EU] (SP)
	{"ULES-00562",	0,							0, 					0,					buttons,		0 		},	//Medal of Honor Heroes [EU]	
	{"ULUS-10141",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes [US]	
	{"ULJM-05213",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes [JP]	
	{"ULAS-42082",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes []	
	{"ULKS-46066",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes []	
	
	{"ULES-00955",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes 2 [EU]	
	{"ULES-00956",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes 2 [EU]	
	{"ULES-00988",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes 2 [EU]	
	{"ULUS-10310",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes 2 [US]	
	{"ULJM-05301",	0,							0, 					0,					buttons,		0  		},	//Medal of Honor Heroes 2 [JP]
	
	{"ULES-01372",	0,							0, 					0,					buttons,		0  		},	//Metal Gear Solid - Peace Walker [EU]					
	{"ULUS-10509",	0,							0, 					0,					buttons,		0  		},	//Metal Gear Solid - Peace Walker [US]		
	{"NPJH-50045",	0,							0, 					0,					buttons,		0  		},	//Metal Gear Solid - Peace Walker [JP]	

	{"ULES-00375",	0,							0, 					0,					analog_x,		0  		},	//Miami Vice [EU]									OK 		(basically copies 1st analog stick left & right only)
	{"ULES-00376",	0,							0, 					0,					analog_x,		0  		},	//Miami Vice [EU] (russian)							OK	
	{"ULUS-10109",	0,							0, 					0,					analog_x,		0  		},	//Miami Vice [US]									OK
	{"ULJM-05165",	0,							0, 					0,					analog_x,		0  		},	//Miami Vice [JP]									OK
	
	{"ULES-01144",	0,							0, 					0,					dpad,			0  		},	//Midnight Club LA Remix [EU]						OK  	up isn't needed though
	{"ULUS-10383",	0,							0, 					0,					dpad,			0  		},	//Midnight Club LA Remix [US]						OK
	{"ULJS-00180",	0,							0, 					0,					dpad,			0  		},	//Midnight Club LA Remix [JP]						OK
	
	{"ULES-00318", 	0,							0,					0,					dpad,			0  		},	//Monster Hunter Freedom [EU]
	{"ULUS-10084",	0, 							0,					0,					dpad,			0  		},	//Monster Hunter Freedom [US]
	
	{"ULES-00851",	0,							0,					0,					dpad,			0  		},	//Monster Hunter Freedom 2 [EU]
	{"ULUS-10266",	0, 							0,					0,					dpad,			0  		},	//Monster Hunter Freedom 2 [US]
	
	{"ULES-01213",	0, 							0,					0,					dpad,			0  		},	//Monster Hunter Freedom Unite [EU]
	{"ULUS-10391",	0, 							0,					0,					dpad,			0  		},	//Monster Hunter Freedom Unite [US]
	
	{"ULJM-05066",	0,							0,					0,					dpad,			0  		},	//Monster Hunter Portable [JP]
	{"UCKS-45036",	0, 							0,					0,					dpad,			0  		},	//Monster Hunter Portable [??]
	
	{"ULJM-05500", 	0,							0,					0,					dpad,			0  		},	//Monster Hunter Portable 2nd [JP]
	{"ULJM-05156",	0, 							0,					0,					dpad,			0  		},	//Monster Hunter Portable 2nd [JP]
	
	{"ULJM-05800",	0, 							0,					0,					dpad,			0  		},	//Monster Hunter Portable 3rd [JP]
	
	{"UCES-01250",	0,							0, 					0,					dpad,			0  		},	//Motorstorm Arctic Edge [EU]						up isn't needed though
	{"UCUS-98743",	0,							0, 					0,					dpad,			0 		},	//Motorstorm Arctic Edge [US]						up isn't needed though
	{"UCKS-45124",	0,							0, 					0,					dpad,			0 		},	//Motorstorm Arctic Edge [KR]						up isn't needed though
	{"NPEK-00175",	0,							0, 					0,					dpad,			0 		},	//Motorstorm Arctic Edge [??]						up isn't needed though
	
	{"ULES-01340",	0,							0, 					0,					dpad,			0  		},	//Obscure: The Aftermath [EU]						OK
	{"ULUS-10484",	0,							0, 					0,					dpad,			0  		},	//Obscure: The Aftermath [US]						OK
	
	{"ULES-00366",	0,							0, 					0,					triggers_inv,	0  		},	//Pirates of C. Dead Mans chest [EU]			
	{"ULUS-10111",	0,							0, 					0,					triggers_inv,	0  		},	//Pirates of C. Dead Mans chest [US]
	
	{"ULES-00223",	0, 							PSP_CTRL_LTRIGGER,	0,					analog_x_inv,	0  		},	//Prince of Persia - Revelations [EU]				OK	
	{"ULUS-10063",	0, 							PSP_CTRL_LTRIGGER,	0,					analog_x_inv,	0  		},	//Prince of Persia - Revelations [US]				OK
	
	{"ULES-00579",	0, 							PSP_CTRL_LTRIGGER,	0,					analog_x_inv,	0  		},	//Prince of Persia - Rival Swords [EU]				OK	
	{"ULUS-10240",	0, 							PSP_CTRL_LTRIGGER,	0,					analog_x_inv,	0  		},	//Prince of Persia - Rival Swords [US]				OK
	
	{"ULES-00736",	0,							0, 					0,					triggers,		0  		},	//Ratatouille [EU]	
	{"ULUS-10247",	0,							0, 					0,					triggers,		0  		},	//Ratatouille [US]	
	
	{"UCES-00420",	0, 							0,					0,					triggers,		0  		},	//Ratched & Clank Size Matters [EU]					OK
	{"UCUS-98633",	0, 							0,					0,					triggers,		0  		},	//Ratched & Clank Size Matters [US]					OK		
	{"UCJS-10052",	0, 							0,					0,					triggers,		0 		},	//Ratched & Clank Size Matters [JP]					OK		
	{"UCKS-45048",	0, 							0,					0,					triggers,		0  		},	//Ratched & Clank Size Matters []					OK
	
	{"ULES-00123",	0, 							PSP_CTRL_RTRIGGER,	0,					analog_x,		0  		},	//Rengoku: The Tower of Purgatory [EU]
	{"ULUS-10013",	0, 							PSP_CTRL_RTRIGGER,	0,					analog_x,		0  		},	//Rengoku: The Tower of Purgatory [US]
	{"ULJM-05006",	0, 							PSP_CTRL_RTRIGGER,	0,					analog_x,		0  		},	//Rengoku: The Tower of Purgatory [JP]
	
	{"ULES-00437",	0, 							0,					0,					dpad_lr,		0  		},	//Rengoku II: The Stairway to Heaven [EU]
	{"ULUS-10127",	0, 							0,					0,					dpad_lr,		0  		},	//Rengoku II: The Stairway to Heaven [US]
	{"ULJM-05055",	0, 							0,					0,					dpad_lr,		0  		},	//Rengoku II: The Stairway to Heaven [JP]
	
 ///{"UCES-01184",	0,							0, 					0,					buttons,		0  		},	//Resistance Retribution [EU]						OK
 ///{"UCUS-98668",	0,							0, 					0,					buttons,		0  		},	//Resistance Retribution [US]						OK	
 ///{"UCUS-98735",	0,							0, 					0,					buttons,		0  		},	//Resistance Retribution [US]						OK	
 ///{"NPEK-00176",	0,							0, 					0,					buttons,		0  		},	//Resistance Retribution []							OK
 
	{"UCES-00942",	0,							0, 					0,					triggers,		0  		},	//Secret Agent Clank [EU]
	{"UCUS-98697",	0,							0, 					0,					triggers,		0  		},	//Secret Agent Clank [US]
	
	{"ULES-01352",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			0  		},	//Silent Hill Shattered Memories [EU]				OK
	{"ULUS-10450",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			0  		},	//Silent Hill Shattered Memories [US]				OK  	Actually zoom and camera
	{"NPJH-50148",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			0 		 },	//Silent Hill Shattered Memories [JP]				OK
	
	{"ULES-00957",	0,							0, 					0,					dpad_inv,		0  		},	//Skate Park City [EU]								OK
	
	{"UCUS-98615",	PSP_CTRL_LTRIGGER,			0,					0,					analog,			0		},	//SOCOM - U S Navy SEALs - Fireteam Bravo [US]		Press Right D-Pad first to enable free aim mode.
	{"ULES-00038",	PSP_CTRL_LTRIGGER,			0,					0,					analog,			0		},	//SOCOM - U S Navy SEALs - Fireteam Bravo [EU]		Press Right D-Pad first to enable free aim mode.
		
	{"UCUS-98645",	PSP_CTRL_LTRIGGER,			0,					0,					analog,			0		},	//SOCOM - U S Navy SEALs - Fireteam Bravo 2 [US]	Press Right D-Pad first to enable free aim mode.
	{"UCES-00543",	PSP_CTRL_LTRIGGER,			0,					0,					analog,			0		},	//SOCOM - U S Navy SEALs - Fireteam Bravo 2	[EU]	Press Right D-Pad first to enable free aim mode.
		
	{"UCUS-98716",	PSP_CTRL_LTRIGGER,			0,					0,					analog,			0		},	//SOCOM - U S Navy SEALs - Fireteam Bravo 3 [US]	Press Up D-Pad first to enable free aim mode.
	{"UCES-01242",	PSP_CTRL_LTRIGGER,			0,					0,					analog,			0		},	//SOCOM - U S Navy SEALs - Fireteam Bravo 3	[EU]	Press Up D-Pad first to enable free aim mode.
	{"UCJS-10102",	PSP_CTRL_LTRIGGER,			0,					0,					analog,			0		},	//SOCOM - U S Navy SEALs - Fireteam Bravo 3	[JP]	Press Up D-Pad first to enable free aim mode.
	
	{"ULES-00022",	0,							0, 					0,					dpad,			0 		},	//Spiderman 2 [EU]	
	{"ULES-00023",	0,							0, 					0,					dpad,			0  		},	//Spiderman 2 [EU]	
	{"ULUS-10015",	0,							0, 					0,					dpad,			0  		},	//Spiderman 2 [US]	
	{"ULJM-05102",	0,							0, 					0,					dpad,			0  		},	//Spiderman 2 [JP]	
	
	{"ULES-00693",	0,							0, 					0,					buttons_inv,	0  		},	//Spinout [EU]										//Vertigo [US]
	
 ///{"ULES-00281",	0, 							PSP_CTRL_CIRCLE, 	0,					analog,			0  		},	//Splinter Cell Essentials [EU]						OK
 ///{"ULUS-10070",	0, 							PSP_CTRL_CIRCLE, 	0,					analog,			0  		},	//Splinter Cell Essentials [US]						OK
 
	{"ULES-00183",	0,							0, 					0,					buttons,		0  		},	//Star Wars Battlefront 2 [EU]			
	{"ULUS-10053",	0,							0, 					0,					buttons,		0  		},	//Star Wars Battlefront 2 [US]
	
	{"ULES-01214",	0,							0, 					0,					analog_x,		0  		},	//Star Wars BF2 Elite Squadron [EU] 				OK 		(basically copies 1st analog stick left & right only)
	{"ULUS-10390",	0,							0, 					0,					analog_x,		0  		},	//Star Wars BF2 Elite Squadron [US]					OK
	
	{"ULES-00861",	0,							0, 					0,					analog_x,		0  		},	//Star Wars BF2 Renegade Squadron [EU] 				OK 		(basically copies 1st analog stick left & right only)
	{"ULUS-10292",	0,							0, 					0,					analog_x,		0  		},	//Star Wars BF2 Renegade Squadron [US]				OK
	
	{"ULES-00599",	0, 							0,					0,					triggers,		0  		},	//Star Wars: Lethal Alliance [EU]					OK
	{"ULUS-10188",	0, 							0,					0,					triggers,		0  		},	//Star Wars: Lethal Alliance [US]					OK		also has full camera control with: L Trigger + R Trigger + Analog
	
	{"UCES-00310",	0, 							0,					0,					buttons,		0  		},	//Syphon Filter Dark Mirror [EU]
	{"UCUS-98641",	0, 							0,					0,					buttons,		0  		},	//Syphon Filter Dark Mirror [US]
	
	{"UCES-00710",	0, 							0,					0,					buttons,		0  		},	//Syphon Filter Logan's Shadow [EU]
	{"UCUS-98606",	0, 							0,					0,					buttons,		0  		},	//Syphon Filter Logan's Shadow [US]
	
 	{"ULES-00394",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			100000 	},	//The Godfather [EU]								OK	
	{"ULES-00397",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			100000 	},	//The Godfather [EU] (french)						OK	
	{"ULES-00398",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			100000 	},	//The Godfather [EU] (german)						OK	
	{"ULES-00399",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			100000 	},	//The Godfather [EU] (italian)						OK	
	{"ULES-00400",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			100000 	},	//The Godfather [EU] (spanish)						OK	
	{"ULUS-10098",	0, 							PSP_CTRL_RTRIGGER, 	0,					analog,			100000 	},	//The Godfather: Mob Wars [US]						OK
	
	{"ULES-01556",	0, 							0,					0,					triggers_inv,	0  		},	//The Legend Of Heroes Trails in the Sky [EU]
	{"ULUS-10540",	0, 							0,					0,					triggers_inv,	0  		},	//The Legend Of Heroes Trails in the Sky [US]
	{"ULJM-05170",	0, 							0,					0,					triggers_inv,	0  		},	//The Legend Of Heroes Trails in the Sky [JP]
	
	{"NPUH-10191",	0, 							0,					0,					triggers_inv,	0  		},	//The Legend Of Heroes Trails in the Sky 2 [JP] (disc1)
	{"NPUH-10197",	0, 							0,					0,					triggers_inv,	0  		},	//The Legend Of Heroes Trails in the Sky 2 [JP] (disc2)
	
	{"ULES-00978",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog,			0  		},	//The Simpsons The Game [EU]	
	{"ULUS-10295",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog,			0  		},	//The Simpsons The Game [US]
	
 ///{"ULUS-10567",	0, 							0,					0,					dpad,			0  		},	//The 3rd Birthday [US]								ERROR	movement stops working	

	{"ULES-00483",	0, 							PSP_CTRL_LTRIGGER, 	0,					analog,			0  		},	//The Warriors [EU]									OK		using while running will trigger sprint though
	{"ULUS-10213",	0,							PSP_CTRL_LTRIGGER, 	0,					analog,			0  		},	//The Warriors [US]									OK		using while running will trigger sprint though
	
	{"ULES-00770",	0,							0, 					0,					buttons,		0  		},	//Tom Clancy's G.R.A.W. 2 [EU]	
	{"ULUS-10237",	0,							0, 					0,					buttons,		0  		},	//Tom Clancy's G.R.A.W. 2 [US]
	
	{"ULES-01350",	PSP_CTRL_LTRIGGER,			0, 					0,					analog,			0  		},	//Tom Clancys GhostRecon Predator [EU]				OK
	{"ULUS-10445",	PSP_CTRL_LTRIGGER,			0, 					0,					analog,			0  		},	//Tom Clancys GhostRecon Predator [US]				OK 		(basically copies 1st analog stick left & right only)
 
	{"ULES-00584",	0,							0, 					0,					buttons,		0  		},	//Tom Clancy's Rainbow Six Vegas [EU]				OK  	Some graphic glitches in menu with debug monitor
	{"ULUS-10206",	0,							0, 					0,					buttons,		0  		},	//Tom Clancy's Rainbow Six Vegas [US]				OK
	
 ///{"ULES-00826",	0, 							0,					0,					triggers,		0  		},	//Tomb Raider Anniversary [EU]			OK
 ///{"ULUS-10253",	0, 							0,					0,					triggers,		0  		},	//Tomb Raider Anniversary [US]			OK
 ///{"ULJS-00133",	0, 							0,					0,					triggers,		0  		},	//Tomb Raider Anniversary [JP]			OK		also has full camera control with: TRIANGLE + Analog (needs delay though)
	
 ///{"ULES-00283",	0, 							PSP_CTRL_SQUARE, 	0,					analog,			250000 	},	//Tomb Raider: Legends [EU]							OK
 ///{"ULUS-10110",	0, 							PSP_CTRL_SQUARE, 	0,					analog,			250000 	},	//Tomb Raider: Legends [US]							OK
 
	{"ULES-00033",	0,							0, 					0,					analog_x_inv,	0  		},	//Tony Hawk's Underground 2 [EU]
	{"ULES-00034",	0,							0, 					0,					analog_x_inv,	0  		},	//Tony Hawk's Underground 2 [EU]
	{"ULES-00035",	0,							0, 					0,					analog_x_inv,	0  		},	//Tony Hawk's Underground 2 [EU]
	{"ULUS-10014",	0,							0, 					0,					analog_x_inv,	0  		},	//Tony Hawk's Underground 2 [US]
 
	{"ULES-01404",	0,							0, 					0,					triggers,		0  		},	//Toy Story 3 [EU] (russian)
	{"ULES-01405",	0,							0, 					0,					triggers,		0  		},	//Toy Story 3 [EU]
	{"ULES-01406",	0,							0, 					0,					triggers,		0  		},	//Toy Story 3 [EU]
	{"ULUS-10507",	0,							0, 					0,					triggers,		0  		},	//Toy Story 3 [US]

	{"ULUS-10230",	0,							0,					0,					dpad,			0		},	//Valhalla Knights [US]
	{"ULES-00657",	0,							0,					0,					dpad,			0		},	//Valhalla Knights [EU]
	{"ULJM-00075",	0,							0,					0,					dpad,			0		},	//Valhalla Knights [JP]

	{"ULUS-10366",	0,							0,					0,					dpad,			0		},	//Valhalla Knights II [US]
	{"ULES-01265",	0,							0,					0,					dpad,			0		},	//Valhalla Knights II [EU]
	{"ULJS-00138",	0,							0,					0,					dpad,			0		},	//Valhalla Knights II [JP]
	
	{"ULUS-10515",	0,							0,					0,					mix_1,			0		},	//Valkyria Chronicles II [US]
	{"ULES-01417",	0,							0,					0,					mix_1,			0		},	//Valkyria Chronicles II [EU]
	{"ULJM-05560",	0,							0,					0,					mix_1,			0		},	//Valkyria Chronicles II [JP]
	
	{"ULJM-05957",	0, 							0,					0,					buttons,		0  		},	//Valkyria Chronicles 3: Extra Edition [JP]
	
	{"NPUH-10092",	0,							0, 					0,					buttons_inv,	0  		},	//Vertigo [US]										//Spinout [EU]

	{NULL,0,}
};
int gamelistsize = (sizeof(gamelist)/sizeof(Game_pack))-1;

int doesFileExist(const char* path) { 
	SceUID dir = sceIoOpen(path, PSP_O_RDONLY, 0777); 
 	if (dir >= 0) { 
 		sceIoClose(dir); 
 		return 1; 
 	} else { 
 		return 0; 
 	} 
} 

int try_get_game_info() {
	SceUID fd;
	do {
		fd = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777); 
		sceKernelDelayThread(1000);
	} while( fd <= 0 );
	sceIoRead(fd, gameid, 10);	
	sceIoClose(fd);
    return 1;
}

int checkAndProcessGame() {	
	int i;
	for ( i = 0; i < gamelistsize; i++ ) {
		if ( strcmp(gameid, gamelist[i].id) == 0 ) {
			PSP_CTRL_MOVEBUTTON = gamelist[i].movebutton;
			PSP_CTRL_BUTTON	= gamelist[i].button;
			PSP_CTRL_BUTTON_2 = gamelist[i].button2;
			contr = gamelist[i].controls;
			delay = gamelist[i].delay;
			return 1;
		}
	}	
	return 0;
}

int (* sceCtrlReadBuf)(SceCtrlData *pad, int nBufs, int a2, int mode);

int sceCtrlReadBufPatched(SceCtrlData *pad, int nBufs, int a2, int mode) { 
		
	/// A static buffer 
	static char buffer[64 * 48];
	
	/// Set k1 to zero to allow all buttons 
	int k1 = pspSdkSetK1(0);

	/// Read PSP hardware buttons 
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	
	/// Call function with new arguments 
	int res = sceCtrlReadBuf((SceCtrlData *)buffer, nBufs, a2, mode); 
	//int res = sceCtrlReadBuf((SceCtrlData *)buffer, nBufs, 1, mode | 4);	//extended mode
	
	/// Copy buffer to pad
	int i; 
	for(i = 0; i < nBufs; i++) {
		memcpy(&pad[i], buffer + (i * 48), 12); 

		//checks if left analog is used so it can add the left trigger for movement
		if((pad[i].Lx >= 127 + THRESHOLD) | (pad[i].Lx < 127 - THRESHOLD) | (pad[i].Ly >= 127 + THRESHOLD) | (pad[i].Ly < 127 - THRESHOLD)) { 
			pad[i].Buttons |= PSP_CTRL_MOVEBUTTON;
		}
			
		//analog is unused (this is for delay handling)
		if((pad[i].Rx <= 127 + THRESHOLD) && (pad[i].Rx > 127 - THRESHOLD) && (pad[i].Ry <= 127 + THRESHOLD) && (pad[i].Ry > 127 - THRESHOLD)) { 
			time = sceKernelGetSystemTimeLow();
		}
		
		//analog is used
		if((pad[i].Rx >= 127 + THRESHOLD) | (pad[i].Rx < 127 - THRESHOLD) | (pad[i].Ry >= 127 + THRESHOLD) | (pad[i].Ry < 127 - THRESHOLD)) { 
			pad[i].Buttons |= PSP_CTRL_BUTTON; //simulate camera button pressed
			pad[i].Buttons |= PSP_CTRL_BUTTON_2; //simulate camera button2 pressed
			
			if( sceKernelGetSystemTimeLow() >= time + delay ) { //wait delay

				if ( contr == 1 ) { //camera is moved with analog stick
					pad[i].Lx = pad[i].Rx; //horizontal
					pad[i].Ly = pad[i].Ry; //vertical 
					
				} else if ( contr == 2 ) { //camera is moved with analog stick (fully inverted)
					pad[i].Lx = 255-pad[i].Rx; //horizontal
					pad[i].Ly = 255-pad[i].Ry; //vertical 
				
				} else if ( contr == 3 ) { //camera is moved with analog stick (x inverted)
					pad[i].Lx = 255-pad[i].Rx; 	//horizontal
					pad[i].Ly = pad[i].Ry; 		//vertical 
					
				} else if ( contr == 4 ) { //camera is moved with analog stick (y inverted)
					pad[i].Lx = pad[i].Rx; 		//horizontal
					pad[i].Ly = 255-pad[i].Ry; 	//vertical 
					
				} else if ( contr == 5 ) { //camera is moved with analog stick x axis only
					pad[i].Lx = pad[i].Rx; //horizontal
					
				} else if ( contr == 6 ) { //camera is moved with analog stick y axis only
					pad[i].Ly = pad[i].Ry; //vertical 
					
				} else if ( contr == 7 ) { //camera is moved with D-Pad
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_RIGHT; 	//right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_LEFT; 	//left
					if ( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_UP; 		//up
					if ( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_DOWN; 	//down
				
				} else if ( contr == 8 ) { //camera is moved with D-Pad (left & right only)
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_RIGHT; 	//right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_LEFT; 	//left
						
				} else if ( contr == 9 ) { //camera is moved with D-Pad (inverted)
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_LEFT; 	//right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_RIGHT; 	//left
					if ( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_DOWN; 	//up
					if ( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_UP; 		//down
				
				} else if ( contr == 10 ) { //camera is moved with X O /\ []
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_CIRCLE;	//right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_SQUARE; 	//left
					if ( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_TRIANGLE;//up
					if ( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_CROSS; 	//down
				
				} else if ( contr == 11 ) { //camera is moved with X O /\ [] (inverted)
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_SQUARE;	//right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_CIRCLE; 	//left
					if ( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_CROSS; 	//up
					if ( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_TRIANGLE;//down
					
				} else if ( contr == 12 ) { //camera is moved with O [] 
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_CIRCLE;	//right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_SQUARE; 	//left
							
				} else if ( contr == 13 ) { //camera is moved with L&R Triggers
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_RTRIGGER; //right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_LTRIGGER; //left
					
				} else if ( contr == 14 ) { //camera is moved with L&R Triggers (inverted)
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_LTRIGGER; //right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_RTRIGGER; //left

				} else if ( contr == 15 ) { //camera is moved with L&R Triggers + Triangle&Cross
					if ( pad[i].Rx >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_RTRIGGER; //right
					if ( pad[i].Rx <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_LTRIGGER; //left
					if ( pad[i].Ry <= 127 - THRESHOLD ) pad[i].Buttons |= PSP_CTRL_TRIANGLE; //up
					if ( pad[i].Ry >= 127 + THRESHOLD ) pad[i].Buttons |= PSP_CTRL_CROSS; 	 //down
				}
			}
		}
		
		pad[i].Ry = 127;
		pad[i].Rx = 127;
	} 

	pspSdkSetK1(k1); //restore k1
	return res; //return result 
} 


int camera_thread(SceSize args, void *argp) {
	
	sceKernelDelayThread(3000000); //delay 3 sec for game to load
	
	while(!sceKernelFindModuleByName("sceKernelLibrary")) //wait for the kernel to boot up
		sceKernelDelayThread(100000);
	
	try_get_game_info(); //read from umd info file
	
	if ( checkAndProcessGame() ) { //if game(id) supported
		
		SceModule2 *mod = (SceModule2 *)sceKernelFindModuleByName("sceController_Service"); //Find ctrl.prx
			
		int i; ///Find function and hook it
		for(i = 0; i < mod->text_size; i += 4) { 
			u32 addr = mod->text_addr + i; 
				
			if(_lw(addr) == 0x35030104) {
				HIJACK_FUNCTION(addr - 0x1C, sceCtrlReadBufPatched, sceCtrlReadBuf); 
				break; 
			} 
		} 
		
		///Clear caches
		sceKernelDcacheWritebackAll(); 
		sceKernelIcacheClearAll(); 
	}	
	return 0;
}

	
int module_start(SceSize args, void *argp) {
	
	if( doesFileExist("flash1:/config.adrenaline") ) {
		
		SceUID thid = sceKernelCreateThread("camera_thread", camera_thread, 0x18, 0x1000, 0, NULL);
		if(thid >= 0) sceKernelStartThread(thid, args, argp);
	
		sceKernelExitDeleteThread(0);
	}
	
	return 0;
}



