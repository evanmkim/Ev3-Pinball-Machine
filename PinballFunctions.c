
void updateStream(int score, int gyroMultiplier, int ultrasonicMultiplier, int ball)
{
	clearDebugStream();
	writeDebugStream("Score: %d", score);
	writeDebugStreamLine("Round: %d", ball);
	if (gyroMultiplier==3)
		writeDebugStreamLine("!!Spinner worth triple points!!");
	if (ultrasonicMultiplier==3)
		writeDebugStreamLine("!!Paddle worth triple points!!");
	if (ultrasonicMultiplier ==2 && gyroMultiplier ==2 )
		writeDebugStreamLine("!!All points doubled!!");
}


//ADDED sum as parameter
bool incrementScore(int increment, int multiplier, int & score, int & sum)
{
 	bool canScore = true;
	score += increment*multiplier;
	if (increment==100)
		canScore = false;
		//This code checks if ultrasonic sensor has been hit consecutively.
		//If it is hit 5 times consecutively then score is doubled
	sum += increment;
	if (sum % 100 != 0)
		sum = 0;

	if (sum == 500)
		score *=2;

return canScore;
}

//ADDED time as parameter

bool multiplier(int & ultrasonicMultiplier, int & gyroMultiplier, int time)
{


	ultrasonicMultiplier = 1;
	gyroMultiplier = 1;

	//Here time is used to increase odds of multiplier occurring every 20 seconds
	//starts with 2 in 7 odds and increases to 2 in 4
	int odds = 7- time / 20000;
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
	}else
		bonus = false;

	return bonus;
}


// in main


task main()
{
	const int GYROTOL = 3;//score will increment if gyro rotates more than 3 degrees
	const int NORMALDISTANCE = 8 ;// score will increment if ultrasonic distance is greater than 8 cm
	const int ULTRASONIC = S2, GYRO = S3;
	const int ULTRASONICVALUE = 100;// ultrasonic increments score by 100 as default
	const int GYROVALUE = 10; // gyro increments score by 10
	int gyroMultiplier = 0, ultrasonicMultiplier = 0, score = 0, ball = 0, previousGyro = 0, period = 20;
	bool bonus = false;
	bool canScore = true;
	int sum = 0;


	time1[T1]=0;
	while (true)
{
	//canScore must be reset to true every time paddles are used or every couple seconds
if (SensorValue[ULTRASONIC] > NORMALDISTANCE && canScore == true)// canScore boolean ensures that while paddle is pressed score is only incremented once
{
	canScore = incrementScore(ULTRASONICVALUE, ultrasonicMultiplier, score, sum);
	updateStream(score, gyroMultiplier, ultrasonicMultiplier, ball);
}

if (abs(SensorValue[GYRO] - previousGyro)>GYROTOL && canScore == true )//canScore allows gyro to continuously increment score while it is spinning
{
	canScore = incrementScore(GYROVALUE, gyroMultiplier, score, sum);
	previousGyro = SensorValue[GYRO];
	updateStream(score, gyroMultiplier, ultrasonicMultiplier, ball);
}
//possibilty of double or triple score every 10 seconds
if (time1[T1] > period*1000)
{
	period +=10;
	//added time as parameter
	bonus = multiplier(ultrasonicMultiplier, gyroMultiplier, time1[T1]);
	updateStream(score, gyroMultiplier, ultrasonicMultiplier, ball);

}


}



}
