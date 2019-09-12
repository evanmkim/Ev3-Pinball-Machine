#include "EV3_FileIO.c"
//const for functions
const int MAXLEADER = 10;
const int MAXNAME = 30;
const int NAMENUM = 3;
const int ALPHANUM = 26;



//Get remote button click
int buttonPress()
{
	int button=0;

	while(!getIRRemoteButtons(S4))
	{}
	button=getIRRemoteButtons(S4);
	while(getIRRemoteButtons(S4))
	{}

	return(button);
}

//Play Sound Effects (sleep and const&)
void soundEffect(int position)
{
	setSoundVolume(100);
	if(position==1)
	{
		playSoundFile("One.rsf");
		playSoundFile("Fanfare");
		sleep(2000);
	}
	else if(position==2)
	{
		playSoundFile("Two.rsf");
		sleep(2000);
	}
	else if(position==3)
	{
		playSoundFile("Three");
		sleep(2000);
	}
	else if(position==4)
	{
		playSoundFile("Four");
		sleep(2000);
	}
	else if(position==5)
	{
		playSoundFile("Five");
		sleep(2000);
	}
}

//Fill leaderboard array
void fillLeaderboard(int * leader,char * name, TFileHandle & fin)
{
	int position=0, score=0;
	char FI=' ',MI=' ',LI=' ';
	while(readCharPC(fin,FI)&&readCharPC(fin,MI)&&readCharPC(fin,LI)&&readIntPC(fin,score))
	{
		name[3*position]=FI;
		name[3*position+1]=MI;
		name[3*position+2]=LI;
		leader[position]=score;
		position++;
	}
}

//Display leaderboard
void displayLeaderboard(int * leaderScore, char * leaderName)
{
	clearDebugStream();
	writeDebugStreamLine("Pinball Game Leaderboard");
	for(int i=0;i<MAXLEADER;i++)
	{
		writeDebugStreamLine("%d. %c%c%c   %d",i+1,leaderName[3*i],leaderName[3*i+1],leaderName[3*i+2],leaderScore[i]);
	}
	writeDebugStream("Click Down Blue to go back to Menu");
}

//Change leaderboard and return position on board
void changeLeaderboard(int * leaderScore,char * leaderName, int const & score, char * namePlayer)
{
	int index=-1;

	//Check which spot it can go in
	for(int i=MAXLEADER-1;i>=0;i--)
	{
		if(leaderScore[i]<=score)
			index=i;
	}
	//If it found spot in top 10
	if(index!=-1)
	{
		for(int i= MAXLEADER-1;i>index;i--)
		{
			leaderScore[i]=leaderScore[i-1];
			leaderName[3*i]=leaderName[3*i-3];
			leaderName[3*i+1]=leaderName[3*i-2];
			leaderName[3*i+2]=leaderName[3*i-1];
		}
		leaderScore[index]=score;
		leaderName[3*index]=namePlayer[0];
		leaderName[3*index+1]=namePlayer[1];
		leaderName[3*index+2]=namePlayer[2];
		soundEffect(index+1);
	}
}

void updateLeaderFile(int * leaderScore, char * leaderName, TFileHandle & fout)
{
	for(int i=0;i<MAXLEADER;i++)
	{
		writeCharPC(fout,leaderName[3*i]);
		writeCharPC(fout,leaderName[3*i+1]);
		writeCharPC(fout,leaderName[3*i+2]);
		writeTextPC(fout," ");
		writeLongPC(fout,leaderScore[i]);
		writeTextPC(fout," ");
	}
}

//Print rules to computer display
void displayRules()
{
	TFileHandle fin;
	bool fileOkay = false;
	fileOkay=openReadPC(fin,"Rules.txt");

	if(fileOkay)
	{
		clearDebugStream();
		string word1;
		writeDebugStreamLine("Pinball Game Rules");
		while(readTextPC(fin,word1))
		{
			if(word1=="n")
				writeDebugStreamLine("");
			else
				writeDebugStream(" %s",word1);
		}
		writeDebugStream("Click Down Blue to go back to Menu");
	}
	closeFilePC(fin);
}



void runMenu(int * leaderScore, char *leaderName)
{
	int button=0;
	while(button!=2)//Down Red
	{
		clearDebugStream();
		writeDebugStreamLine("Welcome to Pinball");
		writeDebugStreamLine("");
		writeDebugStreamLine("Up Red for Rules");
		writeDebugStreamLine("Up Blue for Leaderboard");
		writeDebugStreamLine("Down Red to Start");
		button=buttonPress();
		if(button==1)//Up Red
		{
			displayRules();
			while(button!=4)
				button=buttonPress();
		}
		else if(button==3)//Up Blue
		{
			displayLeaderboard(leaderScore,leaderName);
			while(button!=4)
					button=buttonPress();
		}
	}
}


//Menu
task main()
{
	SensorType[S4]=sensorEV3_IRSensor;

	//Open files
	bool okay1=false, okay2=false;
	TFileHandle fin;
	okay1=openReadPC(fin,"LBoard.txt");
	TFileHandle fin1;
	okay2=openReadPC(fin1,"Rules.txt");

	//Files open
	if(okay1&&okay2)
	{
		//Enter name
		char namePlayer[NAMENUM]={' ',' ',' '};
		char alphabet[ALPHANUM]={'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};
		for(int i=0;i<3;i++)
		{
			int index=0,button=0;
			while(button!=4)
			{
				clearDebugStream();
				writeDebugStreamLine("Name: %c%c%c",namePlayer[0],namePlayer[1],namePlayer[2]);
				writeDebugStreamLine("Change letter with Up/Down Red and select with Blue");
				writeDebugStreamLine("Name is 3 letters long.");
				writeDebugStreamLine("%c",alphabet[index]);
				button=buttonPress();
				if(button==1&&alphabet[index]=='A')
					index=25;
				else if(button==1)
					index--;
				else if(button==2&&alphabet[index]=='Z')
					index=0;
				else if(button==2)
					index++;
			}
			namePlayer[i]=alphabet[index];
		}
		clearDebugStream();
		writeDebugStreamLine("Your name is: %c%c%c",namePlayer[0],namePlayer[1],namePlayer[2]);
		wait1Msec(2000);


		//Leaderboard
		int leaderScore [MAXLEADER]={0,0,0,0,0,0,0,0,0,0};
		char leaderName[MAXNAME]={' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
		fillLeaderboard(leaderScore,leaderName,fin);
		closeFilePC(fin);

		int score =1000;
		changeLeaderboard(leaderScore,leaderName,score,namePlayer);

		//Menu
		runMenu(leaderScore,leaderName);
		closeFilePC(fin1);

		//New file
		bool fileOkay=false;
		TFileHandle fout;
		fileOkay=openWritePC(fout,"LBoard.txt");

		if(fileOkay)
		{
			updateLeaderFile(leaderScore,leaderName,fout);
		}
		closeFilePC(fout);
	}

}
