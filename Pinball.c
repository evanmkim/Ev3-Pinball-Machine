#include "EV3_FileIO.c"

/*
Notes:
motorA->right paddle
motorB->launcher
motorC->left paddle
motorD->obstacle

sensor1->gyro
sensor2->ultrasonic
sensor3->color
sensor4->infrared

timer1->obstacle
timer2->launcher
timer3->score multiplier
timer4->time check for successful launch
*/

const int LAUNCH_MAX = 3240; // Encoder distance to launch the ball
const int PADDLE_MAX = 20; // Encoder for paddle to swing 90 degrees
const int GYROTOL = 3; // Score will increment if gyro rotates more than
const int GYROVALUE = 25; // Score increment for gyro
const float NORMALDISTANCE = 50; // Score will increment if ultrasonic reads more than
const int ULTRASONICVALUE = 100; // Score increment for ultrasonic
const int MAXLEADER = 10; // Leaderboard will include a max of 10 players
const int MAXNAME = 30; // The array for names (10) will include 30 characters
const int NAMENUM = 3; // 3 Characters (initials) per name
const int ALPHANUM = 26; // Number of characters that user can select from to enter their initials

// Paddle Up
void up(int port, int limit, int speed)
{
	if(nMotorEncoder[port]<limit)
		motor[port]=speed;
	else
		motor[port]=0;
}

// Paddle Down
void down(int port, int limit)
{
	if(nMotorEncoder[port]>limit)
		motor[port]=-100;
	else
		motor[port]=0;
}

// Reset Position
void reset(int port, int speedA, int speedB, int max)
{
	bool moving=true;
	motor[port]=speedA;
	wait1Msec(500);
	while(moving)
	{
		if(getMotorRPM(port)<60)
			moving=false;
	}
	resetMotorEncoder(port);
	playSound(soundBlip);
	if(max<0)
		while(nMotorEncoder[port]>max)
			motor[port]=speedB;
	else
		while(nMotorEncoder[port]<=max)
			motor[port]=speedB;
	motor[port]=0;
	resetMotorEncoder(port);
}

//Display Points and Current Round
void updateStream(int score, int gyroMultiplier, int ultrasonicMultiplier, int ball)
{
	clearDebugStream();
	writeDebugStream("Score: %d", score);
	writeDebugStream(" ");
	writeDebugStreamLine("Round: %d", ball);
	if (gyroMultiplier==3)
		writeDebugStreamLine("!! Spinner worth triple points !!");
	if (ultrasonicMultiplier == 3)
		writeDebugStreamLine("!! Paddle worth triple points !!");
	if (ultrasonicMultiplier == 2 && gyroMultiplier == 2 )
		writeDebugStreamLine("!! All points doubled !!");
}

//Play Sound Effects
void soundEffect(int position)
{
	setSoundVolume(100);
	if(position==1)
	{
		playSoundFile("One.rsf");
		playSoundFile("Fanfare");
	}
	else if(position==2)
		playSoundFile("Two.rsf");
	else if(position==3)
		playSoundFile("Three");
	else if(position==4)
		playSoundFile("Four");
	else if(position==5)
		playSoundFile("Five");
	//Get a Point
	else if(position==6)
		playSoundFile("Sonar");
	//Start
	else if(position==7)
		playSoundFile("Start");
	//Game Over
	else if(position==8)
		playSoundFile("Game over");
}

//Increment Score
bool incrementScore(int increment, int multiplier, int & score, int & sum)
{
	soundEffect(6);
 	bool canScore = false;
	score += increment*multiplier;
	//Checks if ultrasonic sensor has been hit consecutively.
	//If it is hit 5 times consecutively then score is doubled
	sum += increment;
	if (sum % 100 != 0)
		sum = 0;
	if (sum == 500)
		score *=2;
	return canScore;
}

//Random Multiplier
bool multiplier(int & ultrasonicMultiplier, int & gyroMultiplier, int time)
{
	ultrasonicMultiplier = 1;
	gyroMultiplier = 1;
	//Here time is used to increase odds of multiplier occurring every 20 seconds
	//starts with 2 in 7 odds and increases to 1 in 2
	int odds = 7 - time / 20000;
	if (time > 60000)
		odds = 4;
	int num = rand() % odds;
	bool bonus = true;
	if (num == 0)
		gyroMultiplier = 3;
	else if (num == 1)
		ultrasonicMultiplier = 3;
	else if (num == 2 && rand() % 3 == 0)
	{
		ultrasonicMultiplier = 2;
		gyroMultiplier = 2;
	}
	else
		bonus = false;
	return bonus;
}

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

//Fill leaderboard array
void fillLeaderboard(int * leader,char * name, TFileHandle & fin)
{
	int position=0, score=0;
	//First Initial, Middle Initial, Last Initial
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
	writeDebugStream("Down Blue to close Leaderboard");
}

//Change leaderboard and return position on board
void changeLeaderboard(int * leaderScore,char * leaderName, int const & score, char * namePlayer, int rType)
{
	int index=-1;
	//Check which spot it can go in
	for(int i=MAXLEADER-1;i>=0;i--)
	{
		if(leaderScore[i]<=score)
			index=i;
	}
	//If it found spot in top 10
	//If end of game and change leaderboard permanently
	if(index!=-1&&rType==0)
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
	}
	if(rType==1)
		soundEffect(index+1);
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
		writeDebugStream("Click DOWN BLUE to close Rules");
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
		writeDebugStreamLine("UP RED for Rules");
		writeDebugStreamLine("UP BLUE for Leaderboard");
		writeDebugStreamLine("DOWN RED to Start");
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


