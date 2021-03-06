// ===============================================
// @file   pong.cpp
// @author Dean Gaffney
// @student 20067423
// @practical pong
// @brief  Main code for PSP+GLFW(OpenGL) pong game 
// ===============================================


// ===============================================
// Headers
// ===============================================
#include "pong.h"

// ===============================================
// Global identifiers
// ===============================================
boost::mt19937 gen;		

Ball balls[BALL_MAX_COUNT];
int ballCount;

PowerUp powerUps[POWER_UP_MAX_COUNT];
int powerUpCount;

float paddle_x_pos;
float paddle_y_pos;
float paddle_speed;
float paddle_y_min, paddle_y_max;
float paddleLength;

float collisionTimer;

int game_quit = 0;						// flag - true for user want to quit game
int score = 0;							// number bounces off paddle
int lives = 3;							// number of balls left to play						
int auto_mode = 1;						// flag - true for computer playing
int pastScore = 0;
int requiredScore = 5;						//every 5th hit a reward is triggered.
										

// frame information
double this_time, old_time, dt, start_time;

//checks collision between balls and power ups
bool checkBallCollisionWithPowerUp(Ball &ball, PowerUp &powerUp){
	bool isCollidingOnX = ball.x + BALL_SIZE >= powerUp.x && powerUp.x + POWER_UP_SIZE >= ball.x;
	bool isCollidingOnY = ball.y + BALL_SIZE >= powerUp.y && powerUp.y + POWER_UP_SIZE >= ball.y;
	return isCollidingOnX && isCollidingOnY;
}


//called when a ball hits the paddle
void onBallHitPaddle(int index,const float maxPos){
	score++;
	paddleLength = clamp(paddleLength-5,PADDLE_MAX_LEN,PADDLE_MIN_LEN);		//shortens the paddle every hit
	balls[index].angle = M_PI - balls[index].angle;
	balls[index].x = maxPos;
}

//called when the ball misses the paddle
void onBallMissPaddle(int index){
	if(ballCount == 1 && lives > 0){
		start_life();
		lives--;
	} else if(ballCount == 1 && lives <= 0){
		lives--;
	} else{
		destroyBall(index);
	}
}

//clamps values of paddles length
float clamp(float value,float max,float min){
	if(value < min)return min;
	if(value > max)return max;
	
	return value;
}

//used for spawning power ups in random places within level boundary
int randomNumber(int min, int max){
	boost::uniform_int<> dist(min, max);
    	boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(gen, dist);
    	return die();
}

void start_life() {
	//set ball count
	ballCount = 1;

	// initial ball position and direction
	balls[ballCount - 1].x     = WINDOW_WIDTH/2;
        balls[ballCount - 1].y     = WINDOW_HEIGHT/2;
	balls[ballCount - 1].angle = static_cast <float> (rand()) / static_cast <float> (RAND_MAX); /* some arbitrary value -- could use rand() */


	// initial paddle position
	paddle_x_pos    = WINDOW_WIDTH - PADDLE_WIDTH/2;
	paddle_y_pos    = WINDOW_HEIGHT/2;
	paddleLength = PADDLE_START_LEN;

	//set score
	score = 0;
	pastScore = 0;
	requiredScore += pastScore;		//starts at 5
	
	powerUpCount = 0;

}

//calculates the chance of spawning a reward
bool calculateChance(const int& chance){
	const int random = randomNumber(1,chance);
	return random % 2 == 0;
}

//triggers rewards
void triggerReward(){
	if(calculateChance(16)){
		//spawn an extra ball
		spawnBall(WINDOW_WIDTH/2,WINDOW_HEIGHT/2,static_cast <float> (rand()) / static_cast <float> (RAND_MAX));
	}else if(calculateChance(10)){
		// spawn a power up
		float x = (float)(randomNumber(BORDER_SIZE * 4, WINDOW_WIDTH - (BORDER_SIZE * 2)));
		float y = (float)(randomNumber(BORDER_SIZE * 4, WINDOW_HEIGHT - (BORDER_SIZE * 2)));
		spawnPowerUp(x,y);
	}else{
		score += 2;		//if none of the chances matched then give player extra points
	}
}

//spawns a ball if one is available to be spawned
void spawnBall(float x,float y,float angle){	
	if(ballCount + 1 > BALL_MAX_COUNT)return;
	ballCount++;					//find a ball that isn't active and use that

	balls[ballCount - 1].x = x;				//set new ball fields
	balls[ballCount - 1].y = y;
	balls[ballCount - 1].angle = angle;
}

void destroyBall(int index){
	balls[index] = balls[ballCount - 1];
	ballCount--;
}

void spawnPowerUp(float x, float y){
	if(powerUpCount + 1 > POWER_UP_MAX_COUNT)return;
	
	powerUpCount++;
	powerUps[powerUpCount - 1].x = x;
	powerUps[powerUpCount - 1].y = y;


	int randomType = randomNumber(0,POWER_UP_TYPE_MAX_COUNT);
	powerUps[powerUpCount - 1].type = powers[randomType];
}

