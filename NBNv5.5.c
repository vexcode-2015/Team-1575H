#pragma config(I2C_Usage, I2C1, i2cSensors)
#pragma config(Sensor, in1,    battery,        sensorAnalog)
#pragma config(Sensor, in2,    intake,         sensorLineFollower)
#pragma config(Sensor, in3,    gyro,           sensorGyro)
#pragma config(Sensor, dgtl1,  limit1,         sensorTouch)
#pragma config(Sensor, dgtl2,  limit2,         sensorTouch)
#pragma config(Sensor, dgtl7,  flywheel,       sensorQuadEncoder)
#pragma config(Sensor, dgtl9,  BaseEncoderR,   sensorQuadEncoder)
#pragma config(Sensor, dgtl11, BaseEncoderL,   sensorQuadEncoder)
#pragma config(Sensor, I2C_1,  ,               sensorQuadEncoderOnI2CPort,    , AutoAssign )
#pragma config(Motor,  port2,           intake,        tmotorVex393HighSpeed_MC29, openLoop)
#pragma config(Motor,  port3,           launcher4,     tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port4,           launcher3,     tmotorVex393_MC29, openLoop, encoderPort, I2C_1)
#pragma config(Motor,  port5,           frontright,    tmotorVex393HighSpeed_MC29, openLoop, reversed)
#pragma config(Motor,  port6,           frontleft,     tmotorVex393HighSpeed_MC29, openLoop)
#pragma config(Motor,  port7,           launcher1,     tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port8,           launcher2,     tmotorVex393_MC29, openLoop, reversed)
#pragma config(Motor,  port9,           intake2,       tmotorVex393HighSpeed_MC29, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#pragma platform(VEX)

//Competition Control and Duration Settings
#pragma competitionControl(Competition)


#include "Vex_Competition_Includes.c"   //Main competition background code...do not modify!
#include "gyro.c" // QCC2 Gyro library
//////////////////////////////////////////////////////////////////////////////////////////////////////
//Autonomous Variables
int autopick = 0;//global autonomous slelction variable used in both the LCD and task "autonomous".
int right = 1;
int left = -1;
int shots = 0;//used for automated firing in Autonomous
//Battery Voltage Code
float battery1 =  nImmediateBatteryLevel/1000.0;//These are variables used for the displaying of values on the LCD screen.
float battery2 =  SensorValue[battery]/282.0;//power expander battery
string battstring = "";//string used for LCD printouts of battery voltages
string screen[8] = {"Blue Outer 1","Blue Outer 2","Blue Inner","Blue Outer","Autonomous 5","Autonomous 6","GoToGoal","ProgramSkills"};/*Autonomous Names*/
/////////////////////////////////////////////////////////////////////////////////////////////////////
//Autonomous Functions
task TBHFlywheel();

void pre_auton()
{
	bLCDBacklight = true;//turn on LCD light so we can see it.
	bStopTasksBetweenModes = false;//prevent cortex from killing the pretty tasks which make the robot work
	startTask(TBHFlywheel);//begin flywheel speed control
	clearLCDLine(0);
	clearLCDLine(1);
	clearDebugStream();
	 SensorType[in3] = sensorNone;
	wait1Msec(500);
	SensorType[in3] = sensorGyro;

}
void updatescreen()//This function updates the LCD screen with the values compiled during Driver Control.
{
	displayLCDCenteredString(0, screen[autopick]);//updates upper row of LCD with autonomous selections
	displayLCDCenteredString(1, battstring);//updates bottom row with battery voltages.
}
void fSpeedControl(int omega, int step = 2)//Flywheel speed control based on motor values with slew rate.
{
	int currentSpeed = motor[launcher1];//get the current motor value.
	if(currentSpeed < omega)//if less than omega, increment slowly
	{
		motor[launcher1] = currentSpeed+step;
		motor[launcher2] = currentSpeed+step;
		motor[launcher3] = currentSpeed+step;//step allows us to change how quickly to spin up.
			motor[launcher4] = currentSpeed+step;
		if((omega - motor[launcher1])<=0)//when greater than or equal to, set equal to omega, and update current speed
		{
			motor[launcher1] = omega;
				motor[launcher2] = omega;
			motor[launcher3] = omega;
				motor[launcher4] = omega;
			currentSpeed = omega;
		}
	}
	if(currentSpeed > omega)//if greater than omega, decrease slowly
	{
		motor[launcher1] = currentSpeed-step;
			motor[launcher2] = currentSpeed-step;
		motor[launcher3] = currentSpeed-step;
			motor[launcher4] = currentSpeed-step;
		if((omega-motor[launcher1])>=0)//when less than or equal to, set equal to omega, and update current speed
		{
			motor[launcher1] = omega;
				motor[launcher2] = omega;
			motor[launcher3] = omega;
				motor[launcher4] = omega;
			currentSpeed = omega;
		}
	}
}
//These variables are global so we can see them in the debugger.
//They are used for the flywheel speed control and allow the
//flywheel to shoot the same distance despite battery voltage
//with the use of a TBH+Integral Velocity control algorithm.