task main()
{
	SensorType[S1] = sensorEV3_Gyro;
	SensorMode[S1] = modeEV3Gyro_Angle;
	SensorType[S2] = sensorEV3_Ultrasonic;
	SensorMode[S2] = modeEV3Ultrasonic_Cm;
	SensorType[S3] = sensorEV3_Color;
	wait1Msec(50);
	SensorMode[S3] = modeEV3Color_Color;
	wait1Msec(50);
	SensorType[S4] = sensorEV3_IRSensor;

//Open files
	bool okay1=false, okay2=false;
	TFileHandle fin;
	okay1=openReadPC(fin,"LBoard.txt");
	TFileHandle fin1;
	okay2=openReadPC(fin1,"Rules.txt");

//Leaderboard
	int leaderScore [MAXLEADER]={0,0,0,0,0,0,0,0,0,0};
	char leaderName[MAXNAME]={' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};
	fillLeaderboard(leaderScore,leaderName,fin);
	closeFilePC(fin);

// Initialize arrays
	char namePlayer[NAMENUM]={' ',' ',' '};
	char alphabet[ALPHANUM]={'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};

//Files open
	if(okay1&&okay2)
	{
	//Enter name
		for(int i=0;i<3;i++)
		{
			int index=0,button=0;
			while(button!=4)
			{
				clearDebugStream();
				writeDebugStreamLine("Name: %c%c%c",namePlayer[0],namePlayer[1],namePlayer[2]);
				writeDebugStreamLine("Change letter with UP/DOWN RED and select with DOWN BLUE");
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
	//Menu
		runMenu(leaderScore,leaderName);
		closeFilePC(fin1);
		clearDebugStream();
	}

	bool again = true;
	do
	{
		int ball = 0, score = 0;
		int obstaclePower = 20, direction=1;
		int gyroMultiplier = 1, ultrasonicMultiplier = 1, previousGyro = 0, period = 20, sum = 0;
		bool launch = false, bonus = false;

		clearTimer(T1);
		clearTimer(T3);
		clearTimer(T4);

		reset(motorB, 25, -100, -4100);
		reset(motorA, -25, 0, 0);
		reset(motorC, -25, 0, 0);

		clearTimer(T2);
		//Say START
		soundEffect(7);
		while(ball <= 3)
		{
			bool canScore = true;
		//right paddle
			if(getIRRemoteButtons(S4)==3)
				up(motorA, PADDLE_MAX,100);
			else
				down(motorA, 0);
			if(getIRRemoteButtons(S4)==4)
				reset(motorA, -25, 0, 0);
		//left paddle
			if(getIRRemoteButtons(S4)==1)
				up(motorC, PADDLE_MAX,100);
			else
				down(motorC, 0);
			if(getIRRemoteButtons(S4)==2)
				reset(motorC, -25, 0, 0);
		//launch
			if(SensorValue[S3]==7)
				clearTimer(T2);
			if(time1[T2]>=3000)
			{
				clearTimer(T2);
				launch=true;
				if(time1(T4)>6000)
					ball++;
				clearTimer(T4);
				obstaclePower=20;
				changeLeaderboard(leaderScore,leaderName,score,namePlayer,1); //Get position
			}
			if(launch && nMotorEncoder[motorB] > -LAUNCH_MAX)
			{
				motor[motorB] = -100;
			}
			else
			{
				motor[motorB] = 0;
				resetMotorEncoder(motorB);
				launch = false;
			}
		// obstacle
			if(time1[T1] >= 10000 && obstaclePower < 100)
			{
				obstaclePower += 10;
				direction = -direction;
				clearTimer(T1);
			}
			motor[motorD]=direction*obstaclePower;
		// scoring
			if (SensorValue[S2] > NORMALDISTANCE && canScore == true)// canScore boolean ensures that while paddle is pressed score is only incremented once
			{
				while (SensorValue[S2] > NORMALDISTANCE)
				{}
				canScore = incrementScore(ULTRASONICVALUE, ultrasonicMultiplier, score, sum);
				updateStream(score, gyroMultiplier, ultrasonicMultiplier, ball);
			}
			if (abs(SensorValue[S1] - previousGyro) > GYROTOL && canScore)//canScore allows gyro to continuously increment score while it is spinning
			{
				canScore = incrementScore(GYROVALUE, gyroMultiplier, score, sum);
				previousGyro = SensorValue[S1];
				updateStream(score, gyroMultiplier, ultrasonicMultiplier, ball);
			}
		//possibilty of double or triple score every 10 seconds
			if (time1[T3] > period*1000)
			{
				period +=10;
				bonus = multiplier(ultrasonicMultiplier, gyroMultiplier, time1[T1]);
				updateStream(score, gyroMultiplier, ultrasonicMultiplier, ball);
			}
		}
		//Say GAME OVER
		soundEffect(8);
		motor[motorB] = 0;
		motor[motorD] = 0;

		changeLeaderboard(leaderScore,leaderName,score,namePlayer,0);

		//New file
			bool fileOkay=false;
			TFileHandle fout;
			fileOkay=openWritePC(fout,"LBoard.txt");

			if(fileOkay)
			{
				updateLeaderFile(leaderScore,leaderName,fout);
			}
			closeFilePC(fout);


		//Output leaderboard and score
		int button1;
		button1=0;
		displayLeaderboard(leaderScore,leaderName);
		writeDebugStreamLine("");
		writeDebugStreamLine("Your score: %d", score);
		while(button1!=4)
		{
			button1=buttonPress();
		}
		//Play Again
		clearDebugStream();
		writeDebugStreamLine("Play Again? DOWN RED to play again and DOWN BLUE to exit");
		while(SensorValue[S4]!=4&&SensorValue[S4]!=2)
		{}

		//Exit
		if(SensorValue[S4]==4)
			again=false;
		clearDebugStream();
	}while(again);
}