void destroyPowerUp(int index){	
	powerUps[index] = powerUps[powerUpCount - 1];
	powerUpCount--;
}

//deals with applying power ups to the game
void activatePowerUp(PowerUp &powerUp, int index){
	if(powerUp.type == "PADDLE_MAX"){paddleLength = PADDLE_MAX_LEN;}
	else if(powerUp.type == "PADDLE_MIN"){paddleLength = PADDLE_MIN_LEN;}
	else if(powerUp.type == "BALL_RAND_DIRECTION"){balls[index].angle = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);;}
	else if(powerUp.type == "BALL_RAND_LOCATION"){
		balls[index].x = (float)(randomNumber(BORDER_SIZE * 2, WINDOW_WIDTH - (BORDER_SIZE * 2)));
		balls[index].y = (float)(randomNumber(BORDER_SIZE * 2, WINDOW_HEIGHT - (BORDER_SIZE * 2)));
	}else if(powerUp.type == "DESTROY_BALL"){	
		if(ballCount == 1) balls[index].angle = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);	//dont destroy the only ball on screen, poor game logic.
		destroyBall(index);
	}
}

void update() {
	// automatically update paddle direction (if required)
	if (auto_mode) {
		if (paddle_y_pos-paddleLength/2>balls[0].y) {
			paddle_speed = -1;
		} else if (paddle_y_pos+paddleLength/2<balls[0].y) {
			paddle_speed = 1;
		}
	}

	// update paddle position
	paddle_y_pos += PADDLE_STEP*paddle_speed;
	
	// calculate limits for paddle movement 
	const float paddle_y_min = (paddleLength/2+MARGIN_SIZE+BORDER_SIZE);
	const float paddle_y_max = (WINDOW_HEIGHT-BORDER_SIZE-paddleLength/2);

	// ensure paddle stays within limits
	if (paddle_y_pos<paddle_y_min) {
		paddle_y_pos = paddle_y_min;
	} else if (paddle_y_pos>paddle_y_max) {
		paddle_y_pos = paddle_y_max;
	}
	
	for(int ballNumber = 0; ballNumber < ballCount;ballNumber++){

		// update ball position (centre)
		balls[ballNumber].x += BALL_STEP*cos(balls[ballNumber].angle);
		balls[ballNumber].y += BALL_STEP*sin(balls[ballNumber].angle);
		
		// calculate limits for ball movement 
		const float ball_y_min = BALL_SIZE + MARGIN_SIZE + BORDER_SIZE;
		const float ball_y_max = WINDOW_HEIGHT - BALL_SIZE - BORDER_SIZE;
		const float ball_x_min = BORDER_SIZE + BALL_SIZE;
		const float ball_x_max = WINDOW_WIDTH - PADDLE_WIDTH - BALL_SIZE;
		
	    	// check - ball hit top or bottom wall
	 	if ((balls[ballNumber].y <= ball_y_min  || balls[ballNumber].y >= ball_y_max )) {
	        	balls[ballNumber].angle = -balls[ballNumber].angle;
			if(balls[ballNumber].y <= ball_y_min)balls[ballNumber].y = ball_y_min + 2;
			else if(balls[ballNumber].y >= ball_y_min)balls[ballNumber].y = ball_y_max - 2;
	    	} 

		// check - ball hit left wall
		if  (balls[ballNumber].x <= ball_x_min) {
			balls[ballNumber].angle = M_PI - balls[ballNumber].angle;
			balls[ballNumber].x = ball_x_min + 2;
		}
		
		// check - ball hits paddle or misses
		if (balls[ballNumber].x >= ball_x_max) {
			if(fabs(balls[ballNumber].y-paddle_y_pos) <= (paddleLength+BALL_SIZE)/2) {
				onBallHitPaddle(ballNumber,WINDOW_WIDTH - PADDLE_WIDTH - BALL_SIZE - 2);
			}else{	
				onBallMissPaddle(ballNumber);
			}
	 	}

	 	//check ball collision with powerups
	 	for(int powerUp = 0; powerUp < powerUpCount;powerUp++){
			if(checkBallCollisionWithPowerUp(balls[ballNumber],powerUps[powerUp])){
				//use power up power
				activatePowerUp(powerUps[powerUp],ballNumber);

				//destroy the power up then
				destroyPowerUp(powerUp);
			}
		}	
	}

 	//trigger powerup or ball every 5th hit, if it has been 5 hits since last reward
 	if(score == pastScore + requiredScore){
		//set up score here again
		pastScore = score;
 		triggerReward();
 	}

	if(ballCount == 0){
		start_life();
		lives --;
	}
}

int main(void) {
	init();							// initial graphics/sound components
	start_life();						// reset game to starting state

	while (!game_quit && lives>0) {
		draw_scene();
		get_input();
		update();	
	}
	deinit();
	return 0;
}

