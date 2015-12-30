#include <Gamer.h>
#include <math.h>

Gamer gamer;

static const short trackLength = 50; // How far down the track do you need to go to finish?
static const bool DEBUG = false; // Serial library seems to leak or cause weirdness so make toggle-able
static const short flickerRate = 50; 

/**
 * How the track is displayed on screen
 */
byte trackParts[4] = {
  B10001111,
  B11000111,
  B11100011,
  B11110001
};

/**
 * Defined allowed transitions from one track part to the next
 */
int trans1[4] = {0,1,1};
int trans2[4] = {0,1,2};
int trans3[4] = {1,2,3};
int trans4[4] = {2,2,3};
int* transitions[4] = {};


byte track[8] = {}; // Remembers the actual deployed track parts
int trackState[8] = {}; // Remembers which transition to apply next (randomly selected)

unsigned short gameCount = 0; // Used to make it seem a little random

enum RaceCondition {
  STOPPED = 0,
  STARTING = 1,
  INPROGRESS = 2,
  ENDING = 3,
  SHOWSCORE = 4
};

RaceCondition raceState = STOPPED;

unsigned long raceStartTime; // Start time in milliseconds
unsigned long lastTrackUpdate = 0; // Time of last track update in milliseconds
unsigned short distanceTravelled = 0; // How many track updates have happened
unsigned short score = 0;

unsigned short racerStartRow = 3; // For the start sequence
unsigned short racerEndCounter = 0; // Tracking the end sequence

/**
 * Current 'speed' of racer for track updates. Speed is measured in the number
 * of milliseconds until the next track update so a smaller value for speed means
 * dotRacer is going _faster_. Max speed: 100, min speed: 1000
 */
unsigned short dotRacerSpeed = 300;

unsigned short dotRacerPosition = 4; // Current horitzontal position of racer
bool dotRacerFlicker = true; // toggle to make the dot visible

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
  }

  // Setup transistions
  transitions[0] = trans1;
  transitions[1] = trans2;
  transitions[2] = trans3;
  transitions[3] = trans4;

  gamer.begin();

  if (DEBUG) {
    Serial.print("DotRacer Ready...\n");
  }

  gamer.printString("DOT RACER");
}

void loop() {
  unsigned long now = millis();
  switch (raceState) {

    
    case STARTING:
      if ( progressTrack(now) ) {
        racerStartRow++;
      }
      flickerRacer();
      redrawScreen(racerStartRow);
      delay(flickerRate);

      if (racerStartRow > 7 ) {
        raceState = INPROGRESS;
      }
      
    break;
    
    case INPROGRESS:
      // End condition...
      if ( distanceTravelled > trackLength ) {
        raceState = ENDING;
        
      } else {
        if ( progressTrack(now) ) {
          addNewTrack();
        }
        adjustRacerSpeed();
        handleRacerHorizontalMovement();
        flickerRacer();
        redrawScreen(7);
        delay(flickerRate);
        
      }
    break;
    
    case ENDING:
      if ( progressTrack(now) ) {
        addEmptyTrack();
        racerEndCounter++;
      }
      adjustRacerSpeed();
      handleRacerHorizontalMovement();
      flickerRacer();
      if (racerEndCounter > 7 ) {
        redrawScreen(7-(racerEndCounter-7));
      } else {
        redrawScreen(7);
      }
      delay(flickerRate);

      if (racerEndCounter > 10 ) {
        raceState = SHOWSCORE;
      }
    break;

    case SHOWSCORE:
      if ( score == 0 ) {
        score = round(
                      (
                        ((trackLength+21)*100.0)
                        /
                        (now-raceStartTime)
                      )*100.0
                     );
        if ( score == 100 ) score = 99;
      }
      
      gamer.showScore(score);

      if ( (now - lastTrackUpdate) > 5000 ) {
        raceState = STOPPED;
        resetRace();
      }
    break;
    
    default:
      flickerRacer();
      redrawScreen(racerStartRow);
      delay(flickerRate);
    break;
    
  }

  // Can restart at any time
  if(gamer.isPressed(START)) {

    if (DEBUG) {
      Serial.print("Race Starting...\n");
    }

    raceStartTime = millis();
    resetRace();
    raceState = STARTING;
  }
}

void resetRace() {
  distanceTravelled = 0;
  lastTrackUpdate = 0;
  score = 0;

  dotRacerSpeed = 300;
  dotRacerPosition = 4;
  dotRacerFlicker = true;

  racerStartRow = 3;
  racerEndCounter = 0;
  setupTrack();
}

bool progressTrack(unsigned long now) {

  if ( (now - lastTrackUpdate) >= dotRacerSpeed ) {
    // Move the track down the screen
    for (int i = 7; i>0; i--) {
      trackState[i] = trackState[i-1];
      track[i] = track[i-1];
    }
    
    lastTrackUpdate = now;
    distanceTravelled++;

    return true;
  }

  return false;

}

void addNewTrack() {
  // Place a new random track part at the top of screen
  trackState[0] = transitions[trackState[0]][random(0,3)];
  track[0] = trackParts[trackState[0]];
}

void addEmptyTrack() {
  track[0] = B00000000;
}

void adjustRacerSpeed() {
  // Player pressed speed up (up button)
  if(gamer.isPressed(UP) && dotRacerSpeed > 100) {
      dotRacerSpeed -= 100;
  }
  // Player pressed slow down (down button)
  if(gamer.isPressed(DOWN) && dotRacerSpeed <= 1000) {
    dotRacerSpeed += 100;
  }
  // We detected a collision - the racer is off the track
  // so slow it down
  if (racerOffTrack() && dotRacerSpeed <= 1000) {
    dotRacerSpeed += 50;
  }
}

/**
 * Detects "collisions". Combined all the bits set by the track
 * with the racer's bit and test to see if the racer is still
 * "visible". If not, it's off the track
 */
int racerOffTrack() {
  byte test = track[7];
  test ^= (-1 ^ test) & (1 << dotRacerPosition);
  if ( test == track[7] ) {
    return 1;
  }
  return 0;
}

void handleRacerHorizontalMovement() {
  if(gamer.isPressed(LEFT) && dotRacerPosition < 7 ) {
    dotRacerPosition += 1;
  }
  if(gamer.isPressed(RIGHT) && dotRacerPosition >= 1) {
    dotRacerPosition -= 1;
  }
}

void flickerRacer() {
  // The flicker helps make the racer visible to the eye even when off the track
  dotRacerFlicker = !dotRacerFlicker;
}

void redrawScreen(int racerPosition) {
  byte screen[8] = {};

  // Copy the current state of the track onto the screen
  for (int i = 0; i<=7; i++) { 
    screen[i] = track[i];
  }

  // Place the dotRacer
  screen[racerPosition] ^= (-dotRacerFlicker ^ screen[racerPosition]) & (1 << dotRacerPosition);
  gamer.printImage(screen);
}

void setupTrack() {
  
  for (int i = 0; i<= 7;i++) {
    track[i] = B00000000; // initialize track with blonk rows
  }
  
}