int setRPM = 0;//The speed we set in drivermode or wherever else.
int error = 0;//The error in flywheel velocity (set - actual)
float speed = 0.0;//the speed that the motors are set to; gets error added to it to change speed;
//speed is seperate from the motor assignment, which is an integer, otherwise all decimals would be truncated and unless a massive error
//was generated, then the flywheel would always under or overshoot.
int InteRPM = 0;//The actual RPM as calculated by calcRPM task
int QuadRPM = 0;//this is RPM measurement from the Quadencoder directly geared down from the flywheel.
int filteredQuadRPM = 0;
int filteredInteRPM = 0;//An averaged rpm between 3 consecutive values which filters out the high frequency noise that comes from RPM measurement
//slows down actual measurement change, but reduces instability and increases accuracy.
int CompRPM = 0;//experimental complimentary filter for RPM. uses IMe in conjunction with Quad to provide better estimate
//for RPM over a given period, and reduces the innaccuracy of other filters.
task calcRPM()
{
	int count = 0;
	int prevIntegrated = 0;//previous postion of the encoder to calculate change in distance
	int prevQuadrature = 0;
	while(true)//run forever
	{
		prevIntegrated = nMotorEncoder[launcher3];//set previous position to position to have a reference point
		prevQuadrature = SensorValue[flywheel];
		delay(15);//wait for encoder to increase in value
		InteRPM = 35 * getMotorVelocity(launcher3);
		//InteRPM = (nMotorEncoder[launcher3]- prevIntegrated)/ 627.2 / 25.0 * 1000.0 * 60.0 * 35.0;//calculate the RPM in a float to save decimal places during calculation

		QuadRPM = (SensorValue[flywheel] - prevQuadrature)/ 360.0 / 15.0 * 1000 * 60 * 5.0;
		filteredInteRPM = (filteredInteRPM*5 + InteRPM)/6;//exponential moving average rpm and store in global variables for TBHFlywheel to see.
		filteredQuadRPM = (filteredQuadRPM*3 + QuadRPM)/4;
		CompRPM = (.85 * filteredQuadRPM) + (.15* InteRPM);
	}
}
int speedapprox = 0;
task TBHFlywheel()//Take Back Half controller with Integral accumulator for live adjustment
{//Used for the first time successfully after a series of breakthroughs using the graphing and analysis ability
	//through debug stream and tools in Excel.
	//Finally got RPM calculation right for both encoders, and also got TBH to work over snow break from 1-20 to 1-29

	//motor[launcher4] = 0;
	motor[launcher3] = 0;
	motor[launcher2] = 0;//stop flywheel ahead of time.
	motor[launcher1] = 0;

	nMotorEncoder[launcher3] = 0;
	SensorValue[flywheel] = 0;//clear flywheel encoder so prevent random data from causing error.
	startTask(calcRPM);//start RPM calculation

	float gain =0.001;// 0.00050;//0.00013;//0.00018;//gain for integral causes aggressive recovery of RPM after firing.

	float tbh = 0.0;//TBH is in a decimal to scale better and have lower gain.
	float power = 0.0;//power is multiplied by the max power to get the motor speed

	bool firstcross = true;//checking if the launcher needs to use the approximate speed
	int preverror = 0;//previous error for checking if the sign of the error has changed
	int slewrate = 10;//default slew rate for checking if the change in speed is too great for the launcher to handle
	int del = 30;//delay for loop.
	while(true)
	{
		if(CompRPM<0)
		{
			CompRPM = 0;
		}
		else
		{
			error = setRPM - filteredQuadRPM;//CompRPM;//calculate error; use quad encoder because it responds semi-accurate and quickly
		}
		if(setRPM  ==0)//prevent the low motor powers which can't break friction from running the motors when spinning down
		{
				speed = 0;
				//power = 0;
		}
		else if(error>0)
		{
			speed  = 120;
		}
		else if (error<0)
		{
			speed = 30;
		}

			motor[launcher4] = speed;
			motor[launcher3] = speed;//set the speed for the motors
			motor[launcher2] = speed;
			motor[launcher1] = speed;

		writeDebugStreamLine("%d,%4d,%4d,%4d,%3d,%4d",nSysTime,setRPM,QuadRPM,filteredInteRPM,speed,CompRPM);
		//^prints data to debug stream for graphing and analysis of the flywheel's performance.
		//enables easier tuning and much quicker adjustment, since it logs all flywheel activities.
		delay(del);
		if(setRPM ==0)//if spinning down, decrease the delay and decrease slew rate to make the flywheel spin down without breaking motors.
		{//edit: no longer need to be as safe on slowdown because of ratchets in flywheel gearboxes preventing the inertia from killing motors.
			del = 30;
			slewrate = 20;
		}
		else//otherwise a default slewrate and delay
		{
			del = 50;
			slewrate = 25;
		}
		if(CompRPM>200)
		{
			playTone((CompRPM*0.9),3.5);
		}
	}
}
void clear()//resets encoders for other functions using the base
{
	SensorValue[BaseEncoderL] = 0;
	SensorValue[BaseEncoderR] = 0;
}
void stopbase()//stops the base motors; for autonomous use. The only function that has remained the same
{		//between robots and years.
	motor[frontright] = 0;
	motor[frontleft] = 0;
}
void basecontrol(int speed, int distance)//PDI based encoder forward and backward control.
{//Proportional, Derivative, and Inverse control, allows correction of error, and differences in speeds of two sides
	float pBase = .005;//gain values for the PDI sums
	float dBase = .04;//needs to be tuned.
	float iBase = 10;//inverse. Gives larger value to slower base, thus speeding them up and keeing the robot in a
	clear();//straight line.
	int error = 0;
	float baseDerivativeL = 0;
	float baseDerivativeR = 0;
	float speedcurveL = 1;
	float speedcurveR = 1;
	int prevErrorL = 0;
	int prevErrorR = 0;
	int errorR = 0;
	int errorL = 0;
	int deltaT = 30;//time is constant because the delay between cycles is constant.
	while(abs(error) >10)
	{
		error = distance -((abs(SensorValue[BaseEncoderL] + SensorValue[BaseEncoderR])/2));//calculate average error
		errorR = distance - SensorValue[BaseEncoderR];//calculate errors of the individual sides for speed reasons
		errorL = distance - SensorValue[BaseEncoderL];
		baseDerivativeR = (errorR - prevErrorR)/deltaT;//calculate speed
		baseDerivativeL = (errorL - prevErrorL)/deltaT;
		prevErrorL = errorL;//reset previous error so the system can measure displacement and error
		prevErrorR = errorR;
		//PDI Sums gain times terms to get a decimal which allows the speed to change over time.
		//our third term is the inverse of the derivative of the same side, which allows us to
		speedcurveR = (pBase*errorR) + dBase*baseDerivativeR + (iBase*(1/baseDerivativeR));//drive straight because the side
		speedcurveL = (pBase*errorL) + dBase*baseDerivativeL + (iBase*(1/baseDerivativeL));//moving faster will get less speed
		if(speedcurveL>1)//than the other side, which will recieve more because the reciprocal of the lower speed is larger
		{
			speedcurveL = 1;
		}
		else if(speedcurveL<-1)//constrain magnitude of speed
		{
			speedcurveL = -1;
		}
		if(speedcurveR>1)
		{
			speedcurveR = 1;
		}
		else if(speedcurveR<-1)
		{
			speedcurveR = 1;
		}

		motor[frontright] = speedcurveR * speed;//assign motor values
		motor[frontleft] = speedcurveL * speed;
		delay(30);
	}
	motor[frontright] = 0;//stop motors
	motor[frontleft] = 0;

}
void EncoderBase(int speed, int distance)//Encoder based distance tracking(simple)
{
	clear();
	while(abs(SensorValue[BaseEncoderL]) < distance && abs(SensorValue[BaseEncoderR]) < distance)
	{
		motor[frontright] = speed;
		motor[frontleft] = speed;
	}
	stopbase();
}
void EncoderTurn(int speed, int distance, int direction)//encoder based turns.
{
	clear();
	float gain = 0.5;
	int error = 0;
	if(direction ==left)
	{
		while(SensorValue[BaseEncoderL] > -distance && SensorValue[BaseEncoderR] < distance)
		{
			error =  distance - SensorValue[BaseEncoderR];
			motor[frontright] = error*gain;
			motor[frontleft] = -error *gain;
		}
	}
	else if(direction==right)
	{
		while(SensorValue[BaseEncoderL] < distance && SensorValue[BaseEncoderR] > -distance)
		{
			error =  distance - SensorValue[BaseEncoderR];
			motor[frontright] = -error*gain;
			motor[frontleft] = speed*gain;
		}
	}
	stopbase();
}
void GyroTurn(int speed, int theta,int direction)//gyro turn does not work because gyro does not work right
{
	SensorValue[gyro] = 0;
	int gyrobegin = 0;
	if(direction ==left)
	{
		gyrobegin = SensorValue[gyro] +theta;
		while(SensorValue[gyro] < gyrobegin)
		{
			motor[frontleft] = -speed;
			motor[frontright] = speed;
		}
	}
	if(direction ==right)
	{
		gyrobegin = SensorValue[gyro]-theta;
		while(SensorValue[gyro] > gyrobegin)
		{
			motor[frontleft] = speed;
			motor[frontright] = -speed;
		}
	}
	stopbase();
}
void autoIntake(int speed,int roller = 0,int programming = 0)
{
	clearTimer(T1);
	if(roller == 0)//both rollers
	{
		do{
			motor[intake] = speed-40;//moves until the limit switch is pressed.
			motor[intake2] = speed;
			//wait1Msec(300);
			if(!programming)// if not programming skills time-out if no fire in 2 seconds.
			{
				if(time1(T1) > 3000)
				{
					break;
				}
			}
		}while(SensorValue[Intake] > 2000);//intake while limit witch is not pressed to get ball ready to fire
			motor[intake] = 0;// and to eliminate the need for wait statements.
		motor[intake2] = 0;
	}
	else if (roller == 1)//front roller
	{
		//do{
		motor[intake] = speed-20;
		//motor[intake2] = speed;
		wait1Msec(700);
		//}while(SensorValue[limit1] != 1);
		motor[intake] = 0;
		motor[intake2] = 0;
	}
	else if(roller ==2)//back roller
	{
		do{
			//motor[intake] = speed-20;
			motor[intake2] = speed;
			wait1Msec(300);
			if(time1(T1) > 2500)
			{
				break;
			}
		}while(SensorValue[limit1] != 1);
		motor[intake] = 0;
		motor[intake2] = 0;
	}
}
void autofire(int shots)
{
	 int shot =0;
	bool newball = true;
	bool oldball = false;
	bool firing = false;
	while(shot < shots)//run while less than n shots
		{
			if(newball && SensorValue[intake] >2500)
			{
				motor[intake] = 50;
				motor[intake2] = 110;
			}
			else if(newball && SensorValue[intake]<2500)
			{
				motor[intake] = 0;
				motor[intake2] = 0;
				newball = false;
				oldball = true;
			}

			if(oldball &&(error > -35&&error<0))
			{
				firing = true;
			}

			while(firing)
			{
				motor[intake] = 50;
				motor[intake2] = 110;
				if(SensorValue[intake] >2500)
				{
					firing = false;
					newball = true;
					oldball = false;
					motor[intake] = 0;
					motor[intake2] = 0;

					shot++;
				}
				wait1Msec(5);
			}

			delay(30);
		}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
task autonomous()
{
	bool recovering = false;
	stopTask(usercontrol);
	shots=0;
	switch(autopick)//We use a switch case statement to choose autonomous for ease of scalability
	{		//and because it is easiest to integrate with the LCD picking in driver mode.
	default://any extraneous case caused by a bug, goto basic autonomous
		autopick = 0;
		break;
		//\/\/\/\/\/\/
	case 0:
		shots = 0;
		clear();
		stopTask(usercontrol);
		setRPM = 2340;//get to correct speed using TBH
		speedapprox = 58;//set approximate speed for quick firing.
		delay(2000);
		clearTimer(T3);
		autofire(4);
		//delay(600);
		setRPM = 2050;//change to 3/4 court for shooting more balls
		speedapprox = 50;
		delay(300);
		clear();
		EncoderBase(70,810);
		wait1Msec(50);
		clear();
		clear();
		GyroTurn(75,220,right);
		stopbase();
		wait1Msec(50);
		clear();
		EncoderBase(-100,430);
		autoIntake(100,0,0);
		motor[intake] = 100;
	//	motor[intake2] = 0;
		EncoderBase(-100,50);
		wait1Msec(300);
		motor[intake] = 0;
		wait1Msec(300);
		setRPM = 1995;
		EncoderBase(100,370);
		GyroTurn(75,65,left);
		stopbase();
		wait1Msec(50);
		//motor[intake2] = 100
		autofire(4);
		wait1Msec(300);
		speedapprox = 0;
		setRPM = 0;//spin down if finished early.
		break;
		//\/\/\/\/\/\/
	case 1:
		shots = 0;
		clear();
		stopTask(usercontrol);
		setRPM = 2290;//get to correct speed using TBH
		speedapprox = 58;//set approximate speed for quick firing.
		delay(2000);
		clearTimer(T3);
		while(shots <5)//run while less than 4 shots
		{
			if(SensorValue[intake] >2000 || (error > -35&&error<0))//if limit switch is not pressed, or error is low enough and
			{//flywheel is not set to not spin, feed into the flywheel
				//autoIntake(100,0,0);
				motor[intake] = 50;
				motor[intake2] = 110;
				//wait1Msec(30);
			}
			else//otherwise run outer intake and stop inner
			{
				motor[intake] = 30;
				motor[intake2] = 0;

			}
			if((QuadRPM <2060&&!recovering)||time1(T3) >1700)// if a ball is fired, or if it's taking too long, add shots
			{
				shots= shots+1;
			//wait1Msec(50);
				recovering = true;
				clearTimer(T3);
			}
			if(QuadRPM > 2200)//fix recovering to prevent over counting of shots
			{
				recovering = false;
			}
			delay(30);
		}
		//delay(600);
		setRPM = 2050;//change to 3/4 court for shooting more balls
		speedapprox = 50;
		delay(300);
		GyroTurn(100,30,left);
		wait1Msec(100);
		clear();
		EncoderBase(100,620);
		wait1Msec(50);
		clear();
		clear();
		GyroTurn(75,180,left);
		stopbase();
		EncoderBase(-50,350);
		autoIntake(100,0,0);
		motor[intake] = 60;
		wait1Msec(200);
		motor[intake] = 0;
		EncoderBase(100,150);
		wait1Msec(100);
		GyroTurn(100,135,right);
		setrpm = 2050;
		clearTimer(T3);
		shots = 0;
		while(shots <5)//run while less than 4 shots
		{
			if(SensorValue[intake] >2000 || (error > -35&&error<0))//if limit switch is not pressed, or error is low enough and
			{//flywheel is not set to not spin, feed into the flywheel
				//autoIntake(100,0,0);
				motor[intake] = 50;
				motor[intake2] = 110;
				//wait1Msec(30);
			}
			else//otherwise run outer intake and stop inner
			{
				motor[intake] = 30;
				motor[intake2] = 0;

			}
			if((QuadRPM <1900&&!recovering)||time1(T3) >1700)// if a ball is fired, or if it's taking too long, add shots
			{
				shots= shots+1;
			//wait1Msec(50);
				recovering = true;
				clearTimer(T3);
			}
			if(QuadRPM > 2000)//fix recovering to prevent over counting of shots
			{
				recovering = false;
			}
			delay(30);
		}
		break;
		//\/\/\/\/\/\/
	case 2:
	shots = 0;
		clear();
		stopTask(usercontrol);
		setRPM = 2340;//get to correct speed using TBH
		speedapprox = 58;//set approximate speed for quick firing.
		delay(2000);
		clearTimer(T3);
		autofire(4);
		//delay(600);
		setRPM = 2050;//change to 3/4 court for shooting more balls
		speedapprox = 50;
		delay(300);
		clear();
		EncoderBase(70,810);
		wait1Msec(50);
		clear();
		clear();
		GyroTurn(75,220,left);
		stopbase();
		wait1Msec(50);
		clear();
		EncoderBase(-100,430);
		autoIntake(100,0,0);
		motor[intake] = 100;
	//	motor[intake2] = 0;
		EncoderBase(-100,50);
		wait1Msec(300);
		motor[intake] = 0;
		wait1Msec(300);
		setRPM = 1995;
		EncoderBase(100,370);
		GyroTurn(75,65,right);
		stopbase();
		wait1Msec(50);
		//motor[intake2] = 100
		autofire(4);
		wait1Msec(300);
		speedapprox = 0;
		setRPM = 0;//spin down if finished early.
		break;
		//\/\/\/\/\/\/
	case 3:
		break;
	case 6:
		break;
	case 7:
		setRPM = 1880;//get to correct speed using TBH
		//speedapprox = 68;//set approximate speed for quick firing.
		delay(2000);
		//clearTimer(T3);
		motor[intake] = 100;
		motor[intake2] = 100;
		wait1Msec(30000);
		//autofire(32);
		setrPM = 0;
		break;
	}
}
task usercontrol()
{setRPM = 0;
	startTask(TBHFlywheel);
	stopTask(autonomous);//stops autonomous from running in background and hogging CPU.
	clearTimer(T4);//clearing timer for LCD update.
	while(true)
	{
		if(nLCDButtons == 1)//checks if left button is pressed, then decrements if true. Moves menu left
		{
			autopick--;
			wait1Msec(200);
		}
		else if(nLCDButtons == 4)//checks for right button, then increments if true.
		{
			autopick++;
			wait1Msec(200);
		}
		if(autopick>7)//constrains menu to number of indices in list, and causes the system to cycle
		{
			autopick = 0;
		}
		else if(autopick<0)
		{
			autopick = 7;
		}
		if(time1(T4) >=500)//This uses the internal timer to update the screen every half second.
		{
			battery1 = nImmediateBatteryLevel/1000.0;//Main Battery voltage value.
			battery2 = SensorValue[battery]/282.0;//Power expander battery voltage level from the analog status port.
			sprintf(battstring, "Mn:%1.2f  Ex:%1.2f", battery1, battery2);// Compile all the voltages into a string for the LCD. Two decimals for the main voltage, one for secondary.
			updatescreen();// Update the LCD using the defined function
			clearTimer(T4);//reset the timer for the LCD update
		}
		motor[frontright] = vexRT[Ch2] * ((vexRT[Btn6U]) ? -1 : 1);
		motor[frontleft] = vexRT[Ch3] * ((vexRT[Btn6U]) ? -1 : 1);
		if(vexRT[Btn8UXmtr2] == 1)//for proportional speed control algorithm. Gives the same speed regardless of battery
		{//works based around RPM of the flywheel, so we know  we are reaching the target speed for a given spot on the field.
			setRPM = 2280;//2280;//full court shots
			speedapprox = 57;
		}
		else if(vexRT[Btn8RXmtr2] == 1)
		{
			setRPM = 2100;// 3/4 court shots (one cross in from full)
			speedapprox = 48;
		}
		else if(vexRT[Btn8LXmtr2] == 1)
		{
			setRPM = 1880;//half court shots (center of field or two crosses distance away from net)
			speedapprox = 46;
		}
		else if(vexRT[Btn8DXmtr2] == 1)
		{
			setRPM = 1750;// bar shots or one cross away from net
			speedapprox = 46;
		}
		else if (vexRT[Btn6UXmtr2] ==1)
		{
			setRPM = 0;// set to zero to keep from running always
			speedapprox = 0;
		}
		//Begins the if statement for controlling the intake on the joystick
		//Allows both to move at once, or each to move independently, or automatically for driverload spamming
		if(vexRT[Btn5DXmtr2] ==1)//||vexRT[Btn5U] == 1)//automatic ball feeding when speed is right
		{
			if(SensorValue[intake] > 2000 || ((error > -35&&error< 0)  && setRPM !=0&&vexRT[Btn5Uxmtr2] ==1))//if limit switch is not pressed, or error is low enough and
			{//flywheel is not set to not spin, feed into the flywheel
				motor[intake] = 70;
				motor[intake2] = 100;
			}
			else//otherwise run outer intake and stop inner
			{
				motor[intake] = 60;
				motor[intake2] = 0;
			}
		}
		else if(vexRT[Btn5DXmtr2] ==1)//||vexRT[Btn5U] == 1)//automatic ball feeding when speed is right
		{
			if(SensorValue[limit1] == 0)//if limit switch is not pressed, or error is low enough and
			{//flywheel is not set to not spin, feed into the flywheel
				motor[intake] = 70;
				motor[intake2] = 100;
			}
			else//otherwise run outer intake and stop inner
			{
				motor[intake] = 60;
				motor[intake2] = 0;
			}
		}
		else if(vexRT[Btn7UXmtr2] == 1)
		{
			motor[intake] = 80;//both at once in
			motor[intake2] = 100;
		}
		else if(vexRT[Btn7DXmtr2] == 1)
		{
			motor[intake] = -80;//both at once out
			motor[intake2] = -100;
		}
		else
		{
			motor[intake] = vexRT[Ch3Xmtr2];//both on independent joysticks to move seperately
			motor[intake2] = vexRT[Ch2Xmtr2];
		}

		delay(35);//wait statement to reduce spam of values to motors. Helps with reducing stalls,
		//and makes "pFlywheel" task able to run in the round robin task system.

	}
}
/*float distance = 0.0;
task accel()
{
	float vel = 0.0;
	float filteraccel = 0.0;
	while(true)
	{
		if(SensorValue[limit2] ==1)
		{
			distance = 0;
		}
		filteraccel = (filteraccel*20 + SensorValue[accelerx])/21;
		vel = vel + filteraccel * 0.010;
		distance += 0.5 * filteraccel * 0.010*0.010+ vel * 0.010;
		delay(10);
	}
}*/
